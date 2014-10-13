#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>
#include <cstdio>

#include "sharding/sharding/metadata_manager/ResourceMetadataChange.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"
#include "sharding/sharding/metadata_manager/Cluster.h"
#include "sharding/sharding/metadata_manager/Cluster_Writeview.h"
#include "sharding/configuration/CoreInfo.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/sharding/metadata_manager/MetadataInitializer.h"
#include "sharding/sharding/ShardManager.h"
#include "server/Srch2Server.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

typedef ClusterShardId R;
typedef NodeOperationId O;

enum MetadataChangeTestCaseCode{
	MetadataChangeTestCaseCode_NodeAddChange0,
	MetadataChangeTestCaseCode_NodeAddChange1,
	MetadataChangeTestCaseCode_ShardAssignChange0,
	MetadataChangeTestCaseCode_ShardAssignChange1,
	MetadataChangeTestCaseCode_ShardMoveChange0,
	MetadataChangeTestCaseCode_ShardMoveChange1,
	MetadataChangeTestCaseCode_ShardLoadChange0,
	MetadataChangeTestCaseCode_EOF
};

MetadataChange * createMetadataChange(const MetadataChangeTestCaseCode caseCode){
	switch (caseCode) {
	case MetadataChangeTestCaseCode_NodeAddChange0:
	{
		NodeId newNodeId = 4;
		vector<ClusterShardId> localClusterShardIds;
		localClusterShardIds.push_back(R(0,1,1));
		localClusterShardIds.push_back(R(0,2,0));
		localClusterShardIds.push_back(R(1,1,3));
		vector<NodeShardId> localNodeShardIds;
		localNodeShardIds.push_back(NodeShardId(0, 4, 6));
		localNodeShardIds.push_back(NodeShardId(0, 4, 7));
		return new NodeAddChange(newNodeId, localClusterShardIds, localNodeShardIds);
	}
	case MetadataChangeTestCaseCode_NodeAddChange1:
	{
		NodeId newNodeId = 0;
		vector<ClusterShardId> localClusterShardIds;
		vector<NodeShardId> localNodeShardIds;
		localNodeShardIds.push_back(NodeShardId(0, 0, 6));
		localNodeShardIds.push_back(NodeShardId(0, 0, 3));
		return new NodeAddChange(newNodeId, localClusterShardIds, localNodeShardIds);
	}
	case MetadataChangeTestCaseCode_ShardAssignChange0:
	{
		ClusterShardId logicalShardToAssign(0,1,1);
		NodeId location = 3;
		double load = -1;
		return new ShardAssignChange(logicalShardToAssign, location, load);
	}
	case MetadataChangeTestCaseCode_ShardAssignChange1:
	{
		ClusterShardId logicalShardToAssign(2,1,3);
		NodeId location = 3;
		double load = 1;
		return new ShardAssignChange(logicalShardToAssign, location, load);
	}
	case MetadataChangeTestCaseCode_ShardMoveChange0:
	{
		ClusterShardId shardId(0,1,2);
		NodeId srcNodeId = 3;
		NodeId destNodeId = 4;
		return new ShardMoveChange(shardId, srcNodeId, destNodeId);
	}
	case MetadataChangeTestCaseCode_ShardMoveChange1:
	{
		ClusterShardId shardId(1,0,1);
		NodeId srcNodeId = 2;
		NodeId destNodeId = 5;
		return new ShardMoveChange(shardId, srcNodeId, destNodeId);
	}
	case MetadataChangeTestCaseCode_ShardLoadChange0:
	{
		map<ClusterShardId, double> addedLoads;
		addedLoads[R(0,1,1)] = 3;
		addedLoads[R(0,2,1)] = 2;
		addedLoads[R(2,1,0)] = -1;
		return new ShardLoadChange(addedLoads);
	}
	case MetadataChangeTestCaseCode_EOF:
		ASSERT(false);
		return NULL;
	}
}

