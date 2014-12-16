/*
 * SynchronizerManager.cpp
 *
 *  Created on: Apr 20, 2014
 *      Author: srch2
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "boost/shared_ptr.hpp"

#include "SynchronizerManager.h"
#include "transport/TransportHelper.h"
#include "discovery/DiscoveryCallBack.h"
#include "discovery/DiscoveryManager.h"
#include "sharding/sharding/metadata_manager/Cluster_Writeview.h"

namespace srch2 {
namespace httpwrapper {

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
	this->discoveryCallBack = NULL;
	this->nodeIds = 0;
	this->configUpdatesDone = false;
	this->uniqueNodeId = 0;
	this->stopSynchManager = false;

	MulticastDiscoveryConfig multicastdiscoverConf;
	UnicastDiscoveryConfig unicastdiscoverConf;

	const vector<std::pair<string, unsigned > >& wellknownHosts = config.getWellKnownHosts();
	for (unsigned i = 0; i < wellknownHosts.size(); ++i) {
		unicastdiscoverConf.knownHosts.push_back(HostAndPort(wellknownHosts[i].first,wellknownHosts[i].second));
	}
	unicastdiscoverConf.clusterName = config.getClusterName();

	multicastdiscoverConf.clusterName = config.getClusterName();
	multicastdiscoverConf.multicastPort = config.getMulticastDiscovery().getPort();
	multicastdiscoverConf.multiCastAddress = config.getMulticastDiscovery().getGroupAddress();
	multicastdiscoverConf.multicastInterface = config.getMulticastDiscovery().getIpAddress();
	multicastdiscoverConf.ttl = config.getMulticastDiscovery().getTtl();
	multicastdiscoverConf.enableLoop = 1;

	// For phase 2 : Unicast is preferred over Multicast.
	// TODO: Later phase: both services should be used
	if (unicastdiscoverConf.knownHosts.size() > 0) {
		Logger::console("Unicast Discovery");
		unicastdiscoverConf.print();
		discoveryMgr = new  UnicastDiscoveryService(unicastdiscoverConf, this);
	} else {
		Logger::console("Multicast Discovery");
		multicastdiscoverConf.print();
		discoveryMgr = new  MulticastDiscoveryService(multicastdiscoverConf, this);
	}
}

SyncManager::~SyncManager() {
	ASSERT(this->stopSynchManager == true);
}

void * dispatchMasterMessageHandler(void *arg);

void SyncManager::startDiscovery() {

	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * clusterWriteView = ShardManager::getWriteview_write(xLock);

	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
	ASSERT(nodesWriteview->getTotalNumberOfNodes() == 0);

	Logger::console("running discovery");

	/*
	 *  start Listening/Accepting to new connection from other nodes. This has to be done
	 *  first so that we reserve the port. This port needs to be communicated to other nodes
	 *  during discovery so that can send connection request.
	 */

	discoveryMgr->init();

	isCurrentNodeMaster = (masterNodeId == currentNodeId);

	clusterWriteView->setCurrentNodeId(currentNodeId);
	nodesWriteview.reset();
	nodesWriteview = ShardManager::getNodesWriteview_write();
	// Also notify shard manager to update its current node id
	ShardManager::getShardManager()->updateCurrentNodeId(clusterWriteView);


	Node node(config.getCurrentNodeName(), transport.getPublisedInterfaceAddress(), transport.getCommunicationPort(), true);
	node.thisIsMe = true;
	node.setId(this->currentNodeId);
	node.setMaster(this->currentNodeId == this->masterNodeId);
	// Add new node in CM
	nodesWriteview->addNode(node);
	nodesWriteview->setNodeState(node.getId(), ShardingNodeStateArrived);
	nodesWriteview.reset();

	// Pass this node to transport manager's connection map object.
	transport.getConnectionMap().setCurrentNode(node);

	discoveryCallBack =  new DiscoveryCallBack(*this);
    transport.registerCallbackHandlerForDiscovery(discoveryCallBack);

	pthread_t listenThread;
	pthread_create(&listenThread, NULL, listenForIncomingConnection, &transport);
	// give thread id to transport manger so that it can reap it later.
	transport.setListeningThread(listenThread);

	xLock.unlock();
	localNodesCopyMutex.lock();
	localNodesCopy.push_back(node);
	localNodesCopyMutex.unlock();

	if (!isCurrentNodeMaster) {
		joinExistingCluster(node, true /*true = discovery phase*/);
	}

	localNodesCopyMutex.lock();
	std::sort(localNodesCopy.begin(), localNodesCopy.end(), NodeComparator());
	localNodesCopyMutex.unlock();

}

