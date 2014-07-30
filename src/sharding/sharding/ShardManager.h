#ifndef __SHARDING_SHARDING_SHARDMANAGER_H__
#define __SHARDING_SHARDING_SHARDMANAGER_H__

#include "core/util/Assert.h"


#include "MetadataManager.h"
#include "LockManager.h"
#include "LoadBalancer.h"
#include "NodeInitializer.h"

#include "sharding/transport/CallbackHandler.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


////////////////////////////////////////////////////////////////////////////////////
// Temporary : will be replaced with what comes from Migration project
enum MIGRATION_STATUS{
	FAIL,
	FINISH
};
struct ShardMigrationStatus{
	unsigned srcOperationId;    // #1
    unsigned dstOperationId;   // #7
	NodeId sourceNodeId;   // NodeA
	NodeId destinationNodeId;   // Current Node
	boost::shared_ptr<Srch2Server> shard;
	MIGRATION_STATUS status;
};
////////////////////////////////////////////////////////////////////////////////////

/*
 * This class is the main responsible class to do operations on shards of data in the cluster.
 * For example, Load balancing upon arrival of new nodes or failure of existing nodes is handled in this class.
 */
class ShardManager : public CallBackHandler{
public:

	static ShardManager * createShardManager(TransportManager * transportManager, ConfigManager * configManager);
	static ShardManager * getShardManager();

	ShardManager(TransportManager * transportManager, ConfigManager * configManager);
	~ShardManager();

	// callback provided to TransportManager, it deserializes the messages and passes them to
	// notification resolve methods
	bool resolveMessage(Message * msg, NodeId node);
	// called from destructor of readview to notify shard manager that the readview is released and
	// no more readers are using it
	void resolveReadviewRelease(unsigned metadataVersion);
	// called from migration manager to inform us about the status of a migration
	void resolveMMNotification(ShardMigrationStatus migrationStatus);
	// called from SM to inform us about the arrival or failure of a node
	void resolveSMNodeArrival(const Node & newNode);
	void resolveSMNodeFailure(const NodeId failedNodeId);



	// getter functions
	TransportManager * getTransportManager() const;
	ConfigManager * getConfigManager() const;


	// saves the change in the broadcastHistory
	void registerChangeBroadcast(ShardingChange * change);
	// when the ACK for each change is received from all nodes,
	// flush is called to remove elements from this buffer up to the point
	// that ACK guarantees that other nodes have this change.
	void flushChangeHistory(unsigned ackedChangeId);
	// sends this sharding notification to destination using TM
	bool send(ShardingNotification * notification);

private:

	static ShardManager * singleInstance;

	TransportManager * transportManager;
	ConfigManager * configManager;

	// This buffer stores the list of all broadcasted changes,
	std::queue<ShardingChange *> broadcastHistoryBuffer;
	// this map contains the latest changeId received from each other node
	// nodeId->changeId
	std::map<NodeId, unsigned> latestChangesReceived;
	unsigned nextChangeId;
	void updateNodeLatestChange(unsigned newChangeId, NodeId senderNodeId);




	boost::mutex cancelLock;
	bool cancelled;
	static void * execute(void * args);
	void handleAll(); // called periodically to handle all notifications in buffers
	void handleFailedNodes(); // called from handleAll to take care of those operations that are affected by a node failure


	void resolve(CommitNotification * commitNotification);
	void resolve(CommitNotification::ACK * commitAckNotification);
	void resolve(LockingNotification * lockingNotification);
	void resolve(LockingNotification::GRANTED * granted);
	void resolve(LockingNotification::REJECTED * rejected);
	void resolve(NodeInitNotification::WELCOME * welcome);
	void resolve(NodeInitNotification::BUSY * busy);
	void resolve(NodeInitNotification::NEW_HOST * newHost);
	void resolve(NodeInitNotification::SHARD_REQUEST * shardRequest);
	void resolve(NodeInitNotification::SHARD_OFFER * shardOffer);
	void resolve(NodeInitNotification::SHARDS_READY * shardsReady);
	void resolve(NodeInitNotification::JOIN_PERMIT * joinPermit);
	void resolve(CopyToMeNotification * copyToMeNotification);
	void resolve(MoveToMeNotification * moveToMeNotification);
	void resolve(ProposalNotification * proposal);
	void resolve(ProposalNotification::OK * proposalAck);
	void resolve(ProposalNotification::NO * proposalNo);




};


}
}

#endif // __SHARDING_SHARDING_SHARDMANAGER_H__
