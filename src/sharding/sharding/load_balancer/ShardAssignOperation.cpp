#include "ShardAssignOperation.h"

#include "core/util/SerializationHelper.h"
#include "src/core/util/Assert.h"
#include "sharding/configuration/ShardingConstants.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "LoadBalancingStartOperation.h"
#include "../metadata_manager/DataShardInitializer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardAssignOperation::ShardAssignOperation(const unsigned operationId, const ClusterShardId & unassignedShard):OperationState(operationId),shardId(shardId){
	this->lockOperation = NULL;
	this->lockOperationResult = new SerialLockResultStatus();
	this->commitOperation = NULL;
	this->releaseOperation = NULL;
}

ShardAssignOperation::~ShardAssignOperation(){
	if(lockOperationResult != NULL){
		delete lockOperationResult;
	}
}

OperationState * ShardAssignOperation::entry(){
	// 1. lock the shardId
	// 2. apply the change on local metadata and commit.
	// 3. release the lock
	return acquireLocks();
}

OperationState * ShardAssignOperation::acquireLocks(){

	vector<SingleResourceLockRequest *> lockBatch;
	SingleResourceLockRequest * lockRequest = new SingleResourceLockRequest(shardId,
			NodeOperationId(ShardManager::getCurrentNodeId(),this->getOperationId()),
			ResourceLockType_X);
	lockBatch.push_back(lockRequest);
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = lockBatch;
	resourceLockRequest->isBlocking = false;
	lockOperation = OperationState::startOperation(new SerialLockOperation(this->getOperationId(), resourceLockRequest, this->lockOperationResult));
	if(lockOperation == NULL){
		if(this->lockOperationResult->grantedFlag == false){
			return LoadBalancingStartOperation::finalizeLoadBalancing();
		}else{
			// move to next step
			return commit();
		}
	}
	return this;
}
OperationState * ShardAssignOperation::handle(LockingNotification::ACK * ack){
	if((lockOperation == NULL && releaseOperation == NULL) || commitOperation != NULL){
		return this; // ignore, we are passed this point.
	}

	if(lockOperation != NULL){
		OperationState::stateTransit(lockOperation, ack);
		if(lockOperation == NULL){
			if(this->lockOperationResult->grantedFlag == false){
				return LoadBalancingStartOperation::finalizeLoadBalancing();
			}else{
				// move to next step
				return commit();
			}
		}
		return this;
	}else if(releaseOperation != NULL){
		OperationState::stateTransit(releaseOperation, ack);
		if(releaseOperation == NULL){
			return LoadBalancingStartOperation::finalizeLoadBalancing();
		}
		return this;
	}
	return this;
}


OperationState * ShardAssignOperation::commit(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
			writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[shardId.coreId]->getName(), &shardId);
	if(indexDirectory.compare("") == 0){
		indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
				writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[shardId.coreId]->getName(), &shardId);
	}
	EmptyShardBuilder emptyShard(new ClusterShardId(shardId), indexDirectory);
	emptyShard.prepare();
	LocalPhysicalShard physicalShard(emptyShard.getShardServer(), emptyShard.getIndexDirectory(), "");
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(shardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);

	commitOperation = new CommitOperation(this->getOperationId(), vector<NodeId>(), shardAssignChange);
	commitOperation = OperationState::startOperation(commitOperation);
	if(commitOperation == NULL){
		return release();
	}
	return this;
}
OperationState * ShardAssignOperation::handle(CommitNotification::ACK * ack){
	if(commitOperation == NULL || lockOperation != NULL || releaseOperation != NULL){
		return this; // ignore.
	}
	OperationState::stateTransit(commitOperation, ack);
	if(commitOperation == NULL){
		return release();
	}
	return this;
}

OperationState * ShardAssignOperation::handle(NodeFailureNotification * nodeFailure){
	// no node failure can abort this transaction, it's separate than others
	// if we are not alone, pass notification to operators.
	if(nodeFailure == NULL){
		ASSERT(false);
		return this;
	}
	if(lockOperation != NULL){
		OperationState::stateTransit(lockOperation, nodeFailure);
		if(lockOperation == NULL){
			return commit();
		}
		return this;
	}
	if(commitOperation != NULL){
		OperationState::stateTransit(commitOperation, nodeFailure);
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

OperationState * ShardAssignOperation::handle(Notification * notification){
	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingCommitACKMessageType:
			return handle((CommitNotification::ACK *)notification);
		case ShardingLockACKMessageType:
			return handle((LockingNotification::ACK * )notification);
		default:
			// ignore;
			return this;
	}
}

string ShardAssignOperation::getOperationName() const {
	return "shard_assignment";
}
string ShardAssignOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Shard : " << shardId.toString() << "%";

	if (lockOperation != NULL) {
		ss << lockOperation->getOperationName() << "%";
		ss << lockOperation->getOperationStatus();
	} else {
		ss << "Lock result : " << lockOperationResult->grantedFlag << "%";
	}

	if(commitOperation != NULL){
		ss << commitOperation->getOperationName() << "%";
		ss << commitOperation->getOperationStatus();
	}

	if(releaseOperation != NULL){
		ss << releaseOperation->getOperationName() << "%";
		ss << releaseOperation->getOperationStatus();
	}
	return ss.str();
}

OperationState * ShardAssignOperation::release(){
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
		return LoadBalancingStartOperation::finalizeLoadBalancing();
	}
	return this;
}

}
}


