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
	lockManagerMutex.lock();
	LockBatch * lockBatch = LockBatch::generateLockBatch(notif);
	resolve(lockBatch);
	lockManagerMutex.unlock();
//	print();
}

void LockManager::resolve(const unsigned readviewReleasedVersion){
    Logger::sharding(Logger::Step, "LockManager| resolving RV release, version %d", readviewReleasedVersion);
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

	clusterShardLocks.release(failedNode);
	primaryKeyLocks.release(failedNode);
	allNodeSharedInfo.release(failedNode);

	if(passedInitialization.find(failedNode) != passedInitialization.end()){
		passedInitialization.erase(passedInitialization.find(failedNode));
	}

	movePendingLockBatchesForward();
	lockManagerMutex.unlock();

}

bool LockManager::canAcquireLock(const ClusterShardId & shardId, const LockLevel & lockLevel){
	lockManagerMutex.lock();
	LockBatch * lockBatch = LockBatch::generateLockBatch(shardId, lockLevel);
	bool result = canAcquireAllBatch(lockBatch);
	delete lockBatch;
	lockManagerMutex.unlock();
	return result;
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
	Logger::sharding(Logger::Step, "LockManager| resolving LOCK request : %s", lockBatch->toString().c_str());
	if(lockBatch->release){
		ASSERT(false);
		finalize(lockBatch, false);
		delete lockBatch;
	}

	// specific to node arrival, nodes must join in ascending nodeId order
	if(lockBatch->batchType == LockRequestType_Metadata && lockBatch->olderNodes.size() > 0){
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
	} // xLock goes out of scope

	// first check if it's blocking or not
	if(! lockBatch->blocking){
		// if all the batch cannot succeed right now, we send reject ack back
		if(! canAcquireAllBatch(lockBatch)){
			// reject the request right here
			Logger::sharding(Logger::Step, "LockManager| REJECTED. ");
			finalize(lockBatch, false);
			delete lockBatch;
			return;
		}
	}

	if(moveLockBatchForward(lockBatch)){
		// batch was done completely, we can finish this request
		Logger::sharding(Logger::Step, "LockManager| GRANTED");
		delete lockBatch;
		return;
	}else{
		// if we must wait for rv-release
		if(lockBatch->isReadviewPending()){
			// just put it in rv release pending lock batches
			Logger::sharding(Logger::Detail, "LockManager| lock request pending for RV release.");
			setPendingForRVRelease(lockBatch);
			return;
		}else{
			Logger::sharding(Logger::Detail, "LockManager| lock request gone to waiting list ...");
			pendingLockBatches.push_back(lockBatch);
			return;
		}
	}
}


void LockManager::resolveRelease(LockBatch * lockBatch){
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
					bool relHap = primaryKeyLocks.release(lockBatch->pkTokens.at(lockBatch->lastGrantedItemIndex).first , lockBatch->opIds.at(i));
					releaseHappened = releaseHappened || relHap;
				}
			}
			break;
		}
	}

	if(releaseHappened){
		if(lockBatch->batchType != LockRequestType_PrimaryKey){
			ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
		}
		Logger::sharding(Logger::Detail, "LockManager| release request triggers some pending lock requests.");
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
			Logger::sharding(Logger::Detail, "LockManager| GRANTED : %s", ((*lockBatchItr)->getLockHoldersStr() + "/" + (*lockBatchItr)->getBatchTypeStr()).c_str());
			delete *lockBatchItr;
			lockBatchItr = pendingLockBatches.erase(lockBatchItr);
		}else{
			if((*lockBatchItr)->isReadviewPending()){
				// erase it from pendingLockRequests and put it in readview release
				LockBatch * lockBatch = *lockBatchItr;
				Logger::sharding(Logger::Detail, "LockManager| %s gone to RV release pending list. ",
						((*lockBatchItr)->getLockHoldersStr() + "/" + (*lockBatchItr)->getBatchTypeStr()).c_str());
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
					Logger::sharding(Logger::Detail, "LockManager| failed to move forward because metadata is locked.");
					return false;
				}

				pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lockBatch->lastGrantedItemIndex + 1);
				if(clusterShardLocks.lock(nxtToken.first, lockBatch->opIds, nxtToken.second)){
					// one more token was granted.
					lockBatch->lastGrantedItemIndex++;
					if(lockBatch->lastGrantedItemIndex == lockBatch->tokens.size() - 1){

						boost::unique_lock<boost::shared_mutex> xLock;
						lockBatch->versionId = ShardManager::getWriteview_write(xLock)->versionId;
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
				ASSERT(lockBatch->lastGrantedItemIndex < (int)(lockBatch->pkTokens.size()));
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
					if(lockBatch->incremental){
						// let's just wait until we can move forward
						if(lastGrantedPreValue < lockBatch->lastGrantedItemIndex){
							// we can send an update at this point
							finalize(lockBatch, true);
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

void LockManager::print(){
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


void LockManager::printPendingRequests(const vector<LockBatch *> & pendingLockBatches, const string & tableName) const{
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

void LockManager::printRVPendingRequests(){
	readviewReleaseMutex.lock();
	printPendingRequests(rvReleasePendingLockBatches, "RV Release %pending Requests");
	readviewReleaseMutex.unlock();
}

void LockManager::printClusterShardIdLocks(){
	clusterShardLocks.print("ClusterShard Locks");
}

void LockManager::printPrimaryKeyLocks(){
	primaryKeyLocks.print("Primary Key Locks");
}

void LockManager::printMetadataLocks(){
	allNodeSharedInfo.print("Metadata locks");
}

}
}
