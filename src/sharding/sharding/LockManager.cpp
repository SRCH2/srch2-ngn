#include "LockManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

GlobalLockOperation::GlobalLockOperation(unsigned operationId, const LockChange * request):OperationState(operationId){
	ASSERT(request != NULL);
	this->request = new LockChange(*request);
	this->waitForReadviewReleaseFlag = false;
	this->versionToBeReleased = 0;
	this->success = true;
	this->needToFlush = false;
}
GlobalLockOperation::~GlobalLockOperation(){
	delete request;
}
OperationStateType GlobalLockOperation::getType(){
	return OperationStateType_Lock;
}
void GlobalLockOperation::entry(map<NodeId, unsigned> specialTargets){
	ASSERT(haveReplied.size() == 0);
	// 1. fist apply the change on local metadata and commit metadata if needed
	bool lockSuccess = LockManager::getLockManager()->localLock(this->request, this->waitForReadviewReleaseFlag , versionToBeReleased);
	if(! lockSuccess){
		// local lock failed, we don't continue
		// when isDone is called after this functions it returns the failed status to the called
		this->waitForReadviewReleaseFlag = false;
		this->success = false;
		return;
	}
	if(this->request->isAcquireOrRelease() == false){ // release doesn't need to wait for RV to be released
		this->waitForReadviewReleaseFlag = false;
	}
	// 2. send lock notification to all other nodes
	// 2.a) first register this change broadcast in ShardManager buffer
	// is there anybody for broadcast ?
	ClusterResourceMetadata_Writeview & writeview = MetadataManager::getMetadataManager()->getWriteview();
	bool otherNodesExist = false;
	for(map<NodeId, Node>::iterator nodeItr = writeview.nodes.begin(); nodeItr != writeview.nodes.end(); ++nodeItr){
		if(nodeItr->first == writeview.currentNodeId){
			continue;
		}
		otherNodesExist = true;
	}
	if(! otherNodesExist){
		this->success = true;
		return;
	}
	ShardManager::getShardManager()->registerChangeBroadcast(this->request);
	this->needToFlush = true;
	for(map<NodeId, Node>::iterator nodeItr = writeview.nodes.begin(); nodeItr != writeview.nodes.end(); ++nodeItr){
		if(nodeItr->first == writeview.currentNodeId){
			continue;
		}
		haveReplied[nodeItr->first] = false;
		unsigned destOperationId = 0;
		if(specialTargets.find(nodeItr->first) != specialTargets.end()){
			destOperationId = specialTargets.find(nodeItr->first)->second;
		}
		LockingNotification * lockingNotification = new LockingNotification(writeview.currentNodeId,
				this->getOperationId(), nodeItr->first, destOperationId, this->request );
		ShardManager::getShardManager()->send(lockingNotification);
		delete lockingNotification;
	}
}
// returns false when it's done.
void GlobalLockOperation::handle(LockingNotification::GRANTED * inputNotification){
	ASSERT(doesExpect(inputNotification));
	NodeOperationId srcOpId = inputNotification->getSrcOperationId();
	if(haveReplied.find(srcOpId.nodeId) == haveReplied.end()){
		return;
	}
	if(haveReplied[srcOpId.nodeId] == true){
		// why did we receive two acks from this node ?
		ASSERT(false);
		return;
	}
	haveReplied[srcOpId.nodeId] = true;
	return;
}
void GlobalLockOperation::handle(LockingNotification::REJECTED * inputNotification){
	ASSERT(doesExpect(inputNotification));
	NodeOperationId srcOpId = inputNotification->getSrcOperationId();
	if(haveReplied.find(srcOpId.nodeId) == haveReplied.end()){
		return;
	}
	if(haveReplied[srcOpId.nodeId] == true){
		// why did we receive two replies from this node ?
		ASSERT(false);
		return;
	}
	// if it's S-lock request, we resend the the notification to this node,
	// if it's X-lock request, we back off and send release to all nodes and wait for RELEASED replies

	ClusterResourceMetadata_Writeview & writeview = MetadataManager::getMetadataManager()->getWriteview();
	// S-lock : rejected so we resend lock request
	if(this->request->isSharedOrExclusive()){
		// resend lock request to this node
		LockingNotification * resendNotification = new LockingNotification(writeview.currentNodeId,
				this->getOperationId(), inputNotification->getSrcOperationId().nodeId, 0, this->request );
		ShardManager::getShardManager()->send(resendNotification);
		delete resendNotification;
		return;
	}

	// X-lock : rejected so we back off and send release request to all nodes
	this->success = false;
	this->waitForReadviewReleaseFlag = false; // we don't need to wait for release of readview anymore
	// request changes to release mode
	// NOTE: old request object will be deleted when we flush the broadcast history
	LockChange * newRequest = new LockChange(*request);
	delete request;
	request = newRequest;
	this->request->setAcquireOrRelease(false);
	// now send this request to all nodes
	ShardManager::getShardManager()->registerChangeBroadcast(this->request);
	ASSERT(this->needToFlush);
	for(map<NodeId, bool>::iterator nodeItr = haveReplied.begin(); nodeItr != haveReplied.end(); ++nodeItr){
		nodeItr->second = false; // because we want to wait for RELEASED replies now
		LockingNotification * lockingNotification = new LockingNotification(writeview.currentNodeId,
				this->getOperationId(), nodeItr->first, 0, this->request );
		ShardManager::getShardManager()->send(lockingNotification);
		delete lockingNotification;
	}

}
void GlobalLockOperation::handle(LockingNotification::RELEASED * inputNotification){
	ASSERT(doesExpect(inputNotification));
	NodeOperationId srcOpId = inputNotification->getSrcOperationId();
	if(haveReplied.find(srcOpId.nodeId) == haveReplied.end()){
		return;
	}
	if(haveReplied[srcOpId.nodeId] == true){
		// why did we receive two acks from this node ?
		ASSERT(false);
		return;
	}
	haveReplied[srcOpId.nodeId] = true;
}

