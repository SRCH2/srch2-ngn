#ifndef __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__
#define __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__

#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "../AtomicLock.h"
#include "../AtomicRelease.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Saves the indices and the cluster metadata on all nodes in the cluster.
 * NOTE : this operation assumes all shards are locked in S mode
 * 1. request all nodes to save their indices
 * 2. When all nodes saved their indices, request all nodes to save their cluster metadata
 * 3. When all nodes acked metadata save, write the metadata on disk and done.
 */
class ShardCommand: public ProducerInterface, public NodeIteratorListenerInterface {
public:

	ShardCommand(ConsumerInterface * consumer,
			unsigned coreId = (unsigned)-1, ShardCommandCode commandCode = ShardCommandCode_Merge,
			const string & filePath = "");
	~ShardCommand();

	SP(Transaction) getTransaction(){
		if(this->getConsumer() == NULL){
			ASSERT(false);
			return SP(Transaction)();
		}
		return this->getConsumer()->getTransaction();
	}

	void produce();

	// process coming back from distributed conversation to aggregate the results of
	// this command
	void end_(map<NodeOperationId , SP(ShardingNotification)> & replies);
	string getName() const {return "shard-command";};

	void consume(bool granted);

private:
	void performCommand();
	void lock();
	void release();

	bool computeTargets(vector<NodeTargetShardInfo> & targets);
	const unsigned coreId;
	ShardCommandCode commandCode;
	vector<NodeTargetShardInfo> targets;
	vector<std::pair<SP(ShardingNotification) , NodeId> > notifications;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

	string filePath ; // holds either jsonFilePath or the newLogFilePath

	bool dataSavedFlag;

	AtomicLock * locker;
	AtomicRelease * releaser;
	NodeOperationId currentOpId;
	map<NodeOperationId , SP(ShardingNotification)> aggregatedResult;

	bool isSaveSuccessful(map<NodeOperationId , SP(ShardingNotification)> & replies) const;

	void finalize(map<NodeOperationId , SP(ShardingNotification)> & replies);


	void getSortedListOfClusterShardIDs(vector<ClusterShardId> & shardIds){
		for(unsigned i = 0 ; i < targets.size(); ++i){
			vector<ClusterShardId> clusterShards = targets.at(i).getTargetClusterShards();
			shardIds.insert(shardIds.begin(), clusterShards.begin(), clusterShards.end());
		}
		std::sort(shardIds.begin(), shardIds.end());
	}

};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__
