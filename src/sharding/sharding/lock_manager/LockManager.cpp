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
	if(lockBatch == NULL){
	    Logger::sharding(Logger::Error, "LockManager| generate lock batch returning NULL");
		return;
	}

	if(! lockBatch->release &&
	        lockBatch->batchType == LockRequestType_Metadata && lockBatch->olderNodes.size() > 0){
	    // specific to node arrival, nodes must join in ascending nodeId order
        SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
        for(unsigned i = 0 ; i < lockBatch->olderNodes.size() ; ++i){
            // 1. is this node in the writeview of this node ?
            if(lockBatch->olderNodes.at(i) > nodesWriteview->currentNodeId){
                ShardingNodeState olderNodeState;
                if(! nodesWriteview->getNodeState(lockBatch->olderNodes.at(i), olderNodeState)){ // not does not exist
                    nodesWriteview->setNodeState(lockBatch->olderNodes.at(i), ShardingNodeStateNotArrived);
                }
            }
        }
	}

	boost::unique_lock<boost::shared_mutex> xLockWriteview ;
	ShardManager::getShardManager()->getWriteview_write(xLockWriteview);
	lockManagerMutex.lock();
	bool releaseHappended = false;
	if(lockBatch->release){
		releaseHappended = resolveRelease(lockBatch);
	}else{
		resolveLock(lockBatch);
	}
	bool shouldCommit = lockBatch->shouldCommitReadview;
	bool shouldFinalize = lockBatch->shouldFinalize;
	if(shouldCommit){
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata(false, false);
	}
	if(shouldFinalize){
		finalize(lockBatch);
		delete lockBatch;
	}
	lockManagerMutex.unlock();
	xLockWriteview.unlock();
	if(releaseHappended){
		movePendingLockBatchesForward();
	}
}

void LockManager::resolve(const unsigned readviewReleasedVersion){
    Logger::sharding(Logger::Step, "LockManager| resolving RV release, version %d", readviewReleasedVersion);
	// lock
	readviewReleaseMutex.lock();
	for(vector<LockBatch *>::iterator lockBItr = rvReleasePendingLockBatches.begin();
			lockBItr != rvReleasePendingLockBatches.end(); ){
		LockBatch * lockBatch = *lockBItr;
		if(readviewReleasedVersion >= lockBatch->versionId){
			lockBatch->shouldFinalize = true; // just for consistency because we are going to finalize it right here
			lockBatch->finalizeResult = true;
			finalize(lockBatch);
			delete lockBatch;
			lockBItr = rvReleasePendingLockBatches.erase(lockBItr);
		}else{
			++lockBItr;
		}
	}
	readviewReleaseMutex.unlock();
}

void LockManager::resolveNodeFailure(const NodeId & failedNode){
	Logger::sharding(Logger::Step, "LockManager| resolving node failure , for node %d", failedNode);
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

	boost::unique_lock<boost::shared_mutex> xLockWriteview ;
	ShardManager::getShardManager()->getWriteview_write(xLockWriteview);
	lockManagerMutex.lock();
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

	bool releaseHappenned = clusterShardLocks.release(failedNode);
	bool releaseHappenned2 = releaseHappenned;
	releaseHappenned2 = releaseHappenned2 || primaryKeyLocks.release(failedNode);
	releaseHappenned2 = releaseHappenned2 || allNodeSharedInfo.release(failedNode);

	if(passedInitialization.find(failedNode) != passedInitialization.end()){
		releaseHappenned2 = true;
		passedInitialization.erase(passedInitialization.find(failedNode));
	}

	// reflect changes on the readview
	if(releaseHappenned){
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata(false, false);
	}
	lockManagerMutex.unlock();
	xLockWriteview.unlock();

	// maybe some other pending lock requests can move forward
	if(releaseHappenned2){
		movePendingLockBatchesForward();
	}

}

