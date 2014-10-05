#include "Partitioner.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


CorePartitioner::CorePartitioner(const	CorePartitionContianer *
		partitionContainer):partitionContainer(partitionContainer){}

unsigned CorePartitioner::getRecordValueToHash(Record * record){

    // When the record is being parsed, configuration is used to compute the hashable value of this
    // record. It will be saved in record.
    string primaryKey = record->getPrimaryKey();
    return hashDJB2(primaryKey.c_str());
}


unsigned CorePartitioner::getRecordValueToHash(string primaryKeyStringValue){
    return hashDJB2(primaryKeyStringValue.c_str());
}


void CorePartitioner::getAllReadTargets(vector<NodeTargetShardInfo> & targets) const{
	vector<const NodePartition *> nodePartitions;
	partitionContainer->getNodePartitionsForRead(nodePartitions);

	map<NodeId, NodeTargetShardInfo> targetNodes;

	// first move on node partitions and add all those nodes to targetNodes;
	for(unsigned nodePartitionIdx = 0 ; nodePartitionIdx < nodePartitions.size(); ++nodePartitionIdx){
		const NodePartition * nodePartition = nodePartitions.at(nodePartitionIdx);
		if(targetNodes.find(nodePartition->getNodeId()) == targetNodes.end()){
			targetNodes[nodePartition->getNodeId()] = NodeTargetShardInfo(nodePartition->getNodeId(), partitionContainer->getCoreId());
		}
		addPartitionToNodeTargetContainer(nodePartition, targetNodes[nodePartition->getNodeId()]);
	}

	// now cover ClusterPartitions with the least number of nodes
	vector<const ClusterPartition *> clusterPartitions;
	partitionContainer->getClusterPartitionsForRead(clusterPartitions);

	// nodeId => list of partitions that have at least one shard in that node
	map<NodeId, vector<const ClusterPartition *> > partitionCoverGraph;

	// first try to cover partitions with those nodes that we added for NodePartitions
	for(unsigned clusterPartitionIdx = 0 ; clusterPartitionIdx < clusterPartitions.size(); ++clusterPartitionIdx){
		const ClusterPartition * clusterPartition = clusterPartitions.at(clusterPartitionIdx);

		// those nodes that cover this partition
		vector<NodeId> partitionCoveringNodes;
		clusterPartition->getShardLocations(partitionCoveringNodes);

		bool partitionCovered = false;
		for(vector<NodeId>::iterator nodeIdItr = partitionCoveringNodes.begin(); nodeIdItr != partitionCoveringNodes.end(); ++nodeIdItr){
			// if one of the covering nodes of this partition is already added, we use that node to cover this partition
			if(targetNodes.find(*nodeIdItr) != targetNodes.end()){
				addReadPartitionToNodeTargetContainer(clusterPartition, targetNodes.find(*nodeIdItr)->second);
				partitionCovered = true;
				break;
			}
		}
		// if the partition is not covered yet,
		if(! partitionCovered){
			// add all coveringNode->partition edges in partitionCover map
			for(vector<NodeId>::iterator nodeIdItr = partitionCoveringNodes.begin(); nodeIdItr != partitionCoveringNodes.end(); ++nodeIdItr){
				if(partitionCoverGraph.find(*nodeIdItr) == partitionCoverGraph.end()){
					partitionCoverGraph[*nodeIdItr] = vector<const ClusterPartition *>();
				}
				partitionCoverGraph[*nodeIdItr].push_back(clusterPartition);
			}
		}
	}

	// if partionCoverGraph has any edges, we still have partitions to cover
	while(partitionCoverGraph.size() > 0){
		//1. find the node which covers the maximum number of partitions
		map<NodeId, vector<const ClusterPartition *> >::iterator mostCoveringNodeEntry = partitionCoverGraph.begin();
		for(map<NodeId, vector<const ClusterPartition *> >::iterator nodeEntry = partitionCoverGraph.begin();
				nodeEntry != partitionCoverGraph.end(); ++nodeEntry){
			if(nodeEntry->second.size() > mostCoveringNodeEntry->second.size()){
				mostCoveringNodeEntry = nodeEntry;
			}
		}
		NodeId mostCoveringNodeId = mostCoveringNodeEntry->first;
		vector<const ClusterPartition *> mostCoveringNodeCoveredPartitions = mostCoveringNodeEntry->second;
		// 2. use this node to cover as mostCoveringNodeCoveredPartitions partitions
		NodeTargetShardInfo coveringNodeTarget(mostCoveringNodeId, partitionContainer->getCoreId());
		for(unsigned partitionIdx = 0; partitionIdx < mostCoveringNodeCoveredPartitions.size() ; ++partitionIdx){
			const ClusterPartition * coveredPartition = mostCoveringNodeCoveredPartitions.at(partitionIdx);
			addReadPartitionToNodeTargetContainer(coveredPartition, coveringNodeTarget);
		}
		targetNodes[mostCoveringNodeId] = coveringNodeTarget;
		// 3. remove the entry from map and remove all ClusterPartition pointers from other entries
		// a) remove entry from map
		partitionCoverGraph.erase(mostCoveringNodeEntry);
		// b) remove all covered partitions from other entries
		for(unsigned partitionIdx = 0; partitionIdx < mostCoveringNodeCoveredPartitions.size() ; ++partitionIdx){
			const ClusterPartition * coveredPartition = mostCoveringNodeCoveredPartitions.at(partitionIdx);
			for(map<NodeId, vector<const ClusterPartition *> >::iterator nodeEntry = partitionCoverGraph.begin();
					nodeEntry != partitionCoverGraph.end(); ++nodeEntry){
				if(std::find(nodeEntry->second.begin(), nodeEntry->second.end(), coveredPartition)
					!= nodeEntry->second.end()){
					nodeEntry->second.erase(std::find(nodeEntry->second.begin(), nodeEntry->second.end(), coveredPartition));
				}
			}
		}
		// c) remove those partitionCoverGraph entries that their vector is empty now
		map<NodeId, vector<const ClusterPartition *> > partitionCoverGraphTemp;
		for(map<NodeId, vector<const ClusterPartition *> >::iterator nodeEntry = partitionCoverGraph.begin();
				nodeEntry != partitionCoverGraph.end(); ++nodeEntry){
			if(nodeEntry->second.size() > 0){
				partitionCoverGraphTemp[nodeEntry->first] = nodeEntry->second;
			}
		}
		partitionCoverGraph = partitionCoverGraphTemp;

	}

	// add all prepared targets to the output
	for(map<NodeId, NodeTargetShardInfo>::iterator nodeEntry = targetNodes.begin(); nodeEntry != targetNodes.end(); ++nodeEntry){
		targets.push_back(nodeEntry->second);
	}

}


