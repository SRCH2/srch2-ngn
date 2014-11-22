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
	const unsigned coreId;
	const unsigned partitionId;
	PartitionLockValue partitionLock;
	// nodeid -> list of replicas of this partition that reside on that node
	map<NodeId, vector<unsigned> > replicaLocations;
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
	const unsigned coreId;
	const NodeId nodeId;
	double load;
	vector<unsigned> nodeInternalPartitionIds;
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
		const_cast<unsigned>(this->coreId) = left.coreId;
		const_cast<unsigned>(this->totalNumberOfPartitions) = left.totalNumberOfPartitions;
		const_cast<unsigned>(this->replicationDegree) = left.replicationDegree;

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

	const ClusterPartition * getClusterPartition(unsigned partitionId) const;
	const NodePartition * getNodePartition(unsigned nodeId) const;
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
	const unsigned coreId;
	const unsigned totalNumberOfPartitions;
	const unsigned replicationDegree;

	// partitionId => clusterPartition
	map<unsigned, ClusterPartition *> clusterPartitions;
	// NodeId => nodePartition
	map<NodeId, NodePartition *> nodePartitions;
};

}
}

#endif //