enum NodeStatusAfterRestart{
	NodeStatus_Unassigned,
	NodeStatus_Pending,
	NodeStatus_Ready
};

void clean(ConfigManager * serverConf, const string & clusterName){
	string clusterFileDirectoryPath = serverConf->getClusterDir(clusterName);
	if(clusterFileDirectoryPath.compare("") == 0){
		clusterFileDirectoryPath = serverConf->createClusterDir(clusterName);
	}
	clusterFileDirectoryPath = clusterFileDirectoryPath + "/Cluster.idx";
	remove( clusterFileDirectoryPath.c_str() );
}

void testFreshClusterInit(ConfigManager * serverConf , ResourceMetadataManager * metadataManager){

	serverConf->loadConfigFile(metadataManager);
	clean(serverConf, metadataManager->getClusterWriteview()->clusterName);
	srch2http::MetadataInitializer nodeInitializer(serverConf, metadataManager);
	nodeInitializer.initializeNode();

	// set the current node ID, and add this node.
	Node * currentNode = new Node("currentNode", "192.168.0.0" , 5050, true, 4, true);
	currentNode->setId(0);
	metadataManager->getClusterWriteview()->setCurrentNodeId(currentNode->getId());
	metadataManager->getClusterWriteview()->addNode(*currentNode);
	metadataManager->getClusterWriteview()->setNodeState(currentNode->getId(), ShardingNodeStateArrived);

	nodeInitializer.initializeCluster();
	// validate :
	// 1. only one shard of each partition must be assigned to this node and the rest of
	//    shards must be unassigned.
	ClusterShardId id;double load;ShardState state;bool isLocal;NodeId nodeId;NodeShardId nodeShardId;
    ClusterShardIterator cShardItr(metadataManager->getClusterWriteview());
    cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(id.replicaId == 0){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
			ASSERT(nodeId == currentNode->getId());
		}else{
			ASSERT(state == SHARDSTATE_UNASSIGNED);
			ASSERT(! isLocal);
		}
	}
	// 2. we must have two node shards in the current node
    NodeShardIterator nShardItr(metadataManager->getClusterWriteview());
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
		ASSERT(nodeShardId.partitionId > 3); // we have 4 partitions
		ASSERT(nodeShardId.nodeId == currentNode->getId());
		ASSERT(isLocal);
	}
	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);
	// 4. we must have 2 local shards both local
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);

}

void testNode1FirstArrival(NodeId currentNodeId, ResourceMetadataManager * metadataManager){

	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	// new node
	Node * newNode = new Node("node1", "192.168.0.1", 5051, false);
	newNode->setId(1);
	// this node has 2 node shards : (1,1,16) , (1,1,17)
	vector<NodeShardId> nodeShards;
	nodeShards.push_back(NodeShardId(0,1,16));
	nodeShards.push_back(NodeShardId(0,1,17));
	vector<ClusterShardId> clusterShards;
	NodeAddChange * nodeAddChange = new NodeAddChange(newNode->getId(), clusterShards, nodeShards);

	// SM comes first
	writeview->addNode(*newNode);
	nodeAddChange->doChange(writeview);



	/// validate
	// 1. we must have 2 nodes, both of them in arrived state
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	ASSERT(allNodes.size() == 2);
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes);
	ASSERT(arrivedNodes.size() == 1);
	arrivedNodes.clear();
	writeview->getArrivedNodes(arrivedNodes, true);
	ASSERT(arrivedNodes.size() == 2);


	// 2. we must have no non local cluster shards
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
    ClusterShardIterator cShardItr(metadataManager->getClusterWriteview());
    cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(id.replicaId == 0){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
		}else{
			ASSERT(state == SHARDSTATE_UNASSIGNED);
			ASSERT(! isLocal);
		}
	}
	// and also if we use the local iteration :
	cShardItr.beginClusterShardsIteration();
	unsigned counter = 0;
	while(cShardItr.getNextLocalClusterShard(id, load, localPhysicalShard)){
		ASSERT(id.replicaId == 0);
		counter ++;
	}
	ASSERT(counter == 4);
	// 3. we must have 4 node shards, 2 of them local, 2 of them from node
    NodeShardIterator nShardItr(metadataManager->getClusterWriteview());
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		if(nodeShardId.nodeId == currentNodeId){
			ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
			ASSERT(nodeShardId.partitionId > 3); // we have 4 partitions
			ASSERT(isLocal);
		}else if(nodeShardId.nodeId == 1){
			ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
			ASSERT(nodeShardId.partitionId == 16 || nodeShardId.partitionId == 17); // we have 4 partitions
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}
	}

	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);
	// 4. we must have 2 local shards
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);


}

