
#include "Shard.h"

#include "src/core/util/SerializationHelper.h"
#include <sstream>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

bool ShardId::isPrimaryShard() const{
	return (replicaId == 0); // replica #0 is always the primary shard
}
std::string ShardId::toString() const {
	// A primary shard starts with a "P" followed by an integer id.
	// E.g., a cluster with 4 shards of core 8 will have shards named "C8_P0", "C8_R0_1", "C8_R0_2", "C8_P3".
	//
	// A replica shard starts with an "R" followed by a replica count and then its primary's id.
	// E.g., for the above cluster, replicas of "P0" will be named "8_R1_0" and "8_R2_0".
	// Similarly, replicas of "P3" will be named "8_R3_1" and "8_R3_2".
	if(coreId != unsigned(-1) || partitionId != unsigned(-1) || replicaId != unsigned(-1)){
		std::stringstream sstm;
		sstm << "C" << coreId << "_";
		if (isPrimaryShard()){
			sstm << "P" << partitionId;
		}
		else{
			sstm << "R" << partitionId << "_" << replicaId;
		}
		return sstm.str();
	}
	else{
		return "";
	}
}

ShardId::ShardId() {
	coreId = unsigned(-1);
	partitionId = unsigned(-1);
	replicaId = unsigned(-1);
}
ShardId::ShardId(unsigned coreId, unsigned partitionId, unsigned replicaId) :
	coreId(coreId), partitionId(partitionId), replicaId(replicaId) {}

bool ShardId::operator==(const ShardId& rhs) const {
	return coreId == rhs.coreId && partitionId == rhs.partitionId
			&& replicaId == replicaId;
}
bool ShardId::operator!=(const ShardId& rhs) const {
	return coreId != rhs.coreId || partitionId != rhs.partitionId
			|| replicaId != replicaId;
}
bool ShardId::operator>(const ShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId > rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId > rhs.replicaId)));
}
bool ShardId::operator<(const ShardId& rhs) const {
	return  coreId < rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId < rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId < replicaId)));
}
bool ShardId::operator>=(const ShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId > rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId >= replicaId)));
}
bool ShardId::operator<=(const ShardId& rhs) const {
	return  coreId < rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId < rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId <= replicaId)));
}


//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* ShardId::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(partitionId, buffer);
	buffer = srch2::util::serializeFixedTypes(replicaId, buffer);
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	return buffer;
}

//given a byte stream recreate the original object
void * ShardId::deserialize(void* buffer) const{
	buffer = srch2::util::deserializeFixedTypes(buffer, partitionId);
	buffer = srch2::util::deserializeFixedTypes(buffer, replicaId);
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	return buffer;
}

unsigned ShardId::getNumberOfBytes() const{
	return 3 * sizeof(unsigned);
}

/////////////////////////////// ShardIdComparator
bool ShardIdComparator::operator() (const ShardId s1, const ShardId s2) {
	if (s1.coreId > s2.coreId)
		return true;

	if (s1.coreId < s2.coreId)
		return false;

	// they have equal coreId; we look at their partitionId
	if (s1.partitionId > s2.partitionId)
		return true;

	if (s1.partitionId < s2.partitionId)
		return false;

	// they have equal partitionId; we look at their replicaId
	if (s1.replicaId > s2.replicaId)
		return true;

	return false;
}



///////////////////////////////////////// Shard and ClusterShard and NodeShard
Shard::Shard(boost::shared_ptr<Srch2Server> srch2Server, double load = 0){
	this->load = load;
	this->srch2Server = srch2Server;
}
double Shard::getLoad() const{
	return load;
}
boost::shared_ptr<Srch2Server> Shard::getSrch2Server() const{
	return this->srch2Server;
}

ClusterShard::ClusterShard(const ShardId shardId, const NodeId nodeId,
		boost::shared_ptr<Srch2Server> srch2Server, double load = 0):
			shardId(shardId), nodeId(nodeId), Shard(srch2Server, load){}

const NodeId ClusterShard::getNodeId() const {
	return nodeId;
}

const ShardId ClusterShard::getShardId() const {
	return shardId;
}

NodeShardId::NodeShard(const unsigned coreId, const unsigned internalPartitionId,
		const NodeId nodeId, boost::shared_ptr<Srch2Server> srch2Server,double load = 0):
			coreId(coreId), internalPartitionId(internalPartitionId), nodeId(nodeId),
			Shard(srch2Server, load){}
const unsigned NodeShardId::getCoreId() const {
	return coreId;
}

const unsigned NodeShardId::getInternalPartitionId() const {
	return internalPartitionId;
}

const NodeId NodeShardId::getNodeId() const {
	return nodeId;
}


