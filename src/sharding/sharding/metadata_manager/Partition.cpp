#include "Partition.h"


#include <sstream>
#include "../../util/FramedPrinter.h"

namespace srch2 {
namespace httpwrapper {


ClusterPartition::ClusterPartition(const unsigned coreId, const unsigned partitionId, PartitionLockValue partitionLockValue)
								:coreId(coreId),partitionId(partitionId){
	this->partitionLock = partitionLockValue;
}
bool ClusterPartition::isPartitionLocked() const{
	return (partitionLock == PartitionLock_Locked);
}
void ClusterPartition::setPartitionLock(PartitionLockValue lockValue){
	this->partitionLock = lockValue;
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


string ClusterPartition::toString() const{
	stringstream ss;
	ss << "Partition ID : " << partitionId << "%";
	if(partitionLock == PartitionLock_Locked){
		ss << "Locked.%";
	}else{
		ss << "Free.%";
	}
	for(map<NodeId, vector<unsigned> >::const_iterator nodeItr = replicaLocations.begin();
			nodeItr != replicaLocations.end(); ++nodeItr){
		ss << "Node " << nodeItr->first << " : %-->";
		for(unsigned i = 0 ; i < nodeItr->second.size(); ++i){
			if(i != 0){
				ss << ",";
			}
			ss << nodeItr->second.at(i);
		}
		ss << "%";
	}
	return ss.str();
}

void ClusterPartition::getShardLocations(vector<NodeId> & shardLocations) const{
	for(map<NodeId, vector<unsigned> >::const_iterator replicaLocationItr = replicaLocations.begin();
			replicaLocationItr != replicaLocations.end(); ++replicaLocationItr){
		shardLocations.push_back(replicaLocationItr->first);
	}
}
// returns false of there is no replica of this partition in this node
bool ClusterPartition::getNodeReplicaIds(NodeId nodeId, vector<unsigned> & replicas) const{
	map<NodeId, vector<unsigned> >::const_iterator replicaLocationItr =
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


string NodePartition::toString() const {
	stringstream ss;
	for(unsigned i = 0 ; i < nodeInternalPartitionIds.size(); ++i){
		if(i != 0){
			ss << ",";
		}
		ss << nodeInternalPartitionIds.at(i);
	}
	return ss.str();
}


const ClusterPartition * CorePartitionContianer::getClusterPartition(unsigned partitionId) const{
	if(clusterPartitions.find(partitionId) == clusterPartitions.end()){
		return NULL;
	}
	return clusterPartitions.find(partitionId)->second;
}

const NodePartition * CorePartitionContianer::getNodePartition(unsigned nodeId) const{
	if(nodePartitions.find(nodeId) == nodePartitions.end()){
		return NULL;
	}
	return nodePartitions.find(nodeId)->second;
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

void CorePartitionContianer::getInvolvedNodes(const ClusterPID pid, vector<NodeId> & nodes) const{
	nodes.clear();
	for(map<unsigned, ClusterPartition *>::const_iterator clusterPartitionsItr = this->clusterPartitions.begin() ;
			clusterPartitionsItr != this->clusterPartitions.end(); ++clusterPartitionsItr){
		if(clusterPartitionsItr->second->getPartitionId() != pid.partitionId){
			continue;
		}
		vector<NodeId> involvedNodes;
		clusterPartitionsItr->second->getShardLocations(involvedNodes);
		for(unsigned i = 0 ; i < involvedNodes.size(); ++i){
			if(std::find(nodes.begin(), nodes.end(), involvedNodes.at(i)) == nodes.end()){
				nodes.push_back(involvedNodes.at(i));
			}
		}
	}
}

void CorePartitionContianer::addClusterShard(NodeId nodeId, ClusterShardId shardId){
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

void CorePartitionContianer::setPartitionLock(unsigned partitionId, PartitionLockValue lockValue){
	if(clusterPartitions.find(partitionId) == clusterPartitions.end()){
		ASSERT(false);
		return;
	}
	ClusterPartition * clusterPartition = clusterPartitions[partitionId];
	clusterPartition->setPartitionLock(lockValue);
}

bool CorePartitionContianer::isCoreLocked() const{
	for(map<unsigned, ClusterPartition *>::const_iterator cItr = clusterPartitions.begin(); cItr != clusterPartitions.end(); ++cItr){
		if(cItr->second->isPartitionLocked()){
			return true;
		}
	}
	return false;
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
	if(totalNumberOfPartitions == 0){
		return NULL;
	}
	unsigned partitionId = hashKey % totalNumberOfPartitions;
	return getClusterPartition(partitionId);
}
const NodePartition * CorePartitionContianer::getNodePartitionForWrite(unsigned hashKey, NodeId nodeId) const{
	if(nodePartitions.find(nodeId) == nodePartitions.end()){
		return NULL;
	}
	return getNodePartition(nodeId);
}


void CorePartitionContianer::print() const{
		stringstream tableName  ;
		tableName << "CoreId : " << coreId << "%";
		tableName << "Total # partitions : " << totalNumberOfPartitions << "%";
		tableName << "Replication Deg. : " << replicationDegree << "%";
		{
			// cluster partitions
			vector<string> clusterHeaders;
			clusterHeaders.push_back("Partition Info");
			vector<string> clusterLabels;
			for(map<unsigned, ClusterPartition *>::const_iterator partItr =
					clusterPartitions.begin(); partItr != clusterPartitions.end(); ++partItr){
				stringstream ss;
				ss << "Partition Id : " << partItr->first;
				clusterLabels.push_back(ss.str());
			}
			srch2::util::TableFormatPrinter clusterTable("Cluster Partitions%" + tableName.str() ,
					120, clusterHeaders, clusterLabels);
			clusterTable.printColumnHeaders();
			clusterTable.startFilling();
			for(map<unsigned, ClusterPartition *>::const_iterator partItr =
					clusterPartitions.begin(); partItr != clusterPartitions.end(); ++partItr){
				clusterTable.printNextCell(partItr->second->toString());
			}
		}
		{
			// node partitions
			vector<string> nodeHeaders;
			nodeHeaders.push_back("Partition Info");
			vector<string> nodeLabels;
			for(map<NodeId, NodePartition *>::const_iterator nodeItr =
					nodePartitions.begin(); nodeItr != nodePartitions.end(); ++nodeItr){
				stringstream ss;
				ss << "Node Id : " << nodeItr->first;
				nodeLabels.push_back(ss.str());
			}
			srch2::util::TableFormatPrinter nodeTable("Cluster Partitions%" + tableName.str() ,
					120, nodeHeaders, nodeLabels);
			nodeTable.printColumnHeaders();
			nodeTable.startFilling();
			for(map<NodeId, NodePartition *>::const_iterator nodeItr =
					nodePartitions.begin(); nodeItr != nodePartitions.end(); ++nodeItr){
				nodeTable.printNextCell(nodeItr->second->toString());
			}
		}

	}

}
}

