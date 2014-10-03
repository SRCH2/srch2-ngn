

#include "ClusterSaveOperation.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../atomic_operations/AtomicLockOperation.h"
#include "../atomic_operations/AtomicMergeOperation.h"
#include "../atomic_operations/AtomicSaveOperation.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ClusterSaveOperation::ClusterSaveOperation(evhttp_request *req):OperationState(OperationState::getNextOperationId()){
	this->req = req;
	this->lockOperation = NULL;
	this->mergeOperation = NULL;
	this->saveOperation = NULL;
	this->releaseOperation = NULL;
	this->releaseFlag = true;
	this->printFlag = true;
}

ClusterSaveOperation::ClusterSaveOperation(unsigned operationId):OperationState(operationId){
	this->req = NULL;
	this->lockOperation = NULL;
	this->mergeOperation = NULL;
	this->saveOperation = NULL;
	this->releaseOperation = NULL;
	this->releaseFlag = false;
	this->printFlag = false;
}

// initialize class members
ClusterSaveOperation::~ClusterSaveOperation(){}

/*
 * lock();
 */
OperationState * ClusterSaveOperation::entry(){
	return lock();
}

/*
 * 1. prepare the lock request and create the lock operation to acquire S locks on all shards
 * NOTE : lock request must be blocking
 * NOTE : all other operation objects must be NULL
 */
OperationState * ClusterSaveOperation::lock(){

	if(! isInStartingPhase()){
		ASSERT(false);
		return NULL;
	}

	Cluster_Writeview * writeview = ShardManager::getWriteview();

	// prepare the lock request
	ResourceLockRequest * lockRequests = new ResourceLockRequest();

	ClusterShardId id;NodeShardId nodeShardId;ShardState state;bool isLocal;
	NodeId nodeId;LocalPhysicalShard physicalShard;	double load;

	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		lockRequests->requestBatch.push_back(new SingleResourceLockRequest(id,
				NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()), ResourceLockType_S));
	}
	lockRequests->isBlocking = true;

	// create a lock operation and pass the request to it.
	lockOperation = OperationState::startOperation(new AtomicLockOperation(this->getOperationId(), lockRequests));
	if(lockOperation == NULL){
		return merge();
	}
	return this;
}

/*
 * When the last lock ack is received,
 * if we are in locking phase start the merge process :
 * merge();
 * else if we are in releasing process finalize :
 * finalize();
 */
OperationState * ClusterSaveOperation::handle(LockingNotification::ACK * ack){
	if(! ( isInLockingPhase() || isInReleasingPhase() )){
		ASSERT(false);
		return this;
	}
	if(isInLockingPhase()){
		OperationState::stateTransit(lockOperation, ack);
		if(lockOperation == NULL){
			return merge();
		}
		return this;
	}

	if(isInReleasingPhase()){
		OperationState::stateTransit(releaseOperation, ack);
		if(releaseOperation == NULL){
			return finalize();
		}
		return this;
	}

	return this;
}

/*
 * Create the AtomicMergeOperation and do a cluster wide merge
 */
OperationState * ClusterSaveOperation::merge(){
	// start an AtomicMergeOperation and wait for cluster to merge
	if(isInLockingPhase() || isInMergingPhase() || isInSavingPhase() || isInReleasingPhase()){
		ASSERT(false);
		return this;
	}
	mergeOperation = OperationState::startOperation(new AtomicMergeOperation(this->getOperationId()));
	if(mergeOperation == NULL){
		return save();
	}
	return this;
}


/*
 * When the last merge ack is received, start the save process :
 * save();
 */
OperationState * ClusterSaveOperation::handle(MergeNotification::ACK * ack){
	if(! isInMergingPhase() ){
		ASSERT(false);
		return this;
	}
	OperationState::stateTransit(mergeOperation, ack);
	if(mergeOperation == NULL){
		return save();
	}
	return this;
}

/*
 * Create the AtomicSaveOperation and do a cluster wide save
 */
OperationState * ClusterSaveOperation::save(){
	// start an AtomicSaveOperation and wait for cluster to be saved.
	if(isInLockingPhase() || isInMergingPhase() || isInSavingPhase() || isInReleasingPhase()){
		ASSERT(false);
		return this;
	}
	saveOperation = OperationState::startOperation(new AtomicSaveOperation(this->getOperationId()));
	if(saveOperation == NULL){
		return release();
	}
	return this;
}

// operation should not return NULL upon receiving these acks
OperationState * ClusterSaveOperation::handle(SaveDataNotification::ACK * ack){
	if(! isInSavingPhase() ){
		ASSERT(false);
		return this;
	}
	OperationState::stateTransit(saveOperation, ack);
	if(saveOperation == NULL){
		// these acks should not finish the operation
		// release is placed here to avoid having stuck locks in case of a bug.
		ASSERT(false);
		return release();
	}
	return this;
}

/*
 * When the last save ack is received start the lock release process:
 * release()
 */
