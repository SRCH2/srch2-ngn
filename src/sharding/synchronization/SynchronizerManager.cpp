/*
 * SynchronizerManager.cpp
 *
 *  Created on: Apr 20, 2014
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "boost/shared_ptr.hpp"

#include "SynchronizerManager.h"
#include "transport/TransportHelper.h"
#include "discovery/DiscoveryManager.h"
#include "sharding/sharding/metadata_manager/Cluster_Writeview.h"

namespace srch2 {
namespace httpwrapper {

// Start synchronization Manager heart beat detection logic in a separate thread.
void * bootSynchronizer(void *arg) {
	SyncManager * obj = (SyncManager *)arg;
	if (!obj) {
		Logger::error("synchronizer cannot be started");
		exit(-1);
	}
	obj->run();
	return NULL;
}

SyncManager::SyncManager(ConfigManager& cm, TransportManager& tm) :
		transport(tm), config(cm) {

	pingInterval = config.getPing().getPingInterval();
	pingTimeout = config.getPing().getPingTimeout();

	this->masterNodeId = 0;
	this->isCurrentNodeMaster = false;
	this->currentNodeId = -1;
	this->messageHandler = NULL;
	this->callBackHandler = NULL;
	this->uniqueNodeIdSequence = 0;
	this->stopSynchManager = false;
	this->hasMajority = true;

	UnicastDiscoverySetting unicastdiscoverConf;
	// first check whether Unicast discovery is specified in the config file.
	const vector<std::pair<string, unsigned > >& wellknownHosts = config.getWellKnownHosts();
	for (unsigned i = 0; i < wellknownHosts.size(); ++i) {
		unicastdiscoverConf.knownHosts.push_back(HostAndPort(wellknownHosts[i].first,wellknownHosts[i].second));
	}
	// if unicast discovery is set then use unicast discovery as a mode of discovery
	if (unicastdiscoverConf.knownHosts.size() > 0) {
		unicastdiscoverConf.clusterName = config.getClusterName();
		unicastdiscoverConf.print();

		discoveryMgr = new  UnicastDiscoveryService(unicastdiscoverConf, this);
	} else {
		// Otherwise, get multicast config settings and use multicast discovery as a mode of discovery.
		MulticastDiscoverySetting multicastdiscoverConf;
		multicastdiscoverConf.clusterName = config.getClusterName();
		multicastdiscoverConf.multicastPort = config.getMulticastDiscovery().getPort();
		multicastdiscoverConf.multiCastAddress = config.getMulticastDiscovery().getGroupAddress();
		multicastdiscoverConf.hostIpAddressForMulticast = config.getMulticastDiscovery().getIpAddress();
		multicastdiscoverConf.ttl = config.getMulticastDiscovery().getTtl();
		multicastdiscoverConf.enableLoop = 1;

		multicastdiscoverConf.print();
		discoveryMgr = new  MulticastDiscoveryService(multicastdiscoverConf, this);
	}
}

SyncManager::~SyncManager() {
	ASSERT(this->stopSynchManager == true);
}

void * dispatchMasterMessageHandler(void *arg);

// Run in new thread to accept TCP connection from new nodes.
void* listenForIncomingConnection(void* arg) {
	SyncManager *syncManager = (SyncManager *) arg;
	syncManager->acceptConnectionFromNewNode();
	return NULL;
}

// The functions first discovers the cluster and then the node joins the cluster.
void SyncManager::startDiscovery() {

	Logger::console("[discovery] started ...");
	// start discovery manager to detect the cluster.
	discoveryMgr->init();

	// after discovery is done there could be two scenarios
	// 1. current node is a master node
	// 2. current node found the master/cluster

	// create a node object for current node
	Node node(config.getCurrentNodeName(), transport.getPublisedInterfaceAddress(), transport.getCommunicationPort(), true);
	node.thisIsMe = true;
	node.setMaster(isCurrentNodeMaster);

	if (isCurrentNodeMaster) {
		node.setId(this->currentNodeId);
		// Add new node in CM
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		ASSERT(nodesWriteview->getTotalNumberOfNodes() == 0);
		nodesWriteview->addNode(node);
		nodesWriteview->setNodeState(node.getId(), ShardingNodeStateArrived);
		nodesWriteview.reset();

	} else {

		// if this node is not master then
		// 1. Connect to master, get its nodeId, and fetch the cluster information.
		// 2. Connect with other nodes in the cluster.
		setupConnectionWithClusterNodes();

		// Add new node to CM
		node.setId(this->currentNodeId);
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		nodesWriteview->addNode(node);
		nodesWriteview->setNodeState(node.getId(), ShardingNodeStateArrived);
		nodesWriteview.reset();

		localNodesCopyMutex.lock();
		std::sort(localNodesCopy.begin(), localNodesCopy.end(), NodeComparator());
		localNodesCopyMutex.unlock();
	}

	// start a thread to accept TCP connection from other nodes.
	pthread_t listenThread;
	pthread_create(&listenThread, NULL, listenForIncomingConnection, this);
	// give the thread id to transport manger so that it stop the thread during the engine shutdown.
	transport.setListeningThread(listenThread);

	// Notify shard manager to update its current node id
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * clusterWriteView = ShardManager::getWriteview_write(xLock);
	clusterWriteView->setCurrentNodeId(currentNodeId);
	ShardManager::getShardManager()->updateCurrentNodeId(clusterWriteView);
	xLock.unlock();

}

// connect with remote node and return a socket
BoostTCP::socket * SyncManager::setupConnectionWithRemoteNode(unsigned remoteNodeIpNumber, short remoteNodePort) {

	BoostTCP::socket * newEndPointPtr = new BoostTCP::socket(networkIoService);
	BoostTCP::socket& newEndPoint = *newEndPointPtr;
	newEndPoint.open(BoostTCP::v4());

	std::string remoteNodeIp;
	TransportUtil::getIpAddressFromNumber(remoteNodeIpNumber, remoteNodeIp);

	Logger::console("Trying to connect to %s:%d", remoteNodeIp.c_str(), remoteNodePort);

	TCPConnectHandler connectHandler(networkIoService, newEndPoint);
	connectHandler.connectWithTimeout(IpAddress::from_string(remoteNodeIp), remoteNodePort);

	return newEndPointPtr;
}

// get current node id and cluster information ( list of ip:port of other nodes) from master node.
void SyncManager::initialHandshakeWithMasterNode(BoostTCP::socket& endPoint, ClusterReplyInfo *&nodesIpPortList,
		unsigned& nodesInCluster, string& masterNodeName) {

	NodeInfo currentNodeInfo;
	currentNodeInfo.nodeId = currentNodeId;
	currentNodeInfo.ipaddress = transport.getPublishedInterfaceNumericAddr();
	currentNodeInfo.communicationPort =  transport.getCommunicationPort();
	memcpy(currentNodeInfo.nodeName, config.getCurrentNodeName().c_str(), config.getCurrentNodeName().size());

	TCPBlockingSender blockingSender(networkIoService, endPoint);
	blockingSender.sendData((char *)&currentNodeInfo, sizeof(NodeInfo));

	// Fetch current node id and total nodes in cluster
	MasterReplyInfo masterReply;
	TCPBlockingReceiver blockingReceiver(networkIoService, endPoint);
	blockingReceiver.receiveData((char *)&masterReply, sizeof(MasterReplyInfo));

	if (masterReply.masterNodeId != masterNodeId) {
		Logger::console("connected to invalid master node = %d", masterReply.masterNodeId);
		throw runtime_error("");
	}

	currentNodeId = masterReply.receiverNodeId;
	nodesInCluster = masterReply.nodesCountInCluster;
	masterNodeName = string(masterReply.masterNodeName);

	nodesIpPortList = NULL;
	if (nodesInCluster) {
		unsigned sizeOfClusterReplyMessage = nodesInCluster * sizeof(ClusterReplyInfo);
		nodesIpPortList = new ClusterReplyInfo[sizeOfClusterReplyMessage];
		// Fetch list of ip:port of cluster nodes
		blockingReceiver.receiveData((char *)nodesIpPortList, sizeOfClusterReplyMessage);
	}

	unsigned fd = endPoint.native();
	fcntl(fd, F_SETFL, O_NONBLOCK);
	Logger::console("Adding node = %d to known connection", masterNodeId);
	transport.getConnectionMap().addNodeConnection(masterNodeId, fd);
	// add event to event base to start listening on this socket
	Connection *conn = &(transport.getConnectionMap().getConnection(masterNodeId));
	transport.registerEventListenerForSocket(fd, conn);
}

// send current node's information to remote node and fetch remote node's information to complete the connection.
void SyncManager::initialHandshakeWithRemoteNode(BoostTCP::socket& endPoint, NodeInfo & remoteNodeInfo) {

	NodeInfo currentNodeInfo;
	currentNodeInfo.nodeId = currentNodeId;
	currentNodeInfo.ipaddress = transport.getPublishedInterfaceNumericAddr();
	currentNodeInfo.communicationPort =  transport.getCommunicationPort();
	memcpy(currentNodeInfo.nodeName, config.getCurrentNodeName().c_str(), config.getCurrentNodeName().size());

	TCPBlockingSender blockingSender(networkIoService, endPoint);
	blockingSender.sendData((char *)&currentNodeInfo, sizeof(NodeInfo));

	TCPBlockingReceiver blockingReceiver(networkIoService, endPoint);
	blockingReceiver.receiveData((char *)&remoteNodeInfo, sizeof(NodeInfo));

	unsigned destinationNodeId = remoteNodeInfo.nodeId;
	ASSERT(destinationNodeId < currentNodeInfo.nodeId);

	if (transport.getConnectionMap().isConnectionExist(destinationNodeId)) {
		Logger::console("Connection to node id = %d already exist", destinationNodeId);
		return;
	}

	unsigned fd = endPoint.native();
	fcntl(fd, F_SETFL, O_NONBLOCK);
	Logger::console("Adding node = %d to known connection", destinationNodeId);
	transport.getConnectionMap().addNodeConnection(destinationNodeId, fd);
	// add event to event base to start listening on this socket
	Connection *conn = &(transport.getConnectionMap().getConnection(destinationNodeId));
	transport.registerEventListenerForSocket(fd, conn);

}

// If a node is not a master node then setup connection with cluster nodes.
void SyncManager::setupConnectionWithClusterNodes() {

	if (transport.getConnectionMap().isConnectionExist(masterNodeId)) {
		Logger::console("Connection to master node id = %d already exist", masterNodeId);
		return ;
	}

	//1. First setup TCP connection with master node
	BoostTCP::socket * masterEndPoint = setupConnectionWithRemoteNode(
			masterConnectionInfo.ipAddress, masterConnectionInfo.port);

	unsigned nonMasterNodesInCluster;
	ClusterReplyInfo *nodesIpPortList = NULL;
	string masterNodeName;

	//2. get current node's id and other nodes ip:port information
	initialHandshakeWithMasterNode(*masterEndPoint, nodesIpPortList, nonMasterNodesInCluster, masterNodeName);

	nodeToSocketMap[masterNodeId] = masterEndPoint;


	std::string masterIpAddress;
	TransportUtil::getIpAddressFromNumber(masterConnectionInfo.ipAddress, masterIpAddress);

	Node masterNode(masterNodeName , masterIpAddress, masterConnectionInfo.port, false);
	masterNode.setId(this->masterNodeId);
	masterNode.setMaster(true);
	//3. Add master node to SM's local copy of node information.
	addNewNodeToLocalCopy(masterNode);

	//4. Also add master node to CM
	{
		// if discovery phase then write to CM directly. There is no shard manager yet.
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		nodesWriteview->addNode(masterNode);
		nodesWriteview->setNodeState(masterNode.getId(), ShardingNodeStateArrived);
	}

	//5. If there are other nodes in cluster other than master, then setup tcp connection with other nodes
	if (nonMasterNodesInCluster) {

		for (unsigned i = 0; i < nonMasterNodesInCluster; ++i) {
			NodeInfo remoteNodeInfo;
			// setup TCP connection with non-master remote node
			BoostTCP::socket *nodeEndPoint = setupConnectionWithRemoteNode(nodesIpPortList[i].numericIpAddress,
					nodesIpPortList[i].port);

			// fetch remote node information
			initialHandshakeWithRemoteNode(*nodeEndPoint, remoteNodeInfo);

			unsigned destinationNodeId = remoteNodeInfo.nodeId;
			nodeToSocketMap[destinationNodeId] = nodeEndPoint;

			std::string nodeIpAddr;
			TransportUtil::getIpAddressFromNumber(remoteNodeInfo.ipaddress, nodeIpAddr);

			// Add node to SM's local copy of node information
			Node remoteNode(remoteNodeInfo.nodeName, nodeIpAddr , remoteNodeInfo.communicationPort, false);
			remoteNode.setId(remoteNodeInfo.nodeId);
			addNewNodeToLocalCopy(remoteNode);

			//Also add master node to CM
			SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
			nodesWriteview->addNode(remoteNode);
			nodesWriteview->setNodeState(remoteNode.getId(), ShardingNodeStateArrived);
		}
	}

	delete[] nodesIpPortList;
}

// Accept connection from new node. It is a blocking function and should be called in a new thread.
void SyncManager::acceptConnectionFromNewNode() {

	BoostTCP::endpoint listenEndPoint(
			IpAddress::from_string(transport.getPublisedInterfaceAddress()), transport.getCommunicationPort());

	BoostNetworkService listenerService;
	BoostTCP::acceptor connectionAcceptor(listenerService);
	connectionAcceptor.open(listenEndPoint.protocol());
	connectionAcceptor.set_option(BoostTCP::acceptor::reuse_address(true));

	connectionAcceptor.bind(listenEndPoint);
	connectionAcceptor.listen();
	while(!stopSynchManager) {

		// newEndPointPtr is a socket to hold new connection request.
		BoostTCP::socket * newEndPointPtr = new BoostTCP::socket(listenerService);
		BoostTCP::socket& newEndPoint = *newEndPointPtr;

		// block and wait for new connection.
		connectionAcceptor.accept(newEndPoint);

		NodeInfo remoteNodeInfo;

		// read remote node information
		TCPBlockingReceiver blockingReceiver(listenerService, newEndPoint);
		blockingReceiver.receiveData((char *)&remoteNodeInfo, sizeof(NodeInfo));

		unsigned remoteNodeID = remoteNodeInfo.nodeId;
		if (isCurrentNodeMaster) {
			// It he accepting node is master node then
			// 1. generate and send new node's id along with count of nodes (excluding master) in cluster
			// 2. send ip:port list of nodes in cluster other than master node.
			localNodesCopyMutex.lock();

			remoteNodeID = getNextNodeId();

			MasterReplyInfo reply;
			memset((char *)&reply, 0, sizeof(MasterReplyInfo));
			reply.masterNodeId = currentNodeId;
			const string& masterNodeName =  config.getCurrentNodeName();
			memcpy(reply.masterNodeName,masterNodeName.c_str(), masterNodeName.length());
			reply.nodesCountInCluster = localNodesCopy.size();
			reply.receiverNodeId = remoteNodeID;

			TCPBlockingSender blockingSender(listenerService, newEndPoint);
			blockingSender.sendData((char *)&reply, sizeof(MasterReplyInfo));

			if (localNodesCopy.size() > 0) {
				unsigned sizeOfClusterReplyMessage = localNodesCopy.size()  * sizeof(ClusterReplyInfo);
				ClusterReplyInfo * nodesIpPortList = new ClusterReplyInfo[sizeOfClusterReplyMessage];

				for (unsigned i = 0; i < localNodesCopy.size(); ++i) {
					nodesIpPortList[i].numericIpAddress =  IpAddress::from_string(localNodesCopy[i].getIpAddress()).to_ulong();
					nodesIpPortList[i].port = localNodesCopy[i].getPortNumber();
				}

				blockingSender.sendData((char *)nodesIpPortList, sizeOfClusterReplyMessage);

				delete[] nodesIpPortList;
			}
			localNodesCopyMutex.unlock();
		} else {

			// It he accepting node is not a  master node then send the current node's information back.
			NodeInfo currentNodeInfo;
			currentNodeInfo.ipaddress = transport.getPublishedInterfaceNumericAddr();
			currentNodeInfo.communicationPort =  transport.getCommunicationPort();
			currentNodeInfo.nodeId = currentNodeId;
			const string& currentNodeName =  config.getCurrentNodeName();
			memcpy(currentNodeInfo.nodeName, currentNodeName.c_str(), currentNodeName.length());

			TCPBlockingSender blockingSender(listenerService, newEndPoint);
			blockingSender.sendData((char *)& currentNodeInfo, sizeof(NodeInfo));
		}

		string remoteNodeIpAddr;
		TransportUtil::getIpAddressFromNumber(remoteNodeInfo.ipaddress, remoteNodeIpAddr);
		string remoteNodeName = string(remoteNodeInfo.nodeName);

		Node remoteNode(remoteNodeName , remoteNodeIpAddr, remoteNodeInfo.communicationPort, false);
		remoteNode.setId(remoteNodeID);
		remoteNode.setMaster(false);

		// add to a local copy
		addNewNodeToLocalCopy(remoteNode);

		unsigned fd = newEndPointPtr->native();
		transport.getConnectionMap().acceptConnection(fd, remoteNodeID);
		// add event to event base to start listening on this socket
		Connection *conn = &(transport.getConnectionMap().getConnection(remoteNodeID));
		transport.registerEventListenerForSocket(fd, conn);

		nodeToSocketMap[remoteNodeID] = newEndPointPtr;

		// notify ShardManger. Do not write directly to cluster write view.
		ShardManager::getShardManager()->resolveSMNodeArrival(remoteNode);
	}
	connectionAcceptor.close();
}

// Helper function to add node to SM's local copy
void SyncManager::addNewNodeToLocalCopy(const Node& newNode) {
	localNodesCopyMutex.lock();
	localNodesCopy.push_back(newNode);
	Logger::console("[%d, %d, %d]", localNodesCopy.size() + 1, masterNodeId, currentNodeId);
	localNodesCopyMutex.unlock();
}

void SyncManager::run(){
	Logger::console("running synchronizer");
	{
		localNodesCopyMutex.lock();
		Logger::console("[%d, %d, %d]", localNodesCopy.size() + 1,
				masterNodeId, currentNodeId);
		localNodesCopyMutex.unlock();
	}

	/*
	 *  1. Create a new callback handler for Transport layer and register it with Transport.
	 */
	callBackHandler = new SMCallBackHandler(isCurrentNodeMaster);
	transport.registerCallbackHandlerForSynchronizeManager(callBackHandler);

	/*
	 *  2. Depending upon whether the current node is master or client , the SM will
	 *     perform different duties.
	 */
	while(!stopSynchManager) {
		if (isCurrentNodeMaster) {
			messageHandler = new MasterMessageHandler(this);
			hasMajority = true;
			/*
			 *   2.1 Master :
			 *   a) Create a new thread to handle incoming messages from client.
			 *   b) Send heartbeat request to each node every pingInterval.
			 */
			pthread_t masterCbHandlerThread;
			pthread_create(&masterCbHandlerThread,  NULL, dispatchMasterMessageHandler, messageHandler);

			MessageAllocator msgAllocator = MessageAllocator();
			Message * heartBeatMessage = msgAllocator.allocateMessage(sizeof(NodeId));
			heartBeatMessage->setType(HeartBeatMessageType);
			heartBeatMessage->setBodyAndBodySize(&currentNodeId, sizeof(NodeId));

			while(isCurrentNodeMaster) {
				sendHeartBeatToAllNodesInCluster(heartBeatMessage);
				if (!hasMajority) {
					//stepDown
					isCurrentNodeMaster = false;
					// stop the discovery thread
					discoveryMgr->stopDiscovery();
				} else {
					sleep(pingInterval);
				}
			}

			msgAllocator.deallocateByMessagePointer(heartBeatMessage);

			((MasterMessageHandler *)messageHandler)->stopMasterMessageHandler();
			pthread_join(masterCbHandlerThread, NULL);
			Logger::console("SM-M%d master stepping down ...", currentNodeId);
			delete messageHandler;
		} else {
			messageHandler = new ClientMessageHandler(this);
			/*
			 *   2.1 Client :
			 *   a) Handle incoming messages at each ping interval.
			 */
			while(!stopSynchManager) {
				messageHandler->lookForCallbackMessages(callBackHandler);
				sleep(pingInterval);
				if (isCurrentNodeMaster) { // if this node get elected as leader.
					localNodesCopyMutex.lock();
					this->uniqueNodeIdSequence = localNodesCopy.back().getId() + 1;
					localNodesCopyMutex.unlock();
					break;
				}
			}
			delete messageHandler;
		}
	}
}

