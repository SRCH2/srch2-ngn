#include "Partition.h"



using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


ClusterPartition::ClusterPartition(const unsigned coreId, const unsigned partitionId, bool freeForWriteFlag)
								:coreId(coreId),partitionId(partitionId){
	this->freeForWriteFlag = freeForWriteFlag;
}
bool ClusterPartition::isPartitionLocked() const{
	return freeForWriteFlag;
}
void ClusterPartition::setPartitionLock(bool lockValue){
	this->freeForWriteFlag = lockValue;
}
void ClusterPartition::addPhysicalReplica(NodeId nodeId, unsigned replicaId){
	map<NodeId, vector<unsigned> >::iterator replicaLocationItr =
			replicaLocations.find(nodeId);
	if(replicaLocationItr == replicaLocations.end()){
		replicaLocations[nodeId] = vector<unsigned>();
		replicaLocations[nodeId].push_back(replicaId);
		return;
	}
	for(vector<unsigned>::iterator replicaIdItr = replicaLocationItr->second.begin();
			replicaIdItr !=  replicaLocationItr->second.end(); ++replicaIdItr){
		if(*replicaIdItr == replicaId){
			ASSERT(false);
			return;
		}
	}
	replicaLocationItr->second.push_back(replicaId);
}


void ClusterPartition::getShardLocations(vector<NodeId> & shardLocations) const{
	for(map<NodeId, vector<unsigned> >::iterator replicaLocationItr = replicaLocations.begin();
			replicaLocationItr != replicaLocations.end(); ++replicaLocationItr){
		shardLocations.push_back(replicaLocationItr->first);
	}
}
// returns false of there is no replica of this partition in this node
bool ClusterPartition::getNodeReplicaIds(NodeId nodeId, vector<unsigned> & replicas) const{
	map<NodeId, vector<unsigned> >::iterator replicaLocationItr =
			replicaLocations.find(nodeId);
	if(replicaLocationItr == replicaLocations.end()){
		return false;
	}
	replicas.insert(replicas.begin(), replicaLocationItr->second.begin(), replicaLocationItr->second.end());
	return true;
}

const unsigned ClusterPartition::getCoreId() const{
	return coreId;
}

const unsigned ClusterPartition::getPartitionId() const{
	return partitionId;
}


NodePartition::NodePartition(const unsigned coreId, const NodeId nodeId, double load):coreId(coreId),nodeId(nodeId){
	this->load = load;
}

double NodePartition::getLoad() const{
	return this->load;
}
void NodePartition::getInternalPartitionIds(vector<unsigned> & nodeInternalPartitionIds) const{
	nodeInternalPartitionIds.insert(nodeInternalPartitionIds.begin(),
			this->nodeInternalPartitionIds.begin(), this->nodeInternalPartitionIds.end());
}
void NodePartition::addInternalPartitionId(unsigned internalPartitionId){
	if(std::find(this->nodeInternalPartitionIds.begin(), this->nodeInternalPartitionIds.end() , internalPartitionId)
									!= this->nodeInternalPartitionIds.end()){
		return;
	}
	this->nodeInternalPartitionIds.push_back(internalPartitionId);
}

const unsigned NodePartition::getCoreId() const	{
	return coreId;
}

const NodeId NodePartition::getNodeId() const{
	return nodeId;
}



const ClusterPartition * CorePartitionContianer::getClusterPartition(unsigned partitionId) const{
	if(clusterPartitions.find(partitionId) == clusterPartitions.end()){
		return NULL;
	}
	return clusterPartitions.find(partitionId)->second;
}

const NodePartition * CorePartitionContianer::getNodePartition(unsigned nodeInternalPartitionId) const{
	if(nodePartitions.find(nodeInternalPartitionId) == nodePartitions.end()){
		return NULL;
	}
	return nodePartitions.find(nodeInternalPartitionId)->second;
}
const unsigned CorePartitionContianer::getCoreId() const	{
	return coreId;
}
const unsigned CorePartitionContianer::getTotalNumberOfPartitions() const	{
	return totalNumberOfPartitions;
}
const unsigned CorePartitionContianer::getReplicationDegree() const{
	return replicationDegree;
}
void CorePartitionContianer::addClusterShard(NodeId nodeId, ShardId shardId){
	if(shardId.coreId != coreId){
		return;
	}
	if(clusterPartitions.find(shardId.partitionId) == clusterPartitions.end()){
		return;
	}
	ClusterPartition * clusterPartition = clusterPartitions[shardId.partitionId];
	clusterPartition->addPhysicalReplica(nodeId, shardId.replicaId);
}

void CorePartitionContianer::addNodeShard(NodeId nodeId, unsigned nodeInternalPartitionId){
	if(nodePartitions.find(nodeId) == nodePartitions.end()){
		nodePartitions[nodeId] = new NodePartition(coreId, nodeId, 0);
	}
	NodePartition * nodePartition = nodePartitions[nodeId];
	nodePartition->addInternalPartitionId(nodeInternalPartitionId);
}

void CorePartitionContianer::setPartitionLock(unsigned partitionId, bool lockValue){
	if(clusterPartitions.find(partitionId) == clusterPartitions.end()){
		ASSERT(false);
		return;
	}
	ClusterPartition * clusterPartition = clusterPartitions[partitionId];
	clusterPartition->setPartitionLock(lockValue);
}


void CorePartitionContianer::getClusterPartitionsForRead(vector<const ClusterPartition *> & clusterPartitions) const{
	for(map<unsigned, ClusterPartition *>::const_iterator clusterPartitionsItr = this->clusterPartitions.begin() ;
			clusterPartitionsItr != this->clusterPartitions.end(); ++clusterPartitionsItr){
		clusterPartitions.push_back(clusterPartitionsItr->second);
	}
}
void CorePartitionContianer::getNodePartitionsForRead(vector<const NodePartition *> & nodePartitions) const{
	for(map<unsigned, NodePartition *>::const_iterator nodePartitionsItr = this->nodePartitions.begin() ;
			nodePartitionsItr != this->nodePartitions.end(); ++nodePartitionsItr){
		nodePartitions.push_back(nodePartitionsItr->second);
	}
}

const ClusterPartition * CorePartitionContianer::getClusterPartitionForWrite(unsigned hashKey) const{
	unsigned partitionId = hashKey % totalNumberOfPartitions;
	return clusterPartitions[partitionId];
}
const NodePartition * CorePartitionContianer::getNodePartitionForWrite(unsigned hashKey, NodeId nodeId) const{
	if(nodePartitions.find(nodeId) == nodePartitions.end()){
		return NULL;
	}
	return nodePartitions[nodeId];
}


}
}

