
#include "ResourceMetadataChange.h"
#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"
#include "../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

NodeAddChange::NodeAddChange(NodeId newNodeId, const vector<ClusterShardId> & localClusterShardIds,
        const vector<NodeShardId> & localNodeShardIds){

    this->newNodeId = newNodeId;
    this->localClusterShardIds = localClusterShardIds;
    this->localNodeShardIds = localNodeShardIds;
};
NodeAddChange::NodeAddChange(){};
NodeAddChange::NodeAddChange(const NodeId & newNodeId){
    this->newNodeId = newNodeId;
};
NodeAddChange::NodeAddChange(const NodeAddChange & copy){
    newNodeId = copy.newNodeId;
    localClusterShardIds = copy.localClusterShardIds;
    localNodeShardIds = copy.localNodeShardIds;
}
bool NodeAddChange::doChange(Cluster_Writeview * metadata){
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

    Cluster_Writeview * writeview = ShardManager::getWriteview();
    writeview->setNodeState(newNodeId, ShardingNodeStateArrived);

    return true;
}
MetadataChangeType NodeAddChange::getType() const{
    return ShardingChangeTypeNodeAdd;
}

vector<ClusterShardId> & NodeAddChange::getLocalClusterShardIds(){
    return localClusterShardIds;
}
vector<NodeShardId> & NodeAddChange::getLocalNodeShardIds(){
    return localNodeShardIds;
}

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

bool NodeAddChange::operator==(const MetadataChange & rightArg){
	const NodeAddChange & right = (const NodeAddChange &) rightArg;
	return (newNodeId == right.newNodeId) && (localClusterShardIds == right.localClusterShardIds) && (localNodeShardIds == right.localNodeShardIds);
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

bool ShardAssignChange::operator==(const MetadataChange & rightArg){
	const ShardAssignChange & right = (const ShardAssignChange &) rightArg;
	return (logicalShardToAssign == right.logicalShardToAssign) && (location == right.location) && (load == right.load);
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
	buffer = shardId.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, srcNodeId);
    return srch2::util::deserializeFixedTypes(buffer, destNodeId);
}

bool ShardMoveChange::operator==(const MetadataChange & rightArg){
	const ShardMoveChange & right = (const ShardMoveChange &) rightArg;
	return (shardId == right.shardId) && (srcNodeId == right.srcNodeId) && (destNodeId == right.destNodeId);
}

ShardLoadChange::ShardLoadChange(const map<ClusterShardId, double> & addedLoads){
	this->addedLoads = addedLoads;
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
	buffer = srch2::util::serializeMapDynamicToFixed(addedLoads, buffer);
	return buffer;
}
unsigned ShardLoadChange::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += srch2::util::getNumberOfBytesMapDynamicToFixed(addedLoads);
	return numberOfBytes;
}
void * ShardLoadChange::deserialize(void * buffer){
	buffer = srch2::util::deserializeMapDynamicToFixed(buffer, addedLoads);
	return buffer;
}

bool ShardLoadChange::operator==(const MetadataChange & rightArg){
	const ShardLoadChange & right = (const ShardLoadChange &) rightArg;
	return (addedLoads == right.addedLoads);
}

}
}
