#include "AtomicLockOperation.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Node.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

AtomicLockOperation::AtomicLockOperation(const unsigned & operationId,
		ResourceLockRequest * lockRequests,
		AtomicLockOperationResult * resultStatus):OperationState(operationId){
	ASSERT(lockRequests != NULL);
	this->lockRequests = lockRequests;
	this->resultStatus = resultStatus;

	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// Get all nodes that are in ARRIVED state
	// and their Node structure is not NULL (they are completely arrived)
	writeview->getArrivedNodes(this->participantNodes, true);
	// participants must be sorted based on the nodeId
	std::sort(this->participantNodes.begin(), this->participantNodes.end());

	this->nodeIndex = (unsigned)-1; // very big number
}

AtomicLockOperation::AtomicLockOperation(const unsigned & operationId, const vector<NodeId> & participants,
		ResourceLockRequest * lockRequests, AtomicLockOperationResult * resultStatus):OperationState(operationId){
	this->lockRequests = lockRequests;
	this->resultStatus = resultStatus;
	this->participantNodes = participants;
	std::sort(this->participantNodes.begin(), this->participantNodes.end());
	this->nodeIndex = -1;
}

OperationState * AtomicLockOperation::entry(){
	// send the batch to the first node in the participants list.
	if(participantNodes.size() == 0){
		if(resultStatus != NULL){
			resultStatus->grantedFlag = true;
		}
		return NULL;
	}

	// send the notification to the first node
	this->nodeIndex = 0;
	return askNextNode(participantNodes.at(nodeIndex));
}

OperationState * AtomicLockOperation::handle(NodeFailureNotification * nodeFailure){
	// erase any failed nodes from the participants list.
	if(nodeFailure == NULL){
		ASSERT(false);
		return this;
	}
	unsigned failedNodeIndex = 0;
	for(failedNodeIndex = 0 ; failedNodeIndex < participantNodes.size(); failedNodeIndex++){
		if(participantNodes.at(failedNodeIndex) == nodeFailure->getFailedNodeID()){
			break;
		}
	}

	if(failedNodeIndex >= participantNodes.size()){
		// this failed node is not in participants list
		return this;
	}

	if(failedNodeIndex < nodeIndex){ // we have passed this node, we can just ignore the failure and continue;
		participantNodes.erase(participantNodes.begin()+failedNodeIndex);
		nodeIndex--;
		ASSERT(nodeIndex < participantNodes.size());
	}else if(failedNodeIndex == nodeIndex){
		participantNodes.erase(participantNodes.begin()+failedNodeIndex);
		// nodeIndex is now pointing to a new node so we must send lock request
		if(nodeIndex < participantNodes.size()){
			// send lock request to this node
			return askNextNode(participantNodes.at(nodeIndex));
		}
	}else{
		ASSERT(failedNodeIndex > nodeIndex);
		participantNodes.erase(participantNodes.begin()+failedNodeIndex);
		ASSERT(nodeIndex < participantNodes.size());
	}

	if(this->nodeIndex >= participantNodes.size()){
		// success
		if(resultStatus != NULL){
			resultStatus->grantedFlag = true;
		}
		return NULL;
	}
	return this;
}

OperationState * AtomicLockOperation::handle(LockingNotification::ACK * ack){
	if(! doesExpect(ack)){
		ASSERT(false);
		return this;
	}
	// is it granted or rejected ?
	if(ack->isGranted()){ // request granted, go to the next node
		nodeIndex ++;
		if(nodeIndex >= participantNodes.size()){
			// success
			if(resultStatus != NULL){
				resultStatus->grantedFlag = true;
			}
			return NULL;
		}
		return askNextNode(participantNodes.at(nodeIndex));
	}else{ // reject case
		if(lockRequests->isBlocking){
			// if it's blocking, we should never receive REJECT
			ASSERT(false);
			if(resultStatus != NULL){
				resultStatus->grantedFlag = false;
			}
			return NULL;
		}else{
			// 2. non-blocking mode : we must send release/downgrade to all
			//    nodes that we have already locked.
			return handleRejectNonBlocking();
		}
	}

}
bool AtomicLockOperation::doesExpect(const LockingNotification::ACK * ack) const{
	if(ack == NULL){
		ASSERT(false);
		return false;
	}
	if(nodeIndex >= participantNodes.size()){
		return false; // we don't expect anything
	}
	if(participantNodes.at(nodeIndex) != ack->getSrc().nodeId){
		return false;
	}
	return true;
}