// Get the node id of a node which is next in line to become master.
NodeId SyncManager::getNextMasterEligbleNode() {

	localNodesCopyMutex.lock();
	vector<Node> localCopy = localNodesCopy;
	localNodesCopyMutex.unlock();

	for (unsigned i = 0; i < localCopy.size(); ++i) {
		if (localCopy[i].getId() == masterNodeId) {
			continue;
		}
		if (localCopy[i].isMasterEligible()) {
			// stop at first non-master but master-eligible node.
			return localCopy[i].getId();
		}
	}
	return -1;
}

// node id sequence generator used by master node to assign ids to
// each new node joining the cluster.
unsigned SyncManager::getNextNodeId() {
	if (isCurrentNodeMaster)
		// atomically increase the node id count by 1
		return __sync_fetch_and_add(&uniqueNodeIdSequence, 1);
	else {
		ASSERT(false);
		return uniqueNodeIdSequence;
	}
}

void SyncManager::storeMasterConnectionInfo(unsigned interfaceNumericAddress,
		short internalCommunicationPort ) {
	masterConnectionInfo.ipAddress = interfaceNumericAddress;
	masterConnectionInfo.port = internalCommunicationPort;
}

/*
 *   Send HeartBeat request to all the clients in the cluster.
 */
void SyncManager::sendHeartBeatToAllNodesInCluster(Message * heartBeatMessage) {

	localNodesCopyMutex.lock();
	for(vector<Node>::iterator nodeItr = localNodesCopy.begin(); nodeItr != localNodesCopy.end(); ++nodeItr){
		if (nodeItr->getId() == currentNodeId)
			continue;
		Logger::debug("SM-M%d-sending heart-beat request to node %d",
				currentNodeId, nodeItr->getId());
		route( nodeItr->getId(), heartBeatMessage);
	}
	localNodesCopyMutex.unlock();
}