bool LockManager::canAcquireLock(const ClusterShardId & shardId, const LockLevel & lockLevel){
	lockManagerMutex.lock();
	LockBatch * lockBatch = LockBatch::generateLockBatch(shardId, lockLevel);
	bool result = canAcquireAllBatch(lockBatch);
	delete lockBatch;
	lockManagerMutex.unlock();
	return result;
}

void LockManager::resolveLock(LockBatch * lockBatch){
	Logger::sharding(Logger::Step, "LockManager| resolving LOCK request : %s", lockBatch->toString().c_str());
	if(lockBatch->release){
		ASSERT(false);
		lockBatch->shouldFinalize = true;
		lockBatch->finalizeResult = false;
		return;
	}

	// first check if it's blocking or not
	if(! lockBatch->blocking){
		// if all the batch cannot succeed right now, we send reject ack back
		if(! canAcquireAllBatch(lockBatch)){
			// reject the request right here
			Logger::sharding(Logger::Step, "LockManager| REJECTED. ");
			lockBatch->shouldFinalize = true;
			lockBatch->finalizeResult = false;
			return;
		}
	}

	if(moveLockBatchForward(lockBatch)){
		// batch was done completely, we can finish this request
		Logger::sharding(Logger::Step, "LockManager| GRANTED");
		lockBatch->shouldFinalize = true;
		lockBatch->finalizeResult = true;
		return;
	}else{
		// if we must wait for rv-release
		if(lockBatch->isReadviewPending()){
			// just put it in rv release pending lock batches
			Logger::sharding(Logger::Detail, "LockManager| lock request pending for RV release. readview vid : %d", lockBatch->versionId);
			return;
		}else{
			Logger::sharding(Logger::Detail, "LockManager| lock request gone to waiting list ...");
			pendingLockBatches.push_back(lockBatch);
			return;
		}
	}
}


bool LockManager::resolveRelease(LockBatch * lockBatch){
	Logger::sharding(Logger::Step, "LockManager| resolving RELEASE request : %s", lockBatch->toString().c_str());
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
					bool relHap = clusterShardLocks.release(lockBatch->tokens.at(lockBatch->lastGrantedItemIndex).first , lockBatch->opIds.at(i));
					releaseHappened = releaseHappened || relHap;
				}
			}
			break;
		}
		case LockRequestType_Metadata:
		{
			for(unsigned i = 0 ;i < lockBatch->opIds.size(); ++i){
				bool relHap = allNodeSharedInfo.release(metadataResourceName , lockBatch->opIds.at(i));
				releaseHappened = releaseHappened || relHap;
			}
			break;
		}
		case LockRequestType_PrimaryKey:
		{
			for(lockBatch->lastGrantedItemIndex = 0 ;
					lockBatch->lastGrantedItemIndex < lockBatch->pkTokens.size(); ++ lockBatch->lastGrantedItemIndex){
				for(unsigned i = 0 ;i < lockBatch->opIds.size(); ++i){
					stringstream ss;
					ss << lockBatch->pkTokens.at(lockBatch->lastGrantedItemIndex).first <<
							PRIMARY_KEY_SHARD_DELIMITER << lockBatch->pid.toString();
					string lockKeyValue = ss.str();
					bool relHap = primaryKeyLocks.release(lockKeyValue , lockBatch->opIds.at(i));
					releaseHappened = releaseHappened || relHap;
				}
			}
			break;
		}
	}

	if(releaseHappened){
		if(lockBatch->batchType != LockRequestType_PrimaryKey){
			lockBatch->shouldCommitReadview = true;
		}
	}
	lockBatch->finalizeResult = true;
	lockBatch->shouldFinalize = true;
	return releaseHappened;
}


