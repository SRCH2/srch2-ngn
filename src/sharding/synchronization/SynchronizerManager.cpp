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

	MulticastDiscoveryConfig multicastdiscoverConf;
	UnicastDiscoveryConfig unicastdiscoverConf;

	const vector<std::pair<string, unsigned > >& wellknownHosts = config.getWellKnownHosts();
	for (unsigned i = 0; i < wellknownHosts.size(); ++i) {
		unicastdiscoverConf.knownHosts.push_back(HostAndPort(wellknownHosts[i].first,wellknownHosts[i].second));
	}

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

}

void * dispatchMasterMessageHandler(void *arg);

void SyncManager::startDiscovery() {

	Cluster_Writeview * clusterWriteView = ShardManager::getWriteview();
	// note: this is temporary. CM should not populate <node> tags from config file

	//////////////////////////////// Changed by Jamshid
//	clusterWriteView->clear();

	ASSERT(clusterWriteView->getTotalNumberOfNodes() == 0);

	Logger::console("running discovery");

	/*
	 *  start Listening/Accepting to new connection from other nodes. This has to be done
	 *  first so that we reserve the port. This port needs to be communicated to other nodes
	 *  during discovery so that can send connection request.
	 */

	discoveryMgr->init();

	isCurrentNodeMaster = (masterNodeId == currentNodeId);

	clusterWriteView->setCurrentNodeId(currentNodeId);


	char nodename[1024];
	sprintf(nodename, "%d", this->currentNodeId);

	Node node(nodename, transport.getPublisedInterfaceAddress(), transport.getCommunicationPort(), true);
	node.thisIsMe = true;
	node.setId(this->currentNodeId);
	node.setMaster(this->currentNodeId == this->masterNodeId);

	// Pass this node to transport manager's connection map object.
	transport.getConnectionMap().setCurrentNode(node);

	discoveryCallBack =  new DiscoveryCallBack(*this);
    transport.registerCallbackHandlerForDiscovery(discoveryCallBack);

	pthread_t listenThread;
	pthread_create(&listenThread, NULL, listenForIncomingConnection, &transport);
	// give thread id to transport manger so that it can reap it later.
	transport.setListeningThread(listenThread);

	// Add new node in CM
	clusterWriteView->addNode(new Node(node), ShardingNodeStateArrived);

	localNodesCopyMutex.lock();
	localNodesCopy.push_back(node);
	localNodesCopyMutex.unlock();

	if (isCurrentNodeMaster) {
		messageHandler = new MasterMessageHandler(this);
	}
	else{
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
		if (sendConnectionRequest(&transport, masterNodeId, node, destinationAddress)) {
			char nodename[1024];
			sprintf(nodename, "%d", this->masterNodeId);
			std::string masterIp(inet_ntoa(destinationAddress.sin_addr));
			Node node(nodename, masterIp, ntohs(destinationAddress.sin_port), false);
			node.setId(this->masterNodeId);
			node.setMaster(true);

			clusterWriteView->addNode(new Node(node), ShardingNodeStateArrived);

			localNodesCopyMutex.lock();
			localNodesCopy.push_back(node);
			localNodesCopyMutex.unlock();
		}
		// use transport to fetch cluster state
		Message *msg = MessageAllocator().allocateMessage(sizeof(NodeId));
		msg->setType(ClusterInfoRequestMessageType);
		msg->setDiscoveryMask();
		char * body = msg->getMessageBody();
		*(unsigned *)body = currentNodeId;
		transport.sendMessage(masterNodeId, msg);
		MessageAllocator().deallocateByMessagePointer(msg);

		while(!__sync_val_compare_and_swap(&configUpdatesDone, true, true));


		localNodesCopyMutex.lock();
		vector<Node> localCopy = localNodesCopy;
		localNodesCopyMutex.unlock();
		for (unsigned i = 0 ; i < localCopy.size(); ++i) {
			unsigned destinationNodeId = localCopy[i].getId();
			if ( destinationNodeId == currentNodeId || destinationNodeId == masterNodeId) {
				continue;
			}
			//Logger::console("Connecting to node %d, %s, %d", destinationNodeId, localCopy[i].getIpAddress().c_str(),
			//		localCopy[i].getPortNumber());
			inet_aton(localCopy[i].getIpAddress().c_str(), &destinationAddress.sin_addr);
			//destinationAddress.sin_addr.s_addr = localCopy[i].getIpAddress();
			destinationAddress.sin_port = htons(localCopy[i].getPortNumber());
			if (!sendConnectionRequest(&transport, destinationNodeId, node, destinationAddress)) {
				Logger::console( "Could not connect to the node %d", destinationNodeId);
			}
		}

		messageHandler = new ClientMessageHandler(this);
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
	while(1) {
		if (isCurrentNodeMaster) {
			/*
			 *   2.1 Master :
			 *   a) Create a new thread to handle incoming messages from client.
			 *   b) Send heartbeat request to each node every pingInterval.
			 */
			pthread_t masterCbHandlerThread;
			pthread_create(&masterCbHandlerThread,  NULL, dispatchMasterMessageHandler, messageHandler);
			while(1) {
				sendHeartBeatToAllNodesInCluster();
				// TODO check whether still master.
				if (!hasMajority()) {
					//stepDown
					isCurrentNodeMaster = false;
					break;
				}
				sleep(pingInterval);
			}
#ifndef ANDROID
			pthread_cancel(masterCbHandlerThread);
#endif
			pthread_join(masterCbHandlerThread, NULL);
		} else {
			/*
			 *   2.1 Client :
			 *   a) Handle incoming messages at each ping interval.
			 */
			while(1) {
				messageHandler->lookForCallbackMessages(callBackHandler);
				sleep(pingInterval);
				if (isCurrentNodeMaster)  // if this node get elected as leader.
					break;
			}
		}
	}
}

bool SyncManager::hasMajority() {
	return true; // TODO V1
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

	////////////////////////////// Changed by Jamshid
	localNodesCopyMutex.lock();
	for(vector<Node>::iterator nodeItr = localNodesCopy.begin(); nodeItr != localNodesCopy.end(); ++nodeItr){
		if (nodeItr->thisIsMe)
			continue;
		Logger::debug("SM-M%d-sending heart-beat request to node %d",
				currentNodeId, nodeItr->getId());
		route( nodeItr->getId(), heartBeatMessage);
	}
	localNodesCopyMutex.unlock();
	////////////////////////////// Changed by Jamshid
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
	nodeState = MASTER_AVAILABLE;
	itselfInitiatedMasterElection = false;
	cMessageAllocator = MessageAllocator();
}

void ClientMessageHandler::lookForCallbackMessages(SMCallBackHandler* callBackHandler) {

	switch(this->nodeState) {
	// Listening for heartbeat requests from master
	case MASTER_AVAILABLE:
	{
		std::time_t timeNow = time(NULL);
		/*
		 *  Check the time elapsed since last heart beat request was received
		 */
		signed timeElapsed = timeNow - callBackHandler->getHeartBeatMessageTime();

		if (timeElapsed > _syncMgrObj->getTimeout()) {
			/*
			 *  If the time elapsed is more than TimeOut then handle time out
			 */
			nodeState = MASTER_UNAVAILABLE;
			Logger::console("SM-C%d: Timeout!!. No heart beat received from master",
					_syncMgrObj->currentNodeId);
			Message * message;
			callBackHandler->getHeartBeatMessages(&message);
			handleTimeOut(message);
			cMessageAllocator.deallocateByMessagePointer(message);
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

					_syncMgrObj->localNodesCopyMutex.lock();

					Logger::console("SM-M%d-cluster node %d failed", _syncMgrObj->currentNodeId, nodeId);
					// update configuration manager ..so that we do not ping this node again.
					// If this node comes back then it is discovery manager's job to handle it.


					for(vector<Node>::iterator nodeItr = _syncMgrObj->localNodesCopy.begin(); nodeItr != _syncMgrObj->localNodesCopy.end(); ++nodeItr){
						if(nodeItr->getId() == nodeId){
							nodeItr = _syncMgrObj->localNodesCopy.erase(nodeItr);
							break;
						}
					}
					Logger::console("[%d, %d, %d]", _syncMgrObj->localNodesCopy.size(),
								_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
					_syncMgrObj->localNodesCopyMutex.unlock();

					ShardManager::getShardManager()->resolveSMNodeFailure(nodeId);


				} else {
					ASSERT(false);  // other operations not supported yet.
				}
				cMessageAllocator.deallocateByMessagePointer(message);
			}
		}
		break;
	}
	// On master timeout, the current node is without master. It should initiate master
	// election.
	case MASTER_UNAVAILABLE:
	{
		//Logger::console("waiting for master ...");
		//TODO: V1 handle master election
		//handleMessage(callBackHandler->heartbeatMessage);
		break;
	}
	}
}
// Callback API for SM