void SyncManager::route(NodeId node, Message *msg) {
	msg->setMask(0);
	msg->setMessageId(transport.getUniqueMessageIdValue());
	transport.sendMessage(node, msg);
}

///
///  ClientMessageHandler logic begins here.
///

ClientMessageHandler::ClientMessageHandler(SyncManager* sm): _syncMgrObj(sm) {
	nodeState = SM_MASTER_AVAILABLE;
	itselfInitiatedMasterElection = false;
	cMessageAllocator = MessageAllocator();
}

void ClientMessageHandler::lookForCallbackMessages(SMCallBackHandler* callBackHandler) {

	switch(this->nodeState) {
	// Listening for heartbeat requests from master
	case SM_MASTER_AVAILABLE:
	{
		/*
		 *  Check the time elapsed since last heart beat request was received
		 */
		std::time_t lastHeartBeatFromMaster = callBackHandler->getHeartBeatMessageTime();
		double timeElapsed = difftime(time(NULL) , lastHeartBeatFromMaster);

		if (timeElapsed > ((double)_syncMgrObj->getTimeout())) {
			/*
			 *  If the time elapsed is more than TimeOut then handle time out
			 */
			Logger::console("SM-C%d: Timeout!!. No heart beat received from master for %f secs",
					_syncMgrObj->currentNodeId, timeElapsed);

			// On master timeout, the current node is without master. It should initiate master
			// election.
			startMasterElection(_syncMgrObj->masterNodeId);
			ShardManager::getShardManager()->resolveSMNodeFailure(_syncMgrObj->masterNodeId);

		} else {
			/*
			 *  Handle heartbeat request by acknowledging back to master.
			 */
			Message * message;
			callBackHandler->getHeartBeatMessages(&message);
			processHeartBeat(message);
			cMessageAllocator.deallocateByMessagePointer(message);

			// check for any cluster updates from master
			callBackHandler->getQueuedMessages(&message, _syncMgrObj->masterNodeId);
			if (message != NULL) {
				unsigned messageSize = message->getBodySize();
				char * messageBody =  message->getMessageBody();
				messageBody += sizeof(unsigned); // skip sender's node Id.
				char operation = *messageBody;
				messageBody += sizeof(char);
				if (operation == OPS_DELETE_NODE) {
					unsigned nodeId = *(unsigned *)messageBody;
					handleNodeFailure(nodeId);
					ShardManager::getShardManager()->resolveSMNodeFailure(nodeId);
				} else {
					ASSERT(false);  // other operations not supported yet.
				}
				cMessageAllocator.deallocateByMessagePointer(message);
			}
		}
		break;
	}
	case SM_MASTER_UNAVAILABLE:
	{
		Logger::console("No master for this node ... searching for cluster!");
//		_syncMgrObj->localNodesCopyMutex.lock();
//		unsigned totalNodes = _syncMgrObj->localNodesCopy.size();
//		Logger::console("Total Nodes: %d", totalNodes);
//		ASSERT(totalNodes == 1);
//		Node node =  _syncMgrObj->localNodesCopy[0];
//		ASSERT(node.getId() == _syncMgrObj->currentNodeId);
//		_syncMgrObj->localNodesCopyMutex.unlock();
//
//		_syncMgrObj->discoveryMgr->init();
//		_syncMgrObj->isCurrentNodeMaster = (_syncMgrObj->masterNodeId == _syncMgrObj->currentNodeId);
//		//ASSERT(_syncMgrObj->isCurrentNodeMaster);
//		node.setId(_syncMgrObj->currentNodeId);
//		node.setMaster(_syncMgrObj->isCurrentNodeMaster);
//		if (!_syncMgrObj->isCurrentNodeMaster) {
//			_syncMgrObj->joinExistingCluster(node, false /*true = discovery phase*/);
//		}
//
//		_syncMgrObj->localNodesCopyMutex.lock();
//		std::sort(_syncMgrObj->localNodesCopy.begin(), _syncMgrObj->localNodesCopy.end(), NodeComparator());
//		_syncMgrObj->localNodesCopyMutex.unlock();

		break;
	}
	case SM_ELECTED_AS_MASTER:
	{
		Logger::console("node %d is selected as master", _syncMgrObj->currentNodeId);
		_syncMgrObj->masterNodeId = _syncMgrObj->currentNodeId;
		_syncMgrObj->isCurrentNodeMaster = true;
		_syncMgrObj->localNodesCopyMutex.lock();
		Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size() + 1,
				_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
		_syncMgrObj->localNodesCopyMutex.unlock();
		_syncMgrObj->discoveryMgr->reInit();
		break;
	}
	case SM_PROPOSED_NEW_MASTER:
	{
		double timeElapsed = difftime(time(NULL) , proposalInitationTime);
		/*
		 *  wait for n times the original timeout to compensate for the load due to master failure
		 */
		_syncMgrObj->localNodesCopyMutex.lock();
		unsigned totalNodes = _syncMgrObj->localNodesCopy.size();
		_syncMgrObj->localNodesCopyMutex.unlock();

		if (timeElapsed > ((double)_syncMgrObj->getTimeout() * totalNodes)) {
			// Timeout occurred while waiting for the new master.  find next eligible master
			// and start master election again.
			NodeId oldMasterNodeId = expectedMasterNodeId;
			startMasterElection(oldMasterNodeId);
			ShardManager::getShardManager()->resolveSMNodeFailure(oldMasterNodeId);
		} else {
			// check for proposal acknowledgment from expect master.
			Logger::console("SM-C%d, got proposal acceptance message from node = %d",
					_syncMgrObj->currentNodeId, expectedMasterNodeId);
			Message * message;
			callBackHandler->getQueuedMessages(&message, expectedMasterNodeId);
			if (message != NULL) {
				// We get response from expected master.
				_syncMgrObj->masterNodeId = expectedMasterNodeId;

				this->nodeState = SM_MASTER_AVAILABLE;
				cMessageAllocator.deallocateByMessagePointer(message);
				_syncMgrObj->localNodesCopyMutex.lock();
				Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size() + 1,
							_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
				_syncMgrObj->localNodesCopyMutex.unlock();
				callBackHandler->setHeartBeatMessageTime(time(NULL));
			}
		}
		break;
	}
	case SM_WAITING_FOR_VOTES:
	{
		double timeElapsed = difftime(time(NULL) , proposalInitationTime);
		_syncMgrObj->localNodesCopyMutex.lock();
		unsigned totalNodes = _syncMgrObj->localNodesCopy.size();
		_syncMgrObj->localNodesCopyMutex.unlock();
		if (timeElapsed > ((double)_syncMgrObj->getTimeout() * totalNodes )) {
			this->nodeState = SM_MASTER_UNAVAILABLE;
		} else {
			handleElectionRequest();
		}
		break;
	}
	}
}
// Callback API for SM

