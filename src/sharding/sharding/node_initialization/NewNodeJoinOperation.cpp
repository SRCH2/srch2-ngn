
#include "NewNodeJoinOperation.h"
#include "../metadata_manager/MetadataInitializer.h"
#include "NewNodeLockOperation.h"
#include "../CommitOperation.h"
#include "../SerialLockOperation.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

NewNodeJoinOperation::NewNodeJoinOperation():OperationState(OperationState::getNextOperationId()){
	lockerOperation = NULL;
	commitOperation = NULL;
	releaseOperation = NULL;
	randomNodeToReadFrom = 0;
}
// when we reach to this function, we the metadata is locked and we can
// read and update the information as we want.
OperationState * NewNodeJoinOperation::entry(){
	/*
	 * 1. acquire X lock on local cluster resources and S lock on the rest.
	 * 2. synchronize the metadata
	 * 3. commit
	 * 4. release all locks
	 */
	acquireLocks();
	return this;
}

OperationState * NewNodeJoinOperation::handle(Notification * notification){
	if(notification == NULL){
		ASSERT(false);
		return this;
	}
	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingNewNodeLockACKMessageType:
			return handle((NewNodeLockNotification::ACK *)notification);
		case ShardingNewNodeReadMetadataReplyMessageType:
			return handle((MetadataReport *)notification);
		case ShardingCommitACKMessageType:
			return handle((CommitNotification::ACK *)notification);
		case ShardingLockACKMessageType:
			return handle((LockingNotification::ACK * )notification);
		default:
			// ignore;
			return this;
	}
}

OperationState * NewNodeJoinOperation::handle(NodeFailureNotification * nodeFailure){

	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// 1. check if we are the only left node:
	unsigned numberOfRemainingOlderNodes = 0;
	bool isMetadataSrcAlive = false;
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = writeview->nodes.begin();
			nodeItr != writeview->nodes.end(); ++nodeItr){
		if(nodeItr->second.first != ShardingNodeStateFailed && nodeItr->first < writeview->currentNodeId){
			numberOfRemainingOlderNodes ++;
			if(randomNodeToReadFrom == nodeItr->first){
				isMetadataSrcAlive = true;
			}
		}
	}

	if(numberOfRemainingOlderNodes == 0){
		//use metadata initializer to make sure no partition remains unassigned.
		MetadataInitializer metadataInitializer(ShardManager::getShardManager()->getConfigManager(),
				ShardManager::getShardManager()->getMetadataManager());
		metadataInitializer.initializeCluster();
		return finalizeJoin();
	}

	// if we are not alone, pass notification to operators.
	if(lockerOperation != NULL){
		OperationState::stateTransit(lockerOperation, nodeFailure);
		if(lockerOperation == NULL){
			return readMetadata();
		}
		return this;
	}

	if(commitOperation != NULL){
		OperationState::stateTransit(commitOperation, nodeFailure);
		OperationState * nextState = startOperation(commitOperation->handle(nodeFailure));
		//TODO : why is the above line needed ?
		if(commitOperation == NULL){
			return releaseLocks();
		}
		return this;
	}

	if(releaseOperation != NULL){
		OperationState::stateTransit(releaseOperation, nodeFailure);
		if(releaseOperation == NULL){
			return finalizeJoin();
		}
		return this;
	}

	// so we are reading the metadata

	if(! isMetadataSrcAlive){
		return readMetadata();
	}

	return this;

}

OperationState * NewNodeJoinOperation::acquireLocks(){
	// create a lock operation.
	NewNodeLockOperation * lockOperation = new NewNodeLockOperation(this->getOperationId());
	lockerOperation = OperationState::startOperation(lockOperation);
	if(lockerOperation == NULL){
		return readMetadata();
	}
	return this;
}
OperationState * NewNodeJoinOperation::handle(NewNodeLockNotification::ACK * ack){
	if(lockerOperation == NULL || commitOperation != NULL ||
			releaseOperation != NULL){
		return this; // ignore, we are passed this point.
	}

	OperationState::stateTransit(lockerOperation, ack);
	if(lockerOperation == NULL){
		return readMetadata();
	}
	return this;
}

OperationState * NewNodeJoinOperation::readMetadata(){
	ASSERT(lockerOperation == NULL && commitOperation == NULL && releaseOperation == NULL);
	// send read_metadata notification to the smallest node id
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);

	srand(time(NULL));
	this->randomNodeToReadFrom = olderNodes.at(rand() % olderNodes.size());

	MetadataReport::REQUEST * readMetadataNotif = new MetadataReport::REQUEST();
	NodeOperationId destAddress(this->randomNodeToReadFrom);
	this->send(readMetadataNotif, destAddress);
	delete readMetadataNotif;
	return this;
}

