/*
 * SynchronizerManager.h
 *
 *  Created on: Apr 20, 2014
 *      Author: srch2
 */

#ifndef __SHARDING_SYNCHRONIZERMANAGER_H__
#define __SHARDING_SYNCHRONIZERMANAGER_H__

#include "configuration/ConfigManager.h"
#include "transport/TransportManager.h"
#include "sharding/sharding/ShardManager.h"
#include <utility>
#include <queue>
#include "util/Assert.h"
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <iostream>
#include <sstream>

using namespace std;

namespace srch2 {
namespace httpwrapper {

#define FETCH_UNSIGNED(x) *((unsigned *)(x))
#define MSG_QUEUE_ARRAY_SIZE 1024


static const char OPS_DELETE_NODE = 1;

class SMCallBackHandler;
class MessageHandler;
class DiscoveryCallBack;
class DiscoveryService;

class NodeComparator {
public:
	bool operator() (const Node& lhs, const Node& rhs) { return (lhs.getId() < rhs.getId());}
};


/*
 *   Entry point for the synchronizer thread. void * => Synchronizer *
 */
void *bootSynchronizer(void *arg) ;

/*
 *  This is a main class in SM module which provide synchronization facilities.
 *
 *  1- Node failure detection
 *  2- Master election on master node failure
 *  3- Keeping consistent cluster information among all nodes in cluster.
 *
 */
class SyncManager {
	friend class ClientMessageHandler;
	friend class MasterMessageHandler;
	friend class DiscoveryCallBack;
public:
	/*
	 *  Initialize internal state.
	 */
	SyncManager(ConfigManager& cm, TransportManager& tm);
	/*
	 *   Free resources.
	 */
	virtual ~SyncManager();
	/*
	 *  Main entry point for SyncManager. Starts Master/Client heartbeat logic.
	 *  Note: should be called in new thread otherwise current thread will block
	 */
	void run();

	/*
	 *  This function implements initial discovery logic of the node. This should be called
	 *  in the main thread. Once this function returns the node is ready for handling external
	 *  request.
	 */
	void startDiscovery();

	/*
	 *  Return nodeId for the new node in cluster.
	 */
	unsigned getNextNodeId();

	void setCurrentNodeId(unsigned id) { currentNodeId = id; }

	unsigned getCurrentNodeId() { return currentNodeId; }

	void setMasterNodeId(unsigned id) { masterNodeId = id;	}

	void addNodeToAddressMappping(unsigned id, struct sockaddr_in addr);

	bool getDestinatioAddressByNodeId(unsigned id, struct sockaddr_in& destinationAddress);

	bool isThisNodeMaster() { return isCurrentNodeMaster; }

	TransportManager* getTransport() { return &transport; }
	ConfigManager* getConfigManager() { return &config; }

	void setNodeIsMaster(bool flag) { isCurrentNodeMaster = flag; }

	/*
	 *  Stop sync Manager thread.
	 */
	void stop() {
		stopSynchManager = true;
	}

private:
	///
	///  Private member functions start here.
	///
	/*
	 *  Join existing cluster once the master is determined and current node id is received.
	 *
	 *  Called from :1.  Discovery Phase  2. Master relection phase.
	 */
	void joinExistingCluster(const Node& node, bool isDiscoveryPhase = false);

	NodeId getNextMasterEligbleNode();

	/*
	 *   fetch timeout interval for SM messages.
	 */
	unsigned getTimeout() { return pingTimeout; }

	//void refresh() {};
	//unsigned findNextEligibleMaster();
	/*
	 *   Send heartbeat request to all nodes in cluster
	 */
	void sendHeartBeatToAllNodesInCluster();

	/*
	 *   Wrapper around TM route function. Sets SM flag
	 */
	void route(NodeId node, Message *msg);


	///
	///  Private member variables start here.
	///

