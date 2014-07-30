#ifndef __SHARDING_SHARDING_METADATA_H__
#define __SHARDING_SHARDING_METADATA_H__


#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/ShardingConstants.h"
#include "Resources.h"



#include <sstream>
#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


/*
 * This class is in fact the writeview of the metadata of the system. This writeview is committed
 * at different points of time to be accessed by other modules of the system.
 * Note: This class is just a data structure and we try to put as less logic as possible in there
 * that's why all members are public.
 */

struct PhysicalShard{
	NodeId nodeLocation;
	double shardLoad;
	PhysicalShard(	NodeId nodeLocation,double shardLoad){
		this->nodeLocation = nodeLocation;
		this->shardLoad = shardLoad;
	}
	PhysicalShard(){
		this->nodeLocation = 0;
		this->shardLoad = 0;
	}
	PhysicalShard(const PhysicalShard & shard){
		this->nodeLocation = shard.nodeLocation;
		this->shardLoad = shard.shardLoad;
	}
	void * serialize(void * buffer) const{
		buffer = srch2::util::serializeFixedTypes(this->nodeLocation, buffer);
		return srch2::util::serializeFixedTypes(this->shardLoad, buffer);
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += sizeof(NodeId);
		numberOfBytes += sizeof(double);
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		buffer = srch2::util::deserializeFixedTypes(buffer, this->nodeLocation);
		return srch2::util::deserializeFixedTypes(buffer, this->shardLoad);
	}
};

class  NodeShardId{
public:
	NodeShardId(unsigned coreId, NodeId nodeId, unsigned partitionId){
		this->coreId = coreId;
		this->nodeId = nodeId;
		this->partitionId = partitionId;
	}

	unsigned coreId;
	NodeId nodeId;
	unsigned partitionId;



	std::string toString() const{
		std::stringstream sstm;
		sstm << "C" << coreId << "_N" << nodeId << "_P" << partitionId ;
		return sstm.str();
	}

	bool operator==(const NodeShardId& rhs) const ;
	bool operator!=(const NodeShardId& rhs) const ;
	bool operator>(const NodeShardId& rhs) const ;
	bool operator<(const NodeShardId& rhs) const ;
	bool operator>=(const NodeShardId& rhs) const ;
	bool operator<=(const NodeShardId& rhs) const ;

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer) const;

    //given a byte stream recreate the original object
    void * deserialize(void* buffer) const;

    unsigned getNumberOfBytes() const;
};


/*
 * 	string clusterName;

	// coreId => core *
	map<unsigned, CoreInfo_t *> allCores;
	// coreId => core partitions
	map<unsigned, CorePartitionContianer *> corePartitioners;
	// coreId => local core shards
	map<unsigned, LocalShardContainer *> localShardContainers;


	// nodeId => node
	map<NodeId, Node> allNodes;

	// this node id
	NodeId currentNodeId;
 */
class ClusterResourceMetadata_Writeview{
public:
	ClusterResourceMetadata_Writeview(const ClusterResourceMetadata_Readview & readview);
	ClusterResourceMetadata_Readview * getNewReadview() const;
	bool isPartitionLocked(unsigned coreId, unsigned partitionId) const;

	const string clusterName;
	const NodeId currentNodeId;
	unsigned versionId;
	map<ShardId, PhysicalShard > physicalShards;
	map<ShardId, ShardState> shardStates;
	map<ShardId, boost::shared_ptr<Srch2Server> > localDataShards;
	// we only use coreId and partitionId of key ShardId
	map<ShardId, bool> partitionLocks;
	vector<NodeShardId> nodeShards;
	map<NodeShardId,  boost::shared_ptr<Srch2Server> > localNodeDataShards;

	map<NodeId, Node> nodes;
	map<unsigned, CoreInfo_t *> cores;



};

