/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__
#define __SHARDING_SHARDING_LOAD_BALANCING_START_OPERATION_H__

#include "../Transaction.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/LoadBalancingReport.h"
#include "../../metadata_manager/Shard.h"

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

class ShardAssignOperation;
class ShardCopyOperation;
class ShardMoveOperation;

class LoadBalancer : public WriteviewTransaction, public NodeIteratorListenerInterface{
public:
	static void runLoadBalancer();


	~LoadBalancer();

	LoadBalancer();

	SP(Transaction) getTransaction(){
		return sharedPointer;
	}

	// tries to balance the shard placement on cluster nodes
	void run();

	// asks other nodes about load information
	void collectInfo();
	void end_(map<NodeOperationId, SP(ShardingNotification) > & _replies);

	// decides on shard copy or shard movements
	void balance();

	// receives the results of ShardAssign, ShardMove and ShardCopy
	void consume(bool done);

	ShardingTransactionType getTransactionType();

	void finalizeWork(Transaction::Params * arg);

	string getName() const {return "load-balancer";};

private:
	enum CurrentOperationType{
		PreStart,
		CollectInfo,
		Assign,
		Copy,
		Move
	};

	SP(LoadBalancingReport::REQUEST) reportReq;

	map<NodeId, double> nodeLoads;

	CurrentOperationType currentOp;

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
