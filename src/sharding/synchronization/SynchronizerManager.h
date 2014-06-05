/*
 * SynchronizerManager.h
 *
 *  Created on: Apr 20, 2014
 *      Author: srch2
 */

#ifndef SYNCHRONIZERMANAGER_H_
#define SYNCHRONIZERMANAGER_H_

#include "configuration/ConfigManager.h"
#include "transport/TransportManager.h"
#include <utility>
#include <queue>
#include "util/Assert.h"
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <iostream>
using namespace std;
using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {

#define FETCH_UNSIGNED(x) *((unsigned *)(x))

class SMCallBackHandler;
class MessageHandler;
void *bootSynchronizer(void *arg) ;

class Synchronizer {
public:
	Synchronizer(ConfigManager& cm, TransportManager& tm, unsigned masterNodeId);
	virtual ~Synchronizer();
	// should be called in new thread otherwise current thread will block
	void run();
private:
	unsigned getTimeout() { return initialTimeout ; }      // temp for V0 replace with pingTimeout in V1
	void resetTimeout() {
		initialTimeout = pingTimeout;
	}  // temp for V0
	//void refresh() {};
	//unsigned findNextEligibleMaster();
	void sendHeartBeatToAllNodesInCluster();
	void registerForCallback();
	void lookForCallbackMessages();
	void route(NodeId node, Message *msg);
	bool hasMajority();
	bool isCurrentNodeMaster;
	unsigned currentNodeId;
	unsigned pingInterval;
	unsigned pingTimeout;
	unsigned masterNodeId;
	unsigned initialTimeout;
	Cluster *cluster;
	TransportManager& transport;
	SMCallBackHandler *callBackHandler;
	MessageHandler *messageHandler;
	friend class ClientMessageHandler;
	friend class MasterMessageHandler;
	ConfigManager& config;
	std::vector<Node> nodesInCluster;

};

class SMCallBackHandler : public CallBackHandler{
public:
	void notifyNoReply(Message * msg) {
		notifyWithReply(msg);
	}
	std::pair<Message*,void*>  notifyWithReply(Message *message) {
                  
		switch(message->getType()){
			case HeartBeatMessageType:
			{
				//cout << "****delivered heart beat ***" << endl;
				if (!isMaster) {
					boost::mutex::scoped_lock lock(hbLock);
					heartbeatMessageTimeEntry = time(NULL);
					memcpy(heartbeatMessage, message, sizeof(Message) + 4);
				} else {
					//cout << "Master should not receive heat beat request" << endl;
				}
				break;
			}
			case ClientStatusMessageType:
			case LeaderElectionAckMessageType:
			case LeaderElectionProposalMessageType:
			{
				if (message->getBodySize() >= 4) {
					unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
				//cout << "*** delivered message received from client " << nodeId << endl ;
					unsigned idx = nodeId % 1024;
					Message * msg = msgAllocator.allocateMessage(message->getBodySize());
					memcpy(msg, message, sizeof(Message) + message->getBodySize());
					boost::mutex::scoped_lock lock(messageQArray[idx].qGuard);
					messageQArray[idx].messageQueue.push(msg);
					//cout << "node" << idx << " :: " << messageQArray[idx].messageQueue.size();
				} else {
					Logger::warn("SM-CB: Incomplete message received!!");
				}
				break;
			}
			default:
				if (message->getBodySize() >= 4) {
					unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
					Logger::warn("SM-CB: Bad message type received from node = %d", nodeId) ;
				}
				break;
		}
		return std::make_pair<Message*,void*>(NULL,NULL);
	}

	SMCallBackHandler(bool isMaster) {
		this->isMaster = isMaster;
		heartbeatMessageTimeEntry = time(NULL);
		msgAllocator = MessageAllocator();
	    heartbeatMessage = msgAllocator.allocateMessage(4);

	}
	void removeFront(unsigned nodeId)
	{
		unsigned idx = nodeId % 1024;
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

	void getHeartBeatMessages(Message**msg) {
		boost::mutex::scoped_lock lock(hbLock);
		*msg = msgAllocator.allocateMessage(4);
		memcpy(*msg, heartbeatMessage, sizeof(Message) + 4);
	}

	std::time_t getHeartBeatMessageTime() {
		boost::mutex::scoped_lock lock(hbLock);
		std::time_t copy = heartbeatMessageTimeEntry;
		return copy;
	}
private:
	bool isMaster;
	// Last timestamp when heartbeat message was recieved from master
	std::time_t heartbeatMessageTimeEntry;
	// should keep the latest one only.
	Message* heartbeatMessage;
	boost::mutex hbLock;
public:
	// per Client entry
	struct MessageQ{
		std::queue<Message *> messageQueue;
		boost::mutex qGuard;
	} messageQArray[1024];

	MessageAllocator msgAllocator;
};

class MessageHandler {
public:
	MessageHandler(Synchronizer* sm) { _syncMgrObj = sm; }
	virtual void handleFailure(Message *message) = 0;

	virtual void handleTimeOut(Message *message)  = 0;

	virtual void handleMessage(Message *message)  = 0;

	virtual void lookForCallbackMessages(SMCallBackHandler*) = 0;

	virtual ~MessageHandler() {}
protected:
	Synchronizer *_syncMgrObj;
};

class ClientMessageHandler : public MessageHandler{
public:
	ClientMessageHandler(Synchronizer* sm): MessageHandler(sm) {
		_state = 0; itselfInitiatedMasterElection = false;
		cMessageAllocator = MessageAllocator();
	}
	void lookForCallbackMessages(SMCallBackHandler*);
	void handleFailure(Message *message) {
		// not implmented for V0
		return;
	}
	virtual void handleTimeOut(Message *message) {
		if (message->getBodySize() < 4 ) {
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

	virtual void handleMessage(Message *message) {
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
			_state = 0;
			break;
		default:
			break;
		}
	}
private:
	void startMasterElection();
	void processHeartBeat(Message *message);
	void handleElectionRequest(Message *message);
	void handleMasterProposalReject(Message *message) {
		// Proposed master rejected the message. Possible reasons.
		// 1. May be it was not eligible for master
		// 2. It already has a master
		// 3. Could not find enough vote in a given time to be elected as master.
		// For V0 do nothing  TODO: v1
	}
	void updateClusterState(Message *message);
	unsigned _state;
	MessageAllocator cMessageAllocator;
	bool itselfInitiatedMasterElection;
	std::set<unsigned> voters;
};

class MasterMessageHandler : public MessageHandler{
public:
	MasterMessageHandler(Synchronizer *sm): MessageHandler(sm) { firstTime = true;}
	void lookForCallbackMessages(SMCallBackHandler*);
	void handleFailure(Message *message) {
	}
	virtual void handleTimeOut(Message *message) {
	}

	virtual void handleMessage(Message *message) {
		switch(message->getType()) {
		case ClientStatusMessageType:
			updateNodeInCluster(message);
			break;
		default:
			Logger::debug("unknown message type");
			ASSERT(true);
		}
	}
private:
	bool firstTime;  // temp for V0
	void updateNodeInCluster(Message *message);
	void handleNodeFailure(unsigned nodeId);
	// key = Node id , Value = Latest time when message was received from this node.
	boost::unordered_map<unsigned, unsigned>  perNodeTimeStampEntry;

};

//class SerializerInterface {
//	virtual void serialize(void *object, char **byte, unsigned* size) = 0;
//	virtual void * Deserialize(char *byte, unsigned size) = 0;
//	virtual ~SerializerInterface() {}
//};
//
//class NodeSerializer : public SerializerInterface {
//	virtual void serialize(void *object, char **byte, unsigned* size){
//		Node * node = (Node *) object;
//
//	}
//};
//
//class ClusterSerializer : public SerializerInterface {
//
//};
//
//class ShardSerializer : public SerializerInterface {
//
//};





} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* SYNCHRONIZERMANAGER_H_ */
