#include "core/util/Logger.h"
#include "LockRepository.h"


using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

LockRequest::LockRequest(NodeOperationId lockHolder, LockLevel lockType,
		ShardingNotification * requestAck):lockType(lockType),id(0){
	this->lockHolders.push_back(lockHolder);
	this->requestAck = requestAck;
	this->granted = false;
	this->arrived = true;
}
LockRequest::LockRequest(NodeOperationId lockHolder1, NodeOperationId lockHolder2,
		LockLevel lockType, ShardingNotification * requestAck):lockType(lockType),id(0){
	this->lockHolders.push_back(lockHolder1);
	this->lockHolders.push_back(lockHolder2);
	this->requestAck = requestAck;
	this->granted = false;
	this->arrived = true;
}

// used for reservation
LockRequest::LockRequest(const unsigned id):id(id){
	requestAck = NULL;
	this->granted = false;
	this->arrived = true;
}

void LockRequest::fill(LockRequest * lockRequest){
	this->requestAck = lockRequest->requestAck;
	lockRequest->requestAck = NULL; // to protect deallocation
	this->lockType = lockRequest->lockType;
	this->lockHolders = lockRequest->lockHolders;
	this->arrived = true;
}

LockRequest::~LockRequest(){
}

/*
 * Returns true if lockHolders list becomes empty,
 * which means lock request must be removed (it's completely released)
 */
bool LockRequest::eraseLockHolders(const NodeOperationId & op){
	for(unsigned i = 0 ; i < lockHolders.size(); ++i){
		if(lockHolders.at(i) == op){
			lockHolders.erase(lockHolders.begin() + i);
			i--;
		}
	}
	if(lockHolders.size() == 0){
		return true;
	}
	return false;
}
/*
 * Returns true if lockHolders list becomes empty,
 * which means lock request must be removed (it's completely released)
 */
bool LockRequest::eraseLockHolders(const NodeId & node){
	for(unsigned i = 0 ; i < lockHolders.size(); ++i){
		if(lockHolders.at(i).nodeId == node){
			lockHolders.erase(lockHolders.begin() + i);
			i--;
		}
	}
	if(lockHolders.size() == 0){
		return true;
	}
	return false;
}



bool WaitingListHandler::isEmpty(){
	if(! priorityWaitingList.empty()){
		return true;
	}
	if(! SwaitingList.empty()){
		return true;
	}
	if(! XwaitingList.empty()){
		return true;
	}
	return false;
}

void WaitingListHandler::push(LockRequest * lockRequest, const vector<unsigned> priorIds = vector<unsigned>()){

	if(priorIds.size() != 0){
		// Priority and id are assumed to have the same order.
		// 1. First make reservations for older nodes
		for(unsigned i = 0 ; i < priorIds.size(); ++i){
			unsigned priorId = priorIds.at(i);
			insertPriorityLockRequest(new LockRequest(priorId));
		}

		// 2. look for lockRequest
		insertPriorityLockRequest(lockRequest)->arrived = true;
		return;
	}
	switch(lockRequest->lockType){
	case LockLevel_S:
		SwaitingList.push(lockRequest);
		return;
	case LockLevel_X:
		XwaitingList.push(lockRequest);
		return;
	}
}

// returns NULL if no lockRequest is waiting in any queue
// S lock requests are prior to X lock requests
// priority list is prior to both S and X requesters
LockRequest * WaitingListHandler::pop(){
	if(SwaitingList.empty() && XwaitingList.empty() && priorityWaitingList.empty()){
		return NULL;
	}

	LockRequest * nextLockRequest = NULL;

	if(priorityWaitingList.size() > 0){
		nextLockRequest = priorityWaitingList.at(0);
		priorityWaitingList.erase(priorityWaitingList.begin());
		return nextLockRequest;
	}

	std::queue<LockRequest *> * wl;
	if(! SwaitingList.empty()){
		wl = &SwaitingList;
	}else if(! XwaitingList.empty()){
		wl = &XwaitingList;
	}

	nextLockRequest = wl->front();
	wl->pop;


	return nextLockRequest;
}

LockRequest * WaitingListHandler::top(){
	if(SwaitingList.empty() && XwaitingList.empty() && priorityWaitingList.empty()){
		return NULL;
	}

	LockRequest * nextLockRequest = NULL;

	if(priorityWaitingList.size() > 0){
		nextLockRequest = priorityWaitingList.at(0);
		return nextLockRequest;
	}

	std::queue<LockRequest *> * wl;
	if(! SwaitingList.empty()){
		wl = &SwaitingList;
	}else if(! XwaitingList.empty()){
		wl = &XwaitingList;
	}

	nextLockRequest = wl->front();

	return nextLockRequest;
}