void testNode1LoadBalancing(NodeId currentNodeId, ResourceMetadataManager * metadataManager){
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	//  current node has 4 cluster shards,
	//  we must assign 4 cluster shards to the second node
	ShardAssignChange * assignChange = new ShardAssignChange(ClusterShardId(0,0,1), 1, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,1,1), 1, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,2,1), 1, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,3,1), 1, 0);
	assignChange->doChange(writeview);

	// validate
	// 1. we must have 8 cluster shards, 4 local for on node 2
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(nodeId == currentNodeId){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
		}else if(nodeId == 1){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(! isLocal);
		}else{
			ASSERT(state == SHARDSTATE_UNASSIGNED);
			ASSERT(! isLocal);
		}
	}
	// and also if we use the local iteration :
	cShardItr.beginClusterShardsIteration();
	unsigned counter = 0;
	while(cShardItr.getNextLocalClusterShard(id, load, localPhysicalShard)){
		ASSERT(id.replicaId == 0);
		counter ++;
	}
	ASSERT(counter == 4);
	// and two local shards
	ASSERT(writeview->localClusterDataShards.size() == 4);

	// 2. we must have 4 node shards, 2 local, 2 on node 2
	NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		if(nodeShardId.nodeId == currentNodeId){
			ASSERT(isLocal);
		}else if(nodeShardId.nodeId == 1){
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}
	}
	ASSERT(writeview->localNodeDataShards.size() == 3);

}

void testNode2FirstArrival(NodeId currentNodeId, ResourceMetadataManager * metadataManager){
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	// new node
	Node * newNode = new Node("node2", "192.168.0.2", 5052, false);
	newNode->setId(2);
	// this node has no node shards
	vector<NodeShardId> nodeShards;
	vector<ClusterShardId> clusterShards;
	NodeAddChange * nodeAddChange = new NodeAddChange(newNode->getId(), clusterShards, nodeShards);

	nodeAddChange->doChange(writeview);
	// SM comes late
	writeview->addNode(*newNode);



	/// validate
	// 1. we must have 3 nodes, all of them in arrived state
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	ASSERT(allNodes.size() == 3);
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes);
	ASSERT(arrivedNodes.size() == 2);
	arrivedNodes.clear();
	writeview->getArrivedNodes(arrivedNodes, true);
	ASSERT(arrivedNodes.size() == 3);



	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);
	// 4. we must have 2 local shards
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);
}

