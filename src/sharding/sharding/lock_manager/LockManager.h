#ifndef __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__
#define __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__

#include "LockRepository.h"
#include "../State.h"
#include "../notifications/Notification.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../../configuration/ShardingConstants.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class LockManagerExternalInterface;
class LockManager {
	friend class LockManagerExternalInterface;
public:

	LockManager();



	void addClusterShardResource(const ClusterShardId & shardId);


	/*
	 * Priorities :
	 * 1. MetadataInfo
	 * 2. ClusterShardIds and NodeShardIds
	 *
	 * 1. PrimaryKeys
	 */

	/*
	 * returns true if lockRequest can be granted immediately and false otherwise
	 */
	bool lockClusterShardId(const ClusterShardId & shardId, LockRequest * lockRequest, bool blocking = false);

	/*
	 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
	 */
	LockRequest * releaseClusterShardId(const ClusterShardId & shardId, const NodeOperationId & releasedOp);

	/*
	 * returns true if lockRequest can be granted immediately and false otherwise
	 */
	bool lockNodeShardId(const NodeShardId & shardId, LockRequest * lockRequest, bool blocking = false);

	/*
	 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
	 */
	LockRequest * releaseNodeShardId(const NodeShardId & shardId, const NodeOperationId & releasedOp);

	/*
	 * returns true if lockRequest can be granted immediately and false otherwise
	 */
	bool lockMetadataInfo(LockRequest * lockRequest, const vector<NodeId> priorIds, bool blocking = true);

	/*
	 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
	 */
	LockRequest * releaseMetadataInfo(const NodeOperationId & releaseOp);

	/*
	 * returns true if lockRequest can be granted immediately and false otherwise
	 */
	bool lockPrimaryKey(const string & primaryKey, LockRequest * lockRequest, bool blocking = true);

	/*
	 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
	 */
	LockRequest * releasePrimaryKey(const string & primaryKey, const NodeOperationId & releasedOp);


	bool isLocked(const ClusterShardId& id);
	bool isLocked(const NodeShardId& id);
	bool isLocked(const string& pk);

private:
	/*
	 * 1. locks for all shards
	 * 2. lock for metadata write version (short term lock)
	 * 3. some data structure for locked primary_keys
	 */

	SingleResourceLocks * metadataInfoWriteLock;

	map<ClusterShardId , SingleResourceLocks * > clusterShardLocks;

	map<NodeShardId, SingleResourceLocks * > nodeShardLocks;

	map<string, SingleResourceLocks *> primaryKeyLocks;
};


class LockManagerExternalInterface{
public:
	LockManagerExternalInterface(){
		lockManager = new LockManager();
	}
	~LockManagerExternalInterface(){
		delete lockManager;
	}
	void resolve(LockingNotification * lockNotif){
		if(lockNotif == NULL){
			return;
		}
		switch (lockNotif->getType()) {
			case LockingNotification::LockRequestType_Copy:
				resolveCopyOperationLockRequest(lockNotif);
				return;
			case LockingNotification::LockRequestType_Move:
				resolveMoveOperationLockRequest(lockNotif);
				return;
			case LockingNotification::LockRequestType_Metadata:
				resolveMetadataLockRequest(lockNotif);
				return;
			case LockingNotification::LockRequestType_PrimaryKey:
				resolvePrimaryKeyLockRequest(lockNotif);
				return;
			case LockingNotification::LockRequestType_GeneralPurpose:
				resolveGeneralLockRequest(lockNotif);
				return;
		}
	}


	void resolveReadviewRelease(const unsigned vid){
		readviewReleaseMutex.lock();
		for(int i = 0 ; i < rvWaitingLockRequests.size(); ++i){
			if(rvWaitingLockRequests.at(i).minVid <= vid){
				sendAck((LockingNotification::ACK *)rvWaitingLockRequests.at(i).request->requestAck);
				delete rvWaitingLockRequests.at(i).request;
				rvWaitingLockRequests.erase(rvWaitingLockRequests.begin()+i);
				i--;
			}
		}
		readviewReleaseMutex.unlock();
	}
private:


	void resolveCopyOperationLockRequest(LockingNotification * lockNotif){
		ClusterShardId srcShardId;
		ClusterShardId destShardId;
		NodeOperationId copyAgent;
		lockNotif->getLockRequestInfo(srcShardId, destShardId, copyAgent);
		LockingNotification::ACK * ack = new LockingNotification::ACK();
		ack->setSrc(lockNotif->getDest());
		ack->setDest(lockNotif->getSrc());

		if(lockNotif->isReleaseRequest()){
			bool commitMetadata = false;
			releaseClusterShardId(srcShardId, copyAgent, commitMetadata, ack);
			releaseClusterShardId(destShardId, copyAgent, commitMetadata, ack, 1);
			if(commitMetadata){
				ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
			}
			sendAck(ack);
			return;
		}
		ASSERT(lockNotif->isBlocking() == false);

		// 1. first try to lock srcShardID
		bool srcShardPreOpLockState = lockManager->isLocked(srcShardId);
		LockRequest * lockRequestSrcShardId = new LockRequest(copyAgent, LockLevel_S, ack);
		if(! lockManager->lockClusterShardId(srcShardId, lockRequestSrcShardId, lockNotif->isBlocking)){
			// send the ack
			ack->setGranted(false);
			sendAck(ack);
			return;
		}
		// 2. then try to lock the destShardId
		bool destShardPreOpLockState = lockManager->isLocked(destShardId);
		LockRequest * lockRequestDestShardId = new LockRequest(copyAgent, LockLevel_X, ack);
		if(! lockManager->lockClusterShardId(srcShardId, lockRequestSrcShardId, lockNotif->isBlocking)){
			// send the ack
			ack->setGranted(false);
			sendAck(ack);
			return;
		}


		if(( srcShardPreOpLockState == false && lockManager->isLocked(srcShardId) )
				|| (destShardPreOpLockState == false && lockManager->isLocked(destShardId))){
			// we need to commit
			applyOnMetadata(lockRequest);
		}

		// send positive ack
		ack->setGranted(true);
		sendAck(ack);
		return;
	}


	void resolveMoveOperationLockRequest(LockingNotification * lockNotif){
		ClusterShardId shardId;
		NodeOperationId srcMoveAgent;
		NodeOperationId destMoveAgent;
		lockNotif->getLockRequestInfo(shardId, srcMoveAgent, destMoveAgent);
		LockingNotification::ACK * ack = new LockingNotification::ACK();
		ack->setSrc(lockNotif->getDest());
		ack->setDest(lockNotif->getSrc());
		if(lockNotif->isReleaseRequest()){
			bool commitMetadata = false;
			releaseClusterShardId(shardId, srcMoveAgent, commitMetadata, ack);
			releaseClusterShardId(shardId, destMoveAgent, commitMetadata, ack, 1);
			if(commitMetadata){
				ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
			}
			sendAck(ack);
			return;
		}
		ASSERT(lockNotif->isBlocking() == false);
		LockRequest * lockRequest = new LockRequest(srcMoveAgent, destMoveAgent, LockLevel_X, ack);


		bool shardPreOpLockState = lockManager->isLocked(shardId);
		if(! lockManager->lockClusterShardId(shardId, lockRequest, lockNotif->isBlocking())){
			// send the ack
			ack->setGranted(false);
			sendAck(ack);
			return;
		}

		if(! shardPreOpLockState){
			if(lockManager->isLocked(shardId)){
				// we need to commit
				applyOnMetadata(lockRequest);
				return;
			}
		}

		// send positive ack
		ack->setGranted(true);
		sendAck(ack);
		return;
	}


	void resolveMetadataLockRequest(LockingNotification * lockNotif){
		NodeOperationId newNodeOpId;
		LockLevel lockLevel;
		vector<NodeId> listOfOlderNodes;
		lockNotif->getLockRequestInfo(newNodeOpId, listOfOlderNodes, lockLevel);
		LockingNotification::ACK * ack = new LockingNotification::ACK();
		ack->setSrc(lockNotif->getDest());
		ack->setDest(lockNotif->getSrc());
		if(lockNotif->isReleaseRequest()){
			releaseMetadata(newNodeOpId, ack);
			sendAck(ack);
			return;
		}
		ASSERT(lockNotif->isBlocking() == true);
		LockRequest * lockRequest = new LockRequest(newNodeOpId.nodeId);
		if(! lockManager->lockMetadataInfo(lockRequest, listOfOlderNodes, lockNotif->isBlocking())){
			if(lockNotif->isBlocking()){
				// we will send the ack later.
				return;
			}
		}else{
			ack->setGranted(true);
		}
		sendAck(ack);
		return;
	}


	void resolvePrimaryKeyLockRequest(LockingNotification * lockNotif){
		vector<string> primaryKeys;
		NodeOperationId writerAgent;
		lockNotif->getLockRequestInfo(primaryKeys, writerAgent);
		LockingNotification::ACK * ack = new LockingNotification::ACK();
		ack->setSrc(lockNotif->getDest());
		ack->setDest(lockNotif->getSrc());
		if(primaryKeys.size() == 0){
			ASSERT(false);
			sendAck(ack);
			return;
		}
		if(lockNotif->isReleaseRequest()){
			for(unsigned pkIndex = 0 ; pkIndex < primaryKeys.size(); ++pkIndex){
				releasePrimaryKey(primaryKeys.at(pkIndex), writerAgent, ack, pkIndex);
			}
			sendAck(ack);
			return;
		}

		ASSERT(lockNotif->isBlocking() == false);

		for(unsigned pkIndex = 0 ; pkIndex < primaryKeys.size(); ++pkIndex){
			LockRequest * lockRequest = new LockRequest(writerAgent, LockLevel_X, ack);
			if(! lockManager->lockPrimaryKey(primaryKeys.at(pkIndex), lockRequest)){
				ack->setGranted(false, pkIndex);
			}else{
				ack->setGranted(true, pkIndex);
			}
		}
		sendAck(ack);
		return;

	}

