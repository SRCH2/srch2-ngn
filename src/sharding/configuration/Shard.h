#ifndef __SHARDING_CONFIGURATION_SHARD_H__
#define __SHARDING_CONFIGURATION_SHARD_H__

#include <map>
#include <vector>
#include <set>
#include "src/core/util/Assert.h"
#include "CoreInfo.h"
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>




using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;

class ShardId {
public:
	unsigned coreId;
	unsigned partitionId; // ID for a partition, numbered 0, 1, 2, ...

	// ID for a specific primary/replica for a partition; assume #0 is always the primary shard.  For V0, replicaId is always 0
	unsigned replicaId;

	bool isPrimaryShard() const;
	std::string toString() const;

	ShardId() ;
	ShardId(unsigned coreId, unsigned partitionId, unsigned replicaId=0) ;

	bool operator==(const ShardId& rhs) const ;
	bool operator!=(const ShardId& rhs) const ;
	bool operator>(const ShardId& rhs) const ;
	bool operator<(const ShardId& rhs) const ;
	bool operator>=(const ShardId& rhs) const ;
	bool operator<=(const ShardId& rhs) const ;

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer) const;

    //given a byte stream recreate the original object
    void * deserialize(void* buffer) const;

    unsigned getNumberOfBytes() const;

};

class ShardIdComparator {
public:
	// returns s1 > s2
	bool operator() (const ShardId s1, const ShardId s2);
};


/*
 *     //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serializeForNetwork(void * buffer);

    //given a byte stream recreate the original object
    static Shard * deserializeForNetwork(void* buffer);

    unsigned getNumberOfBytesForNetwork();
 */

class Shard{
public:
	Shard(boost::shared_ptr<Srch2Server> srch2Server, double load = 0);
	double getLoad() const;
	boost::shared_ptr<Srch2Server> getSrch2Server() const;
	virtual string getShardIdentifier() const = 0;
	virtual ~Shard(){};
private:
	boost::shared_ptr<Srch2Server> srch2Server;
	double load;
};


class ClusterShard : public Shard{
public:
	ClusterShard(const ShardId shardId, const NodeId nodeId,
			boost::shared_ptr<Srch2Server> srch2Server, double load = 0);
	const NodeId getNodeId() const ;
	const ShardId getShardId() const ;
	string getShardIdentifier() const {
		return shardId.toString();
	}

	bool operator==(const ClusterShard & right) const{
		return (this->shardId == right.shardId)&&(this->nodeId == right.nodeId);
	}

private:
	const ShardId shardId;
	const NodeId nodeId; // this must always be equal to currentNodeId
};

class NodeShard : public Shard{
public:
	NodeShard(const unsigned coreId, const unsigned internalPartitionId,
			const NodeId nodeId, boost::shared_ptr<Srch2Server> srch2Server,double load = 0);
	const unsigned getCoreId() const;
	const unsigned getInternalPartitionId() const ;
	const NodeId getNodeId() const ;
	string getShardIdentifier() const {
		ShardId tmpShardId(coreId, internalPartitionId, 0);
		return "NodeShard-" + tmpShardId.toString();
	}
private:
	const unsigned coreId;
	const unsigned internalPartitionId;
	const NodeId nodeId;
};

class NodeTargetShardInfo{
public:
	NodeTargetShardInfo(const NodeId nodeId, const unsigned coreId);

	void addClusterShard(ShardId shardId);
	void addNodeShard(unsigned internalPartitionId);

	const unsigned getCoreId() const;
	const NodeId getNodeId() const;
	vector<ShardId> getTargetClusterShards() const;
	vector<unsigned> getTargetNodeInternalPartitions() const;

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer);

    unsigned getNumberOfBytes() const;

    //given a byte stream recreate the original object
    void * deserialize(void* buffer);

private:
	NodeId nodeId;
	unsigned coreId;
	vector<ShardId> targetClusterShards;
	vector<unsigned> targetNodeInternalPartitions;

};

class LocalShardContainer{
public:

	LocalShardContainer(const unsigned coreId, const NodeId nodeId);

	const unsigned getCoreId() const	;
	const NodeId getNodeId() const;
	void getShards(const NodeTargetShardInfo & targets, vector<const Shard *> & shards) const;

	void addClusterShard(const ShardId shardId, const NodeId nodeId,
			boost::shared_ptr<Srch2Server> srch2Server, double load){
		if(localClusterShards.find(shardId.partitionId) == localClusterShards.end()){
			localClusterShards[shardId.partitionId] = vector<ClusterShard *>();
		}
		ClusterShard * newClusterShard = new ClusterShard(shardId, nodeId, srch2Server, load);
		for(unsigned sid = 0 ; sid < localClusterShards[shardId.partitionId].size(); ++sid){
			if(*(localClusterShards[shardId.partitionId].at(sid)) == *newClusterShard ){
				ASSERT(false);
				return;
			}
		}
		localClusterShards[shardId.partitionId].push_back(newClusterShard);
	}


	void addNodeShard(const unsigned coreId, const unsigned internalPartitionId,
			const NodeId nodeId, boost::shared_ptr<Srch2Server> srch2Server,double load){
		if(localNodeShards.find(internalPartitionId) == localNodeShards.end()){
			localNodeShards[internalPartitionId] = new NodeShard(coreId, internalPartitionId, nodeId, srch2Server, load);
			return;
		}
		ASSERT(false);
		return;
	}

private:
	const unsigned coreId;
	const NodeId nodeId; // always equal to current node Id
	// partitionId => list of shards on this node
	map<unsigned, vector<ClusterShard *> > localClusterShards;
	// node internal partitionid => nodeshard
	map<unsigned, NodeShard * > localNodeShards;
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARD_H__