void validateRestart(ConfigManager * serverConf, ResourceMetadataManager * metadataManager,
		NodeStatusAfterRestart * nodeState){
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	// validate :

	// 1. We must have 4 local cluster shards which are ready and 4 pending shards
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){

		if(nodeId == writeview->currentNodeId){
			// current node
			ASSERT(nodeState[nodeId] == NodeStatus_Ready);
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(id.replicaId == 0);
			ASSERT(id.coreId == 0);
			ASSERT(isLocal);
		}else if(nodeId == 1 || nodeId == 2){
			ASSERT(nodeState[nodeId] == NodeStatus_Ready);
			ASSERT(id.replicaId == nodeId); // we give all same number replicas to one node in this test
			ASSERT(id.coreId == 0);
			ASSERT(! isLocal);
			break;

		}else if (nodeId == (unsigned)-1){
			ASSERT(state == SHARDSTATE_PENDING || state == SHARDSTATE_UNASSIGNED);
			ASSERT(id.coreId == 0);
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}

	}

	// 2. we must have 2 node shards which are local
	NodeShardIterator nShardItr(metadataManager->getClusterWriteview());
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		if(nodeState[2] == NodeStatus_Ready){
			if(nodeShardId.nodeId == writeview->currentNodeId){
				ASSERT(isLocal);
				ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
				ASSERT(nodeShardId.partitionId > 3); // we have 4 partitions
			}else{
				ASSERT(nodeShardId.nodeId == 2);
				ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
				ASSERT(nodeShardId.partitionId > 15); // we have 4 partitions
			}
		}else{
			ASSERT(nodeShardId.nodeId == writeview->currentNodeId);
			ASSERT(isLocal);
			ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
			ASSERT(nodeShardId.partitionId > 3); // we have 4 partitions
		}
	}
	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);

	// 4. we must have 2 local shards both local
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);
}


void testNode1Reclaim(NodeId currentNodeId, ResourceMetadataManager * metadataManager){

	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	// new node
	Node * newNode = new Node("node1", "192.168.0.1", 5051, false);
	newNode->setId(1);
	// this node has 2 node shards : (1,2,16) , (1,2,17)
	vector<NodeShardId> nodeShards;
	nodeShards.push_back(NodeShardId(0,1,16));
	nodeShards.push_back(NodeShardId(0,1,17));
	// this node has 4 cluster shards (1,0,1), (1,1,1), (1,2,1) , (1,3,1)
	vector<ClusterShardId> clusterShards;
	clusterShards.push_back(ClusterShardId(0,0,1));
	clusterShards.push_back(ClusterShardId(0,1,1));
	clusterShards.push_back(ClusterShardId(0,2,1));
	clusterShards.push_back(ClusterShardId(0,3,1));
	NodeAddChange * nodeAddChange = new NodeAddChange(newNode->getId(), clusterShards, nodeShards);

	// SM comes first
	writeview->addNode(*newNode);
	nodeAddChange->doChange(writeview);



	/// validate
	// 1. we must have 2 nodes, both of them in arrived state
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	ASSERT(allNodes.size() == 2);
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes);
	ASSERT(arrivedNodes.size() == 1);
	arrivedNodes.clear();
	writeview->getArrivedNodes(arrivedNodes, true);
	ASSERT(arrivedNodes.size() == 2);


	// 2. we must have 4 local cluster shards and 4 on node 2
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
	ClusterShardIterator cShardItr(metadataManager->getClusterWriteview());
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(id.replicaId == 0){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
			ASSERT(writeview->currentNodeId == nodeId);
		}else if (id.replicaId == 1){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(! isLocal);
			ASSERT(nodeId == 1);
		}else{
			ASSERT(state == SHARDSTATE_UNASSIGNED);
			ASSERT(! isLocal);
		}
	}
	// and also if we use the local iteration :
	cShardItr.beginClusterShardsIteration();
	unsigned counter = 0;
	while(cShardItr.getNextLocalClusterShard(id, load, localPhysicalShard)){
		ASSERT(id.replicaId == 0);
		counter ++;
	}
	ASSERT(counter == 4);
	// 3. we must have 4 node shards, 2 of them local, 2 of them from node
    NodeShardIterator nShardItr(metadataManager->getClusterWriteview());
    nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		if(nodeShardId.nodeId == currentNodeId){
			ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
			ASSERT(nodeShardId.partitionId > 3); // we have 4 partitions
			ASSERT(isLocal);
		}else if(nodeShardId.nodeId == 1){
			ASSERT(nodeShardId.coreId == 0); // core id in this test is 0
			ASSERT(nodeShardId.partitionId == 16 || nodeShardId.partitionId == 17); // we have 4 partitions
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}
	}

	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);
	// 4. we must have 2 local shards
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);


}