void ClientMessageHandler::handleNodeFailure(NodeId nodeId) {
	_syncMgrObj->localNodesCopyMutex.lock();

	Logger::console("SM-C%d-cluster node %d failed", _syncMgrObj->currentNodeId, nodeId);

	for(vector<Node>::iterator nodeItr = _syncMgrObj->localNodesCopy.begin();
			nodeItr != _syncMgrObj->localNodesCopy.end(); ++nodeItr){
		if(nodeItr->getId() == nodeId){
			_syncMgrObj->unreachableNodes.push_back(*nodeItr);
			nodeItr = _syncMgrObj->localNodesCopy.erase(nodeItr);
			break;
		}
	}
	Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size() + 1,
				_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
	_syncMgrObj->localNodesCopyMutex.unlock();
}

void ClientMessageHandler::handleElectionRequest() {

	Logger::console("handling proposals");
	_syncMgrObj->localNodesCopyMutex.lock();
	const vector<Node>& smLocalNodes = _syncMgrObj->localNodesCopy;

//	if (smLocalNodes.size() == 1) {
//		this->nodeState = SM_MASTER_UNAVAILABLE;
//		_syncMgrObj->localNodesCopyMutex.unlock();
//		return;
//	}

	// Try to fetch the messages from all the nodes in the cluster.
	for (unsigned i = 0 ; i < smLocalNodes.size(); ++i) {

		if (smLocalNodes[i].getId() == _syncMgrObj->currentNodeId)
			continue;

		Message * message;
		_syncMgrObj->callBackHandler->getQueuedMessages(&message, smLocalNodes[i].getId());
		if (message != NULL) {
			if (message->getBodySize() >= sizeof(NodeId) &&
					message->getType() == LeaderElectionProposalMessageType) {
				Logger::console("SM-C%d got master election proposal from node = %d", _syncMgrObj->currentNodeId
						, smLocalNodes[i].getId());
				char * buffer = message->getMessageBody();
				unsigned nodeId = *((unsigned *)buffer);
				if (nodeId == smLocalNodes[i].getId()) {
					masterElectionProposers.insert(nodeId);
				}else {
					Logger::console("Mismatch");
					ASSERT(false);
				}
				cMessageAllocator.deallocateByMessagePointer(message);
			} else {
				Logger::console("SM-C%d bad message type while waiting for more votes",
						_syncMgrObj->currentNodeId);
			}
		}
	}
	// if got response from more than N/2 clients. Start sending confirmation
	// message back to all the client.
	if (masterElectionProposers.size() >= smLocalNodes.size() / 2) {
		MessageAllocator msgAllocator = MessageAllocator();
		Message * proposalAckMessage = msgAllocator.allocateMessage(sizeof(NodeId));
		proposalAckMessage->setBodySize(sizeof(NodeId));
		char *messageBody = proposalAckMessage->getMessageBody();
		*(unsigned *)messageBody = _syncMgrObj->currentNodeId;
		proposalAckMessage->setType(LeaderElectionAckMessageType);

		if (smLocalNodes.size() != masterElectionProposers.size()) {
			Logger::console("Electing master with a majority but not complete support!");
		}

		std::set<unsigned>::iterator iter = masterElectionProposers.begin();
		while(iter != masterElectionProposers.end()) {
			if (*iter == _syncMgrObj->currentNodeId) {
				++iter;
				continue;
			}
			_syncMgrObj->route(*iter, proposalAckMessage);
			++iter;
		}
		msgAllocator.deallocateByMessagePointer(proposalAckMessage);
		this->nodeState = SM_ELECTED_AS_MASTER;
	}

	vector<Node> newClusterNodes;
	vector<Node> missingNodes;
 	for (unsigned i = 0; i < smLocalNodes.size(); ++i) {
		if (masterElectionProposers.end() !=
				masterElectionProposers.find(smLocalNodes[i].getId())) {
			newClusterNodes.push_back(smLocalNodes[i]);
		} else {
			missingNodes.push_back(smLocalNodes[i]);
		}
	}

 	_syncMgrObj->localNodesCopy.swap(newClusterNodes);
 	_syncMgrObj->unreachableNodes.insert(_syncMgrObj->unreachableNodes.end(),
 			missingNodes.begin(), missingNodes.end());
	_syncMgrObj->localNodesCopyMutex.unlock();
}

