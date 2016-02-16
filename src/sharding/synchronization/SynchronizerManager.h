/*
 * SynchronizerManager.h
 *
 *  Created on: Apr 20, 2014
 */

#ifndef __SHARDING_SYNCHRONIZERMANAGER_H__
#define __SHARDING_SYNCHRONIZERMANAGER_H__

#include "configuration/ConfigManager.h"
#include "transport/TransportManager.h"
#include "sharding/sharding/ShardManager.h"
#include "SMCallBackHandler.h"
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

typedef boost::asio::ip::tcp BoostTCP;
typedef boost::asio::ip::address_v4 IpAddress;

static const char OPS_DELETE_NODE = 1;

struct MasterReplyInfo {
	u_int32_t masterNodeId;
	u_int32_t receiverNodeId;
	u_int32_t nodesCountInCluster;
	char masterNodeName[1024];
};

struct ClusterReplyInfo {
	u_int32_t numericIpAddress;
	u_int32_t port;
};

class SMCallBackHandler;
class MessageHandler;
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
	friend void* listenForIncomingConnection(void* arg);
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
	 *  node id sequence generator used by master node to assign ids to each new node joining the cluster.
	 */
	unsigned getNextNodeId();

	void setCurrentNodeId(unsigned id) { currentNodeId = id; }

	unsigned getCurrentNodeId() { return currentNodeId; }

	void setMasterNodeId(unsigned id) { masterNodeId = id;	}

	void storeMasterConnectionInfo( unsigned interfaceNumericAddress,
			short internalCommunicationPort);

	bool isThisNodeMaster() { return isCurrentNodeMaster; }

	TransportManager* getTransportManager() { return &transport; }

	ConfigManager* getConfigManager() { return &config; }

	void setNodeIsMaster(bool flag) { isCurrentNodeMaster = flag; }

	/*
	 *  Stop sync Manager thread.
	 */
	void stop() {
		stopSynchManager = true;
	}

	void addNewNodeToLocalCopy(const Node& node);

	/*
	 *   fetch timeout interval for SM messages.
	 */
	unsigned getTimeout() { return pingTimeout; }


private:
	///
	///  Private member functions start here.
	///

	NodeId getNextMasterEligbleNode();

	/*
	 *   Send heartbeat request to all nodes in cluster
	 */
	void sendHeartBeatToAllNodesInCluster(Message *message);

	/*
	 *   Wrapper around TM route function. Sets SM flag
	 */
	void route(NodeId node, Message *msg);

	// connect with remote node and return a socket
	BoostTCP::socket * setupConnectionWithRemoteNode(unsigned remoteNodeIpNumber, short remoteNodePort);

	// get current node id and cluster information ( list of ip:port of other nodes) from master node.
	void initialHandshakeWithMasterNode(BoostTCP::socket& endPoint,
			ClusterReplyInfo *&nodesIpPortList, unsigned& nodesInCluster, string& masterNodeName);

	// send current node's information to remote node and fetch remote node's information to complete the connection.
	void initialHandshakeWithRemoteNode(BoostTCP::socket& endPoint, NodeInfo & remoteNodeInfo);

	// non-master node calls this function to complete the TCP connections with all nodes in cluster.
	void setupConnectionWithClusterNodes();

	// accept TCP connection from other nodes.
	void acceptConnectionFromNewNode();

	///
	///  Private member variables start here.
	///

	bool isCurrentNodeMaster;
	unsigned currentNodeId;
	unsigned masterNodeId;

	// heartbeat related parameter.
	unsigned pingInterval;  // interval gap between heartbeat
	unsigned pingTimeout;   // max time before heartbeat should be detected


	TransportManager& transport;
	SMCallBackHandler *callBackHandler;
	MessageHandler *messageHandler;
	ConfigManager& config;
	DiscoveryService *discoveryMgr;

	// Node id sequence.(Starts with 0)
	unsigned uniqueNodeIdSequence;

	struct MasterConnectionInfo {
		unsigned ipAddress;
		unsigned port;
	} masterConnectionInfo;

	// flag to stop SM on node shutdown.
	bool stopSynchManager;

	// stores nodes information of the cluster
	vector<Node> localNodesCopy;
	boost::mutex localNodesCopyMutex;

	// keep track of nodes not reachable on node failure.
	vector<Node> unreachableNodes;

	// flag to indicate whether the master node can see more than N/2 nodes in cluster.
	bool hasMajority;

	BoostNetworkService networkIoService;

	// temporary map of nodeid -> socket ...will be moved to TM
	map<unsigned, BoostTCP::socket *> nodeToSocketMap;
};

// Different states of non-master node
enum ClientNodeState {
	SM_MASTER_AVAILABLE,
	SM_MASTER_UNAVAILABLE,
	SM_PROPOSED_NEW_MASTER,
	SM_WAITING_FOR_VOTES,
	SM_ELECTED_AS_MASTER
};

class MessageHandler {
public:
	virtual void lookForCallbackMessages(SMCallBackHandler*) = 0;
	virtual ~MessageHandler() {}
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

	SyncManager *_syncMgrObj;
};

class MasterMessageHandler : public MessageHandler{
public:
	MasterMessageHandler(SyncManager *sm): _syncMgrObj(sm) { stopMessageHandler = false; }
	/*
	 *   The function should handle main logic of processing
	 *   messages delivered by TM.
	 */
	void lookForCallbackMessages(SMCallBackHandler*);

	void stopMasterMessageHandler() { stopMessageHandler = true; }

private:

	bool stopMessageHandler;
	void handleNodeFailure(NodeId nodeId);
	// key = Node id , Value = Latest time when message was received from this node.
	boost::unordered_map<unsigned, unsigned>  perNodeTimeStampEntry;
	SyncManager *_syncMgrObj;
};

} /* namespace httpwrapper */
} /* namespace srch2 */
#endif /* __SHARDING_SYNCHRONIZERMANAGER_H__ */
