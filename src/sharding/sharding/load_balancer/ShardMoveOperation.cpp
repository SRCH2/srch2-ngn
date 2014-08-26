#include "ShardMoveOperation.h"
#include "core/util/SerializationHelper.h"
#include "src/core/util/Assert.h"
#include "sharding/configuration/ShardingConstants.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "LoadBalancingStartOperation.h"
#include "server/Srch2Server.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ShardMoveOperation::ShardMoveOperation(const unsigned operationId, const NodeId & srcNodeId, const ClusterShardId & moveShardId):
	OperationState(operationId),shardId(moveShardId){
	this->srcAddress.nodeId = srcNodeId;
	this->lockOperation = NULL;
	this->lockOperationResult = new SerialLockResultStatus();
	this->commitOperation = NULL;
	this->releaseOperation = NULL;
}

OperationState * ShardMoveOperation::entry(){
	/*
	 * 1. make a connection with src node.
	 * 2. lock the shard with self and src node handlers
	 * 3. transfer data
	 * 4. commit
	 * 5. close the connection with src node
	 * 6. release self lock
	 */

	return connect();
}


OperationState * ShardMoveOperation::connect(){
	// send MoveToMeNotification::START to src node.
	MoveToMeNotification::START * moveNotif = new MoveToMeNotification::START(shardId);
	this->send(moveNotif, srcAddress);
	return this;
}
OperationState * ShardMoveOperation::handle(MoveToMeNotification::ACK * ack){
	if(lockOperation != NULL ||  commitOperation != NULL || releaseOperation != NULL){
		return this;// ignore.
	}
	srcAddress = ack->getSrc();
	return acquireLocks();
}

OperationState * ShardMoveOperation::handle(MoveToMeNotification::ABORT * ack){
	if(lockOperation != NULL ||  commitOperation != NULL || releaseOperation != NULL){
		return this;// ignore.
	}
	return LoadBalancingStartOperation::finalizeLoadBalancing();
}

OperationState * ShardMoveOperation::acquireLocks(){
	vector<SingleResourceLockRequest *> lockBatch;
	vector<NodeOperationId> lockHolders;
	lockHolders.push_back(NodeOperationId(ShardManager::getCurrentNodeId(),this->getOperationId()));
	lockHolders.push_back(srcAddress);
	SingleResourceLockRequest * lockRequest = new SingleResourceLockRequest(shardId,
			lockHolders,
			ResourceLockType_X);
	lockBatch.push_back(lockRequest);
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = lockBatch;
	resourceLockRequest->isBlocking = false;
	lockOperation = OperationState::startOperation(new SerialLockOperation(this->getOperationId(), resourceLockRequest, this->lockOperationResult));
	if(lockOperation == NULL){
		if(this->lockOperationResult->grantedFlag == false){
			return abort();
		}else{
			// move to next step
			return transfer();
		}
	}
	return this;
}
OperationState * ShardMoveOperation::handle(LockingNotification::ACK * ack){
	if((lockOperation == NULL && releaseOperation == NULL) || commitOperation != NULL){
		return this;
	}
	if(lockOperation != NULL){
		OperationState::stateTransit(lockOperation, ack);
		if(lockOperation == NULL){
			if(this->lockOperationResult->grantedFlag == false){
				return abort();
			}else{
				// move to next step
				return transfer();
			}
		}
		return this;
	}else if(releaseOperation != NULL){
		OperationState::stateTransit(releaseOperation, ack);
		if(releaseOperation == NULL){
			return finish();
		}
		return this;
	}
	return this;
}

OperationState * ShardMoveOperation::transfer(){
	// start transfering the data
	// call MM to transfer the shard.
	start();
	return this;
}
OperationState * ShardMoveOperation::handle(MMNotification * mmStatus){
	if(mmStatus->getStatus().status == MIGRATION_STATUS_FAIL){
		return release();
	}else if(mmStatus->getStatus().status == MIGRATION_STATUS_FINISH){

		Cluster_Writeview * writeview = ShardManager::getWriteview();

		string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
				writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[shardId.coreId]->getName(), &shardId);
		if(indexDirectory.compare("") == 0){
			indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
					writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[shardId.coreId]->getName(), &shardId);
		}

		physicalShard = LocalPhysicalShard(mmStatus->getStatus().shard, indexDirectory, "");
		return commit();
	}
	return this;
}

OperationState * ShardMoveOperation::commit(){

	// prepare the shard change
	ShardMoveChange * shardMoveChange = new ShardMoveChange(shardId, srcAddress.nodeId, ShardManager::getCurrentNodeId());
	shardMoveChange->setPhysicalShard(physicalShard);
	commitOperation = new CommitOperation(this->getOperationId(), vector<NodeId>(), shardMoveChange);
	commitOperation = OperationState::startOperation(commitOperation);
	if(commitOperation == NULL){
		return release();
	}
	return this;
}
OperationState * ShardMoveOperation::handle(CommitNotification::ACK * ack){
	if(commitOperation == NULL || lockOperation != NULL || releaseOperation != NULL){
		return this; // ignore.
	}
	OperationState::stateTransit(commitOperation, ack);
	if(commitOperation == NULL){
		return release();
	}
	return this;
}