bool LockManager::isPartitionLocked(const ClusterPID & pid){
	lockManagerMutex.lock();
	vector<ClusterShardId> allLockedShards;
	clusterShardLocks.getAllLockedResource(allLockedShards);
	for(unsigned i = 0 ; i < allLockedShards.size(); ++ i){
		if(allLockedShards.at(i).getPartitionId() == pid){
			lockManagerMutex.unlock();
			return true;
		}
	}
	lockManagerMutex.unlock();
	return false;
}
void LockManager::getLockedPartitions(vector<ClusterPID> & lockedPartitions, bool shouldLock){
	if(shouldLock){
		lockManagerMutex.lock();
	}
	vector<ClusterShardId> allLockedShards;
	clusterShardLocks.getAllLockedResource(allLockedShards);
	for(unsigned i = 0 ; i < allLockedShards.size(); ++ i){
		if(std::find(lockedPartitions.begin(), lockedPartitions.end(), allLockedShards.at(i).getPartitionId()) == lockedPartitions.end()){
			lockedPartitions.push_back(allLockedShards.at(i).getPartitionId());
		}
	}
	if(shouldLock){
		lockManagerMutex.unlock();
	}
}


void LockManager::movePendingLockBatchesForward(unsigned pendingLockBatchIdx){

	boost::unique_lock<boost::shared_mutex> xLockWriteview ;
	ShardManager::getShardManager()->getWriteview_write(xLockWriteview);
	lockManagerMutex.lock();
	if(pendingLockBatchIdx >= pendingLockBatches.size()){
		lockManagerMutex.unlock();
		return;
	}

	unsigned nextPendingLockBatchIdx = 0;
	LockBatch * lockBatch = pendingLockBatches.at(pendingLockBatchIdx);
	pendingLockBatches.erase(pendingLockBatches.begin()+pendingLockBatchIdx);

	ASSERT(! lockBatch->release);
	bool lockBatchMoveResult = moveLockBatchForward(lockBatch);
	bool shouldCommit = lockBatch->shouldCommitReadview;
	if(lockBatchMoveResult){
		Logger::sharding(Logger::Detail, "LockManager| GRANTED : %s",
				(lockBatch->getLockHoldersStr() + "/" + lockBatch->getBatchTypeStr()).c_str());
		lockBatch->shouldFinalize = true;
		lockBatch->finalizeResult = true;
		// RV pending : we will just commit and move to the next request
		nextPendingLockBatchIdx = pendingLockBatchIdx;
	}else{
		if(lockBatch->isReadviewPending()){ // lock is acquired but must wait for rv release
			// erase it from pendingLockRequests and put it in readview release
			Logger::sharding(Logger::Detail, "LockManager| %s gone to RV release pending list. ",
					(lockBatch->getLockHoldersStr() + "/" + lockBatch->getBatchTypeStr()).c_str());

			// it's already placed in rv pending requests
			// RV pending : we will just commit and move to the next request
			nextPendingLockBatchIdx = pendingLockBatchIdx;
		}else{ // lock is not acquired and must stay in pending requests
			pendingLockBatches.insert(pendingLockBatches.begin() + pendingLockBatchIdx, lockBatch);
			nextPendingLockBatchIdx = pendingLockBatchIdx + 1;
		}
	}

	if(shouldCommit){
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata(false, false);
	}
	if(lockBatchMoveResult){
		finalize(lockBatch);
		delete lockBatch;
	}

	lockManagerMutex.unlock();
	xLockWriteview.unlock() ;
	movePendingLockBatchesForward(nextPendingLockBatchIdx);
}


void LockManager::setPendingForRVRelease(LockBatch * lockBatch){
	readviewReleaseMutex.lock();
	rvReleasePendingLockBatches.push_back(lockBatch);
	readviewReleaseMutex.unlock();
}