void ClientMessageHandler::handleElectionRequest(Message *message) {
//	if (message->getType() != LeaderElectionProposalMessageType) {
//		Logger::warn("Bad Request: expecting %d but got %d",
//				LeaderElectionProposalMessageType, message->getType());
//		return;
//	}
//
//	if (!itselfInitiatedMasterElection) {
//		if (message->getBodySize() > 4) {
//			MessageAllocator msgAllocator = MessageAllocator();
//			Message * rejectMessage = msgAllocator.allocateMessage(4);
//			rejectMessage->setBodyAndBodySize(&_syncMgrObj->currentNodeId, 4);
//			rejectMessage->setType(LeaderElectionDenyMessageType);
//			_syncMgrObj->route(FETCH_UNSIGNED(message->getMessageBody()), rejectMessage);
//			msgAllocator.deallocateByMessagePointer(rejectMessage);
//		}
//		return;
//	}
//
//	bool masterEligible = true;  // TODO - fetch from CM  ..as of now every one is eligible
//	bool isMaster = false;       // TODO -- fetch from CM
//	unsigned majority = (_syncMgrObj->cluster->getTotalNumberOfNodes() / 2) + 1;
//
//	if (masterEligible) {
//		if (message->getBodySize() > 4) {
//			char * buffer = message->getMessageBody();
//			unsigned nodeId = *((unsigned *)buffer);
//			voters.insert(nodeId);
//			// if got response from more than N/2 clients
//			// send thanks to all clients
//			// else wait for more votes.
//			if (voters.size() >= majority) {
//				// x-auto-lock
//				if (voters.size() == 0)
//					return;
//				MessageAllocator msgAllocator = MessageAllocator();
//				Message * thanksMessage = msgAllocator.allocateMessage(0);
//				thanksMessage->setBodySize(0);
//				thanksMessage->setType(LeaderElectionAckMessageType);
//				std::set<unsigned>::iterator iter = voters.begin();
//				while(iter != voters.end()) {
//					if (*iter == _syncMgrObj->currentNodeId)
//						continue;
//					_syncMgrObj->route(*iter, thanksMessage);
//					++iter;
//				}
//				msgAllocator.deallocateByMessagePointer(thanksMessage);
//				voters.clear();
//				// mark yourself master.
//				isMaster = true;         // todo: set in CM
//				masterEligible = false;  // todo: set in CM
//			}
//		}
//	} else if (isMaster){
//		if (message->getBodySize() > 4) {
//			// it appears that some node didn't get the master election notice..send them again
//			MessageAllocator msgAllocator = MessageAllocator();
//			Message * thanksMessage = msgAllocator.allocateMessage(0);
//			thanksMessage->setBodySize(0);
//			thanksMessage->setType(LeaderElectionAckMessageType);
//			_syncMgrObj->route(FETCH_UNSIGNED(message->getMessageBody()), thanksMessage);
//			msgAllocator.deallocateByMessagePointer(thanksMessage);
//		}
//	} else {
//			//not eligible for master ..should not get this request ..log it and do nothing
//	}
}

