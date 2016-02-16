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
#ifndef __SHARDING_CONFIGURATION_PARTITION_H__
#define __SHARDING_CONFIGURATION_PARTITION_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "Shard.h"
#include "../../configuration/ShardingConstants.h"
#include "core/util/Assert.h"


#include <vector>
#include <map>

using namespace std;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {



class ClusterPartition{
public:
	ClusterPartition(const unsigned coreId, const unsigned partitionId, PartitionLockValue pLockValue);

	bool isPartitionLocked() const;
	void getShardLocations(vector<NodeId> & shardLocations) const;
	// returns false of there is no replica of this partition in this node
	bool getNodeReplicaIds(NodeId nodeId, vector<unsigned> & replicas) const;
	const unsigned getCoreId() const;
	const unsigned getPartitionId() const;

	void setPartitionLock(PartitionLockValue lockValue);
	void addPhysicalReplica(NodeId nodeId, unsigned replicaId);
	string toString() const;
private:
	const uint32_t coreId;
	const uint32_t partitionId;
	PartitionLockValue partitionLock;
	// nodeid -> list of replicas of this partition that reside on that node
	map<NodeId, vector<uint32_t> > replicaLocations;
	// TODO : next to each replicaId, we can also have a load of shard
};

class NodePartition{
public:
	NodePartition(const unsigned coreId, const NodeId nodeId, double load);

	double getLoad() const;
	void getInternalPartitionIds(vector<unsigned> & nodeInternalPartitionIds) const;
	void addInternalPartitionId(unsigned internalPartitionId);
	const unsigned getCoreId() const	;
	const NodeId getNodeId() const;
	string toString() const ;
private:
	const uint32_t coreId;
	const NodeId nodeId;
	double load;
	vector<uint32_t> nodeInternalPartitionIds;
};


class CorePartitionContianer{
public:
	CorePartitionContianer(const unsigned coreId,
			const unsigned totalNumberOfPartitions,
			const unsigned replicationDegree):coreId(coreId),
			totalNumberOfPartitions(totalNumberOfPartitions),replicationDegree(replicationDegree){
		// we will have totalNumberOfPartitions primary shardLocations and
		// replicationDegree replicas for each primary shard
		for(unsigned pid = 0 ; pid < totalNumberOfPartitions ; ++pid){
			this->clusterPartitions[pid] = new ClusterPartition(coreId, pid, PartitionLock_Unlocked);
		}
	}

	CorePartitionContianer(const CorePartitionContianer & left):
		coreId(coreId), totalNumberOfPartitions(totalNumberOfPartitions), replicationDegree(replicationDegree){

		for(map<unsigned, ClusterPartition *>::const_iterator cPartItr = left.clusterPartitions.begin();
				cPartItr != left.clusterPartitions.end(); ++cPartItr){
			this->clusterPartitions[cPartItr->first] = new ClusterPartition(*(cPartItr->second));
		}

		for(map<NodeId, NodePartition *>::const_iterator cPartItr = left.nodePartitions.begin();
				cPartItr != left.nodePartitions.end(); ++cPartItr){
			this->nodePartitions[cPartItr->first] = new NodePartition(*(cPartItr->second));
		}
	}

	CorePartitionContianer & operator=(const CorePartitionContianer & left){
		if(this != &left){
			return *this;
		}
		*(const_cast<unsigned *>(&(this->coreId))) = left.coreId;
		*(const_cast<unsigned *>(&(this->totalNumberOfPartitions))) = left.totalNumberOfPartitions;
		*(const_cast<unsigned *>(&(this->replicationDegree))) = left.replicationDegree;

		for(map<unsigned, ClusterPartition *>::const_iterator cPartItr = left.clusterPartitions.begin();
				cPartItr != left.clusterPartitions.end(); ++cPartItr){
			this->clusterPartitions[cPartItr->first] = new ClusterPartition(*(cPartItr->second));
		}

		for(map<NodeId, NodePartition *>::const_iterator cPartItr = left.nodePartitions.begin();
				cPartItr != left.nodePartitions.end(); ++cPartItr){
			this->nodePartitions[cPartItr->first] = new NodePartition(*(cPartItr->second));
		}

		return *this;
	}

	~CorePartitionContianer(){
		for(map<unsigned, ClusterPartition *>::iterator cPartItr = this->clusterPartitions.begin();
				cPartItr != this->clusterPartitions.end(); ++cPartItr){
			delete this->clusterPartitions[cPartItr->first];
		}

		for(map<NodeId, NodePartition *>::iterator cPartItr = this->nodePartitions.begin();
				cPartItr != this->nodePartitions.end(); ++cPartItr){
			delete this->nodePartitions[cPartItr->first];;
		}
	}
	void addClusterShard(NodeId nodeId, ClusterShardId shardId);
	void addNodeShard(NodeId nodeId, unsigned nodeInternalPartitionId);
	void setPartitionLock(unsigned partitionLock, PartitionLockValue lockValue);
	bool isCoreLocked() const; // if at least one partition is locked

	const ClusterPartition * getClusterPartition(uint32_t partitionId) const;
	const NodePartition * getNodePartition(NodeId nodeId) const;
	const unsigned getCoreId() const	;
	const unsigned getTotalNumberOfPartitions() const;
	const unsigned getReplicationDegree() const;
	void getInvolvedNodes(const ClusterPID pid, vector<NodeId> & nodes) const;

	void getClusterPartitionsForRead(vector<const ClusterPartition *> & clusterPartitions) const;
	void getNodePartitionsForRead(vector<const NodePartition *> & nodePartitions) const;
	const ClusterPartition * getClusterPartitionForWrite(unsigned hashKey) const;
	const NodePartition * getNodePartitionForWrite(unsigned hashKey, NodeId nodeId) const;
	void print() const;

private:
	const uint32_t coreId;
	const uint32_t totalNumberOfPartitions;
	const uint32_t replicationDegree;

	// partitionId => clusterPartition
	map<uint32_t, ClusterPartition *> clusterPartitions;
	// NodeId => nodePartition
	map<NodeId, NodePartition *> nodePartitions;
};

}
}

#endif //