OperationState * ShardMoveOperation::handle(NodeFailureNotification * nodeFailure){
	vector<NodeId> arrivedNodes;
	ShardManager::getWriteview()->getArrivedNodes(arrivedNodes);
	if(std::find(arrivedNodes.begin(), arrivedNodes.end(), srcAddress.nodeId) == arrivedNodes.end()){ // src node died : abort
		if(lockOperation != NULL){
			delete lockOperation;
			lockOperation = NULL;
			return release();
		}
		if(lockOperation == NULL && commitOperation == NULL && releaseOperation == NULL){
			// still transferring data when src node died.
			return release();
		}
		if(commitOperation != NULL){
			// we were lucky, we could move the shard right before this node died.
			// just continue normally.
		}
		if(releaseOperation != NULL){
			// just continue ....
		}
	}
	// now pass the notification to all operations
	if(commitOperation != NULL){
		OperationState::stateTransit(commitOperation, nodeFailure);
		OperationState * nextState = startOperation(commitOperation->handle(nodeFailure));
		if(commitOperation == NULL){
			return release();
		}
		return this;
	}
	if(releaseOperation != NULL){
		OperationState::stateTransit(releaseOperation, nodeFailure);
		if(releaseOperation == NULL){
			return LoadBalancingStartOperation::finalizeLoadBalancing();
		}
		return this;
	}
	return this;
}

OperationState * ShardMoveOperation::handle(Notification * notification){
	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingMoveToMeAbortMessageType:
			return handle((MoveToMeNotification::ABORT *)notification);
		case ShardingMoveToMeACKMessageType:
			return handle((MoveToMeNotification::ACK *)notification);
		case ShardingMMNotificationMessageType:
			return handle((MMNotification *)notification);
		case ShardingCommitACKMessageType:
			return handle((CommitNotification::ACK *)notification);
		case ShardingLockACKMessageType:
			return handle((LockingNotification::ACK * )notification);
		default:
			// ignore;
			return this;
	}
}

string ShardMoveOperation::getOperationName() const {
	return "shard_move";
}
string ShardMoveOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Moving " << shardId.toString() << " from node " << srcAddress.toString() << " %";
	if (lockOperation != NULL) {
		ss << lockOperation->getOperationName() << "%";
		ss << lockOperation->getOperationStatus();
	} else {
		ss << "Lock result : " << lockOperationResult->grantedFlag << "%";
		if(commitOperation == NULL && releaseOperation == NULL){ // we are in transfer session
			ss << "Transferring data ...%" ;
		}else{
			ss << "Transferred " << physicalShard.server->getIndexer()->getNumberOfDocumentsInIndex() << " records.%";
		}

		if(commitOperation != NULL){
			ss << commitOperation->getOperationName() << "%";
			ss << commitOperation->getOperationStatus();
		}

		if(releaseOperation != NULL){
			ss << releaseOperation->getOperationName() << "%";
			ss << releaseOperation->getOperationStatus();
		}
	}

	return ss.str();
}

OperationState * ShardMoveOperation::release(){
	vector<SingleResourceLockRequest *> releaseBatch;
	SingleResourceLockRequest * releaseRequest = new SingleResourceLockRequest(shardId,
			NodeOperationId(ShardManager::getCurrentNodeId(),this->getOperationId()));
	releaseBatch.push_back(releaseRequest);
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = releaseBatch;
	resourceLockRequest->isBlocking = true;
	releaseOperation = new SerialLockOperation(this->getOperationId(), resourceLockRequest);
	releaseOperation = OperationState::startOperation(releaseOperation);
	if(releaseOperation == NULL){
		return finish();
	}
	return this;
}

void ShardMoveOperation::start(){
	MoveToMeNotification *  startNotif = new MoveToMeNotification(shardId);
	this->send(startNotif, srcAddress);
	delete startNotif;
	return;
}

OperationState * ShardMoveOperation::abort(){
	MoveToMeNotification::ABORT * abortNotif = new MoveToMeNotification::ABORT();
	this->send(abortNotif, srcAddress);
	delete abortNotif;
	return LoadBalancingStartOperation::finalizeLoadBalancing();
}

OperationState * ShardMoveOperation::finish(){
	vector<NodeId> arrivedNodes;
	ShardManager::getWriteview()->getArrivedNodes(arrivedNodes);
	if(std::find(arrivedNodes.begin(), arrivedNodes.end(), srcAddress.nodeId) != arrivedNodes.end()){ // src node alive
		MoveToMeNotification::FINISH * finishNotif = new MoveToMeNotification::FINISH();
		this->send(finishNotif, srcAddress);
		delete finishNotif;
	}
	return LoadBalancingStartOperation::finalizeLoadBalancing();
}


