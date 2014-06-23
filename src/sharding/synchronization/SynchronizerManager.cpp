/*
 * SynchronizerManager.cpp
 *
 *  Created on: Apr 20, 2014
 *      Author: srch2
 */

#include "SynchronizerManager.h"

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

	// cluster = cm.getCluster(); // commented out by Jamshid and replaced with next block
	/**Added by Jamshid */
	boost::shared_ptr<const Cluster> clusterReadview;
	// as long as this read view is not destroyed reading from cluster is safe
	// Jamshid : I changed "Cluster * cluster" to "boost::shared_ptr<const Cluster> cluster"
	// just to keep your class member but if you keep readview as a member it never gets updated
	// and readview is never destried. (You might want to ask me)
	cm.getClusterReadView(clusterReadview);
	/**********************/

	//pingInterval = cm.getDiscoveryParams().pingInterval;  TODO
	//pingTimeout = cm.getDiscoveryParams().pingTimeout;  TODO

	pingInterval = 2;
	pingTimeout = 6;
	initialTimeout = 30;  // for V0

	/* Commented out by Jamshid and replaced with the following block.
	 *

	nodesInCluster = *(cluster->getNodes());
	for (unsigned i = 0; i < nodesInCluster.size(); ++i) {
		if (nodesInCluster[i].thisIsMe)
			currentNodeId = nodesInCluster[i].getId();
	}
	this->masterNodeId = master;
	this->isCurrentNodeMaster = (this->currentNodeId == this->masterNodeId);

	*
	*/
	/* This block is added by Jamshid to replace the above commented code*/

	std::vector<const Node *> clusterNodes;
	clusterReadview->getAllNodes(clusterNodes);
	for(std::vector<const Node *>::iterator nodeItr = clusterNodes.begin(); nodeItr != clusterNodes.end(); ++nodeItr){
		this->nodesInCluster.push_back(Node(**nodeItr));
	}
	this->currentNodeId = clusterReadview->getCurrentNode()->getId();
	this->isCurrentNodeMaster = clusterReadview->isMasterNode(this->currentNodeId);
	this->masterNodeId = clusterReadview->getMasterNodeId();

	/*********************************************************************/
	if (isCurrentNodeMaster) {
		messageHandler = new MasterMessageHandler(this);
	} else {

		messageHandler = new ClientMessageHandler(this);
	}
	callBackHandler = NULL; // initialize later on run
	Logger::console("[%d, %d, %d]", nodesInCluster.size() , masterNodeId, currentNodeId);
}

SyncManager::~SyncManager() {

}

void * dispatchMasterMessageHandler(void *arg);
void SyncManager::run(){
	Logger::console("running synchronizer");
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
			pthread_cancel(masterCbHandlerThread);
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

/*
 *   Send HeartBeat request to all the clients in the cluster.
 */
void SyncManager::sendHeartBeatToAllNodesInCluster() {
	MessageAllocator msgAllocator = MessageAllocator();
	Message * heartBeatMessage = msgAllocator.allocateMessage(sizeof(NodeId));
	heartBeatMessage->setType(HeartBeatMessageType);
	heartBeatMessage->setBodyAndBodySize(&currentNodeId, sizeof(NodeId));

	for (unsigned i = 0; i < nodesInCluster.size(); ++i) {
		if (nodesInCluster[i].thisIsMe)
			continue;
		Logger::debug("SM-M%d-sending heart-beat request to node %d",
				currentNodeId, nodesInCluster[i].getId());
		route(nodesInCluster[i].getId(), heartBeatMessage);
	}
	msgAllocator.deallocateByMessagePointer(heartBeatMessage);
}

void SyncManager::route(NodeId node, Message *msg) {
	msg->setMask(0);
	msg->setMessageId(transport.getUniqueMessageIdValue());
	transport.sendMessage(node, msg);
}
//unsigned Synchronizer::findNextEligibleMaster() {
//	 return masterNodeId + 1;  // Todo go over node list and find eligible master
//}



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
void SMCallBackHandler::resolveMessage(Message *message, NodeId node){
	switch(message->getType()){
	case HeartBeatMessageType:
	{
		//cout << "****delivered heart beat ***" << endl;
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
	_syncMgrObj->resetTimeout();
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

void MasterMessageHandler::handleNodeFailure(unsigned nodeIdIndex) {

	unsigned nodeId = _syncMgrObj->nodesInCluster[nodeIdIndex].getId();
	Logger::console("SM-M%d-cluster node %d failed", _syncMgrObj->currentNodeId,
			nodeId);
	// update configuration manager ..so that we do not ping this node again.
	// If this node comes back then it is discovery manager's job to handle it.
	// For V0 update only local sate.
	_syncMgrObj->nodesInCluster.erase(_syncMgrObj->nodesInCluster.begin() + nodeIdIndex);
	_syncMgrObj->removeNodeFromCluster(nodeId);
	//_syncMgrObj->refresh();
	/** Commented out by Jamshid and replaced with next statement
	 *
	Logger::console("[%d, %d, %d]", _syncMgrObj->config.getCluster()->getTotalNumberOfNodes(),
			_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
	*
	**/
	/* Added by Jamshid ***/
	boost::shared_ptr<const Cluster> clusterReadview;
	_syncMgrObj->config.getClusterReadView(clusterReadview);
	Logger::console("[%d, %d, %d]", clusterReadview->getTotalNumberOfNodes(),
			_syncMgrObj->masterNodeId, _syncMgrObj->currentNodeId);
	/************************************/

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
	while(1) {
		std::vector<Node>& nodes = _syncMgrObj->nodesInCluster;
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
					handleNodeFailure(i);
				}
			}
		}

		if (firstTime && perNodeTimeStampEntry.size() == nodes.size()) {
			_syncMgrObj->resetTimeout();
			firstTime = false;
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