void ClientMessageHandler::startMasterElection() {
	Logger::debug("SM-C%d :starting master election", _syncMgrObj->currentNodeId);
//	MessageAllocator msgAllocator = MessageAllocator();
//	Message * leaderElectionMessage = msgAllocator.allocateMessage(4);
//	leaderElectionMessage->bodySize = 4;
//	leaderElectionMessage->type = LeaderElectionProposalMessageType;
//	FETCH_UNSIGNED(leaderElectionMessage->buffer) = _syncMgrObj->currentNodeId;
//	const std::vector<Node> &nodesInCluster = *(_syncMgrObj->cluster->getNodes());
//	unsigned eligibleMasterIndex = _syncMgrObj->findNextEligibleMaster();
//	if (nodesInCluster[eligibleMasterIndex].thisIsMe) {
//		itselfInitiatedMasterElection = true;
//	} else {
//		unsigned newMasterId = nodesInCluster[eligibleMasterIndex].getId();
//		_syncMgrObj->route(newMasterId, leaderElectionMessage);
//	}
//	msgAllocator.deallocate(leaderElectionMessage);
}

void ClientMessageHandler::processHeartBeat(Message *message) {
	// TODO: Send node status as response for heartbeat message.
	// TODO: send only when node status changes.
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
	Message * heartBeatResponse = msgAllocator.allocateMessage(4);
	heartBeatResponse->setType(ClientStatusMessageType);
	heartBeatResponse->setBodyAndBodySize(&_syncMgrObj->currentNodeId, sizeof(NodeId));
	_syncMgrObj->route(masterId, heartBeatResponse);
	msgAllocator.deallocateByMessagePointer(heartBeatResponse);
}