ShardMoveSrcOperation::ShardMoveSrcOperation(const NodeOperationId & destination, const ClusterShardId & moveShardId):
	OperationState(OperationState::getNextOperationId()),shardId(moveShardId),destination(destination){
	this->compensateOperation = NULL;
	this->releaseOperation = NULL;
}

OperationState * ShardMoveSrcOperation::entry(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	if(writeview->localClusterDataShards.find(shardId) == writeview->localClusterDataShards.end()){
		ASSERT(false); // Why ? This one might be dangerious
		return abort();
	}
	physicalShard = writeview->localClusterDataShards.find(shardId)->second;
	return connect();
}

OperationState * ShardMoveSrcOperation::handle(MoveToMeNotification::ABORT * ack){
	return NULL;
}
OperationState * ShardMoveSrcOperation::handle(MoveToMeNotification::FINISH * ack){
	return release();
}

OperationState * ShardMoveSrcOperation::handle(MMNotification * mmStatus){
	if(mmStatus->getStatus().status == MIGRATION_STATUS_FAIL){ // dest could not have committed the change
		return release();
	}else if(mmStatus->getStatus().status == MIGRATION_STATUS_FINISH){
		return this; // we should wait for FINISH notification
	}
	ASSERT(false);
	return this;
}

OperationState * ShardMoveSrcOperation::compensate(){
	// prepare a shard assign change and commit it.
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(shardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);

	compensateOperation = new CommitOperation(this->getOperationId(), vector<NodeId>(), shardAssignChange);
	compensateOperation = OperationState::startOperation(compensateOperation);
	if(compensateOperation == NULL){
		return release();
	}
	return this;
}
OperationState * ShardMoveSrcOperation::handle(CommitNotification::ACK * ack){
	if(compensateOperation == NULL || releaseOperation != NULL){
		return this; // ignore.
	}
	OperationState::stateTransit(compensateOperation, ack);
	if(compensateOperation == NULL){
		return release();
	}
	return this;
}

OperationState * ShardMoveSrcOperation::release(){
	vector<SingleResourceLockRequest *> releaseBatch;
	SingleResourceLockRequest * releaseRequest = new SingleResourceLockRequest(shardId,
			NodeOperationId(ShardManager::getCurrentNodeId(),this->getOperationId()));
	releaseBatch.push_back(releaseRequest);
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = releaseBatch;
	resourceLockRequest->isBlocking = true;
	releaseOperation = new SerialLockOperation(this->getOperationId(), resourceLockRequest);
	releaseOperation = OperationState::startOperation(releaseOperation);
	if(releaseOperation == NULL){
		return NULL;
	}
	return this;
}

OperationState * ShardMoveSrcOperation::handle(LockingNotification::ACK * ack){
	if(compensateOperation != NULL || releaseOperation == NULL){
		return this; // ignore.
	}
	OperationState::stateTransit(releaseOperation, ack);
	if(releaseOperation == NULL){
		return NULL;
	}
	return this;
}

OperationState * ShardMoveSrcOperation::handle(NodeFailureNotification * nodeFailure){
	vector<NodeId> arrivedNodes;
	ShardManager::getWriteview()->getArrivedNodes(arrivedNodes);
	if(std::find(arrivedNodes.begin(), arrivedNodes.end(), destination.nodeId) == arrivedNodes.end()){ // dest node died : abort
		if(compensateOperation == NULL && releaseOperation == NULL){
			return compensate();
		}
		// compensate cannot be non-NULL because dest node cannot die two times
		if(releaseOperation != NULL){
			// just continue with release ... NOTE : we lost the data
		}
	}
	if(releaseOperation != NULL){
		OperationState::stateTransit(releaseOperation, nodeFailure);
		if(releaseOperation == NULL){
			return NULL;
		}
		return this;
	}
	return this;
}


OperationState * ShardMoveSrcOperation::handle(Notification * notification){
	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingMoveToMeAbortMessageType:
			return handle((MoveToMeNotification::ABORT *)notification);
		case ShardingMoveToMeFinishMessageType:
			return handle((MoveToMeNotification::FINISH *)notification);
		case ShardingMMNotificationMessageType:
			return handle((MMNotification *)notification);
		case ShardingCommitACKMessageType:
			return handle((CommitNotification::ACK *)notification);
		case ShardingLockACKMessageType:
			return handle((LockingNotification::ACK * )notification);
		default:
			// ignore;
			return this;
	}
}

OperationState * ShardMoveSrcOperation::connect(){
	MoveToMeNotification::ACK * ack = new MoveToMeNotification::ACK();
	this->send(ack, destination);
	delete ack;
	return this;
}

OperationState * ShardMoveSrcOperation::abort(){
	MoveToMeNotification::ABORT * abortNotif = new MoveToMeNotification::ABORT();
	this->send(abortNotif, destination);
	delete abortNotif;
	return NULL;
}


}
}