enum MetadataChangeType{
	MetadataChangeType_LockChange,
	MetadataChangeType_ShardAssignChange,
	MetadataChangeType_ShardCopyChange,
	MetadataChangeType_ShardMoveChange,
	MetadataChangeType_ShardLoadChange
};

class MetadataChange : public ShardingChange{
public:
	bool apply(bool doOrUndo, ClusterResourceMetadata_Writeview * metadata){
		if(doOrUndo){
			return doChange(metadata);
		}else{
			return undoChange(metadata);
		}
	}
	virtual bool doChange(ClusterResourceMetadata_Writeview * metadata) = 0;
	virtual bool undoChange(ClusterResourceMetadata_Writeview * metadata) = 0;
	virtual MetadataChangeType getType() = 0;
	virtual ~MetadataChange(){};
};

class MetadataLockChange : public MetadataChange{
public:
	MetadataLockChange(const map<ShardId, bool> & locks){
		this->locks = locks;
	}
	bool doChange(ClusterResourceMetadata_Writeview * metadata){
		if(metadata == NULL){
			ASSERT(false);
			return false;
		}
		for(map<ShardId, bool>::iterator lockItr = locks.begin(); lockItr != locks.end(); ++lockItr){
			if(metadata->partitionLocks.find(lockItr->first) == metadata->partitionLocks.end()){
				ASSERT(false);
				return false;
			}
			locksOldValues[lockItr->first] = metadata->partitionLocks.find(lockItr->first)->second;
			metadata->partitionLocks.find(lockItr->first)->second = lockItr->second;
		}

		return true;
	}
	bool undoChange(ClusterResourceMetadata_Writeview * metadata){
		if(metadata == NULL){
			ASSERT(false);
			return false;
		}
		for(map<ShardId, bool>::iterator lockItr = locksOldValues.begin(); lockItr != locksOldValues.end(); ++lockItr){
			if(metadata->partitionLocks.find(lockItr->first) == metadata->partitionLocks.end()){
				ASSERT(false);
				return false;
			}
			metadata->partitionLocks.find(lockItr->first)->second = lockItr->second;
		}
		return true;
	}
	MetadataChangeType getType(){
		return MetadataChangeType_LockChange;
	}

	void * serialize(void * buffer) const{
		ShardingChange::serialize(buffer);
		buffer = srch2::util::serializeFixedTypes((unsigned)(locks.size()), buffer);
		for(map<ShardId, bool>::iterator lockItr = locks.begin(); lockItr != locks.end(); ++lockItr){
			buffer = lockItr->first.serialize(buffer);
			buffer = srch2::util::serializeFixedTypes(lockItr->second, buffer);
		}
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += ShardingChange::getNumberOfBytes();
		numberOfBytes += sizeof(unsigned); // size of locks
		for(map<ShardId, bool>::iterator lockItr = locks.begin(); lockItr != locks.end(); ++lockItr){
			numberOfBytes += lockItr->first.getNumberOfBytes(); // key
			numberOfBytes += sizeof(bool); // value
		}
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		ShardingChange::deserialize(buffer);
		unsigned sizeOfMap;
		buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap);
		for(unsigned i = 0 ; i < sizeOfMap; ++i){
			ShardId key;
			bool value;
			buffer = key.deserialize(buffer);
			buffer = srch2::util::deserializeFixedTypes(buffer, value);
			locks[key] = value;
		}
		return buffer;
	}

	ShardingChange * clone(){
		return new MetadataLockChange(*this);
	}
private:
	// {coreId, partitionId} -> lockOrUnlock
	map<ShardId, bool> locks;
	map<ShardId, bool> locksOldValues;
};

class ShardAssignChange : public MetadataChange {
public:
	ShardAssignChange(){
		// temporary initialization
	}
	ShardAssignChange(ShardId logicalShard, NodeId location, double load = 0):physicalShard(location,load){
		logicalShardToAssign = logicalShard;
	}
	ShardAssignChange(const ShardAssignChange & change):physicalShard(change.physicalShard){
		logicalShardToAssign = change.logicalShardToAssign;
	}


