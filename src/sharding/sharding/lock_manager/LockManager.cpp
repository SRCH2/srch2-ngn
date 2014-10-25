#include "LockManager.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "../ShardManager.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../metadata_manager/ResourceMetadataManager.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

const string LockManager::metadataResourceName = string("srch2-cluster-metadata");

LockManager::LockManager(){
}


void LockManager::resolve(SP(LockingNotification)  notif){
	if(! notif){
		ASSERT(false);
		return;
	}
	LockBatch * lockBatch = LockBatch::generateLockBatch(notif);
	resolve(lockBatch);
}

void LockManager::resolve(const unsigned readviewReleasedVersion){
    Logger::debug("STEP :  Lock Manager : resolving RV release, version %d", readviewReleasedVersion);
	// lock
	readviewReleaseMutex.lock();
	for(vector<LockBatch *>::iterator lockBItr = rvReleasePendingLockBatches.begin();
			lockBItr != rvReleasePendingLockBatches.end(); ){
		LockBatch * lockBatch = *lockBItr;
		if(readviewReleasedVersion >= lockBatch->versionId){
			finalize(lockBatch, true);
			delete lockBatch;
			lockBItr = rvReleasePendingLockBatches.erase(lockBItr);
		}else{
			++lockBItr;
		}
	}
	readviewReleaseMutex.unlock();
}

void LockManager::resolveNodeFailure(const NodeId & failedNode){
    Logger::debug("STEP :  Lock Manager : resolving node failure , for node %d", failedNode);
	readviewReleaseMutex.lock();
	for(vector<LockBatch *>::iterator lockBItr = rvReleasePendingLockBatches.begin();
			lockBItr != rvReleasePendingLockBatches.end(); ){
		LockBatch * lockBatch = *lockBItr;
		if(! lockBatch->update(failedNode)){
			delete lockBatch;
			lockBItr = rvReleasePendingLockBatches.erase(lockBItr);
		}else{
			++lockBItr;
		}
	}
	readviewReleaseMutex.unlock();

	for(vector<LockBatch *>::iterator lockBItr = pendingLockBatches.begin();
			lockBItr != pendingLockBatches.end(); ){
		LockBatch * lockBatch = *lockBItr;
		if(! lockBatch->update(failedNode)){
			delete lockBatch;
			lockBItr = pendingLockBatches.erase(lockBItr);
		}else{
			++lockBItr;
		}
	}

	clusterShardLocks.release(failedNode);
	primaryKeyLocks.release(failedNode);
	allNodeSharedInfo.release(failedNode);

	if(passedInitialization.find(failedNode) != passedInitialization.end()){
		passedInitialization.erase(passedInitialization.find(failedNode));
	}

	movePendingLockBatchesForward();

}

bool LockManager::canAcquireLock(const ClusterShardId & shardId, const LockLevel & lockLevel){
	LockBatch * lockBatch = LockBatch::generateLockBatch(shardId, lockLevel);
	return canAcquireAllBatch(lockBatch);
}

// entry point of LockBatch to LockManager
void LockManager::resolve(LockBatch * lockBatch){
	if(lockBatch == NULL){
		ASSERT(false);
		return;
	}
	if(lockBatch->release){
		resolveRelease(lockBatch);
		finalize(lockBatch, true);
		delete lockBatch;
		return;
	}
	resolveLock(lockBatch);
}


