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

class LoadBalancingStartOperation : public OperationState{
public:
	LoadBalancingStartOperation():OperationState(OperationState::getNextOperationId()){}

	OperationState * entry();

	OperationState * handle(LoadBalancingReport * n);
	OperationState * handle(NodeFailureNotification * n);

	OperationState * handle(Notification * n);
	static OperationState * finalizeLoadBalancing();

	string getOperationName() const ;
	string getOperationStatus() const ;

private:

	map<NodeId, double> nodeLoads;
	map<NodeId, bool> nodeReportArrived;
	bool haveAllReportsArrived();

	OperationState * tryShardMove();

	OperationState * tryShardAssginmentAndShardCopy();


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

	bool isLightLoadedNode(NodeId nodeId);

	// The first shard of a partition
	OperationState * assignShard(const ClusterShardId & unassignedShard);
	OperationState * replicateShard(const ClusterShardId & unassignedShard, NodeId srcNodeId, const ClusterShardId & shardToReplicate);

	OperationState * moveShard(const NodeId & srcNodeId, const ClusterShardId & moveShardId);

};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__