void ClientMessageHandler::startMasterElection(NodeId oldMasterNodeId) {

	/*
	 *    Each node which is not a master starts master election on heartbeat timeout. The current node
	 *    proposes a node as new master having lowest node-id from the list of remaining master-eligible
	 *    nodes.
	 *
	 *    e.g Original Cluster:  [ N1 (M) , N2, N3 (ME), N4, N5 (ME), N6]
	 *    where N1-6 are node ids, M = master, and ME = Master Eligible
	 *
	 *    1. If node N1 is dead, then nodes N2-6 propose N3 as a new master.
	 *    2. The node N3 receives master proposal from N2-6 and determine whether it has majority
	 *       proposals (i.e proposal count is more than half of original cluster size).
	 *    3. If node N3 gets majority proposal then it sends master election confirmation to all the
	 *       proposing nodes.
	 *
	 */
	Logger::console("SM-C%d :starting master election", _syncMgrObj->currentNodeId);
	this->nodeState = SM_PROPOSED_NEW_MASTER;

	_syncMgrObj->localNodesCopyMutex.lock();

	// remove previous master from the list.
	for(vector<Node>::iterator nodeItr = _syncMgrObj->localNodesCopy.begin(); nodeItr != _syncMgrObj->localNodesCopy.end(); ++nodeItr){
		if(nodeItr->getId() == oldMasterNodeId){
			_syncMgrObj->unreachableNodes.push_back(*nodeItr);
			nodeItr = _syncMgrObj->localNodesCopy.erase(nodeItr);
			break;
		}
	}

	const vector<Node>& smLocalNodes = _syncMgrObj->localNodesCopy;

//	if (smLocalNodes.size() == 1) {
//		this->nodeState = SM_MASTER_UNAVAILABLE;
//		sleep(_syncMgrObj->pingInterval);
//		_syncMgrObj->localNodesCopyMutex.unlock();
//		return;
//	}
	_syncMgrObj->localNodesCopyMutex.unlock();

	expectedMasterNodeId = _syncMgrObj->getNextMasterEligbleNode();
	if (expectedMasterNodeId == -1) {
		Logger::console("No master eligible node found !!");
		this->nodeState = SM_MASTER_UNAVAILABLE;
		return;
	}
	proposalInitationTime = time(NULL);
	if (expectedMasterNodeId == _syncMgrObj->currentNodeId) {
		Logger::console("Proposing itself as master");
		masterElectionProposers.clear();
		this->nodeState = SM_WAITING_FOR_VOTES;
		masterElectionProposers.insert(_syncMgrObj->currentNodeId);
		itselfInitiatedMasterElection = true;
	} else {
		MessageAllocator msgAllocator = MessageAllocator();
		Message * leaderElectionMessage = msgAllocator.allocateMessage(sizeof(NodeId));
		leaderElectionMessage->setType(LeaderElectionProposalMessageType);
		FETCH_UNSIGNED(leaderElectionMessage->getMessageBody()) = _syncMgrObj->currentNodeId;
		_syncMgrObj->route(expectedMasterNodeId, leaderElectionMessage);
		msgAllocator.deallocateByMessagePointer(leaderElectionMessage);
	}
}

