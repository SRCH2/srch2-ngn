#include "ShardMoveOperation.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "server/Srch2Server.h"
#include "./cluster_transactions/LoadBalancer.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../../configuration/ShardingConstants.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ShardMoveOperation::ShardMoveOperation(const NodeId & srcNodeId,
		const ClusterShardId & moveShardId, ConsumerInterface * consumer):shardId(moveShardId){
	this->srcAddress = NodeOperationId(srcNodeId, OperationState::DataRecoveryOperationId);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId());
	this->successFlag = true;
	this->finalizedFlag = false;
	this->releasingMode = false;
	this->locker = NULL;
	this->releaser = NULL;
	this->committer = NULL;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(consumer);
	this->moveToMeNotif = NULL;
	this->cleaupNotif = NULL;

	this->currentOp = PreStart;
}

ShardMoveOperation::~ShardMoveOperation(){
	if(this->locker != NULL){
		delete this->locker;
	}
	if(this->releaser != NULL){
		delete this->releaser;
	}
	if(this->moveToMeNotif != NULL){
		delete this->moveToMeNotif;
	}
	if(this->committer != NULL){
		delete this->committer;
	}
	if(this->cleaupNotif != NULL){
		delete this->cleaupNotif;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	if(mmStatus->getStatus().status == MM_STATUS_FAILURE){ // dest could not have committed the change
		return release();
	}else if(mmStatus->getStatus().status == MM_STATUS_SUCCESS){
		return this; // we should wait for FINISH notification
	}
	ASSERT(false);
	return this;
}

OperationState * ShardMoveSrcOperation::compensate(){
	if(compensateOperation != NULL){
		return this;
	}
	// prepare a shard assign change and commit it.
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(shardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);

	compensateOperation = new AtomicCommitOperation(this->getOperationId(), vector<NodeId>(), shardAssignChange);
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
	if(releaseOperation != NULL){
		return this;
	}
	vector<SingleResourceLockRequest *> releaseBatch;
	SingleResourceLockRequest * releaseRequest = new SingleResourceLockRequest(shardId,
			NodeOperationId(ShardManager::getCurrentNodeId(),this->getOperationId()));
	releaseBatch.push_back(releaseRequest);
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = releaseBatch;
	resourceLockRequest->isBlocking = true;
	releaseOperation = new AtomicLockOperation(this->getOperationId(), resourceLockRequest);
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
	if(nodeFailure == NULL){
		ASSERT(false);
		return this;
	}
	if(nodeFailure->getFailedNodeID() == destination.nodeId){
		if(compensateOperation == NULL && releaseOperation == NULL){
			return compensate();
		}
		// compensate cannot be non-NULL because dest node cannot die two times
		if(releaseOperation != NULL){
			// just continue with release ... NOTE : we lost the data
			Logger::error("Data loss because of node failure.");
		}
	}

	if(compensateOperation != NULL){
		OperationState::stateTransit(compensateOperation, nodeFailure);
		if(compensateOperation == NULL){
			return release();
		}
		return this;
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


string ShardMoveSrcOperation::getOperationName() const {
	return "shard_move_src_side";
}
string ShardMoveSrcOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Cooperating with operation " << this->destination.toString() << " to move shard " << shardId.toString() << ".%";
	if(compensateOperation != NULL){
		ss << compensateOperation->getOperationName() << "%";
		ss << compensateOperation->getOperationStatus() ;
	}
	if(releaseOperation != NULL){
		ss << releaseOperation->getOperationName() << "%";
		ss << releaseOperation->getOperationStatus() ;
	}

	return ss.str();
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

