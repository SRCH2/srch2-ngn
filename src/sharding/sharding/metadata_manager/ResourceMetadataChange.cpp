
#include "ResourceMetadataChange.h"
#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"
#include "../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

void * NodeAddChange::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(newNodeId, buffer);
	buffer = srch2::util::serializeVectorOfDynamicTypes(this->localClusterShardIds, buffer);
	buffer = srch2::util::serializeVectorOfDynamicTypes(this->localNodeShardIds, buffer);
	return buffer;
}
unsigned NodeAddChange::getNumberOfBytes() const{
	unsigned numberOfBytes=  0;
	numberOfBytes += sizeof(NodeId);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(this->localClusterShardIds);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(this->localNodeShardIds);
	return numberOfBytes;
}
void * NodeAddChange::deserialize(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, newNodeId);
	buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, this->localClusterShardIds);
	buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, this->localNodeShardIds);
	return buffer;
}

ShardAssignChange::ShardAssignChange(){
	// temporary initialization
}
ShardAssignChange::ShardAssignChange(ClusterShardId logicalShard, NodeId location, double load){
	logicalShardToAssign = logicalShard;
	this->load = load;
	this->location = location;
}
ShardAssignChange::ShardAssignChange(const ShardAssignChange & change){
	logicalShardToAssign = change.logicalShardToAssign;
	load = change.load;
	location = change.load;
}

MetadataChangeType ShardAssignChange::getType() const{
	return ShardingChangeTypeShardAssign;
}
bool ShardAssignChange::doChange(Cluster_Writeview * metadata){

	if(ShardManager::getCurrentNodeId() == this->location){
		// local node
		metadata->assignLocalClusterShard(logicalShardToAssign, physicalShard);
	}else{
		metadata->assignExternalClusterShard(logicalShardToAssign, location, load);
	}

	return true;
}

void * ShardAssignChange::serialize(void * buffer) const{
	buffer = logicalShardToAssign.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(location, buffer);
	buffer = srch2::util::serializeFixedTypes(load, buffer);
	return buffer;
}
unsigned ShardAssignChange::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += logicalShardToAssign.getNumberOfBytes();
	numberOfBytes += sizeof(location);
	numberOfBytes += sizeof(load);
	return numberOfBytes;
}
void * ShardAssignChange::deserialize(void * buffer){
	buffer = logicalShardToAssign.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, location);
	buffer = srch2::util::deserializeFixedTypes(buffer, load);
	return buffer;
}

ShardMoveChange::ShardMoveChange(ClusterShardId shardId, NodeId srcNodeId, NodeId destNodeId){
	this->shardId = shardId;
	this->srcNodeId = srcNodeId;
	this->destNodeId = destNodeId;
}
ShardMoveChange::ShardMoveChange(){
	// temp initialization used for deserializing the object.
}
ShardMoveChange::ShardMoveChange(const ShardMoveChange & change){
	this->shardId = change.shardId;
	this->srcNodeId = change.srcNodeId;
	this->destNodeId = change.destNodeId;
}

MetadataChangeType ShardMoveChange::getType() const{
	return ShardingChangeTypeShardMove;
}


bool ShardMoveChange::doChange(Cluster_Writeview * metadata){
	if(destNodeId == ShardManager::getCurrentNodeId()){
		metadata->moveClusterShard(shardId, physicalShard);
	}else{
		metadata->moveClusterShard(shardId, destNodeId);
	}
	return true;
}


void * ShardMoveChange::serialize(void * buffer) const{
	buffer = shardId.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
    return srch2::util::serializeFixedTypes(destNodeId, buffer);
}
unsigned ShardMoveChange::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += shardId.getNumberOfBytes();
	numberOfBytes += sizeof(srcNodeId);
	numberOfBytes += sizeof(destNodeId);
	return numberOfBytes;
}
void * ShardMoveChange::deserialize(void * buffer){
	buffer = shardId.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(srcNodeId, buffer);
    return srch2::util::serializeFixedTypes(destNodeId, buffer);
}

ShardLoadChange::ShardLoadChange(){
	// temp initialization used for deserializing the object.
}
ShardLoadChange::ShardLoadChange(const ShardLoadChange & change){
	this->addedLoads = change.addedLoads;
}

MetadataChangeType ShardLoadChange::getType() const{
	return ShardingChangeTypeLoadChange;
}


bool ShardLoadChange::doChange(Cluster_Writeview * metadata){
	//TODO
	return true;
}


void * ShardLoadChange::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes((unsigned)(addedLoads.size()), buffer); // size of map
	for(map<ClusterShardId, double>::const_iterator i = addedLoads.begin() ; i != addedLoads.end(); ++i){
		buffer = i->first.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(i->second, buffer);
	}
	return buffer;
}
unsigned ShardLoadChange::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += sizeof(unsigned); // size of map
	for(map<ClusterShardId, double>::const_iterator i = addedLoads.begin() ; i != addedLoads.end(); ++i){
		numberOfBytes += i->first.getNumberOfBytes();
		numberOfBytes += sizeof(double);
	}
	return numberOfBytes;
}
void * ShardLoadChange::deserialize(void * buffer){
	unsigned sizeOfMap;
	buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap); // size of map
	for(unsigned i = 0; i < sizeOfMap; ++i){
		ClusterShardId shardIdKey;
		buffer = shardIdKey.deserialize(buffer);
		double addedLoad ;
		buffer = srch2::util::deserializeFixedTypes(buffer, addedLoad);
		addedLoads[shardIdKey] = addedLoad;
	}
	return buffer;
}

}
}