void ClientMessageHandler::processHeartBeat(Message *message) {
	if (message->getBodySize() < sizeof(NodeId)) {
		Logger::debug("SM-C%d: heart beat request does not have master id", _syncMgrObj->currentNodeId);
		return;
	}
	unsigned masterId = FETCH_UNSIGNED(message->getMessageBody());
	if (masterId != _syncMgrObj->masterNodeId){
		Logger::debug("SM-C%d : stray heart beat request from %d received",
				_syncMgrObj->currentNodeId, masterId);
		return;
	}
	Logger::debug("SM-C%d: heart beat request from %d received", _syncMgrObj->currentNodeId, masterId);
	MessageAllocator msgAllocator = MessageAllocator();
	Message * heartBeatResponse = msgAllocator.allocateMessage(sizeof(NodeId));
	heartBeatResponse->setType(ClientStatusMessageType);
	heartBeatResponse->setBodyAndBodySize(&_syncMgrObj->currentNodeId, sizeof(NodeId));
	_syncMgrObj->route(masterId, heartBeatResponse);
	msgAllocator.deallocateByMessagePointer(heartBeatResponse);
}

///
///   MasterMessageHandler (Master Node's message handler) logic begins here.
///

void MasterMessageHandler::handleNodeFailure(NodeId nodeId) {


	_syncMgrObj->localNodesCopyMutex.lock();

	Logger::console("SM-M%d-cluster node %d failed", _syncMgrObj->currentNodeId, nodeId);

	for(vector<Node>::iterator nodeItr = _syncMgrObj->localNodesCopy.begin();
			nodeItr != _syncMgrObj->localNodesCopy.end(); ++nodeItr){
		if(nodeItr->getId() == nodeId){
			_syncMgrObj->unreachableNodes.push_back(*nodeItr);
			nodeItr = _syncMgrObj->localNodesCopy.erase(nodeItr);
			break;
		}
	}
	_syncMgrObj->localNodesCopyMutex.unlock();

	ShardManager::getShardManager()->resolveSMNodeFailure(nodeId);



	Message * removeNodeMessage = MessageAllocator().allocateMessage(
			sizeof(NodeId) + sizeof(OPS_DELETE_NODE) + sizeof(NodeId));
	removeNodeMessage->setType(ClusterUpdateMessageType);
	char * messageBody = removeNodeMessage->getMessageBody();

	*(unsigned *)messageBody = _syncMgrObj->currentNodeId;
	messageBody += sizeof(unsigned);

	*messageBody = OPS_DELETE_NODE;     // operation to be performed.
	messageBody += sizeof(OPS_DELETE_NODE);

	*(unsigned *)messageBody = nodeId;  // node to be deleted
	messageBody += sizeof(unsigned);


	_syncMgrObj->localNodesCopyMutex.lock();
	vector<Node> nodes = _syncMgrObj->localNodesCopy;
	_syncMgrObj->localNodesCopyMutex.unlock();

	for (unsigned i = 0; i < nodes.size(); ++i) {
		if (nodes[i].getId() != _syncMgrObj->currentNodeId) {
			_syncMgrObj->route( nodes[i].getId(), removeNodeMessage);
		}
	}

	MessageAllocator().deallocateByMessagePointer(removeNodeMessage);


	Logger::console("[%d, %d, %d]", nodes.size() + 1,
			_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);

}
/*
 *  Should be called in a separate thread.
 */