void SyncManager::joinExistingCluster(const Node& node, bool isDiscoveryPhase) {
	/*
	 * if this node is not master then
	 * 1. Connect to master and fetch all cluster information.
	 * 2. Connect with other nodes in the cluster.
	 */
	sockaddr_in destinationAddress;
	bool status = getDestinatioAddressByNodeId(masterNodeId, destinationAddress);
	if (status == false) {
		Logger::console("Master node %d destination address is not found", masterNodeId);
		exit(-1);
	}
	NodeInfo masterNodeInfo;
	if (sendConnectionRequest(&transport, masterNodeId, masterNodeInfo, node, destinationAddress)) {
		std::string masterIp(inet_ntoa(destinationAddress.sin_addr));
		string masterNodeName = string(masterNodeInfo.nodeName);
		Node masterNode(masterNodeName , masterIp, ntohs(destinationAddress.sin_port), false);
		masterNode.setId(this->masterNodeId);
		masterNode.setMaster(true);

		if (isDiscoveryPhase){
			// if discovery phase then write to CM directly. There is no shard manager yet.
			SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
			nodesWriteview->addNode(masterNode);
			// Todo : move this inside addNode call above.
			nodesWriteview->setNodeState(masterNode.getId(), ShardingNodeStateArrived);
		} else {
			Logger::console("shard Manager Notified of new master");
			// else notify shard manager
		}

		localNodesCopyMutex.lock();
		localNodesCopy.push_back(masterNode);
		localNodesCopyMutex.unlock();
	}
	// use transport manager to fetch cluster state
	configUpdatesDone = false;
	Message *msg = MessageAllocator().allocateMessage(sizeof(NodeId));
	msg->setType(ClusterInfoRequestMessageType);
	if (isDiscoveryPhase) {
		msg->setDiscoveryMask();
	} else {
		msg->setMask(0);
	}
	char * body = msg->getMessageBody();
	*(unsigned *)body = currentNodeId;
	transport.sendMessage(masterNodeId, msg);
	MessageAllocator().deallocateByMessagePointer(msg);

	// Wait till we get reply from master.
	int retryCount = 10;
	while(!__sync_val_compare_and_swap(&configUpdatesDone, true, true) && retryCount) {
		sleep(1);
		--retryCount;
	}
	if (configUpdatesDone == false) {
		// master did not respond. Retun false. Let the caller handle the situation.
		Logger::error("Cluster Information not received from master!!");
		return;
	}
	// At this stage the cluster info is available to this node.

	localNodesCopyMutex.lock();
	vector<Node> localCopy = localNodesCopy;
	localNodesCopyMutex.unlock();
	for (unsigned i = 0 ; i < localCopy.size(); ++i) {
		unsigned destinationNodeId = localCopy[i].getId();
		if ( destinationNodeId >= currentNodeId || destinationNodeId == masterNodeId) {
			continue;
		}
		inet_aton(localCopy[i].getIpAddress().c_str(), &destinationAddress.sin_addr);
		destinationAddress.sin_port = htons(localCopy[i].getPortNumber());
		NodeInfo destinationNodeInfo;
		if (!sendConnectionRequest(&transport, destinationNodeId, destinationNodeInfo, node, destinationAddress)) {
			Logger::console( "Could not connect to the node %d", destinationNodeId);
			//TODO:
		}
	}
}

void SyncManager::run(){
	Logger::console("running synchronizer");
	{
		localNodesCopyMutex.lock();
		Logger::console("[%d, %d, %d]", localNodesCopy.size(),
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
	 *
	 *     Note: the node can change from master to client or vice-versa during cluster
	 *     life cycle.
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
			while(!stopSynchManager) {
				sendHeartBeatToAllNodesInCluster();
				if (!hasMajority) {
					//stepDown
					isCurrentNodeMaster = false;
					// stop the discovery thread
					discoveryMgr->stopDiscovery();
					break;
				}
				sleep(pingInterval);
			}
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
					this->uniqueNodeId = localNodesCopy.back().getId() + 1;
					localNodesCopyMutex.unlock();
					break;
				}
			}
			delete messageHandler;
		}
	}
}

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

unsigned SyncManager::getNextNodeId() {
	if (isCurrentNodeMaster)
		return __sync_fetch_and_add(&uniqueNodeId, 1);
	else {
		ASSERT(false);
		return uniqueNodeId;
	}
}

void SyncManager::addNodeToAddressMappping(unsigned id, struct sockaddr_in address) {
	nodeToAddressMap[id] = address;
}

bool SyncManager::getDestinatioAddressByNodeId(unsigned id, struct sockaddr_in& address) {
	if (nodeToAddressMap.count(id) > 0) {
		address = nodeToAddressMap[id];
		return true;
	}
	return false;
}

/*
 *   Send HeartBeat request to all the clients in the cluster.
 */