string AtomicLockOperation::getOperationName() const {
	return "lock_operation";
};
string AtomicLockOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Participants : ";
	for(unsigned i  = 0 ; i < participantNodes.size(); ++i){
		if(i != 0){
			ss << " - ";
		}
		ss << i << ":" << participantNodes.at(i) ;
	}
	ss << "%";
	ss << "Node index : " << nodeIndex << "%";

	if(nodeIndex == (unsigned) -1){
		ss << "Entry not called yet.%";
	}
	if(nodeIndex >= participantNodes.size()){
		ss << "Result of lock : ";
		if(resultStatus->grantedFlag){
			ss << "Granted. %";
		}else{
			ss << "Rejected. %";
		}
	}

	ss << "Lock request : ";
	if(lockRequests == NULL){
		ss << "NULL%";
	}else{
		ss << "%";
		ss << lockRequests->toString();
	}
	return ss.str();
};

OperationState * AtomicLockOperation::askNextNode(const NodeId & targetNodeId){
	NodeId currentNodeID = ShardManager::getCurrentNodeId();
	if(targetNodeId == currentNodeID){
		// ask our own repository
		pthread_t localLockThread;
		// start another thread and give this lock request to local lock manager
		LockRequestArguments * args = new LockRequestArguments();
		args->lockRequest =new ResourceLockRequest(*lockRequests);
		args->ackType = ShardingLockACKMessageType;
		args->priority = LOCK_REQUEST_PRIORITY_LOAD_BALANCING;
		args->requester = NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId());
	    if (pthread_create(&localLockThread, NULL, localLockRequest , args) != 0){
	        // Logger::console("Cannot create thread for handling local message");
	        perror("Cannot create thread for handling local message");
	        return NULL;
	    }
	}else{
		// send the notification
		LockingNotification * lockingRequestNotif = new LockingNotification(lockRequests);
		this->send(lockingRequestNotif, NodeOperationId(targetNodeId));
		delete lockingRequestNotif;
	}
	// we must wait for result
	return this;
}

OperationState * AtomicLockOperation::handleRejectNonBlocking(){
	// I) If it was the first node that we tried, we have no compensation to do,
	//    so we should just return REJECTED
	if(nodeIndex == 0){
		if(resultStatus != NULL){
			resultStatus->grantedFlag = false;
		}
		return NULL;
	}
	vector<SingleResourceLockRequest *> lockRequestsCompensation ;
	for(unsigned i = 0; i < lockRequests->requestBatch.size(); ++i){
		if(lockRequests->requestBatch.at(i)->requestType == ResourceLockRequestTypeLock){
			lockRequestsCompensation.push_back(new SingleResourceLockRequest(*(lockRequests->requestBatch.at(i))));
			lockRequestsCompensation.back()->requestType = ResourceLockRequestTypeRelease;
			lockRequestsCompensation.back()->holders.clear();
		}else if(lockRequests->requestBatch.at(i)->requestType == ResourceLockRequestTypeUpgrade){
			lockRequestsCompensation.push_back(new SingleResourceLockRequest(*(lockRequests->requestBatch.at(i))));
			lockRequestsCompensation.back()->requestType = ResourceLockRequestTypeDowngrade;
			lockRequestsCompensation.back()->holders.clear();
		}else{
			ASSERT(false);
			continue;
		}
	}
	vector<NodeId> participantsInCompensation;
	for(unsigned i = 0 ; i < nodeIndex ; ++i){
		participantsInCompensation.push_back(participantNodes.at(i));
	}
	// move to another lock operation to do the release/downgrade job and pass the nextState
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = lockRequestsCompensation;
	resourceLockRequest->isBlocking = true; // release is blocking although it doesn't matter because it's always granted
	AtomicLockOperation * compensationOp = new AtomicLockOperation(this->getOperationId(), participantsInCompensation, resourceLockRequest);
	// set result of locking
	if(resultStatus != NULL){
		resultStatus->grantedFlag = false;
	}
	return compensationOp;
}

void * AtomicLockOperation::localLockRequest(void *arg){
	LockRequestArguments * args = (LockRequestArguments * )arg;
	boost::unique_lock<boost::mutex> lock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardManager::getShardManager()->getLockManager()->resolveBatch(args->requester,
			args->priority , args->lockRequest, args->ackType);
	delete args;
	return NULL;
}

}
}