void * dispatchMasterMessageHandler(void *arg) {
	MasterMessageHandler * obj = (MasterMessageHandler *) arg;
	obj->lookForCallbackMessages(NULL);
	return NULL;
}
void MasterMessageHandler::lookForCallbackMessages(SMCallBackHandler* /*not used*/) {
	SMCallBackHandler* callBackHandler  = _syncMgrObj->callBackHandler;


	while(!stopMessageHandler) {
		_syncMgrObj->localNodesCopyMutex.lock();
		vector<Node> nodes = _syncMgrObj->localNodesCopy;
		_syncMgrObj->localNodesCopyMutex.unlock();
		unsigned failedNodesCount = 0;
		/*
		 *   Loop over all the nodes in a cluster and check their message queues.
		 */
		for (unsigned i = 0 ;  i < nodes.size(); ++i) {
			if (nodes[i].getId() == _syncMgrObj->currentNodeId)
				continue;

			unsigned nodeId = nodes[i].getId();
			unsigned idx = nodeId % MSG_QUEUE_ARRAY_SIZE;    // index into array of message Queues

			/*
			 *  Check whether there is any message on this node's queue.
			 */
			if (callBackHandler->messageQArray[idx].messageQueue.size() > 0) {  // lock before access ?

				/*
				 *  Fetch the first message and get its nodeId. Update time stamp entry in
				 *  perNodeTimeStampEntry hashtable for this node.
				 */

				Message * msg = callBackHandler->messageQArray[idx].messageQueue.front();
				if (msg->getBodySize() >= sizeof(NodeId)) {
					unsigned msgNodeId = FETCH_UNSIGNED(msg->getMessageBody());
					boost::unordered_map<unsigned, unsigned>::iterator iter =
							perNodeTimeStampEntry.find(msgNodeId);
					if (iter != perNodeTimeStampEntry.end()) {
						Logger::debug("SM-M%d-Message from active node %d",
								_syncMgrObj->currentNodeId, msgNodeId);
						(*iter).second = time(0);
					} else {
						Logger::debug("SM-M%d-new node = %d found!!..start book keeping",
													_syncMgrObj->currentNodeId, nodeId);
						perNodeTimeStampEntry.insert(make_pair(msgNodeId, time(NULL)));
					}
				} else {
					Logger::debug("SM-M%d-Invalid message of size %d", _syncMgrObj->currentNodeId,
							msg->getBodySize());
				}
				/*
				 *  remove the message from the queue.
				 */
				callBackHandler->removeMessageFromQueue(nodeId);

			} else { // no message for this node
				/*
				 *  If there is no message for this node. Check whether we should timeout this
				 *  node or check again later. Also, if there was no previous timestamp for this
				 *  node, create one now.
				 */
				double timeElapsed;
				boost::unordered_map<unsigned, unsigned>::iterator iter =
						perNodeTimeStampEntry.find(nodeId);
				if (iter != perNodeTimeStampEntry.end()) {
					std::time_t timeNow = time(NULL);
					timeElapsed = difftime(timeNow , (*iter).second);
					Logger::debug("SM-M%d-message queue is empty for node %d",
											_syncMgrObj->currentNodeId, nodeId);
				} else {
					Logger::debug("SM-M%d-new node = %d found!!..start book keeping",
							_syncMgrObj->currentNodeId, nodeId);
					timeElapsed = 0;
					perNodeTimeStampEntry.insert(make_pair(nodeId, time(NULL)));
				}

				if (timeElapsed > ((double)_syncMgrObj->getTimeout())) {
					// mark node as inactive.
					++failedNodesCount;
					handleNodeFailure(nodeId);
				}
			}
		}

//		_syncMgrObj->localNodesCopyMutex.lock();
//		if (_syncMgrObj->unreachableNodes.size() > nodes.size()) {
//			_syncMgrObj->hasMajority = false;
//		}
//		_syncMgrObj->localNodesCopyMutex.unlock();

		sleep(_syncMgrObj->pingInterval);
	}
}

}} // srch2::httpwrapper