	bool doChange(ClusterResourceMetadata_Writeview * metadata);
	bool undoChange(ClusterResourceMetadata_Writeview * metadata);
	MetadataChangeType getType() {
		return MetadataChangeType_ShardAssignChange;
	}
	void setNewLocalDataPointer(boost::shared_ptr<Srch2Server> nextLocalDataPointer){
		this->localDataPointer = nextLocalDataPointer;
	}

	void * serialize(void * buffer) const{
		ShardingChange::serialize(buffer);
		buffer = logicalShardToAssign.serialize(buffer);
		return physicalShard.serialize(buffer);
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += ShardingChange::getNumberOfBytes();
		numberOfBytes += logicalShardToAssign.getNumberOfBytes();
		numberOfBytes += physicalShard.getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		ShardingChange::deserialize(buffer);
		buffer = logicalShardToAssign.deserialize(buffer);
		return physicalShard.deserialize(buffer);
	}

	ShardingChange * clone(){
		return new ShardAssignChange(*this);
	}

private:
	ShardId logicalShardToAssign;
	PhysicalShard physicalShard;
	// only set if we are on destination node ....
	// it's set through setNewLocalDataPointer();
	boost::shared_ptr<Srch2Server> localDataPointer;
	ShardState prevShardState;
};


class ShardCopyChange : public MetadataChange{
public:


	ShardCopyChange(ShardId srcShardId, ShardId destShardId, NodeId srcNodeId, NodeId destNodeId){
		this->srcShardId = srcShardId;
		this->destShardId = destShardId;
		this->srcNodeId = srcNodeId;
		this->destNodeId = destNodeId;
	}
	ShardCopyChange(){
		// temp initialization used for deserializing the object.
	}
	ShardCopyChange(const ShardCopyChange & change){
		this->srcShardId = change.srcShardId;
		this->destShardId = change.destShardId;
		this->srcNodeId = change.srcNodeId;
		this->destNodeId = change.destNodeId;
	}
	bool doChange(ClusterResourceMetadata_Writeview * metadata);
	bool undoChange(ClusterResourceMetadata_Writeview * metadata);
 	MetadataChangeType getType() {
		return MetadataChangeType_ShardCopyChange;
	}
	void setLocalDataPointer(boost::shared_ptr<Srch2Server> localDataPointer){
		this->localDataPointer = localDataPointer;
	}

	~ShardCopyChange(){
		if(this->sac != NULL){
			delete this->sac;
		}
	}

	void * serialize(void * buffer) const{
		buffer = ShardingChange::serialize(buffer);
		buffer = srcShardId.serialize(buffer);
		buffer = destShardId.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
	    return srch2::util::serializeFixedTypes(destNodeId, buffer);
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += ShardingChange::getNumberOfBytes();
		numberOfBytes += srcShardId.getNumberOfBytes();
		numberOfBytes += destShardId.getNumberOfBytes();
		numberOfBytes += sizeof(srcNodeId);
		numberOfBytes += sizeof(destNodeId);
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		buffer = ShardingChange::deserialize(buffer);
		buffer = srcShardId.serialize(buffer);
		buffer = destShardId.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
	    return srch2::util::serializeFixedTypes(destNodeId, buffer);
	}

	ShardingChange * clone(){
		return new ShardCopyChange(*this);
	}
private:
	ShardId srcShardId, destShardId;
	NodeId srcNodeId, destNodeId;
	// only set if we are on destination node ....
	// it's set through setNewLocalDataPointer();
	boost::shared_ptr<Srch2Server> localDataPointer;

	ShardAssignChange * sac;
};


class ShardMoveChange : public MetadataChange{
public:


	ShardMoveChange(ShardId shardId, NodeId srcNodeId, NodeId destNodeId){
		this->shardId = shardId;
		this->srcNodeId = srcNodeId;
		this->destNodeId = destNodeId;
	}
	ShardMoveChange(){
		// temp initialization used for deserializing the object.
	}
	ShardMoveChange(const ShardMoveChange & change){
		this->shardId = change.shardId;
		this->srcNodeId = change.srcNodeId;
		this->destNodeId = change.destNodeId;
	}
	bool doChange(ClusterResourceMetadata_Writeview * metadata);
	bool undoChange(ClusterResourceMetadata_Writeview * metadata);
	MetadataChangeType getType() {
		return MetadataChangeType_ShardMoveChange;
	}
	void setNewLocalDataPointer(boost::shared_ptr<Srch2Server> nextLocalDataPointer){
		this->nextLocalDataPointer = nextLocalDataPointer;
	}

	void * serialize(void * buffer) const{
		buffer = ShardingChange::serialize(buffer);
		buffer = shardId.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
	    return srch2::util::serializeFixedTypes(destNodeId, buffer);
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += ShardingChange::getNumberOfBytes();
		numberOfBytes += shardId.getNumberOfBytes();
		numberOfBytes += sizeof(srcNodeId);
		numberOfBytes += sizeof(destNodeId);
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		buffer = ShardingChange::deserialize(buffer);
		buffer = shardId.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
	    return srch2::util::serializeFixedTypes(destNodeId, buffer);
	}

	ShardingChange * clone(){
		return new ShardMoveChange(*this);
	}
private:
	ShardId shardId;
	NodeId srcNodeId, destNodeId;
	// only gets value after calling doChange if we are in the current node
	// and the shard data is moving to some other node
	boost::shared_ptr<Srch2Server> prevLocalDataPointer;
	// only set if we are on destination node ....
	// it's set through setNewLocalDataPointer();
	boost::shared_ptr<Srch2Server> nextLocalDataPointer;
	PhysicalShard prevPhysicalShard;


};

// if a shard doesn't have physical shard, it ignores it ...
class ShardLoadChange : public MetadataChange{
public:

	ShardLoadChange(){
		// temp initialization used for deserializing the object.
	}
	ShardLoadChange(const ShardLoadChange & change){
		this->addedLoads = change.addedLoads;
	}

	bool doChange(ClusterResourceMetadata_Writeview * metadata);
	bool undoChange(ClusterResourceMetadata_Writeview * metadata);

	MetadataChangeType getType() {
		return MetadataChangeType_ShardLoadChange;
	}
	void * serialize(void * buffer) const{
		buffer = ShardingChange::serialize(buffer);
		buffer += srch2::util::serializeFixedTypes((unsigned)addedLoads.size(), buffer); // size of map
		for(map<ShardId, double>::const_iterator i = addedLoads.begin() ; i != addedLoads.end(); ++i){
			buffer = i->first.serialize(buffer);
			buffer = srch2::util::serializeFixedTypes(i->second, buffer);
		}
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += ShardingChange::getNumberOfBytes();
		numberOfBytes += sizeof(unsigned); // size of map
		for(map<ShardId, double>::const_iterator i = addedLoads.begin() ; i != addedLoads.end(); ++i){
			numberOfBytes += i->first.getNumberOfBytes();
			numberOfBytes += sizeof(double);
		}
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		buffer = ShardingChange::deserialize(buffer);
		unsigned sizeOfMap;
		buffer += srch2::util::deserializeFixedTypes(buffer, sizeOfMap); // size of map
		for(unsigned i = 0; i < sizeOfMap; ++i){
			ShardId shardIdKey;
			buffer = shardIdKey.deserialize(buffer);
			double addedLoad ;
			buffer = srch2::util::deserializeFixedTypes(buffer, addedLoad);
			addedLoads[shardIdKey] = addedLoad;
		}
		return buffer;
	}

	ShardingChange * clone(){
		return new ShardLoadChange(*this);
	}

private:
	map<ShardId, double> addedLoads;
};

}
}

#endif //__SHARDING_SHARDING_METADATA_H__
