#ifndef __SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__
#define __SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__

#include "core/util/Assert.h"
#include "sharding/configuration/ShardingConstants.h"
#include "Shard.h"
#include "Cluster_Writeview.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class MetadataChange {
public:
	virtual ~MetadataChange(){};
	virtual void * serialize(void * buffer) const = 0;
	virtual unsigned getNumberOfBytes() const = 0;
	virtual void * deserialize(void * buffer) = 0;

	virtual bool doChange(Cluster_Writeview * metadata) = 0;
	virtual MetadataChangeType getType() const = 0;
	virtual bool operator==(const MetadataChange & right) = 0;
	virtual string toString() const = 0;
};

class NodeAddChange : public MetadataChange {
public:
	NodeAddChange(NodeId newNodeId, const vector<ClusterShardId> & localClusterShardIds,
			const vector<NodeShardId> & localNodeShardIds);
	NodeAddChange();
	NodeAddChange(const NodeId & newNodeId);
	NodeAddChange(const NodeAddChange & copy);
	bool doChange(Cluster_Writeview * metadata);
	MetadataChangeType getType() const;

	vector<ClusterShardId> & getLocalClusterShardIds();
	vector<NodeShardId> & getLocalNodeShardIds();

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);

	bool operator==(const MetadataChange & right);
	NodeAddChange & operator=(const NodeAddChange & rhs);

	string toString() const;

private:
	NodeId newNodeId;

	vector<ClusterShardId> localClusterShardIds;
	vector<NodeShardId> localNodeShardIds;
};
class ShardAssignChange : public MetadataChange {
public:
	ShardAssignChange();
	ShardAssignChange(ClusterShardId logicalShard, NodeId location, double load = 0);
	ShardAssignChange(const ShardAssignChange & change);

	MetadataChangeType getType() const;
	bool doChange(Cluster_Writeview * metadata);


	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool operator==(const MetadataChange & right);
	ShardAssignChange & operator=(const ShardAssignChange & rhs);
	void setPhysicalShard(const LocalPhysicalShard & physicalShard){
		this->physicalShard = physicalShard;
	}

	string toString() const;

private:
	ClusterShardId logicalShardToAssign;
	NodeId location;
	double load;
	LocalPhysicalShard physicalShard;
};

class ShardMoveChange : public MetadataChange{
public:


	ShardMoveChange(ClusterShardId shardId, NodeId srcNodeId, NodeId destNodeId);
	ShardMoveChange();
	ShardMoveChange(const ShardMoveChange & change);

	MetadataChangeType getType() const;
	bool doChange(Cluster_Writeview * metadata);

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool operator==(const MetadataChange & right);
	ShardMoveChange & operator=(const ShardMoveChange & rhs);
	ClusterShardId getShardId() const {
		return shardId;
	}
	NodeId getSrcNodeId() const {
		return srcNodeId;
	}
	NodeId getDestNodeId() const {
		return destNodeId;
	}
	LocalPhysicalShard getPhysicalShard(){
		return physicalShard;
	}
	void setPhysicalShard(const LocalPhysicalShard & physicalShard){
		this->physicalShard = physicalShard;
	}

	string toString() const;

private:
	ClusterShardId shardId;
	NodeId srcNodeId, destNodeId;
	LocalPhysicalShard physicalShard;
};

// if a shard doesn't have physical shard, it igShardingChangeTypeLockingnores it ...
class ShardLoadChange : public MetadataChange{
public:

	ShardLoadChange(const map<ClusterShardId, double> & addedLoads);
	ShardLoadChange();
	ShardLoadChange(const ShardLoadChange & change);
	MetadataChangeType getType() const;

	bool doChange(Cluster_Writeview * metadata);

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool operator==(const MetadataChange & right);
	ShardLoadChange & operator=(const ShardLoadChange & rhs);

	string toString() const;

	map<ClusterShardId, double> getAddedLoads() const;

private:
	map<ClusterShardId, double> addedLoads;
};


}
}

#endif //__SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__
