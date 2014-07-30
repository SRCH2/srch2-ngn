#include "ResourceMetadata.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



std::string NodeShardId::toString() const{
	std::stringstream sstm;
	sstm << "C" << coreId << "_N" << nodeId << "_P" << partitionId ;
	return sstm.str();
}

bool NodeShardId::operator==(const NodeShardId& rhs) const {
	return (coreId == rhs.coreId)&&(nodeId == rhs.nodeId)&&(partitionId == rhs.partitionId);
}
bool NodeShardId::operator!=(const NodeShardId& rhs) const {
	return !(*this == rhs);
}
bool NodeShardId::operator>(const NodeShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(nodeId > rhs.nodeId ||
							(nodeId == rhs.nodeId && partitionId > rhs.partitionId)));
}
bool NodeShardId::operator<(const NodeShardId& rhs) const {
	return !((*this == rhs) || (*this > rhs));
}
bool NodeShardId::operator>=(const NodeShardId& rhs) const {
	return (*this > rhs) || (*this == rhs);
}
bool NodeShardId::operator<=(const NodeShardId& rhs) const {
	return !(*this > rhs);
}

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* NodeShardId::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(partitionId, buffer);
	return buffer;
}

//given a byte stream recreate the original object
void * NodeShardId::deserialize(void* buffer) const{
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, partitionId);
	return buffer;
}

unsigned NodeShardId::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(NodeId);
	numberOfBytes += sizeof(unsigned);
	return numberOfBytes;
}


ClusterResourceMetadata_Writeview::ClusterResourceMetadata_Writeview(const ClusterResourceMetadata_Readview & readview):
	clusterName(readview.getClusterName(), currentNodeId(readview.getCurrentNodeId())){

	this->versionId = readview.getVersionId();

	vector<const CoreInfo_t *> cores;
	readview.getAllCores(cores);
	for(unsigned cid = 0; cid < cores.size(); ++cid){
		unsigned coreId = cores.at(cid)->getCoreId();
		this->cores[coreId] = cores.at(cid);
		const CorePartitionContianer * corePartitioner = readview.getCorePartitionContianer(cores.at(cid)->getCoreId());
		ASSERT(corePartitioner != NULL);
		for(unsigned pid = 0 ; pid < corePartitioner->getTotalNumberOfPartitions() ; ++pid){
			for(unsigned rid = 0 ; rid < corePartitioner->getReplicationDegree(); ++rid){
				shardStates[ShardId(coreId, pid, rid)] = SHARDSTATE_UNASSIGNED;
			}
			const ClusterPartition * clusterPartition = corePartitioner->getClusterPartition(pid);
			if(clusterPartition == NULL){
				ASSERT(false);
				partitionLocks[ShardId(coreId, pid, 0)] = false;
			}else{
				partitionLocks[ShardId(coreId, pid, 0)] = clusterPartition->isPartitionLocked();
			}
		}

	}
	vector<Node> nodesVector;
	readview.getAllNodes(nodesVector);
	for(unsigned nId = 0 ; nId < nodesVector.size(); ++nId){
		this->nodes[nodesVector.at(nId).getId()] = nodesVector.at(nId);
	}
}