void ClientMessageHandler::updateClusterState(Message *message){
	if (message->getBodySize() == 0)
	{
		Logger::debug("cluster state is same since last heartbeat!");
		return;
	}
	/* TODO V1
	 * 1. deserialize cluster
	 * 2. update and store it back in cluster.
	 */
	Logger::debug("cluster state updated");
}

void ClientMessageHandler::handleTimeOut(Message *message) {
	if (message->getBodySize() < sizeof(NodeId) ) {
		Logger::error("Timeout with no node information!!");
		return;
	}
	unsigned nodeId = FETCH_UNSIGNED( message->getMessageBody());
	switch(message->getType()) {
	case LeaderElectionProposalMessageType:
		// check whether current master is up if not then send the request again.
		startMasterElection();  // resend election request
		break;
	case HeartBeatMessageType:
		startMasterElection();
		break;
	default:
		break;
	}
}

void ClientMessageHandler::handleMessage(Message *message) {
	switch(message->getType()) {
	case HeartBeatMessageType:
		processHeartBeat(message);
		break;
	case LeaderElectionProposalMessageType:
		handleElectionRequest(message);
		break;
	case LeaderElectionDenyMessageType:
		handleMasterProposalReject(message);
		break;
	case LeaderElectionAckMessageType:
		updateClusterState(message);
		nodeState = MASTER_AVAILABLE;
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
	// update configuration manager ..so that we do not ping this node again.
	// If this node comes back then it is discovery manager's job to handle it.


	for(vector<Node>::iterator nodeItr = _syncMgrObj->localNodesCopy.begin(); nodeItr != _syncMgrObj->localNodesCopy.end(); ++nodeItr){
		if(nodeItr->getId() == nodeId){
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

	while(1) {
		_syncMgrObj->localNodesCopyMutex.lock();
		vector<Node> nodes = _syncMgrObj->localNodesCopy;
		_syncMgrObj->localNodesCopyMutex.unlock();
		/*
		 *   Loop over all the nodes in a cluster and check their message queues.
		 */
		for (unsigned i = 0 ;  i < nodes.size(); ++i) {
			if (nodes[i].thisIsMe)
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
					handleNodeFailure(nodeId);
				}
			}
		}

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