void CorePartitioner::getAllTargets(vector<NodeTargetShardInfo> & target) const{

	map<NodeId, NodeTargetShardInfo> targetsMap;

	vector<const NodePartition *> nodePartitions;
	partitionContainer->getNodePartitionsForRead(nodePartitions);

	for(unsigned i = 0; i < nodePartitions.size(); ++i){
		NodeId nodeId = nodePartitions.at(i)->getNodeId();
		unsigned coreId = nodePartitions.at(i)->getCoreId();
		if(targetsMap.find(nodeId) == targetsMap.end()){
			targetsMap[nodeId] = NodeTargetShardInfo(nodeId, coreId);
		}
		addPartitionToNodeTargetContainer(nodePartitions.at(i), targetsMap[nodeId]);
	}

	vector<const ClusterPartition *> clusterPartitions;
	partitionContainer->getClusterPartitionsForRead(clusterPartitions);

	for(unsigned i = 0 ; i < clusterPartitions.size(); ++i){

		if(clusterPartitions.at(i)->isPartitionLocked()){
			continue;
		}

		unsigned coreId = clusterPartitions.at(i)->getCoreId();

		vector<NodeId> shardLocations;
		clusterPartitions.at(i)->getShardLocations(shardLocations);
		for(vector<NodeId>::iterator nodeItr = shardLocations.begin();
				nodeItr != shardLocations.end(); ++nodeItr){
			if(targetsMap.find(*nodeItr) == targetsMap.end()){
				targetsMap[*nodeItr] = NodeTargetShardInfo(*nodeItr, coreId);
			}
			addWritePartitionToNodeTargetContainer(clusterPartitions.at(i), targetsMap[*nodeItr]);
		}
	}

	for(map<NodeId, NodeTargetShardInfo>::iterator targetsMapItr = targetsMap.begin();
			targetsMapItr != targetsMap.end(); ++targetsMapItr){
		target.push_back(targetsMapItr->second);
	}



}

