#include "SerialLockOperation.h"

#include "core/util/SerializationHelper.h"
#include "src/core/util/Assert.h"
#include "metadata_manager/Shard.h"
#include "metadata_manager/Node.h"
#include "./metadata_manager/Cluster_Writeview.h"
#include "./ShardManager.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

SerialLockOperation::SerialLockOperation(const unsigned & operationId,
		ResourceLockRequest * lockRequests,
		SerialLockResultStatus * resultStatus):OperationState(operationId){
	this->lockRequests = lockRequests;
	this->resultStatus = resultStatus;

	Cluster_Writeview * writeview = ShardManager::getWriteview();
	writeview->getArrivedNodes(this->participantNodes, true);
	// participants must be sorted based on the nodeId
	std::sort(this->participantNodes.begin(), this->participantNodes.end());

	this->nodeIndex = -1; // very big number
}

SerialLockOperation::SerialLockOperation(const unsigned & operationId, const vector<NodeId> & participants,
		ResourceLockRequest * lockRequests, SerialLockResultStatus * resultStatus):OperationState(operationId){
	this->lockRequests = lockRequests;
	this->resultStatus = resultStatus;
	this->participantNodes = participants;
	std::sort(this->participantNodes.begin(), this->participantNodes.end());
	this->nodeIndex = -1;
}

OperationState * SerialLockOperation::entry(){
	// send the batch to the first node in the participants list.
	if(participantNodes.size() == 0){
		if(resultStatus != NULL){
			resultStatus->grantedFlag = true;
		}
		return NULL;
	}

	// send the notification to the first node
	NodeId targetNodeId = participantNodes.at(0);
	this->nodeIndex = 0;
	return askNextNode(participantNodes.at(nodeIndex));
}

OperationState * SerialLockOperation::handle(NodeFailureNotification * nodeFailure){
	// erase any failed nodes from the participants list.
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	map<NodeId, ShardingNodeState> nodeStates;
	map<NodeId, std::pair<ShardingNodeState, Node *> > & allNodes = writeview->nodes;
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr =
			allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		nodeStates[nodeItr->first] = nodeItr->second.first;
	}
	vector<NodeId> participantsFixed;
	unsigned nodeIndexFixed = this->nodeIndex;
	bool mustAskNextNode = false;
	for(unsigned i = 0 ; i < participantNodes.size(); ++i){
		if(nodeStates[participantNodes.at(i)] == ShardingNodeStateFailed){
			if(i < nodeIndex){ // we are passed this node, so we don't care
				nodeIndexFixed--;
			}else if(i == nodeIndex){ // we are waiting for ACK from this node, move on to the next node.
//				nodeIndexFixed++;
				mustAskNextNode = true;
			}//else{
			//// We haven't reached to this node so we are fine.
			//}
		}else{
			participantsFixed.push_back(participantNodes.at(i));
		}
	}

	participantNodes = participantsFixed;
	this->nodeIndex = nodeIndexFixed;

	if(this->nodeIndex >= participantNodes.size()){
		// success
		if(resultStatus != NULL){
			resultStatus->grantedFlag = true;
		}
		return NULL;
	}

	if(mustAskNextNode){
		return askNextNode(participantNodes.at(nodeIndex));
	}
	return this;
}

OperationState * SerialLockOperation::handle(LockingNotification::ACK * ack){
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
		// 1. blocking mode : we must try until we get it
		if(lockRequests->isBlocking){
			// if it's blocking, we should never receive REJECT
			ASSERT(false);
			if(resultStatus != NULL){
				resultStatus->grantedFlag = false;
			}
			return NULL;
		}else{
			// 2. non-blocking mode : we must send release/downgrade to all previous nodes
			return handleRejectNonBlocking();
		}
	}

}
bool SerialLockOperation::doesExpect(const LockingNotification::ACK * ack) const{
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

string SerialLockOperation::getOperationName() const {
	return "lock_operation";
};
string SerialLockOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Participants : " ;
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

OperationState * SerialLockOperation::askNextNode(NodeId targetNodeId){
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
	        //        Logger::console("Cannot create thread for handling local message");
	        perror("Cannot create thread for handling local message");
	        return NULL;
	    }
	}else{
		// send the notification
		LockingNotification * lockingRequest = new LockingNotification(lockRequests);
		this->send(lockingRequest, NodeOperationId(targetNodeId));
		delete lockingRequest;
	}
	// we must wait for result
	return this;
}

OperationState * SerialLockOperation::handleRejectNonBlocking(){
	// I) If it was the first node that we tried, we have no compensation to do,
	//    so we should just move to the next state
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
	// move to a parallel lock operation to do the release/downgrade job and pass the nextState
	ResourceLockRequest * resourceLockRequest = new ResourceLockRequest();
	resourceLockRequest->requestBatch = lockRequestsCompensation;
	resourceLockRequest->isBlocking = lockRequests->isBlocking;
	SerialLockOperation * compensationOp = new SerialLockOperation(this->getOperationId(), participantsInCompensation, resourceLockRequest);
	// set result of locking
	if(resultStatus != NULL){
		resultStatus->grantedFlag = false;
	}
	return compensationOp;
}

void * SerialLockOperation::localLockRequest(void *arg){
	LockRequestArguments * args = (LockRequestArguments * )arg;
	boost::unique_lock<boost::mutex> lock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardManager::getShardManager()->getLockManager()->resolveBatch(args->requester,args->priority , args->lockRequest, args->ackType);
	return NULL;
}

}
}