void SyncManager::sendHeartBeatToAllNodesInCluster() {
	MessageAllocator msgAllocator = MessageAllocator();
	Message * heartBeatMessage = msgAllocator.allocateMessage(sizeof(NodeId));
	heartBeatMessage->setType(HeartBeatMessageType);
	heartBeatMessage->setBodyAndBodySize(&currentNodeId, sizeof(NodeId));

	localNodesCopyMutex.lock();
	for(vector<Node>::iterator nodeItr = localNodesCopy.begin(); nodeItr != localNodesCopy.end(); ++nodeItr){
		if (nodeItr->getId() == currentNodeId)
			continue;
		Logger::debug("SM-M%d-sending heart-beat request to node %d",
				currentNodeId, nodeItr->getId());
		route( nodeItr->getId(), heartBeatMessage);
	}
	localNodesCopyMutex.unlock();
	msgAllocator.deallocateByMessagePointer(heartBeatMessage);
}

void SyncManager::route(NodeId node, Message *msg) {
	msg->setMask(0);
	msg->setMessageId(transport.getUniqueMessageIdValue());
	transport.sendMessage(node, msg);
}

///
///   SM Handler implementation start here.
///

/*
 *  Constructor
 */
SMCallBackHandler::SMCallBackHandler(bool isMaster) {
	this->isMaster = isMaster;
	heartbeatMessageTimeEntry = time(NULL);
	msgAllocator = MessageAllocator();
    heartbeatMessage = msgAllocator.allocateMessage(sizeof(NodeId));

}
/*
 *   The function gets Message form TM and process it based on the message type.
 *
 *   1. Master heart beat message is stored with it arrival timestamp.
 *   2. Client message is stored in a per message queue array.
 *
 */
bool SMCallBackHandler::resolveMessage(Message *message, NodeId node){
	switch(message->getType()){
	case HeartBeatMessageType:
	{
		if (!isMaster) {
			Logger::debug("SM-CB-getting heart-beat request from node %d", node);
			boost::mutex::scoped_lock lock(hbLock);
			heartbeatMessageTimeEntry = time(NULL);
			memcpy(heartbeatMessage, message, sizeof(Message) + sizeof(NodeId));
		} else {
			//cout << "Master should not receive heat beat request" << endl;
			ASSERT(false);
		}
		break;
	}
	case ClusterUpdateMessageType:
	case ClientStatusMessageType:
	case LeaderElectionAckMessageType:
	case LeaderElectionProposalMessageType:
	{
		if (message->getBodySize() >= sizeof(NodeId)) {
			unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
			/*
			 *  We have an array of message queues. Each nodes is assigned its
			 *  queue base on its nodeId ( nodeId % array_size). Once the node
			 *  gets its queue, we append its message to the queue so that it
			 *  can be processed by master's message handler.
			 */
			unsigned idx = nodeId % MSG_QUEUE_ARRAY_SIZE;
			Message * msg = msgAllocator.allocateMessage(message->getBodySize());
			memcpy(msg, message, sizeof(Message) + message->getBodySize());
			boost::mutex::scoped_lock lock(messageQArray[idx].qGuard);
			messageQArray[idx].messageQueue.push(msg);

		} else {
			Logger::warn("SM-CB: Incomplete message received!!");
			ASSERT(false);
		}
		break;
	}
	default:
		if (message->getBodySize() >= sizeof(NodeId)) {
			unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
			Logger::warn("SM-CB: Bad message type received from node = %d", nodeId) ;
		}
		break;
	}
	return true;
}

/*
 *  Fetch heartbeat message from SMcallback Handler.
 */
void SMCallBackHandler::getHeartBeatMessages(Message**msg) {
	boost::mutex::scoped_lock lock(hbLock);
	*msg = msgAllocator.allocateMessage(sizeof(NodeId));
	memcpy(*msg, heartbeatMessage, sizeof(Message) + sizeof(NodeId));
}

/*
 *  Fetch recent heartbeat message's timestamp.
 */
std::time_t  SMCallBackHandler::getHeartBeatMessageTime(){
	boost::mutex::scoped_lock lock(hbLock);
	std::time_t copy = heartbeatMessageTimeEntry;
	return copy;
}

void SMCallBackHandler::setHeartBeatMessageTime(std::time_t time){
	boost::mutex::scoped_lock lock(hbLock);
	heartbeatMessageTimeEntry = time;
}

void SMCallBackHandler::getQueuedMessages(Message**inputMessage, unsigned masterNodeId){

	*inputMessage = NULL;
	unsigned index = masterNodeId % MSG_QUEUE_ARRAY_SIZE;
	if (this->messageQArray[index].messageQueue.size() > 0) {
		Message * queuedMessage = this->messageQArray[index].messageQueue.front();
		*inputMessage = msgAllocator.allocateMessage(queuedMessage->getBodySize());
		memcpy(*inputMessage, queuedMessage, sizeof(Message) + queuedMessage->getBodySize());
		removeMessageFromQueue(masterNodeId);
	}
}