ClusterResourceMetadata_Readview * ClusterResourceMetadata_Writeview::getNewReadview() const {
	// we need cores for construction
	vector<CoreInfo_t *> coresVector;
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		coresVector.push_back(coreItr->second);
	}
	ClusterResourceMetadata_Readview * newReadview = new ClusterResourceMetadata_Readview(++versionId, clusterName, coresVector);
	newReadview->setCurrentNodeId(currentNodeId);
	// add nodes
	for(map<NodeId, Node>::iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
		newReadview->addNode(nodeItr->second);
	}
	// per core ...
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * core = coreItr->second;
		CorePartitionContianer * corePartitionContainer = newReadview->getCorePartitionContianer(coreId);
		if(corePartitionContainer == NULL){
			ASSERT(false);
			return NULL;
		}
		// add all shards information to corePartitioners because they are used by partitioner
		for(map<ShardId, PhysicalShard >::iterator physicalShardItr = physicalShards.begin();
				physicalShardItr != physicalShards.end(); ++physicalShardItr){
			if(physicalShardItr->first.coreId == coreId){
				corePartitionContainer->addClusterShard(physicalShardItr->second.nodeLocation, physicalShardItr->first);
			}
		}
		for(vector<NodeShardId>::iterator nodeItr = nodeShards.begin(); nodeItr != nodeShards.end(); ++nodeItr){
			if(nodeItr->coreId == coreId){
				corePartitionContainer->addNodeShard(nodeItr->nodeId, nodeItr->partitionId);
			}
		}

		LocalShardContainer * localShardContainer = newReadview->getLocalShardContainer(coreId);
		if(localShardContainer == NULL){
			ASSERT(false);
			return NULL;
		}

		// add local shard information to be used in DPInternal
		for(map<ShardId, boost::shared_ptr<Srch2Server> >::iterator clusterShardItr = localDataShards.begin();
				clusterShardItr != localDataShards.end(); ++clusterShardItr){
			if(clusterShardItr->first.coreId == coreId){
				ASSERT(physicalShards.find(clusterShardItr->first) != physicalShards.end());
				NodeId clusterShardNodeId = physicalShards.find(clusterShardItr->first)->second.nodeLocation;
				double clusterShardLoad = physicalShards.find(clusterShardItr->first)->second.shardLoad;
				ASSERT(clusterShardNodeId == currentNodeId);
				localShardContainer->addClusterShard(clusterShardItr->first, currentNodeId, clusterShardItr->second, clusterShardLoad);
			}
		}

		for(map<NodeShardId,  boost::shared_ptr<Srch2Server> >::iterator nodeShardItr = localNodeDataShards.begin();
				nodeShardItr != localNodeDataShards.end(); ++nodeShardItr){
			if(nodeShardItr->first.coreId == coreId){
				ASSERT(nodeShardItr->first.nodeId == currentNodeId);
				localShardContainer->addNodeShard(coreId, nodeShardItr->first.partitionId, currentNodeId , nodeShardItr->second, 0);
			}
		}
	}

	// set locks per core ...
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * core = coreItr->second;
		CorePartitionContianer * corePartitionContainer = newReadview->getCorePartitionContianer(coreId);
		if(corePartitionContainer == NULL){
			ASSERT(false);
			return NULL;
		}
		for(map<ShardId, bool>::iterator corePartitionItr = partitionLocks.begin();
				corePartitionItr != partitionLocks.end(); ++ corePartitionItr){
			if(corePartitionItr->first.coreId == coreId){
				corePartitionContainer->setPartitionLock(corePartitionItr->first.partitionId, corePartitionItr->second);
			}
		}
	}

	return newReadview;
}

bool ClusterResourceMetadata_Writeview::isPartitionLocked(unsigned coreId, unsigned partitionId) const{
	for(map<ShardId, bool>::iterator corePartitionItr = partitionLocks.begin();
			corePartitionItr != partitionLocks.end(); ++corePartitionItr){
		if(corePartitionItr->first.coreId == coreId && corePartitionItr->first.partitionId == partitionId){
			return corePartitionItr->second;
		}
	}
	ASSERT(false);
	return true;
}

bool ShardAssignChange::doChange(ClusterResourceMetadata_Writeview * metadata){
	if(metadata->shardStates.find(logicalShardToAssign) == metadata->shardStates.end()){
		return false;
	}
	if(metadata->physicalShards.find(logicalShardToAssign) != metadata->physicalShards.end()){
		return false;
	}
	if(metadata->localDataShards.find(logicalShardToAssign) != metadata->localDataShards.end()){
		return false;
	}
	map<ShardId, ShardState>::iterator currentShardState = metadata->shardStates.find(logicalShardToAssign);
	this->prevShardState = currentShardState->second;
	ASSERT(SHARDSTATE_READY != this->prevShardState);
	currentShardState->second = SHARDSTATE_READY;
	metadata->physicalShards.insert({logicalShardToAssign, physicalShard});
	// if it's the dest node, also set the data pointer
	if(physicalShard.nodeLocation == metadata->currentNodeId){
		metadata->localDataShards[logicalShardToAssign] = localDataPointer;
	}
	return true;
}

bool ShardAssignChange::undoChange(ClusterResourceMetadata_Writeview * metadata){
	if(metadata->shardStates.find(logicalShardToAssign) == metadata->shardStates.end()){
		return false;
	}
	if(metadata->physicalShards.find(logicalShardToAssign) == metadata->physicalShards.end()){
		return false;
	}
	if(metadata->localDataShards.find(logicalShardToAssign) == metadata->localDataShards.end()){
		return false;
	}
	map<ShardId, ShardState>::iterator currentShardState = metadata->shardStates.find(logicalShardToAssign);
	currentShardState->second = prevShardState;
	metadata->physicalShards.erase(metadata->physicalShards.find(logicalShardToAssign));
	// if it's the dest node, also delete the data pointer
	if(physicalShard.nodeLocation == metadata->currentNodeId){
		metadata->localDataShards.erase(metadata->localDataShards.find(logicalShardToAssign));
	}
	return true;
}