void GlobalLockOperation::handle(LockingNotification::RV_RELEASED * inputNotification){
	ASSERT(doesExpect(inputNotification));
	waitForReadviewReleaseFlag = false;
}

bool GlobalLockOperation::doesExpect(LockingNotification::GRANTED * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	return (this->getOperationId() == inputNotification->getDestOperationId())  && (this->request->isAcquireOrRelease() == true);
}
bool GlobalLockOperation::doesExpect(LockingNotification::REJECTED * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	return (this->getOperationId() == inputNotification->getDestOperationId()) && (this->request->isAcquireOrRelease() == true);
}
bool GlobalLockOperation::doesExpect(LockingNotification::RELEASED * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	return (this->getOperationId() == inputNotification->getDestOperationId()) && (this->request->isAcquireOrRelease() == false);
}

bool GlobalLockOperation::doesExpect(LockingNotification::RV_RELEASED * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	return waitForReadviewReleaseFlag &&
			(this->versionToBeReleased <= inputNotification->getMetadataVersionId());
}

bool GlobalLockOperation::isDone(bool & success) const{
	if(hasAllNodesReplied() && !waitForReadviewReleaseFlag){
		// flush the change buffer
		if(needToFlush){
			ShardManager::getShardManager()->flushChangeHistory(this->request->getChangeId());
		}
		success = this->success;
		return true;
	}
	return false;
}

bool GlobalLockOperation::hasAllNodesReplied() const{
	for(map<NodeId, bool>::iterator nodeItr = haveReplied.begin(); nodeItr != haveReplied.end(); ++nodeItr){
		if(! nodeItr->second){
			return false;
		}
	}
	return true;
}


LockManager * LockManager::singleInstance = 0x0;
LockManager * LockManager::createLockManager(){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}
	singleInstance = new LockManager();
	return singleInstance;
}
LockManager * LockManager::getLockManager(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

/*
* the first method called when a notification comes for LockManager,
* this method uses other functions and the buffer to take care of this notification.
*/
void LockManager::resolve(LockingNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	ClusterResourceMetadata_Writeview & writeview = MetadataManager::getMetadataManager()->getWriteview();
	// 1. fist apply the change on local metadata and commit metadata if needed
	bool waitForReadviewReleaseFlag = false;
	unsigned versionToBeReleased = 0;
	bool lockSuccess = LockManager::getLockManager()->localLock(notification->getLockRequest(), waitForReadviewReleaseFlag , versionToBeReleased);
	if(! lockSuccess){
		// local lock failed, reply REJECTED
		LockingNotification::REJECTED * replyRejected = new LockingNotification::REJECTED(writeview.currentNodeId, 0, notification->getSrcOperationId().nodeId,
				notification->getSrcOperationId().operationId);
		ShardManager::getShardManager()->send(replyRejected);
		delete replyRejected;
		return;
	}
	if(notification->getLockRequest()->isAcquireOrRelease() == false){ // if it's release, reply GRANTED right away
		LockingNotification::GRANTED * replyGranted = new LockingNotification::GRANTED(writeview.currentNodeId, 0, notification->getSrcOperationId().nodeId,
				notification->getSrcOperationId().operationId);
		ShardManager::getShardManager()->send(replyGranted);
		delete replyGranted;
		return;
	}
	if(waitForReadviewReleaseFlag){ // we must wait for RV release
		pendingLockNotifications.push_back({versionToBeReleased, notification->getSrcOperationId()});
	}
}

void LockManager::resolve(LockingNotification::RV_RELEASED * inputNotification){
	if(inputNotification == NULL){
		ASSERT(false);
		return;
	}
	ClusterResourceMetadata_Writeview & writeview = MetadataManager::getMetadataManager()->getWriteview();
	for(vector<std::pair<unsigned, NodeOperationId> >::iterator pendingNotifItr = pendingLockNotifications.begin();
			pendingNotifItr != pendingLockNotifications.end(); ++pendingNotifItr){
		if(inputNotification->getMetadataVersionId() >= pendingNotifItr->first){
			// we can send GRANTED to the requester
			LockingNotification::GRANTED * replyGranted = new LockingNotification::GRANTED(writeview.currentNodeId, 0, pendingNotifItr->second.nodeId,
					pendingNotifItr->second.operationId);
			ShardManager::getShardManager()->send(replyGranted);
			delete replyGranted;
		}
	}
}
// Local lock request
// sets waitForReadViewRelease to true if caller needs to wait for readview release
// returns false if lock is not appliable
bool LockManager::localLock(LockChange * lockRequest, bool & waitForReadViewRelease, unsigned & metadataVersionToBeReleased){
	if(lockRequest == NULL){
		ASSERT(false);
		return false;
	}
	map<ShardId, bool> needToChangeMetadata;
	if(! lockRequest->apply(&(this->clusterLock), needToChangeMetadata)){
		return false;
	}
	if(needToChangeMetadata.size() == 0){
		waitForReadViewRelease = false;
		return true;
	}
	waitForReadViewRelease = true;
	MetadataLockChange metadataLockChange(needToChangeMetadata);
	metadataVersionToBeReleased = MetadataManager::getMetadataManager()->applyAndCommit(&metadataLockChange);
	return true;
}

}
}