OperationState * ClusterSaveOperation::handle(SaveMetadataNotification::ACK * ack){
	if(! isInSavingPhase() ){
		ASSERT(false);
		return this;
	}
	OperationState::stateTransit(saveOperation, ack);
	if(saveOperation == NULL){
		return release();
	}
	return this;
}


// releases the locks on all resources
OperationState * ClusterSaveOperation::release(){

	if(isInLockingPhase() || isInMergingPhase() || isInSavingPhase() || isInReleasingPhase()){
		ASSERT(false);
		return this;
	}

	if(! releaseFlag ){
		return finalize();
	}

	Cluster_Writeview * writeview = ShardManager::getWriteview();

	// prepare the release lock request
	ResourceLockRequest * lockRequests = new ResourceLockRequest();

	ClusterShardId id;NodeShardId nodeShardId;ShardState state;bool isLocal;
	NodeId nodeId;LocalPhysicalShard physicalShard;	double load;

	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		lockRequests->requestBatch.push_back(new SingleResourceLockRequest(id,
				NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId())));
	}
	lockRequests->isBlocking = true;
	releaseOperation = OperationState::startOperation(new AtomicLockOperation(this->getOperationId(), lockRequests));
	if(releaseOperation == NULL){
		return finalize();
	}
	return this;
}

/*
 * Write the response to the HTTP request channel that was received in construction
 * and return NULL to finish this transaction.
 */
OperationState * ClusterSaveOperation::finalize(){
	if(printFlag){
		bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
				"{\"message\":\"The cluster state was saved.\"}\n");
	}
    return NULL;
}


/*
 * Based on the phase of this transaction
 * (locking/merging/saving/releasing) give this notification to one of the operation members...
 */
OperationState * ClusterSaveOperation::handle(NodeFailureNotification * nodeFailureNotif){
	if(isInStartingPhase()){
		return this; // at this point node failure doesn't affect us
	}

	if(isInLockingPhase()){
		OperationState::stateTransit(lockOperation, nodeFailureNotif);
		if(lockOperation == NULL){
			return merge();
		}
		return this;
	}

	if(isInMergingPhase()){
		OperationState::stateTransit(mergeOperation, nodeFailureNotif);
		if(mergeOperation == NULL){
			return save();
		}
		return this;
	}


	if(isInSavingPhase()){
		OperationState::stateTransit(saveOperation, nodeFailureNotif);
		if(saveOperation == NULL){
			return release();
		}
		return this;
	}

	if(isInReleasingPhase()){
		OperationState::stateTransit(releaseOperation, nodeFailureNotif);
		if(releaseOperation == NULL){
			return finalize();
		}
		return this;
	}

	return this;
}

OperationState * ClusterSaveOperation::handle(Notification * notification){

	if(notification == NULL){
		ASSERT(false);
		return NULL;
	}

	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingLockACKMessageType:
			return handle((LockingNotification::ACK *)notification);
		case ShardingMergeACKMessageType:
			return handle((MergeNotification::ACK *)notification);
		case ShardingSaveDataACKMessageType:
			return handle((SaveDataNotification::ACK *)notification);
		case ShardingSaveMetadataACKMessageType:
			return handle((SaveMetadataNotification::ACK *) notification);
		default:
			ASSERT(false);
			return this;
	}

}

string ClusterSaveOperation::getOperationName() const {
	return "cluster_save_operation";
}


string ClusterSaveOperation::getOperationStatus() const {
	stringstream ss;
	if(isInStartingPhase()){
		ss << "No sub operation is running.%";
	}

	if(isInLockingPhase()){
		ss << lockOperation->getOperationName() << "%";
		ss << lockOperation->getOperationStatus();
		return ss.str();
	}

	if(isInMergingPhase()){
		ss << mergeOperation->getOperationName() << "%";
		ss << mergeOperation->getOperationStatus();
		return ss.str();
	}

	if(isInSavingPhase()){
		ss << saveOperation->getOperationName() << "%";
		ss << saveOperation->getOperationStatus();
		return ss.str();
	}

	if(isInReleasingPhase()){
		ss << releaseOperation->getOperationName() << "%";
		ss << releaseOperation->getOperationStatus();
		return ss.str();
	}

	return ss.str();
}

bool ClusterSaveOperation::isInStartingPhase() const{
	return (lockOperation == NULL && mergeOperation == NULL && saveOperation == NULL && releaseOperation == NULL);
}
bool ClusterSaveOperation::isInLockingPhase() const{
	return (lockOperation != NULL && mergeOperation == NULL && saveOperation == NULL && releaseOperation == NULL);
}
bool ClusterSaveOperation::isInMergingPhase() const{
	return (lockOperation == NULL && mergeOperation != NULL && saveOperation == NULL && releaseOperation == NULL);
}
bool ClusterSaveOperation::isInSavingPhase() const{
	return (lockOperation == NULL && mergeOperation == NULL && saveOperation != NULL && releaseOperation == NULL);
}
bool ClusterSaveOperation::isInReleasingPhase() const{
	return (lockOperation == NULL && mergeOperation == NULL && saveOperation == NULL && releaseOperation != NULL);
}


}
}