void LockManager::resolveLock(LockBatch * lockBatch){
    Logger::debug("STEP :  Lock Manager : resolving lock request : %s", lockBatch->toString().c_str());
	if(lockBatch->release){
		ASSERT(false);
		finalize(lockBatch, false);
		delete lockBatch;
	}

	// specific to node arrival, nodes must join in ascending nodeId order
	if(lockBatch->batchType == LockRequestType_Metadata && lockBatch->olderNodes.size() > 0){
		Cluster_Writeview * writeview = ShardManager::getWriteview();
		for(unsigned i = 0 ; i < lockBatch->olderNodes.size() ; ++i){
			// 1. is this node in the writeview of this node ?
			if(writeview->nodes.find(lockBatch->olderNodes.at(i)) == writeview->nodes.end()){
				//add it to the list of nodes in writeview as NotArrived node
				writeview->setNodeState(lockBatch->olderNodes.at(i), ShardingNodeStateNotArrived);
			}
		}
	}

	// first check if it's blocking or not
	if(! lockBatch->blocking){
		// if all the batch cannot succeed right now, we send reject ack back
		if(! canAcquireAllBatch(lockBatch)){
			// reject the request right here
	        Logger::debug("DETAILS :  Lock Manager : lock request rejected. ");
			finalize(lockBatch, false);
			delete lockBatch;
			return;
		}
	}

	if(moveLockBatchForward(lockBatch)){
		// batch was done completely, we can finish this request
        Logger::debug("DETAILS :  Lock Manager : lock request batch was done completely.");
		delete lockBatch;
		return;
	}else{
		// if we must wait for rv-release
		if(lockBatch->isReadviewPending()){
			// just put it in rv release pending lock batches
		    Logger::debug("DETAILS :  Lock Manager : lock request pending for RV release.");
			setPendingForRVRelease(lockBatch);
			return;
		}else{
		    Logger::debug("DETAILS :  Lock Manager : lock request gone to waiting list ...");
			pendingLockBatches.push_back(lockBatch);
			return;
		}
	}
}


void LockManager::resolveRelease(LockBatch * lockBatch){
    Logger::debug("STEP :  Lock Manager : resolving release request : ", lockBatch->toString().c_str());
	bool releaseHappened = false;

	switch (lockBatch->batchType) {
		case LockRequestType_Copy:
		case LockRequestType_Move:
		case LockRequestType_GeneralPurpose:
		case LockRequestType_ShardIdList:
		{
			for(lockBatch->lastGrantedItemIndex = 0 ;
					lockBatch->lastGrantedItemIndex < lockBatch->tokens.size(); ++ lockBatch->lastGrantedItemIndex){
				for(unsigned i = 0 ;i < lockBatch->opIds.size(); ++i){
					releaseHappened = releaseHappened ||
							clusterShardLocks.release(lockBatch->tokens.at(lockBatch->lastGrantedItemIndex).first , lockBatch->opIds.at(i));
				}
			}
			break;
		}
		case LockRequestType_Metadata:
		{
			for(unsigned i = 0 ;i < lockBatch->opIds.size(); ++i){
				releaseHappened = releaseHappened ||
						allNodeSharedInfo.release(metadataResourceName , lockBatch->opIds.at(i));
			}
			break;
		}
		case LockRequestType_PrimaryKey:
		{
			for(lockBatch->lastGrantedItemIndex = 0 ;
					lockBatch->lastGrantedItemIndex < lockBatch->pkTokens.size(); ++ lockBatch->lastGrantedItemIndex){
				for(unsigned i = 0 ;i < lockBatch->opIds.size(); ++i){
					releaseHappened = releaseHappened ||
							primaryKeyLocks.release(lockBatch->pkTokens.at(lockBatch->lastGrantedItemIndex).first , lockBatch->opIds.at(i));
				}
			}
			break;
		}
	}

	if(releaseHappened){
        Logger::debug("DETAILS :  Lock Manager : release request triggers some pending lock requests to be applied ....");
		movePendingLockBatchesForward();
	}
}


bool LockManager::isPartitionLocked(const ClusterPID & pid){
	vector<ClusterShardId> allLockedShards;
	clusterShardLocks.getAllLockedResource(allLockedShards);
	for(unsigned i = 0 ; i < allLockedShards.size(); ++ i){
		if(allLockedShards.at(i).getPartitionId() == pid){
			return true;
		}
	}
	return false;
}
void LockManager::getLockedPartitions(vector<ClusterPID> & lockedPartitions){
	vector<ClusterShardId> allLockedShards;
	clusterShardLocks.getAllLockedResource(allLockedShards);
	for(unsigned i = 0 ; i < allLockedShards.size(); ++ i){
		if(std::find(lockedPartitions.begin(), lockedPartitions.end(), allLockedShards.at(i).getPartitionId()) == lockedPartitions.end()){
			lockedPartitions.push_back(allLockedShards.at(i).getPartitionId());
		}
	}
}


