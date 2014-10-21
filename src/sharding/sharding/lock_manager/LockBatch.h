#ifndef __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__
#define __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__

#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../../configuration/ShardingConstants.h"
#include "./ItemLockHolder.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

template <class Resource>
class ItemLockHolder{
public:
	bool lock(const Resource & resource, const vector<NodeOperationId> & opids , const LockLevel & lockLevel){
		if(opids.empty()){
			return false;
		}
		if(grantedLocks.find(resource) == grantedLocks.end()){
			grantedLocks[resource] = vector<pair<NodeOperationId, LockLevel> >();
		}
		if(grantedLocks[resource].size() == 0){
			for(unsigned i = 1; i < opids.size(); ++i){
				grantedLocks[resource].push_back(std::make_pair(opids.at(i), lockLevel));
			}
			return true;
		}
		if(! conflict(grantedLocks[resource].at(0).second, lockLevel) ){
			for(unsigned i = 1; i < opids.size(); ++i){
				grantedLocks[resource].push_back(std::make_pair(opids.at(i), lockLevel));
			}
			return true;
		}
		return false;
	}

	bool canLock(const Resource & resource, const LockLevel & lockLevel){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			grantedLocks[resource] = vector<pair<NodeOperationId, LockLevel> >();
		}
		if(grantedLocks[resource].size() == 0){
			return true;
		}
		if(! conflict(grantedLocks[resource].at(0).second, lockLevel) ){
			return true;
		}
		return false;
	}

	bool release(const Resource & resource, const NodeOperationId & opid ){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			return false;
		}
		bool releaseHappened = false;
		for(vector<pair<NodeOperationId, LockLevel> >::iterator resItr = grantedLocks[resource].begin();
				resItr != grantedLocks[resource].end(); ++resItr){
			if(resItr->first == opid){
				resItr = grantedLocks[resource].erase(resItr);
				releaseHappened = true;
			}
		}
		if(grantedLocks[resource].size() == 0){
			grantedLocks.erase(resource);
		}
		return releaseHappened;
	}

	void clear(){
		grantedLocks.clear();
	}

	void release(const NodeId & failedNodeId){
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::iterator resItr = grantedLocks.begin();
				resItr != grantedLocks.end(); ++resItr){
			for(vector<pair<NodeOperationId, LockLevel> >::iterator nodeItr = resItr->second.begin();
					nodeItr != resItr->second.end(); ++nodeItr){
				if(nodeItr->first.nodeId == failedNodeId){
					resItr = resItr->second.erase(nodeItr);
				}
			}
			if(resItr->second.size() == 0){
				resItr = grantedLocks.erase(resItr);
			}
		}
	}
	bool isLock(const Resource & resource){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			return false;
		}
		if(grantedLocks[resource].size() == 0){
			return false;
		}
		return true;
	}

private:
	map<Resource, vector<pair<NodeOperationId, LockLevel> > > grantedLocks;
	bool conflict(const LockLevel & level1, const LockLevel & level2){
		if(level1 == LockLevel_X){
			return true;
		}
		if(level2 == LockLevel_X){
			return true;
		}
		return false;
	}
};


class LockBatch{
public:
	LockBatch(){
		this->blocking = false;
		this->release = false;
		this->incremental = false;
		this->granted = false;
		this->lastGrantedItemIndex = -1;
		this->batchType = LockRequestType_ShardIdList;
		this->ack = NULL;
		this->versionId = 0;
	}
	~LockBatch(){
		if(ack != NULL){
			delete ack;
		}
	}
	bool blocking;
	bool release;
	bool incremental;
	bool granted;
	int lastGrantedItemIndex;
	LockRequestType batchType;
	vector<NodeOperationId> opIds;
	LockingNotification::ACK * ack;

	unsigned versionId;


	// for all cluster shard related locks
	vector<pair<ClusterShardId, LockLevel> > tokens;

	// for primary key related locks
	vector<pair<string, LockLevel> > pkTokens;
	ClusterPID pid; // the partition id of these primary keys

	// for metadata locks
	LockLevel metadataLockLevel;
	vector<NodeId> olderNodes;

	bool isReadviewPending() const{
		return (versionId > 0);
	}