void testNode2LoadBalancing(NodeId currentNodeId, ResourceMetadataManager * metadataManager){
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	//  current node has 4 cluster shards,
	//  we must assign 4 cluster shards to the second node
	ShardAssignChange * assignChange = NULL;
	assignChange = new ShardAssignChange(ClusterShardId(0,0,2), 2, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,1,2), 2, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,2,2), 2, 0);
	assignChange->doChange(writeview);
	assignChange = new ShardAssignChange(ClusterShardId(0,3,2), 2, 0);
	assignChange->doChange(writeview);

	// validate
	// 1. we must have 12 cluster shards, 4 local, 4 on node 2, 4 on node 3
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(nodeId == currentNodeId){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
		}else if(nodeId == 1){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(! isLocal);
		}else if(nodeId == 2){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}
	}
	// and also if we use the local iteration :
	cShardItr.beginClusterShardsIteration();
	unsigned counter = 0;
	while(cShardItr.getNextLocalClusterShard(id, load, localPhysicalShard)){
		ASSERT(id.replicaId == 0);
		counter ++;
	}
	ASSERT(counter == 4);
	// and two local shards
	ASSERT(writeview->localClusterDataShards.size() == 4);

	// 2. we must have 4 node shards, 2 local, 2 on node 2
    NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		if(nodeShardId.nodeId == currentNodeId){
			ASSERT(isLocal);
		}else if(nodeShardId.nodeId == 1){
			ASSERT(! isLocal);
		}else{
			ASSERT(false);
		}
	}
	ASSERT(writeview->localNodeDataShards.size() == 3);

}


void testNode1Failure(ResourceMetadataManager * metadataManager){
	// node id 2 is going to die
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	writeview->removeNode(1);

	// validate

	// 1. we must have two arrived nodes and three nodes in total.
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	ASSERT(allNodes.size() == 3);
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes);
	ASSERT(arrivedNodes.size() == 1);
	arrivedNodes.clear();
	writeview->getArrivedNodes(arrivedNodes, true);
	ASSERT(arrivedNodes.size() == 2);


	// 2. we must have 4 cluster shards on local node and 4 on node Id 3
	ClusterShardId id;double load;ShardState state;bool isLocal;
	NodeId nodeId;NodeShardId nodeShardId;LocalPhysicalShard localPhysicalShard;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state == SHARDSTATE_UNASSIGNED){
			continue;
		}
		if(id.replicaId == 0){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(isLocal);
			ASSERT(nodeId == writeview->currentNodeId);
		}else if (id.replicaId == 1){
			ASSERT(state == SHARDSTATE_UNASSIGNED );
			ASSERT(! isLocal);
			ASSERT(nodeId == 0);
		}else if(id.replicaId == 2){
			ASSERT(state == SHARDSTATE_READY);
			ASSERT(! isLocal);
			ASSERT(nodeId == 2);
		}else{
			ASSERT(false);
		}
	}

	// 3. we must have only two node shards that belong to current node
	NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		ASSERT(nodeShardId.nodeId == writeview->currentNodeId);
		ASSERT(isLocal);
	}

	// 4. we must have 4 local cluster shards and two local node shards
	ASSERT(writeview->localClusterDataShards.size() == 4);
	ASSERT(writeview->localNodeDataShards.size() == 3);

}