NodeTargetShardInfo::NodeTargetShardInfo(const NodeId nodeId, const unsigned coreId):nodeId(nodeId), coreId(coreId){

}

void NodeTargetShardInfo::addClusterShard(ShardId shardId){
	if(std::find(targetClusterShards.begin(), targetClusterShards.end(), shardId) == targetClusterShards.end()){
		targetClusterShards.push_back(shardId);
	}
}

void NodeTargetShardInfo::addNodeShard(unsigned internalPartitionId){
	if(std::find(targetNodeInternalPartitions.begin(), targetNodeInternalPartitions.end(),
			internalPartitionId) == targetNodeInternalPartitions.end()){
		targetNodeInternalPartitions.push_back(internalPartitionId);
	}
}

const unsigned NodeTargetShardInfo::getCoreId() const{
	return coreId;
}

const NodeId NodeTargetShardInfo::getNodeId() const {
	return nodeId;
}

vector<ShardId> NodeTargetShardInfo::getTargetClusterShards() const{
	return targetClusterShards;
}
vector<unsigned> NodeTargetShardInfo::getTargetNodeInternalPartitions() const{
	return targetNodeInternalPartitions;
}



//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* NodeTargetShardInfo::serialize(void * bufferWritePointer){

    bufferWritePointer = srch2::util::serializeFixedTypes(nodeId, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes(coreId, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)(targetClusterShards.size()), bufferWritePointer);
    for(unsigned i =0; i<targetClusterShards.size(); i++){
    	bufferWritePointer = targetClusterShards.at(i).serialize(bufferWritePointer);
    }
    bufferWritePointer = srch2::util::serializeVectorOfFixedTypes(targetNodeInternalPartitions, bufferWritePointer);
    return bufferWritePointer;
}

unsigned NodeTargetShardInfo::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += sizeof(NodeId);
    numberOfBytes += sizeof(unsigned); // coreId
    numberOfBytes += sizeof(unsigned); // targetClusterShards size
    for(unsigned shardIndex = 0 ; shardIndex < targetClusterShards.size(); ++shardIndex){
    	numberOfBytes += targetClusterShards.at(shardIndex).getNumberOfBytes();
    }
    numberOfBytes += sizeof(unsigned); // targetNodeInternalPartitions size
    numberOfBytes += targetNodeInternalPartitions.size() * sizeof(unsigned);
    return numberOfBytes;
}

//given a byte stream recreate the original object
void * NodeTargetShardInfo::deserialize(void* buffer){

	if(buffer == NULL){
		ASSERT(false);
		return NULL;
	}

    buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
    buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
    unsigned sizeOfVector = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfVector);
    for(unsigned i =0; i<sizeOfVector; i++){
    	ShardId shardId;
    	buffer = shardId.deserialize(buffer);
    	targetClusterShards.push_back(shardId);
    }
    buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, targetNodeInternalPartitions);
    return buffer;
}



LocalShardContainer::LocalShardContainer(const unsigned coreId, const NodeId nodeId)
:coreId(coreId), nodeId(nodeId){}

const unsigned LocalShardContainer::getCoreId() const	{
	return coreId;
}

const NodeId LocalShardContainer::getNodeId() const{
	return nodeId;
}

void LocalShardContainer::getShards(const NodeTargetShardInfo & targets, vector<const Shard *> & shards) const{
	//cluster partitions
	vector<ShardId> shardIdTargets = targets.getTargetClusterShards();
	for(vector<ShardId>::iterator shardIdTarget = shardIdTargets.begin();
			shardIdTarget != shardIdTargets.end(); ++shardIdTarget){
		if(localClusterShards.find(shardIdTarget->partitionId) == localClusterShards.end()){
			continue;
		}
		// move on ClusterShard s and use the one which has the same shardId
		vector<ClusterShard *> & partitionClusterShards = localClusterShards.find(shardIdTarget->partitionId);
		for(vector<ClusterShard *>::iterator clusterShard = partitionClusterShards.begin();
				clusterShard != partitionClusterShards.end(); ++clusterShard){
			if((*clusterShard)->getShardId() == *shardIdTarget){
				shards.push_back(*clusterShard);
			}
		}
	}

	// node partitions
	vector<unsigned> internalPartitionIds = targets.getTargetNodeInternalPartitions();
	for(vector<unsigned>::iterator internalPartitionIdTarget = internalPartitionIds.begin();
			internalPartitionIdTarget != internalPartitionIds.end(); ++internalPartitionIdTarget){
		if(localNodeShards.find(*internalPartitionIdTarget) == localNodeShards.end()){
			continue;
		}
		NodeShardId * nodeShard = localNodeShards[*internalPartitionIdTarget];
		shards.push_back(nodeShard);
	}
}


}
}