void LockManager::movePendingLockBatchesForward(){
	for(vector<LockBatch *>::iterator lockBatchItr = pendingLockBatches.begin();
			lockBatchItr != pendingLockBatches.end(); ){
		ASSERT(! (*lockBatchItr)->release);
		if(moveLockBatchForward(*lockBatchItr)){
			delete *lockBatchItr;
			lockBatchItr = pendingLockBatches.erase(lockBatchItr);
            Logger::debug("DETAILS :  Lock Manager : lock request finished.");
		}else{
			if((*lockBatchItr)->isReadviewPending()){
				// erase it from pendingLockRequests and put it in readview release
				LockBatch * lockBatch = *lockBatchItr;
	            Logger::debug("DETAILS :  Lock Manager : lock request gone to RV release pending list. ");
				lockBatchItr = pendingLockBatches.erase(lockBatchItr);
				setPendingForRVRelease(lockBatch);
			}
			++lockBatchItr;
		}
	}
}


void LockManager::setPendingForRVRelease(LockBatch * lockBatch){
	readviewReleaseMutex.lock();
	rvReleasePendingLockBatches.push_back(lockBatch);
	readviewReleaseMutex.unlock();

	// commit
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
}

bool LockManager::canAcquireAllBatch(LockBatch * lockBatch){

	ASSERT(lockBatch->lastGrantedItemIndex == -1);
	unsigned lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

	while(true){
		switch (lockBatch->batchType) {
			// NOTE: metadata lock conflicts with all cluster shard id locks
			case LockRequestType_Copy:
			case LockRequestType_Move:
			case LockRequestType_GeneralPurpose:
			case LockRequestType_ShardIdList:
			{
					pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lastGrantedPreValue + 1);

					if(allNodeSharedInfo.isLock(metadataResourceName)){
						return false;
					}
					if(! clusterShardLocks.canLock(nxtToken.first, nxtToken.second)){
						return false;
					}else{
						lastGrantedPreValue++;
						if(lastGrantedPreValue == lockBatch->tokens.size() - 1){
							return true;
						}
					}
					break;
			}
			case LockRequestType_Metadata:
			{
				ASSERT(lockBatch->lastGrantedItemIndex == -1);
				LockLevel lockLevel = lockBatch->metadataLockLevel;
				for(unsigned i = 0 ; i < lockBatch->olderNodes.size(); ++i){
					if(! isNodePassedInitialization(lockBatch->olderNodes.at(i))){
						return false; // stil not allowed to run this.
					}
				}
				vector<ClusterShardId> allResource;
				clusterShardLocks.getAllLockedResource(allResource);
				if(! allResource.empty()){
					return false;
				}
				// we can try to lock for this new node now
				return allNodeSharedInfo.canLock(metadataResourceName, lockBatch->metadataLockLevel);
			}
			case LockRequestType_PrimaryKey:
			{
				ASSERT(lockBatch->lastGrantedItemIndex < lockBatch->pkTokens.size());
				ASSERT(lockBatch->incremental);
				pair<string, LockLevel> & nxtToken = lockBatch->pkTokens.at(lastGrantedPreValue + 1);
				if(! primaryKeyLocks.canLock(nxtToken.first, nxtToken.second)){
					return false;
				}else{
					if(lastGrantedPreValue == lockBatch->pkTokens.size() - 1){
						return true;
					}
				}
				break;
			}
		}
	}
	return false;
}
/*
 * returns true only of lockBatch is finished and must be removed.
 * This method moves a pending lock batch forward as much as possible
 */