	bool isCurrentNodeMaster;
	unsigned currentNodeId;
	unsigned pingInterval;
	unsigned pingTimeout;
	unsigned masterNodeId;
	TransportManager& transport;
	SMCallBackHandler *callBackHandler;
	MessageHandler *messageHandler;
	ConfigManager& config;
	DiscoveryService *discoveryMgr;
	DiscoveryCallBack  *discoveryCallBack;
	unsigned nodeIds;
	bool configUpdatesDone;
	// Node identifier sequence.
	unsigned uniqueNodeId;
	std::map<NodeId, struct sockaddr_in>  nodeToAddressMap;
	bool stopSynchManager;
	boost::mutex localNodesCopyMutex;
	vector<Node> localNodesCopy;
	vector<Node> unreachableNodes;
	bool hasMajority;
	string serializeClusterNodes(){
		localNodesCopyMutex.lock();
		stringstream ss;
		unsigned localCopySize = localNodesCopy.size();
	    ss.write((const char *)&localCopySize, sizeof(unsigned));
		for(vector<Node>::iterator nodeItr = localNodesCopy.begin(); nodeItr != localNodesCopy.end(); ++nodeItr){
	        unsigned nodeSize = nodeItr->serialize().size();
		    ss.write((const char *)&nodeSize, sizeof(unsigned));
			ss << nodeItr->serialize();
		}
		localNodesCopyMutex.unlock();
		return ss.str();
	}
};

class SMCallBackHandler : public CallBackHandler{
public:
	/*
	 *   The function gets Message form TM and process it based on the message type.
	 *
	 *   1. Master heart beat message is stored with it arrival timestamp.
	 *   2. Client message is stored in a per message queue array.
	 *
	 */
	bool resolveMessage(Message * msg, NodeId node);

	/*
	 *  Constructor
	 */
	SMCallBackHandler(bool isMaster);

	/*
	 *  Remove message from node's queue.
	 */
	void removeMessageFromQueue(unsigned nodeId);

	/*
	 *  Remove message from node's queue.
	 */
	void getHeartBeatMessages(Message**msg);

	/*
	 *  Get heartbeat message's timestamp.
	 */
	std::time_t getHeartBeatMessageTime();

	void setHeartBeatMessageTime(std::time_t time);

	/*
	 *   Get queued message from a given node's queue
	 */
	void getQueuedMessages(Message**inputMessage, unsigned nodeId);

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
	} messageQArray[MSG_QUEUE_ARRAY_SIZE];

	MessageAllocator msgAllocator;
};

/*
 *   The abstract class which provides a general interface for handling
 *   messages.
 */
class MessageHandler {
public:
	MessageHandler(SyncManager* sm) { _syncMgrObj = sm; }

	/*
	 *   Any kind of failure should be handled in this function.
	 */
	virtual void handleFailure(Message *message) = 0;
	/*
	 *   Timeout case should be handled in this function.
	 */
	virtual void handleTimeOut(Message *message)  = 0;
	/*
	 *   Success case should be handled in this function.
	 */
	virtual void handleMessage(Message *message)  = 0;
	/*
	 *   The function should handle main logic of processing
	 *   messages delivered by TM.
	 */
	virtual void lookForCallbackMessages(SMCallBackHandler*) = 0;

	virtual ~MessageHandler() {}
protected:
	SyncManager *_syncMgrObj;
};

enum ClientNodeState {
	SM_MASTER_AVAILABLE,
	SM_MASTER_UNAVAILABLE,
	SM_PROPOSED_NEW_MASTER,
	SM_WAITING_FOR_VOTES,
	SM_ELECTED_AS_MASTER
};

/*
 *   This class implements the client node's message handling
 */
class ClientMessageHandler : public MessageHandler{
public:
	ClientMessageHandler(SyncManager* sm);
	/*
	 *   The function handles the main logic of processing
	 *   messages delivered by TM.
	 */
	void lookForCallbackMessages(SMCallBackHandler*);

	void handleFailure(Message *message) {
		ASSERT(false);
	}

	virtual void handleTimeOut(Message *message);

	virtual void handleMessage(Message *message) ;

private:

	void handleNodeFailure(NodeId nodeId);

	void startMasterElection(NodeId oldMasterNodeId);

	void processHeartBeat(Message *message);

	void handleElectionRequest();

	ClientNodeState nodeState;
	MessageAllocator cMessageAllocator;
	bool itselfInitiatedMasterElection;
	std::set<unsigned> masterElectionProposers;

	NodeId expectedMasterNodeId;
	std::string masterUnavailbleReason;
	unsigned proposalInitationTime;
};

class MasterMessageHandler : public MessageHandler{
public:
	MasterMessageHandler(SyncManager *sm): MessageHandler(sm) { stopMessageHandler = false; }
	/*
	 *   The function should handle main logic of processing
	 *   messages delivered by TM.
	 */
	void lookForCallbackMessages(SMCallBackHandler*);

	void handleFailure(Message *message) {
		ASSERT(false);
	}
	virtual void handleTimeOut(Message *message) {
		ASSERT(false);
	}

	virtual void handleMessage(Message *message);

	void stopMasterMessageHandler() { stopMessageHandler = true; }

private:

	bool stopMessageHandler;
	void updateNodeInCluster(Message *message);
	void handleNodeFailure(NodeId nodeId);
	// key = Node id , Value = Latest time when message was received from this node.
	boost::unordered_map<unsigned, unsigned>  perNodeTimeStampEntry;

};

} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* SYNCHRONIZERMANAGER_H_ */