	static LockBatch * generateLockBatch(LockingNotification * notif){
		LockBatch * lockBatch = new LockBatch();
		lockBatch->blocking = notif->isBlocking();
		lockBatch->release = notif->isReleaseRequest();
		lockBatch->batchType = notif->getType();
		lockBatch->ack = new LockingNotification::ACK();
		switch (notif->getType()) {
			case LockRequestType_Copy:
			{
				ClusterShardId srcShardId, destShardId;
				NodeOperationId copyAgent;
				notif->getLockRequestInfo(srcShardId, destShardId, copyAgent);
				lockBatch->opIds.push_back(copyAgent);
				if(srcShardId <= destShardId){
					lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_S));
					lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_X));
				}else{
					lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_X));
					lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_S));
				}
				return lockBatch;
			}
			case LockRequestType_Move:
			{
				ClusterShardId shardId;
				NodeOperationId srcMoveAgent, destMoveAgent;
				notif->getLockRequestInfo(shardId, srcMoveAgent, destMoveAgent);
				lockBatch->opIds.push_back(srcMoveAgent);
				lockBatch->opIds.push_back(destMoveAgent);
				lockBatch->tokens.push_back(std::make_pair(shardId, LockLevel_X));
				return lockBatch;
			}
			case LockRequestType_Metadata:
			{
				NodeOperationId newNodeOpId;
				vector<NodeId> listOfOlderNodes;
				LockLevel lockLevel;
				notif->getLockRequestInfo(newNodeOpId, listOfOlderNodes, lockLevel);
				lockBatch->opIds.push_back(newNodeOpId);
				lockBatch->metadataLockLevel = lockLevel;
				lockBatch->olderNodes = listOfOlderNodes;
				return lockBatch;
			}
			case LockRequestType_PrimaryKey:
			{
				vector<string> primaryKeys;
				NodeOperationId writerAgent;
				ClusterPID pid;
				notif->getLockRequestInfo(primaryKeys, writerAgent, pid);
				lockBatch->pid = pid;
				lockBatch->opIds.push_back(writerAgent);
				for(unsigned pkIdx = 0; pkIdx < primaryKeys.size(); ++pkIdx){
					lockBatch->pkTokens.push_back(std::make_pair(primaryKeys.at(pkIdx), LockLevel_X));
				}
				return lockBatch;
			}
			case LockRequestType_GeneralPurpose:
			{
				ClusterShardId shardId;
				NodeOperationId agent;
				LockLevel lockLevel;
				notif->getLockRequestInfo(shardId, agent, lockLevel);
				lockBatch->opIds.push_back(agent);
				lockBatch->tokens.push_back(std::make_pair(shardId, lockLevel));
				return lockBatch;
			}
			case LockRequestType_ShardIdList:
			{
				vector<ClusterShardId> shardIdList;
				NodeOperationId shardIdListLockHolder;
				LockLevel shardIdListLockLevel;
				notif->getLockRequestInfo(shardIdList, shardIdListLockHolder, shardIdListLockLevel);
				lockBatch->opIds.push_back(shardIdListLockHolder);
				for(unsigned i = 0 ; i < shardIdList.size() ; ++i){
					lockBatch->tokens.push_back(std::make_pair(shardIdList.at(i), shardIdListLockLevel));
				}
				return lockBatch;
			}
		}
	}

};


class LockManager{
public:
	LockManager(){

	}


	void resolve(LockingNotification *  notif){
		if(notif == NULL){
			ASSERT(false);
			return;
		}
		LockBatch * lockBatch = LockBatch::generateLockBatch(notif);
		resolve(lockBatch);
	}

	void resolve(const unsigned readviewReleasedVersion){
		// lock
		readviewReleaseMutex.lock();
		for(vector<LockBatch *>::iterator lockBItr = rvReleasePendingLockBatches.begin();
				lockBItr != rvReleasePendingLockBatches.end(); ++lockBItr){
			LockBatch * lockBatch = *lockBItr;
			if(readviewReleasedVersion >= lockBatch->versionId){
				finalize(lockBatch, true);
				delete lockBatch;
				lockBItr = rvReleasePendingLockBatches.erase(lockBItr);
			}
		}
		readviewReleaseMutex.unlock();
	}



private:

	// entry point of LockBatch to LockManager
	void resolve(LockBatch * lockBatch){
		if(lockBatch == NULL){
			ASSERT(false);
			return;
		}
		if(lockBatch->release){
			resolveRelease(lockBatch);
			finalize(lockBatch, true);
			delete lockBatch;
		}
		// first check if it's blocking or not
		if(! lockBatch->blocking){
			// if all the batch cannot succeed right now, we send reject ack back
			if(! canAcquireAllBatch(lockBatch)){
				// reject the request right here
				finalize(lockBatch, false);
				delete lockBatch;
				return;
			}
		}
		if(moveLockBatchForward(lockBatch)){
			// batch was done completely, we can finish this request
			delete lockBatch;
			return;
		}else{
			// if we must wait for rv-release
			if(lockBatch->isReadviewPending()){
				// just put it in rv release pending lock batches
				setPendingForRVRelease(lockBatch);
				return;
			}else{
				pendingLockBatches.push_back(lockBatch);
				return;
			}
		}
	}


	void movePendingLockBatchesForward(){
		for(vector<LockBatch *>::iterator lockBatchItr = pendingLockBatches.begin();
				lockBatchItr != pendingLockBatches.end(); ++lockBatchItr){
			ASSERT(! (*lockBatchItr)->release);
			if(moveLockBatchForward(*lockBatchItr)){
				delete *lockBatchItr;
				lockBatchItr = pendingLockBatches.erase(lockBatchItr);
			}else{
				if((*lockBatchItr)->isReadviewPending()){
					// erase it from pendingLockRequests and put it in readview release
					LockBatch * lockBatch = *lockBatchItr;
					lockBatchItr = pendingLockBatches.erase(lockBatchItr);
					setPendingForRVRelease(lockBatch);
				}
			}
		}
	}


	void setPendingForRVRelease(LockBatch * lockBatch){
		readviewReleaseMutex.lock();
		rvReleasePendingLockBatches.push_back(lockBatch);
		readviewReleaseMutex.unlock();

		// commit
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
	}

	bool canAcquireAllBatch(LockBatch * lockBatch){

		ASSERT(lockBatch->lastGrantedItemIndex == -1);
		unsigned lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

		while(true){
			switch (lockBatch->batchType) {
				case LockRequestType_Copy:
				case LockRequestType_Move:
				case LockRequestType_GeneralPurpose:
				case LockRequestType_ShardIdList:
				{
						pair<ClusterShardId, LockLevel> & nxtToken = lockBatch->tokens.at(lastGrantedPreValue + 1);

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
	bool moveLockBatchForward(LockBatch * lockBatch){

		unsigned lastGrantedPreValue = lockBatch->lastGrantedItemIndex ;

		while(true){
			switch (lockBatch->batchType) {
				case LockRequestType_Copy:
				case LockRequestType_Move:
				case LockRequestType_GeneralPurpose:
				case LockRequestType_ShardIdList:
				{
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
						ASSERT(lockBatch->lastGrantedItemIndex == 0);
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


	void resolveRelease(LockBatch * lockBatch){

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
			movePendingLockBatchesForward();
		}
	}


	void finalize(LockBatch * lockBatch, bool result ){
		lockBatch->granted = result;
		if( lockBatch->ack != NULL){
			lockBatch->ack->setGranted(result);
			lockBatch->ack->setIndexOfLastGrantedItem(lockBatch->lastGrantedItemIndex);
		}
		if(lockBatch->batchType == LockRequestType_Metadata && lockBatch->olderNodes.size() > 0){
			ASSERT(lockBatch->opIds.size() == 1);
			setNodePassed(lockBatch->opIds.at(0).nodeId);
		}
		// send the ack
		ShardManager::getShardManager()->send(lockBatch->ack);
	}

	vector<LockBatch *> pendingLockBatches;
	vector<LockBatch *> rvReleasePendingLockBatches;
	boost::mutex readviewReleaseMutex

	ItemLockHolder<ClusterShardId> clusterShardLocks;
	ItemLockHolder<string> primaryKeyLocks;

	ItemLockHolder<string> allNodeSharedInfo;
	map<NodeId, bool> passedInitialization;
	bool isNodePassedInitialization(const NodeId & nodeId){
		if(passedInitialization.find(nodeId) == passedInitialization.end()){
			passedInitialization[nodeId] = false;
			return false;
		}
		return passedInitialization[nodeId];
	}
	void setNodePassed(const NodeId & nodeId){
		passedInitialization[nodeId] = true;
	}

	static const string metadataResourceName;
};

const string LockManager::metadataResourceName = string("srch2-cluster-metadata");

}
}

#endif // __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__