void CorePartitioner::getAllWriteTargets(unsigned hashKey, NodeId currentNodeId, vector<NodeTargetShardInfo> & targets) const{

	map<NodeId, NodeTargetShardInfo> targetNodes;
	// first get local write targets if this node has any node partition
	const NodePartition * localNodePartition = partitionContainer->getNodePartitionForWrite(hashKey, currentNodeId);
	if(localNodePartition != NULL){
		if(targetNodes.find(currentNodeId) == targetNodes.end()){
			targetNodes[currentNodeId] = NodeTargetShardInfo(currentNodeId, partitionContainer->getCoreId());
		}
		addPartitionToNodeTargetContainer(localNodePartition, targetNodes[currentNodeId]);
	}

	// now add cluster write targets
	const ClusterPartition * writeClusterPartition = partitionContainer->getClusterPartitionForWrite(hashKey);
	if(writeClusterPartition != NULL && ! writeClusterPartition->isPartitionLocked()){

		vector<NodeId> partitionCoveringNodes;
		writeClusterPartition->getShardLocations(partitionCoveringNodes);
		for(vector<NodeId>::iterator coveringNodeIdItr = partitionCoveringNodes.begin();
				coveringNodeIdItr != partitionCoveringNodes.end(); ++coveringNodeIdItr){
			if(targetNodes.find(*coveringNodeIdItr) == targetNodes.end()){
				targetNodes[*coveringNodeIdItr] = NodeTargetShardInfo(*coveringNodeIdItr, partitionContainer->getCoreId());
			}
			addWritePartitionToNodeTargetContainer(writeClusterPartition, targetNodes[*coveringNodeIdItr]);
		}
	}

	// add all prepared targets to the output
	for(map<NodeId, NodeTargetShardInfo>::iterator nodeEntry = targetNodes.begin(); nodeEntry != targetNodes.end(); ++nodeEntry){
		targets.push_back(nodeEntry->second);
	}
}


void CorePartitioner::getAllShardIds(vector<ClusterShardId> & allShardIds) const{
	for(unsigned pid = 0 ; pid < partitionContainer->getTotalNumberOfPartitions() ; ++pid){
		for(unsigned rid = 0; rid <= partitionContainer->getReplicationDegree(); ++rid ){
			allShardIds.push_back(ClusterShardId(partitionContainer->getCoreId(), pid, rid));
		}
	}
}


void CorePartitioner::addReadPartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const{
	vector<unsigned> replicas;
	ASSERT(partition->getNodeReplicaIds(targets.getNodeId(), replicas));
	ASSERT(replicas.size() > 0);
	//TODO always choose one replica?
	unsigned replicaId = replicas.at(0);
	targets.addClusterShard(ClusterShardId(partitionContainer->getCoreId(), partition->getPartitionId(), replicaId));
}

void CorePartitioner::addWritePartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const{
	vector<unsigned> replicas;
	if(! partition->getNodeReplicaIds(targets.getNodeId(), replicas)){
		ASSERT(false);
		return;
	}
	ASSERT(replicas.size() > 0);
	for(vector<unsigned>::iterator replicaIdItr = replicas.begin(); replicaIdItr != replicas.end(); ++replicaIdItr){
		targets.addClusterShard(ClusterShardId(partitionContainer->getCoreId(), partition->getPartitionId(), *replicaIdItr));
	}
}

void CorePartitioner::addPartitionToNodeTargetContainer(const NodePartition * partition, NodeTargetShardInfo & targets ) const{
	vector<unsigned> nodeInternalPartitionIds;
	partition->getInternalPartitionIds(nodeInternalPartitionIds);
	for(vector<unsigned>::iterator nodeInternalPIdItr = nodeInternalPartitionIds.begin();
			nodeInternalPIdItr < nodeInternalPartitionIds.end(); ++nodeInternalPIdItr){
		targets.addNodeShard(NodeShardId(partition->getCoreId(), partition->getNodeId(), *nodeInternalPIdItr));
	}
}

// computes the hash value of a string
unsigned CorePartitioner::hashDJB2(const char *str) const {
    unsigned hash = 5381;
    unsigned c;
    do
    {
        c = *str;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }while(*str++);
    return hash;
}

}
}


