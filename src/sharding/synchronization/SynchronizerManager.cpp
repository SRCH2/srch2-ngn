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
	Synchronizer * obj = (Synchronizer *)arg;
	if (!obj) {
		Logger::error("synchronizer cannot be started");
		exit(-1);
	}
	obj->run();
	return NULL;
}

Synchronizer::Synchronizer(ConfigManager& cm, TransportManager& tm, unsigned master) :
		transport(tm), config(cm) {

	cluster = cm.getCluster();
	//pingInterval = cm.getDiscoveryParams().pingInterval;  TODO
	//pingTimeout = cm.getDiscoveryParams().pingTimeout;  TODO

	pingInterval = 2000;
	pingTimeout = 6000;
	initialTimeout = 60000;  // for V0

	nodesInCluster = *(cluster->getNodes());
	for (unsigned i = 0; i < nodesInCluster.size(); ++i) {
		if (nodesInCluster[i].thisIsMe)
			currentNodeId = nodesInCluster[i].getId();
	}
	this->masterNodeId = master;
	this->isCurrentNodeMaster = (this->currentNodeId == this->masterNodeId);

	if (isCurrentNodeMaster) {
		messageHandler = new MasterMessageHandler(this);
	} else {

		messageHandler = new ClientMessageHandler(this);
	}
	callBackHandler = NULL; // initialize later on run
	Logger::console("[%d, %d, %d]", nodesInCluster.size() , masterNodeId, currentNodeId);
}

Synchronizer::~Synchronizer() {

}

void * dispatchMasterMessageHandler(void *arg);
void Synchronizer::run(){
	Logger::console("running synchronizer");
	callBackHandler = new SMCallBackHandler(isCurrentNodeMaster);
	transport.registerCallbackHandlerForSynchronizeManager(callBackHandler);
	while(1) {
		if (isCurrentNodeMaster) {
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
			while(1) {
				messageHandler->lookForCallbackMessages(callBackHandler);
				sleep(pingInterval);
				if (isCurrentNodeMaster)  // if this node get elected as leader.
					break;
			}
		}
	}
}

bool Synchronizer::hasMajority() {
	return true; // TODO V1
}

void Synchronizer::sendHeartBeatToAllNodesInCluster() {
	MessageAllocator msgAllocator = MessageAllocator();
	Message * heartBeatMessage = msgAllocator.allocateMessage(4);
	heartBeatMessage->setType(HeartBeatMessageType);
	heartBeatMessage->setBodyAndBodySize(&currentNodeId, 4);
	//const std::vector<Node> &nodesInCluster = *(cluster->getNodes());
	for (unsigned i = 0; i < nodesInCluster.size(); ++i) {
		if (nodesInCluster[i].thisIsMe)
			continue;
		Logger::debug("SM-M%d-sending heart-beat request to node %d",
				currentNodeId, nodesInCluster[i].getId());
		route(nodesInCluster[i].getId(), heartBeatMessage);
	}
	msgAllocator.deallocateByMessagePointer(heartBeatMessage);
}

void Synchronizer::route(NodeId node, Message *msg) {
	msg->setMask(0);
	msg->setMessageId(transport.getUniqueMessageIdValue());
	transport.route(node, msg);
}
unsigned Synchronizer::findNextEligibleMaster() {
	 return masterNodeId + 1;  // Todo go over node list and find eligible master
}