bool LockManager::canAcquireAllBatch(LockBatch * lockBatch){

	ASSERT(lockBatch->lastGrantedItemIndex == -1);
	int lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

	while(true){
		switch (lockBatch->batchType) {
			// NOTE: metadata lock conflicts with all cluster shard id locks
			case LockRequestType_Copy:
			case LockRequestType_Move:
			case LockRequestType_GeneralPurpose:
			case LockRequestType_ShardIdList:
			{
				bool allSLocks = true;
				for(unsigned t = 0; t < lockBatch->tokens.size(); ++t){
					if(lockBatch->tokens.at(t).second == LockLevel_X){
						allSLocks = false;
						break;
					}
				}
				// if there is any X lock, it conflicts with metadata lock
				if(allNodeSharedInfo.isLockTypeConflicting(metadataResourceName, allSLocks ? LockLevel_S:LockLevel_X)){
					return false;
				}

				pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lastGrantedPreValue + 1);
				if(! clusterShardLocks.canLock(nxtToken.first, nxtToken.second)){
					return false;
				}else{
					lastGrantedPreValue++;
					if(lastGrantedPreValue == ((int)lockBatch->tokens.size()) - 1){
						return true;
					}
				}
				break;
			}
			case LockRequestType_Metadata:
			{
				vector<ClusterShardId> allResource;
				clusterShardLocks.getAllLockedResource(allResource);
				bool conflictsWithShardLocks = false;
				for(unsigned i = 0 ; i < allResource.size(); ++i){
					if(clusterShardLocks.isLockTypeConflicting(allResource.at(i), lockBatch->metadataLockLevel)){
						conflictsWithShardLocks = true;
						break;
					}
				}
				if(conflictsWithShardLocks){
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
				return allNodeSharedInfo.canLock(metadataResourceName, lockBatch->metadataLockLevel);
			}
			case LockRequestType_PrimaryKey:
			{
				ASSERT(lockBatch->lastGrantedItemIndex < lockBatch->pkTokens.size());
				ASSERT(lockBatch->incremental);
				pair<string, LockLevel> & nxtToken = lockBatch->pkTokens.at(lastGrantedPreValue + 1);
				stringstream ss;
				ss << nxtToken.first << PRIMARY_KEY_SHARD_DELIMITER << lockBatch->pid.toString();
				if(! primaryKeyLocks.canLock(ss.str() , nxtToken.second)){
					return false;
				}else{
					lastGrantedPreValue++;
					if(lastGrantedPreValue == ((int)lockBatch->pkTokens.size()) - 1){
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

	int lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

	bool continueMovingForward = true;
	while(continueMovingForward){
		switch (lockBatch->batchType) {
			case LockRequestType_Copy:
			case LockRequestType_Move:
			case LockRequestType_GeneralPurpose:
			case LockRequestType_ShardIdList:
			{
				bool allSLocks = true;
				for(unsigned t = 0; t < lockBatch->tokens.size(); ++t){
					if(lockBatch->tokens.at(t).second == LockLevel_X){
						allSLocks = false;
						break;
					}
				}
				// if there is any X lock, it conflicts with metadata lock
				if(allNodeSharedInfo.isLockTypeConflicting(metadataResourceName, allSLocks ? LockLevel_S:LockLevel_X)){
					Logger::sharding(Logger::Detail, "LockManager| failed to move forward because metadata is locked.");
					return false;
				}

				pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lockBatch->lastGrantedItemIndex + 1);
				if(clusterShardLocks.lock(nxtToken.first, lockBatch->opIds, nxtToken.second)){
					// one more token was granted.
					lockBatch->lastGrantedItemIndex++;
					if(lockBatch->lastGrantedItemIndex == ((int)lockBatch->tokens.size()) - 1){

						lockBatch->versionId = ShardManager::getShardManager()->getMetadataManager()->getClusterWriteviewVersionID();
						ASSERT(lockBatch->isReadviewPending());
						setPendingForRVRelease(lockBatch);
						lockBatch->shouldCommitReadview = true;
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
				bool conflictsWithShardLocks = false;
				for(unsigned i = 0 ; i < allResource.size(); ++i){
					if(clusterShardLocks.isLockTypeConflicting(allResource.at(i), lockBatch->metadataLockLevel)){
						conflictsWithShardLocks = true;
						break;
					}
				}
				if(conflictsWithShardLocks){
					stringstream ss;
					for(unsigned i = 0 ; i < allResource.size(); i++){
						if(i != 0){
							ss << ",";
						}
						ss << allResource.at(i).toString();
					}
					Logger::sharding(Logger::Detail, "LockManager| failed to move forward because clusterShardIds are locked : %s", ss.str().c_str());
					return false;
				}

				ASSERT(lockBatch->lastGrantedItemIndex == -1);
				LockLevel lockLevel = lockBatch->metadataLockLevel;
				for(unsigned i = 0 ; i < lockBatch->olderNodes.size(); ++i){
					if(! isNodePassedInitialization(lockBatch->olderNodes.at(i))){
						Logger::sharding(Logger::Detail, "LockManager| node %d is ahead of this node.", lockBatch->olderNodes.at(i));
						return false; // still not allowed to run this.
					}
				}
				// we can try to lock for this new node now
				if(allNodeSharedInfo.lock(metadataResourceName, lockBatch->opIds, lockBatch->metadataLockLevel)){
					// we can got the metadata lock, let's get back to the user.
					lockBatch->lastGrantedItemIndex ++;
					if(lockBatch->olderNodes.size() > 0){
						ASSERT(lockBatch->opIds.size() == 1);
						setNodePassedInitialization(lockBatch->opIds.at(0).nodeId);
					}
					return true;
				}
				return false;
			}
			case LockRequestType_PrimaryKey:
			{
				ASSERT(lockBatch->pkTokens.size() > 0);
				ASSERT(lockBatch->lastGrantedItemIndex < ((int)lockBatch->pkTokens.size()) - 1);
				pair<string, LockLevel> & nxtToken = lockBatch->pkTokens.at(lockBatch->lastGrantedItemIndex + 1);
				stringstream ss;
				ss << nxtToken.first << PRIMARY_KEY_SHARD_DELIMITER << lockBatch->pid.toString();
				if(primaryKeyLocks.lock(ss.str(), lockBatch->opIds, nxtToken.second)){
					// one more token was granted.
					lockBatch->lastGrantedItemIndex++;
					if(lockBatch->lastGrantedItemIndex == ((int)lockBatch->pkTokens.size())-1){
						// let's send the last update and we are done.
						return true;
					}
				}else{
					if(lockBatch->incremental){
						// let's just wait until we can move forward
						if(lastGrantedPreValue < lockBatch->lastGrantedItemIndex){
							// we can send an update at this point
							ASSERT(false);
//							finalize(lockBatch, true);
						}
					}
					return false; // we are not done yet, we must wait until we reach to the end of batch
				}
				break;
			}
		}
	}
	return false;
}

void LockManager::finalize(LockBatch * lockBatch){
	bool result  = lockBatch->finalizeResult;
	lockBatch->granted = result;
	if( lockBatch->ack != NULL){
		lockBatch->ack->setGranted(result);
		lockBatch->ack->setIndexOfLastGrantedItem(lockBatch->lastGrantedItemIndex);
	}
	// send the ack
	ShardingNotification::send(lockBatch->ack);
}

bool LockManager::isNodePassedInitialization(const NodeId & nodeId){
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	if(! nodesWriteview->isNodeAlive(nodeId)){
		return true; // failed nodes are assumed to have passed the initialization step
	}
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
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
    const map<NodeId, std::pair<ShardingNodeState, Node *> > & nodes = nodesWriteview->getNodes_read();
    for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
    	if(nodeItr->second.first == ShardingNodeStateArrived){
			setNodePassedInitialization(nodeItr->second.second->getId());
    	}
    }
}

void LockManager::print(JsonResponseHandler * response){
	if(response != NULL){
		lockManagerMutex.lock();
		if(this->isLockManagerClean()){
			lockManagerMutex.unlock();
			return;
		}
		// print pending lock requests
		printPendingRequests(pendingLockBatches, "pending-lock-requests", response);

		//  print rv pending lock requests
		printRVPendingRequests(response);

		// print cluster shard locks
		printClusterShardIdLocks(response);
		// print primary key locks
		printPrimaryKeyLocks(response);
		// print metadata locks
		printMetadataLocks(response);

		lockManagerMutex.unlock();

		return;
	}
	lockManagerMutex.lock();
	if(this->isLockManagerClean()){
		lockManagerMutex.unlock();
		return;
	}
	cout << "**************************************************************************************************" << endl;
	cout << "LockManager : " << endl;
	cout << "**************************************************************************************************" << endl;

	// print pending lock requests
	printPendingRequests(pendingLockBatches, "Pending Lock %Requests");

	//  print rv pending lock requests
	printRVPendingRequests();

	// print cluster shard locks
	printClusterShardIdLocks();
	// print primary key locks
	printPrimaryKeyLocks();
	// print metadata locks
	printMetadataLocks();

	lockManagerMutex.unlock();
}


void LockManager::printPendingRequests(const vector<LockBatch *> & pendingLockBatches, const string & tableName, JsonResponseHandler * response) const{
	if(response != NULL){
		Json::Value pendingLockBatchesJson(Json::arrayValue);
		for(unsigned i = 0; i < pendingLockBatches.size(); ++i){
			pendingLockBatchesJson[i] = Json::Value(Json::objectValue);
			pendingLockBatches.at(i)->toString(&(pendingLockBatchesJson[i]));
		}
		response->setResponseAttribute(tableName.c_str() , pendingLockBatchesJson);
		return;
	}
	if(pendingLockBatches.empty()){
		return;
	}
	vector<string> colomnHeaders;
	colomnHeaders.push_back("bach type");
	colomnHeaders.push_back("lock holders");
	colomnHeaders.push_back("resource/lock level");
	colomnHeaders.push_back("blocking%release%incremental%versionId");

	vector<string> labels;
	for(unsigned i = 0; i < pendingLockBatches.size(); ++i){
		labels.push_back(pendingLockBatches.at(i)->ack->getDest().toString());
	}

	srch2::util::TableFormatPrinter reqTable(tableName, 120, colomnHeaders, labels );
	reqTable.printColumnHeaders();
	reqTable.startFilling();
	for(unsigned i = 0; i < pendingLockBatches.size(); ++i){
		LockBatch * lb = pendingLockBatches.at(i);
		reqTable.printNextCell(lb->getBatchTypeStr());
		reqTable.printNextCell(lb->getLockHoldersStr());
		reqTable.printNextCell(lb->getResourceStr());
		reqTable.printNextCell(lb->getExtraInfoStr());
	}

}

void LockManager::printRVPendingRequests(JsonResponseHandler * response){
	if(response != NULL){
		printPendingRequests(rvReleasePendingLockBatches, "rv-release-pending-requests", response);
	}else{
		readviewReleaseMutex.lock();
		printPendingRequests(rvReleasePendingLockBatches, "RV Release %pending Requests", response);
		readviewReleaseMutex.unlock();
	}
}

void LockManager::printClusterShardIdLocks(JsonResponseHandler * response){
	if(response != NULL){
		clusterShardLocks.print("cluster-shard-locks", response);
	}else{
		clusterShardLocks.print("ClusterShard Locks", response);
	}
}

void LockManager::printPrimaryKeyLocks(JsonResponseHandler * response){
	if(response != NULL){
		primaryKeyLocks.print("primary-key-locks", response);
	}else{
		primaryKeyLocks.print("Primary Key Locks", response);
	}
}

void LockManager::printMetadataLocks(JsonResponseHandler * response){
	if(response != NULL){
		allNodeSharedInfo.print("metadata-locks", response);
	}else{
		allNodeSharedInfo.print("Metadata locks", response);
	}
}

}
}