bool ShardCopyChange::doChange(ClusterResourceMetadata_Writeview * metadata){
	//1. find the srcShard to get its load
	if(metadata->physicalShards.find(srcShardId) == metadata->physicalShards.end()){
		return false;
	}
	this->sac = new ShardAssignChange(destShardId, destNodeId, metadata->physicalShards.find(srcShardId)->second.shardLoad);
	return this->sac->doChange(metadata);
}
bool ShardCopyChange::undoChange(ClusterResourceMetadata_Writeview * metadata){
	if(this->sac == NULL){
		return false;
	}
	return this->sac->undoChange(metadata);
}


bool ShardMoveChange::doChange(ClusterResourceMetadata_Writeview * metadata){
	// find the shardId physical shard
	map<ShardId, PhysicalShard>::iterator currentPhysicalShard = metadata->physicalShards.find(shardId);
	if(currentPhysicalShard == metadata->physicalShards.end()){
		return false;
	}
	// if we are on the src node, save the data pointer of this shard
	if(srcNodeId == metadata->currentNodeId){ // srcnode
		// data pointer of this shard must be here
		if(metadata->localDataShards.end() == metadata->localDataShards.find(shardId)){
			return false;
		}
	}

	if(destNodeId == metadata->currentNodeId && destNodeId != srcNodeId){
		// data pointer of this shard must not be here
		if(metadata->localDataShards.end() != metadata->localDataShards.find(shardId)){
			return false;
		}
	}
	// save old state for undo
	prevPhysicalShard = currentPhysicalShard->second;
	// apply new changes
	// 1. shard location
	// I) shard location must change from srcNodeId to destNodeId
	ASSERT(currentPhysicalShard->second.nodeLocation == srcNodeId);
	currentPhysicalShard->second.nodeLocation = destNodeId;
	// 2. shard data pointers
	// I) remove the localDataPointer from the src node and keep it in
	//    prevLocalDataPointer so that we have it in case of undo
	if(srcNodeId == metadata->currentNodeId){ // srcnode
		// data pointer of this shard must be here
		prevLocalDataPointer = metadata->localDataShards.find(shardId)->second;
		metadata->localDataShards.erase(metadata->localDataShards.find(shardId));
	}
	// II) add the new data pointer if we are in the destination node
	if(destNodeId == metadata->currentNodeId){
		// data pointer of this shard must be here
		metadata->localDataShards.insert({shardId, nextLocalDataPointer});
	}
	return true;
}
bool ShardMoveChange::undoChange(ClusterResourceMetadata_Writeview * metadata){
	// find the shardId physical shard
	map<ShardId, PhysicalShard>::iterator currentPhysicalShard = metadata->physicalShards.find(shardId);
	if(currentPhysicalShard == metadata->physicalShards.end()){
		return false;
	}

	// if we are on the src node, save the data pointer of this shard
	if(srcNodeId == metadata->currentNodeId && srcNodeId != destNodeId){ // srcnode
		// data pointer of this shard must not be here anymore
		if(metadata->localDataShards.end() != metadata->localDataShards.find(shardId)){
			return false;
		}
	}

	if(destNodeId == metadata->currentNodeId){
		// data pointer of this shard must be here now (because do was executed before ...)
		if(metadata->localDataShards.end() == metadata->localDataShards.find(shardId)){
			return false;
		}
	}

	// 1. undo changes on physical shard
	currentPhysicalShard->second = prevPhysicalShard;
	// 2. undo data pointer changes
	if(srcNodeId == metadata->currentNodeId){
		metadata->localDataShards[shardId] = prevLocalDataPointer;
	}
	if(destNodeId == metadata->currentNodeId){
		metadata->localDataShards.erase(metadata->localDataShards.find(shardId));
	}

	return true;

}


bool ShardLoadChange::doChange(ClusterResourceMetadata_Writeview * metadata){
	for(map<ShardId, double>::iterator addedLoad = addedLoads.begin(); addedLoad != addedLoads.end(); ++addedLoad){
		if(metadata->physicalShards.find(addedLoad->first) == metadata->physicalShards.end()){
			continue;
		}
		metadata->physicalShards.find(addedLoad->first)->second.shardLoad += addedLoad->second;
	}
	return true;
}
bool ShardLoadChange::undoChange(ClusterResourceMetadata_Writeview * metadata){
	for(map<ShardId, double>::iterator addedLoad = addedLoads.begin(); addedLoad != addedLoads.end(); ++addedLoad){
		if(metadata->physicalShards.find(addedLoad->first) == metadata->physicalShards.end()){
			continue;
		}
		metadata->physicalShards.find(addedLoad->first)->second.shardLoad -= addedLoad->second;
	}
	return true;
}

}
}
