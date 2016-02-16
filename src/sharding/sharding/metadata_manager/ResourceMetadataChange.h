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
#ifndef __SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__
#define __SHARDING_SHARDING_RESOURCE_METADATA_CHANGE_H__

#include "Shard.h"
#include "Cluster_Writeview.h"
#include "../../configuration/ShardingConstants.h"
#include "core/util/Assert.h"

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
	string toNameString() const;
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