LockRequest * WaitingListHandler::insertPriorityLockRequest(LockRequest * priorLockRequest){
	if(priorityWaitingList.empty()){
		priorityWaitingList.push_back(priorLockRequest);
		return priorLockRequest;
	}
	for(unsigned insertIndex = 0 ; insertIndex < priorityWaitingList.size(); ++insertIndex){
		if(priorityWaitingList.at(insertIndex)->id == priorLockRequest->id){
			if(! priorityWaitingList.at(insertIndex)->arrived){
				priorityWaitingList.at(insertIndex)->fill(priorLockRequest);
			}
			delete priorLockRequest;
			return priorityWaitingList.at(insertIndex);
		}else if(priorityWaitingList.at(insertIndex)->id > priorLockRequest->id){
			// the first place that id is larger than priorId
			priorityWaitingList.insert(priorityWaitingList.begin() + insertIndex , priorLockRequest);
			return priorityWaitingList.at(insertIndex);
		}
	}
	priorityWaitingList.push_back(priorLockRequest);
	return priorLockRequest;
}



SingleResourceLocks::SingleResourceLocks(){
	this->waitingListHandler = new WaitingListHandler();
}

/*
 * returns false if it's not in locked state
 * returns true if it's locked, and in this case
 * lockType is set to either 'S' or 'X'
 */
bool SingleResourceLocks::isLocked(){
	if(currentLockHolder_Queue.empty()){
		return false;
	}
	return true;
}

/*
 * Returns NULL if no pendingRequest is waiting in the WaitingList
 */
LockRequest * SingleResourceLocks::grantPendingRequest(){
	if(! waitingListHandler->isEmpty()){
		return NULL;
	}
	LockRequest * requestToGrant = waitingListHandler->top(); // returns the top of waitingList
	if(requestToGrant == NULL){
		ASSERT(false);
		return NULL;
	}
	if(! canGrant(requestToGrant)){
		return NULL;
	}
	waitingListHandler->pop();
	if(requestToGrant == NULL){
		ASSERT(false);
		return NULL;
	}
	grantLock(requestToGrant);
	return requestToGrant;
}

bool SingleResourceLocks::hasPendingRequest(){
	return (! waitingListHandler->isEmpty());
}

/*
 * returns true if lockRequest can be granted immediately and false otherwise
 */
bool SingleResourceLocks::lock(LockRequest * lockRequest, bool blocking = false,
		const vector<unsigned> priorIds = vector<unsigned>()){
	if(lockRequest == NULL){
		ASSERT(false);
		return false;
	}
	if(isLocked()){
		// note: it's either multiple S locks or a single X lock
		LockLevel currentLockType = currentLockHolder_Queue.at(0)->lockType;
		if(currentLockType == LockLevel_S){
			if(lockRequest->lockType == LockLevel_S){
				// S is OK with S
				grantLock(lockRequest);
				return true;
			}else{ // request  lockType == X
				ASSERT(lockRequest->lockType == LockLevel_X);
				// we have S, X is rejected.
				resolveLockConflict(lockRequest, blocking, priorIds);
				return false;
			}
		}else{ // current lockType == X
			ASSERT(currentLockType == LockLevel_X);
			// all request lock types are rejected
			resolveLockConflict(lockRequest, blocking, priorIds);
			return false;
		}
	}else{
		// not locked, we can just grant this request
		grantLock(lockRequest);
		return true;
	}
}


bool SingleResourceLocks::canGrant(LockRequest * lockRequest){
	if(! isLocked()){
		return true;
	}
	if(lockRequest->lockType == LockLevel_X){
		return false;
	}
	if(lockRequest->lockType == LockLevel_S){
		return (this->currentLockHolder_Queue.at(0)->lockType == LockLevel_S);
	}
	return false;
}

/*
 * When an operation is released, even if the resource is unlocked
 * and there are other requests waiting to be granted, we don't
 * grant any new requests because and we wait for the container of
 * this lock manager to ask us to do so.
 */
void SingleResourceLocks::release(const NodeOperationId & releasedOp){
	if(this->currentLockHolder_Queue.empty()){
		return;
	}

	unsigned i = 0;
	while(i++ < this->currentLockHolder_Queue.size()){
		LockRequest * grantedRequest = this->currentLockHolder_Queue.at(i);
		if(grantedRequest->eraseLockHolders(releasedOp)){
			// if grantedRequest is all released after removing this op.
			// remove it from granted requests
			this->currentLockHolder_Queue.erase(this->currentLockHolder_Queue.begin() + i);
			i--;
			delete grantedRequest;
		}
	}
}

void SingleResourceLocks::resolveLockConflict(LockRequest * lockRequest, bool blocking, const vector<unsigned> priorIds){
	if(blocking){
		waitingListHandler->push(lockRequest, priorIds);
		return; // always true in blocking mode
	}
}

void SingleResourceLocks::grantLock(LockRequest * lockRequest){
	currentLockHolder_Queue.push_back(lockRequest);
	lockRequest->granted = true;
}

}
}
