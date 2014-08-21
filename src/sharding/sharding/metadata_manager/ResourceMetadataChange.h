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
};

class NodeAddChange : public MetadataChange {
public:
	NodeAddChange(NodeId newNodeId, const vector<ClusterShardId> & localClusterShardIds,
			const vector<NodeShardId> & localNodeShardIds){

		this->newNodeId = newNodeId;
		this->localClusterShardIds = localClusterShardIds;
		this->localNodeShardIds = localNodeShardIds;
	};
	NodeAddChange(){};
	NodeAddChange(const NodeId & newNodeId){
		this->newNodeId = newNodeId;
	};
	NodeAddChange(const NodeAddChange & copy){
		newNodeId = copy.newNodeId;
		localClusterShardIds = copy.localClusterShardIds;
		localNodeShardIds = copy.localNodeShardIds;
	}
	bool doChange(Cluster_Writeview * metadata){
		if(metadata == NULL){
			ASSERT(false);
			return false;
		}

		ClusterShardId id;
		NodeShardId nodeShardId;
		ShardState state;
		bool isLocal;
		NodeId nodeId;
		LocalPhysicalShard physicalShard;
		double load;
		metadata->beginClusterShardsIteration();
		while(metadata->getNextClusterShard(id, load, state, isLocal, nodeId)){
			if(state != SHARDSTATE_READY){
				if(std::find(localClusterShardIds.begin(), localClusterShardIds.end(), id) !=
						localClusterShardIds.end()){
					metadata->assignExternalClusterShard(id, newNodeId, 0);
				}
			}
		}

		for(unsigned i = 0 ; i < this->localNodeShardIds.size() ; ++i){
		    metadata->addExternalNodeShard(this->localNodeShardIds.at(i),1);
		}

		return true;
	}
	MetadataChangeType getType() const{
		return ShardingChangeTypeNodeAdd;
	}

	vector<ClusterShardId> & getLocalClusterShardIds(){
		return localClusterShardIds;
	}
	vector<NodeShardId> & getLocalNodeShardIds(){
		return localNodeShardIds;
	}

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);

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

	void setPhysicalShard(const LocalPhysicalShard & physicalShard){
		this->physicalShard = physicalShard;
	}

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
private:
	ClusterShardId shardId;
	NodeId srcNodeId, destNodeId;
	LocalPhysicalShard physicalShard;
};

// if a shard doesn't have physical shard, it igShardingChangeTypeLockingnores it ...
class ShardLoadChange : public MetadataChange{
public:

	ShardLoadChange();
	ShardLoadChange(const ShardLoadChange & change);

	MetadataChangeType getType() const;

	bool doChange(Cluster_Writeview * metadata);

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);

	map<ClusterShardId, double> getAddedLoads() const{
		return addedLoads;
	}

private:
	map<ClusterShardId, double> addedLoads;
};


}
}

#endif //__SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__