void testNode2Reclaim(NodeId currentNodeId, ResourceMetadataManager * metadataManager){
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview();
	// new node
	Node * newNode = new Node("node2", "192.168.0.2", 5052, false);
	newNode->setId(2);
	// this node has no node shards
	vector<NodeShardId> nodeShards;
	vector<ClusterShardId> clusterShards;
	clusterShards.push_back(ClusterShardId(0,0,2));
	clusterShards.push_back(ClusterShardId(0,1,2));
	clusterShards.push_back(ClusterShardId(0,2,2));
	clusterShards.push_back(ClusterShardId(0,3,2));
	NodeAddChange * nodeAddChange = new NodeAddChange(newNode->getId(), clusterShards, nodeShards);

	nodeAddChange->doChange(writeview);
	// SM comes late
	writeview->addNode(*newNode);



	/// validate
	// 1. we must have 3 nodes, all of them in arrived state
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	ASSERT(allNodes.size() == 2);
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes);
	ASSERT(arrivedNodes.size() == 1);
	arrivedNodes.clear();
	writeview->getArrivedNodes(arrivedNodes, true);
	ASSERT(arrivedNodes.size() == 2);



	// 3. we must have four local cluster shards
	ASSERT(metadataManager->getClusterWriteview()->localClusterDataShards.size() == 4);
	// 4. we must have 2 local shards
	ASSERT(metadataManager->getClusterWriteview()->localNodeDataShards.size() == 3);
}

void restart(const string & confPath, ConfigManager * & serverConf, ResourceMetadataManager * metadataManager){
	// save and kill
	MetadataInitializer initializer(serverConf, metadataManager);
	initializer.saveToDisk(metadataManager->getClusterWriteview()->clusterName);
	delete serverConf;
	serverConf = new ConfigManager(confPath);

	Cluster_Writeview * writeviewOld = new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	ASSERT(*(metadataManager->getClusterWriteview()) == *writeviewOld);

	serverConf->loadConfigFile(metadataManager);
	srch2http::MetadataInitializer nodeInitializer(serverConf, metadataManager);
	nodeInitializer.initializeNode();

	// set current node info
	// set the current node ID, and add this node.
	Node * currentNode = new Node("currentNode", "192.168.0.0" , 5050, true, 4, true);
	currentNode->setId(0);
	metadataManager->getClusterWriteview()->setCurrentNodeId(currentNode->getId());
	metadataManager->getClusterWriteview()->addNode(*currentNode);
	metadataManager->getClusterWriteview()->setNodeState(currentNode->getId(), ShardingNodeStateArrived);

}


Cluster_Writeview * createWriteview(const unsigned code, const string & confPath){

	unsigned codeCounter = 0;

	ConfigManager * serverConf = new ConfigManager(confPath);
	ResourceMetadataManager * metadataManager = new ResourceMetadataManager();
	ShardManager * shardManager = ShardManager::createShardManager(serverConf, metadataManager);

	NodeStatusAfterRestart * nodeState = new NodeStatusAfterRestart[3];
	testFreshClusterInit(serverConf, metadataManager);
	nodeState[0] = NodeStatus_Ready;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	testNode1FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	testNode1LoadBalancing(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	testNode2FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	restart(confPath, serverConf, metadataManager);
	nodeState[1] = NodeStatus_Pending;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	validateRestart(serverConf, metadataManager, nodeState);

	// start adding nodes and and reclaim the shards.
	// first node comes back to reclaim its shards
	testNode1Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}
	// second node comes again
	testNode2FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	// move 4 last shards to the second arrived node
	testNode2LoadBalancing(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	// now the first arrived node fails
	testNode1Failure(metadataManager);
	nodeState[1] = NodeStatus_Unassigned;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	// save and kill
	restart(confPath, serverConf, metadataManager);
	nodeState[2] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}


	restart(confPath, serverConf, metadataManager);
	validateRestart(serverConf, metadataManager, nodeState);

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	// second node comes again
	testNode2Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	restart(confPath, serverConf, metadataManager);
	nodeState[2] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	if(code == codeCounter++){
		delete serverConf;
		ShardManager::deleteShardManager();
		return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
	}

	// second node comes again
	testNode2Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	delete serverConf;
	ShardManager::deleteShardManager();
	return new Cluster_Writeview(*(metadataManager->getClusterWriteview()));
}