void ClientMessageHandler::lookForCallbackMessages(SMCallBackHandler* callBackHandler) {

	switch(this->_state) {
	case 0:
	{
		std::time_t timeNow = time(NULL);
		signed timeElapsed = timeNow - callBackHandler->getHeartBeatMessageTime();
		if (timeElapsed > _syncMgrObj->getTimeout()) {
			_state = 1;
			Logger::console("SM-C%d: Timeout!!. No heart beat received from master",
					_syncMgrObj->currentNodeId);
			Message * message;
			callBackHandler->getHeartBeatMessages(&message);
			handleTimeOut(message);
			cMessageAllocator.deallocateByMessagePointer(message);
		} else {
			Message * message;
			callBackHandler->getHeartBeatMessages(&message);
			handleMessage(message);
			cMessageAllocator.deallocateByMessagePointer(message);
		}
		break;
	}
	case 1:
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
	if (message->getType() != LeaderElectionProposalMessageType) {
		Logger::warn("Bad Request: expecting %d but got %d",
				LeaderElectionProposalMessageType, message->getType());
		return;
	}

	if (!itselfInitiatedMasterElection) {
		if (message->getBodySize() > 4) {
			MessageAllocator msgAllocator = MessageAllocator();
			Message * rejectMessage = msgAllocator.allocateMessage(4);
			rejectMessage->setBodyAndBodySize(&_syncMgrObj->currentNodeId, 4);
			rejectMessage->setType(LeaderElectionDenyMessageType);
			_syncMgrObj->route(FETCH_UNSIGNED(message->getMessageBody()), rejectMessage);
			msgAllocator.deallocateByMessagePointer(rejectMessage);
		}
		return;
	}

	bool masterEligible = true;  // TODO - fetch from CM  ..as of now every one is eligible
	bool isMaster = false;       // TODO -- fetch from CM
	unsigned majority = (_syncMgrObj->cluster->getTotalNumberOfNodes() / 2) + 1;

	if (masterEligible) {
		if (message->getBodySize() > 4) {
			char * buffer = message->getMessageBody();
			unsigned nodeId = *((unsigned *)buffer);
			voters.insert(nodeId);
			// if got response from more than N/2 clients
			// send thanks to all clients
			// else wait for more votes.
			if (voters.size() >= majority) {
				// x-auto-lock
				if (voters.size() == 0)
					return;
				MessageAllocator msgAllocator = MessageAllocator();
				Message * thanksMessage = msgAllocator.allocateMessage(0);
				thanksMessage->setBodySize(0);
				thanksMessage->setType(LeaderElectionAckMessageType);
				std::set<unsigned>::iterator iter = voters.begin();
				while(iter != voters.end()) {
					if (*iter == _syncMgrObj->currentNodeId)
						continue;
					_syncMgrObj->route(*iter, thanksMessage);
					++iter;
				}
				msgAllocator.deallocateByMessagePointer(thanksMessage);
				voters.clear();
				// mark yourself master.
				isMaster = true;         // todo: set in CM
				masterEligible = false;  // todo: set in CM
			}
		}
	} else if (isMaster){
		if (message->getBodySize() > 4) {
			// it appears that some node didn't get the master election notice..send them again
			MessageAllocator msgAllocator = MessageAllocator();
			Message * thanksMessage = msgAllocator.allocateMessage(0);
			thanksMessage->setBodySize(0);
			thanksMessage->setType(LeaderElectionAckMessageType);
			_syncMgrObj->route(FETCH_UNSIGNED(message->getMessageBody()), thanksMessage);
			msgAllocator.deallocateByMessagePointer(thanksMessage);
		}
	} else {
			//not eligible for master ..should not get this request ..log it and do nothing
	}
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
	if (message->getBodySize() < 4) {
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
	heartBeatResponse->setBodyAndBodySize(&_syncMgrObj->currentNodeId, 4);
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
	/* TODO
	 * 1. deserialize cluster
	 * 2. update and store it back in cluster.
	 */
	Logger::debug("cluster state updated");
}

void MasterMessageHandler::updateNodeInCluster(Message *message) {
	if (message->getBodySize() < 4)
		return;

	unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
	if (message->getBodySize() == 4)
	{
		Logger::debug("SM-M%d-node %d state is same since last heartbeat!",
				_syncMgrObj->currentNodeId, nodeId);
		return;
	}
	/*  TODO
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
	_syncMgrObj->config.removeNodeFromCluster(nodeId);
	//_syncMgrObj->refresh();
	Logger::console("[%d, %d, %d]", _syncMgrObj->config.getCluster()->getTotalNumberOfNodes(),
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
	while(1) {
		std::vector<Node>& nodes = _syncMgrObj->nodesInCluster;
		for (unsigned i = 0 ;  i < nodes.size(); ++i) {
			if (nodes[i].thisIsMe)
				continue;

			unsigned nodeId = nodes[i].getId();
			unsigned idx = nodeId % 1024;
			if (callBackHandler->messageQArray[idx].messageQueue.size() > 0) {  // lock before access ?
				Message * msg = callBackHandler->messageQArray[idx].messageQueue.front();
				if (msg->getBodySize() >= 4) {
					unsigned msgNodeId = FETCH_UNSIGNED(msg->getMessageBody());
					boost::unordered_map<unsigned, unsigned>::iterator iter =
							perNodeTimeStampEntry.find(msgNodeId);
					if (iter != perNodeTimeStampEntry.end()) {
						Logger::debug("SM-M%d-Message from active node %d",
								_syncMgrObj->currentNodeId, msgNodeId);
						(*iter).second = time(0);
						handleMessage(msg);
					} else {
						// Message from inactive node. Synchronizer should add this node.
						// Then only we can accept message from this node.
						Logger::debug("SM-M%d-Message from non active node = %d",
								_syncMgrObj->currentNodeId, msgNodeId);
						//perNodeTimeStampEntry.insert(make_pair(msgNodeId, time(NULL)));
					}
				} else {
					Logger::debug("SM-M%d-Invalid message of size %d", _syncMgrObj->currentNodeId,
							msg->getBodySize());
				}
				callBackHandler->removeFront(nodeId);

			} else { // no message for this node
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

} /* namespace instantsearch */
} /* namespace srch2 */