OperationState * NewNodeJoinOperation::handle(MetadataReport * report){
	if(report == NULL){
		ASSERT(false);
		return this;
	}
	if(lockerOperation != NULL || commitOperation != NULL ||
			releaseOperation != NULL){
		return this; // ignore, we are passed this point.
	}
	Cluster_Writeview * clusterWriteview = report->getWriteview();
	if(clusterWriteview == NULL){
		ASSERT(false);
		//use metadata initializer to make sure no partition remains unassigned.
		MetadataInitializer metadataInitializer(ShardManager::getShardManager()->getConfigManager(),
				ShardManager::getShardManager()->getMetadataManager());
		metadataInitializer.initializeCluster();
		return finalizeJoin();
	}
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	// attach data pointers of current writeview to the new writeview coming from
	// the minID node.
	currentWriteview->fixClusterMetadataOfAnotherNode(clusterWriteview);

	// new writeview is ready, replace current writeview with the new one
	ShardManager::getShardManager()->getMetadataManager()->setWriteview(clusterWriteview);
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();

	// ready to commit.
	return commit();
}


OperationState * NewNodeJoinOperation::commit(){
	// prepare the commit operation
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	vector<ClusterShardId> localClusterShards;
	vector<NodeShardId> nodeShardIds;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextLocalClusterShard(id, load, physicalShard)){
		localClusterShards.push_back(id);
	}

	NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextLocalNodeShard(nodeShardId, load, physicalShard)){
		nodeShardIds.push_back(nodeShardId);
	}

	NodeAddChange * nodeAddChange =
			new NodeAddChange(ShardManager::getCurrentNodeId(),localClusterShards, nodeShardIds);
	vector<NodeId> olderNodes;
	CommitOperation * commitOperation = new CommitOperation(this->getOperationId(), nodeAddChange, olderNodes);
	this->commitOperation = OperationState::startOperation(commitOperation);
	if(this->commitOperation == NULL){
		return releaseLocks();
	}
	return this;
}

OperationState * NewNodeJoinOperation::handle(CommitNotification::ACK * ack){
	if(lockerOperation != NULL || commitOperation == NULL ||
			releaseOperation != NULL){
		return this; // ignore, we are passed this point.
	}

	OperationState::stateTransit(commitOperation, ack);
	if(commitOperation == NULL){
		return releaseLocks();
	}
	return this;
}

OperationState * NewNodeJoinOperation::releaseLocks(){

	Cluster_Writeview * writeview = ShardManager::getWriteview();

	vector<SingleResourceLockRequest *> batch;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		batch.push_back(new SingleResourceLockRequest(id, NodeOperationId(writeview->currentNodeId, this->getOperationId())));
	}

	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = batch;
	resourceLockRequest->isBlocking = true;

	SerialLockOperation * releaseOperation = new SerialLockOperation(this->getOperationId(), resourceLockRequest);
	this->releaseOperation = OperationState::startOperation(releaseOperation);
	if(releaseOperation == NULL){
		return finalizeJoin();
	}
	return this;
}

OperationState * NewNodeJoinOperation::handle(LockingNotification::ACK * ack){
	if(lockerOperation != NULL || commitOperation != NULL ||
			releaseOperation == NULL){
		return this; // ignore, we are passed this point.
	}

	OperationState::stateTransit(releaseOperation, ack);
	if(releaseOperation == NULL){
		return finalizeJoin();
	}
	return this;
}

string NewNodeJoinOperation::getOperationName() const {
	return "new_node_join";
};
string NewNodeJoinOperation::getOperationStatus() const {
	stringstream ss;
	if(lockerOperation != NULL){
		ss << lockerOperation->getOperationName() << "%";
		ss << lockerOperation->getOperationStatus();
    }else if (commitOperation != NULL){
		ss << commitOperation->getOperationName() << "%";
		ss << commitOperation->getOperationStatus();
	}else if(releaseOperation != NULL){
		ss << releaseOperation->getOperationName() << "%";
		ss << releaseOperation->getOperationStatus();
	}else { // all null
		ss << "Node to give metadata report : " << randomNodeToReadFrom << "%";
	}
	return ss.str();
};

OperationState * NewNodeJoinOperation::finalizeJoin(){
	// release is also done.
	// just setJoined and done.
	ShardManager::getShardManager()->setJoined();
	return NULL;
}

void NewNodeJoinOperation::getOlderNodesList(vector<NodeId> & olderNodes){
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = currentWriteview->nodes.begin();
			nodeItr != currentWriteview->nodes.end(); ++nodeItr){
		if(nodeItr->first >= currentWriteview->currentNodeId){
			continue;
		}
		if(nodeItr->second.first == ShardingNodeStateFailed){
			continue;
		}
		ASSERT(nodeItr->second.second != NULL); // we must have seen this node
		olderNodes.push_back(nodeItr->first);
	}
}

}
}