/*
 *  Remove the processed message from Node's queue
 */
void SMCallBackHandler::removeMessageFromQueue(unsigned nodeId)
{
	unsigned idx = nodeId % MSG_QUEUE_ARRAY_SIZE;
	Message *ptr = NULL;
	std::queue<Message *> & ref = messageQArray[idx].messageQueue;
	{
		// x-auto-lock
		boost::mutex::scoped_lock lock(messageQArray[idx].qGuard);
		if (ref.size()) {
			ptr = ref.front();
			ref.pop();
		}
	}
	if (ptr)
		msgAllocator.deallocateByMessagePointer(ptr);
}

///
///  ClientMessageHandler logic begins here.
///

ClientMessageHandler::ClientMessageHandler(SyncManager* sm): MessageHandler(sm) {
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
		unsigned lastHeartBeatFromMaster = callBackHandler->getHeartBeatMessageTime();
		std::time_t timeNow = time(NULL);
		signed timeElapsed = timeNow - lastHeartBeatFromMaster;

		if (timeElapsed > _syncMgrObj->getTimeout()) {
			/*
			 *  If the time elapsed is more than TimeOut then handle time out
			 */
			Logger::console("SM-C%d: Timeout!!. No heart beat received from master for %d secs",
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
			handleMessage(message);
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
		Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size(),
				_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
		_syncMgrObj->localNodesCopyMutex.unlock();
		_syncMgrObj->discoveryMgr->reInit();
		break;
	}
	case SM_PROPOSED_NEW_MASTER:
	{
		signed timeElapsed = time(NULL) - proposalInitationTime;
		/*
		 *  wait for n times the original timeout to compensate for the load due to master failure
		 */
		_syncMgrObj->localNodesCopyMutex.lock();
		unsigned totalNodes = _syncMgrObj->localNodesCopy.size();
		_syncMgrObj->localNodesCopyMutex.unlock();

		if (timeElapsed > _syncMgrObj->getTimeout() * totalNodes) {
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
				Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size(),
							_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
				_syncMgrObj->localNodesCopyMutex.unlock();
				callBackHandler->setHeartBeatMessageTime(time(NULL));
			}
		}
		break;
	}
	case SM_WAITING_FOR_VOTES:
	{
		signed timeElapsed = time(NULL) - proposalInitationTime;
		_syncMgrObj->localNodesCopyMutex.lock();
		unsigned totalNodes = _syncMgrObj->localNodesCopy.size();
		_syncMgrObj->localNodesCopyMutex.unlock();
		if (timeElapsed > _syncMgrObj->getTimeout() * totalNodes ) {
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
	Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size(),
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

void ClientMessageHandler::handleTimeOut(Message *message) {

}

void ClientMessageHandler::handleMessage(Message *message) {
	switch(message->getType()) {
	case HeartBeatMessageType:
		processHeartBeat(message);
		break;
	default:
		break;
	}
}

///
///   MasterMessageHandler (Master Node's message handler) logic begins here.
///
void MasterMessageHandler::updateNodeInCluster(Message *message) {
	if (message->getBodySize() < sizeof(NodeId))
		return;

	unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
	if (message->getBodySize() == sizeof(NodeId))
	{
		Logger::debug("SM-M%d-node %d state is same since last heartbeat!",
				_syncMgrObj->currentNodeId, nodeId);
		return;
	}
	/*  TODO V1
	 * 1. Deserialize node
	 * 2. fetch node from cluster, update and store it back in cluster.
	 */
	Logger::debug("cluster node %d updated", nodeId);
}

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


	Logger::console("[%d, %d, %d]", nodes.size(),
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

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

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
						handleMessage(msg);
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
				unsigned timeElapsed;
				boost::unordered_map<unsigned, unsigned>::iterator iter =
						perNodeTimeStampEntry.find(nodeId);
				if (iter != perNodeTimeStampEntry.end()) {
					std::time_t timeNow = time(NULL);
					timeElapsed = timeNow - (*iter).second;
					Logger::debug("SM-M%d-message queue is empty for node %d",
											_syncMgrObj->currentNodeId, nodeId);
				} else {
					Logger::debug("SM-M%d-new node = %d found!!..start book keeping",
							_syncMgrObj->currentNodeId, nodeId);
					timeElapsed = 0;
					perNodeTimeStampEntry.insert(make_pair(nodeId, time(NULL)));
				}

				if (timeElapsed > _syncMgrObj->getTimeout()) {
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

void MasterMessageHandler::handleMessage(Message *message) {
	switch(message->getType()) {
	case ClientStatusMessageType:
		updateNodeInCluster(message);
		break;
	default:
		Logger::debug("unknown message type");
		ASSERT(false);
	}
}
}} // srch2::httpwrapper
