#ifndef __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__
#define __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/LoadBalancingReport.h"
#include "../metadata_manager/Shard.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


// only non pending partitions
struct AssignCandidatePartition{
	AssignCandidatePartition(const ClusterPID & pid);
	ClusterPID pid;
	// all ready replicas, some of them may not be available
	vector<std::pair<ClusterShardId, NodeId> > readyReplicas;
	// any of these shards can be used for assignment
	// it must not be empty.
	// we must be able to acquire S lock on these shards
	vector<ClusterShardId> assignCandidateReplicas;
	unsigned getAttentionNeedScore() const;
	void removeUnavailableReadyReplicas();
	bool hasReplicaOnNode(NodeId nodeId) const;
	inline bool operator()(const AssignCandidatePartition& a, const AssignCandidatePartition& b);
};

class LoadBalancer : public Transaction, public AggregatorCallbackInterface, public BooleanCallbackInterface{
public:
	static void run();


	~LoadBalancer();
private:
	LoadBalancer();
	// tries to balance the shard placement on cluster nodes
	void balance();

	// asks other nodes about load information
	void collectInfo();
	void receiveReplies(map<NodeOperationId , ShardingNotification *> replies);
	void abort(int error_code);

	// decides on shard copy or shard movements
	void _balance();

	// receives the results of ShardAssign, ShardMove and ShardCopy
	void finish(bool done);

	TRANS_ID lastCallback(void *);

	ShardingTransactionType getTransactionType();

	void finalize();

	LoadBalancingReport::REQUEST * reportReq;

	map<NodeId, double> nodeLoads;
	bool finalizedFlag;


	ShardAssignOperation * shardAssigner;
	ShardCopyOperation * shardCopyer;
	ShardMoveOperation * shardMover;

	void tryShardMove();

	void tryShardAssginmentAndShardCopy(bool assignOnly = false);


	/*
     *          I)   The shard is UNASSIGNED
	 *  		II)  the partition of the shard is not pending.
	 *  		III) acquiring X lock on local node on this shard
	 *  		     is possible (don't acquire, just check.)
	 *  		IV)  The second value of pair is the list of all nodes
	 *  		     that have a READY replica of that ClusterShardId
	 *  		     *** if that vector is empty, this partition is UNASSIGNED
	 */
	void prepareAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates);

	void deleteAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates);
	/*
	 * I)   The shard is READY
	 * II)  There is no replica of the shard on current node
	 * III) There is no X or U lock on local repository.
	 * IV)  This shard is not the only READY shard of its partition
	 * V)   The srcNodeId must have a load greater than or equal to 2 + local load
	 */
	void prepareMoveCandidates(vector<std::pair<NodeId, ClusterShardId> > & candidateSrcShards);

	bool canAcceptMoreShards(NodeId nodeId);

	// The first shard of a partition
	ShardAssignOperation * assignShard(const ClusterShardId & unassignedShard);
	ShardCopyOperation * replicateShard(const ClusterShardId & unassignedShard, NodeId srcNodeId, const ClusterShardId & shardToReplicate);

	ShardMoveOperation * moveShard(const NodeId & srcNodeId, const ClusterShardId & moveShardId);

};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__