bool LockManager::moveLockBatchForward(LockBatch * lockBatch){

	unsigned lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

	while(true){
		switch (lockBatch->batchType) {
			case LockRequestType_Copy:
			case LockRequestType_Move:
			case LockRequestType_GeneralPurpose:
			case LockRequestType_ShardIdList:
			{
				if(allNodeSharedInfo.isLock(metadataResourceName)){
					return false;
				}

				pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lockBatch->lastGrantedItemIndex + 1);
				if(clusterShardLocks.lock(nxtToken.first, lockBatch->opIds, nxtToken.second)){
					// one more token was granted.
					lockBatch->lastGrantedItemIndex++;
					if(lockBatch->lastGrantedItemIndex == lockBatch->tokens.size() - 1){
						lockBatch->versionId = ShardManager::getWriteview()->versionId;
						ASSERT(lockBatch->isReadviewPending());
						return false; // because we should still wait for the release of readview
					}else{
						ASSERT(! lockBatch->incremental);
						// let's just wait for next tokens to acquire lock entirely
					}
				}else{
					// let's just wait until we can move forward
					return false;
				}
				break;
			}
			case LockRequestType_Metadata:
			{
				vector<ClusterShardId> allResource;
				clusterShardLocks.getAllLockedResource(allResource);
				if(! allResource.empty()){
					return false;
				}

				ASSERT(lockBatch->lastGrantedItemIndex == -1);
				LockLevel lockLevel = lockBatch->metadataLockLevel;
				for(unsigned i = 0 ; i < lockBatch->olderNodes.size(); ++i){
					if(! isNodePassedInitialization(lockBatch->olderNodes.at(i))){
						return false; // stil not allowed to run this.
					}
				}
				// we can try to lock for this new node now
				if(allNodeSharedInfo.lock(metadataResourceName, lockBatch->opIds, lockBatch->metadataLockLevel)){
					// we can got the metadata lock, let's get back to the user.
					lockBatch->lastGrantedItemIndex ++;
					finalize(lockBatch, true);
					return true;
				}
				return false;
			}
			case LockRequestType_PrimaryKey:
			{
				ASSERT(lockBatch->lastGrantedItemIndex < lockBatch->pkTokens.size());
				ASSERT(lockBatch->incremental);
				pair<string, LockLevel> & nxtToken = lockBatch->pkTokens.at(lockBatch->lastGrantedItemIndex + 1);
				if(primaryKeyLocks.lock(nxtToken.first, lockBatch->opIds, nxtToken.second)){
					// one more token was granted.
					lockBatch->lastGrantedItemIndex++;
					if(lockBatch->lastGrantedItemIndex == lockBatch->pkTokens.size()-1){
						// let's send the last update and we are done.
						finalize(lockBatch, true);
						return true;
					}
				}else{
					// let's just wait until we can move forward
					if(lastGrantedPreValue < lockBatch->lastGrantedItemIndex){
						// we can send an update at this point
						finalize(lockBatch, true);
					}
					return false; // but we are not done yet.
				}
				break;
			}
		}
	}
	return false;
}

void LockManager::finalize(LockBatch * lockBatch, bool result ){
	lockBatch->granted = result;
	if( lockBatch->ack != NULL){
		lockBatch->ack->setGranted(result);
		lockBatch->ack->setIndexOfLastGrantedItem(lockBatch->lastGrantedItemIndex);
	}
	if(lockBatch->batchType == LockRequestType_Metadata && lockBatch->olderNodes.size() > 0){
		ASSERT(lockBatch->opIds.size() == 1);
		setNodePassedInitialization(lockBatch->opIds.at(0).nodeId);
	}
	// send the ack
	ShardingNotification::send(lockBatch->ack);
}

bool LockManager::isNodePassedInitialization(const NodeId & nodeId){
	if(passedInitialization.find(nodeId) == passedInitialization.end()){
		passedInitialization[nodeId] = false;
		return false;
	}
	return passedInitialization[nodeId];
}
void LockManager::setNodePassedInitialization(const NodeId & nodeId){
	passedInitialization[nodeId] = true;
}

void LockManager::initialize(){
    map<NodeId, std::pair<ShardingNodeState, Node *> > & nodes = ShardManager::getShardManager()->getWriteview()->nodes;
    for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
        setNodePassedInitialization(nodeItr->second.second->getId());
    }
}

}
}