	void resolveGeneralLockRequest(LockingNotification * lockNotif){
		ClusterShardId generalPurposeShardId;
		NodeOperationId generalPurposeAgent;
		LockLevel lockLevel;
		lockNotif->getLockRequestInfo(generalPurposeShardId, generalPurposeAgent, lockLevel);
		LockingNotification::ACK * ack = new LockingNotification::ACK();
		ack->setSrc(lockNotif->getDest());
		ack->setDest(lockNotif->getSrc());
		if(lockNotif->isReleaseRequest()){
			bool commitMetadata = false;
			releaseClusterShardId(generalPurposeShardId, generalPurposeAgent, commitMetadata, ack);
			if(commitMetadata){
				ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
			}
			sendAck(ack);
			return;
		}
		ASSERT(lockNotif->isBlocking() == false);
		LockRequest * lockRequest = new LockRequest(generalPurposeAgent, lockLevel, ack);
		if(! lockManager->lockClusterShardId(generalPurposeShardId, lockRequest, lockNotif->isBlocking())){
			ack->setGranted(false);
		}else{
			ack->setGranted(true);
		}

		sendAck(ack);
		return;
	}


	void releaseClusterShardId(const ClusterShardId & srcShardId,
			const NodeOperationId & copyAgent,
			bool & commitRequired, LockingNotification::ACK * ack , bool resultIndex = 0){
		if(lockManager->isLocked(srcShardId)){ // because if it's not locked, we should just send ack
			LockRequest * grantedReq = lockManager->releaseClusterShardId(srcShardId, copyAgent);
			if(grantedReq != NULL){
				// pending requests can never chan).ge the RV state
				// send the ack for this new granted req
				if(grantedReq->requestAck != NULL){
					sendAck((LockingNotification::ACK *)grantedReq->requestAck);
				}
			}else{
				if(! lockManager->isLocked(srcShardId)){
					// we must commit the metadata
					commitRequired = true;
				}
			}

		}
		ack->setGranted(true, resultIndex);
	}

	void releaseMetadata(const NodeOperationId & agent, LockingNotification::ACK * ack , bool resultIndex = 0){
		if(lockManager->metadataInfoWriteLock->isLocked()){ // because if it's not locked, we should just send ack
			LockRequest * grantedReq = lockManager->releaseMetadataInfo(agent);
			if(grantedReq != NULL){
				// pending requests can never change the RV state
				// send the ack for this new granted req
				if(grantedReq->requestAck != NULL){
					sendAck((LockingNotification::ACK *)grantedReq->requestAck);
				}
			}
		}
		ack->setGranted(true, resultIndex);
	}


	void releasePrimaryKey(const string & primaryKey, const NodeOperationId & writer,
			LockingNotification::ACK * ack , bool resultIndex = 0){
		if(lockManager->isLocked(primaryKey)){ // because if it's not locked, we should just send ack
			LockRequest * grantedReq = lockManager->releasePrimaryKey(primaryKey, writer);
			if(grantedReq != NULL){
				// pending requests can never change the RV state
				// send the ack for this new granted req
				if(grantedReq->requestAck != NULL){
					sendAck((LockingNotification::ACK *)grantedReq->requestAck);
				}
			}
		}
		ack->setGranted(true, resultIndex);
	}

	void sendAck(LockingNotification::ACK * ack){
		if(ack->getDest().nodeId == ShardManager::getCurrentNodeId()){
			ShardManager::getShardManager()->resolveLocal(ack);
		}else{
			ShardManager::getShardManager()->send(ack);
			delete ack;
		}
	}

	/*
	 * LockRequest * grantedReq
	 */


	void applyOnMetadata(LockRequest * lockRequest){
		ReadViewWaitingLockRequest pendingLockRequest;
		pendingLockRequest.minVid = ShardManager::getWriteview()->versionId;
		pendingLockRequest.request = lockRequest;
		readviewReleaseMutex.lock();
		rvWaitingLockRequests.push_back(pendingLockRequest);
		readviewReleaseMutex.unlock();
		// commit
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
	}


	LockManager * lockManager;

	struct ReadViewWaitingLockRequest{
		LockRequest * request;
		unsigned minVid;
	};
	vector<ReadViewWaitingLockRequest> rvWaitingLockRequests;
    boost::mutex readviewReleaseMutex;
};


}
}
#endif // __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__
