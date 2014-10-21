#include "core/util/Logger.h"
#include "LockManager.h"


using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

LockManager::LockManager(){
	this->metadataInfoWriteLock = new SingleResourceLocks();
}

void LockManager::addClusterShardResource(const ClusterShardId & shardId){
	if(clusterShardLocks.find(shardId) != clusterShardLocks.end()){
		return;
	}
	clusterShardLocks[shardId] = new SingleResourceLocks();
}


/*
 * Priorities :
 * 1. MetadataInfo
 * 2. ClusterShardIds and NodeShardIds
 *
 * 1. PrimaryKeys
 */

/*
 * if blocking is true, it always returns true
 * else, returns true if lockRequest can be granted immediately
 */
bool LockManager::lockClusterShardId(const ClusterShardId & shardId, LockRequest * lockRequest, bool blocking = false){
	if(clusterShardLocks.find(shardId) == clusterShardLocks.end()){
		ASSERT(false);
		return false;
	}
	return clusterShardLocks[shardId]->lock(lockRequest, blocking);
}

/*
 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
 */
LockRequest * LockManager::releaseClusterShardId(const ClusterShardId & shardId, const NodeOperationId & releasedOp){
	if(clusterShardLocks.find(shardId) == clusterShardLocks.end()){
		ASSERT(false);
		return NULL;
	}
	clusterShardLocks[shardId]->release(releasedOp);

	// check for pending requests
	return clusterShardLocks[shardId]->grantPendingRequest();
}

bool LockManager::lockNodeShardId(const NodeShardId & shardId, LockRequest * lockRequest, bool blocking = false){
	if(nodeShardLocks.find(shardId) == nodeShardLocks.end()){
		nodeShardLocks[shardId] = new SingleResourceLocks();
	}
	return nodeShardLocks[shardId]->lock(lockRequest, blocking);
}

/*
 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
 */
LockRequest * LockManager::releaseNodeShardId(const NodeShardId & shardId, const NodeOperationId & releasedOp){
	if(nodeShardLocks.find(shardId) == nodeShardLocks.end()){
		return NULL;
	}
	nodeShardLocks[shardId]->release(releasedOp);
	if(! (nodeShardLocks[shardId]->isLocked() || nodeShardLocks[shardId]->hasPendingRequest())){
		delete nodeShardLocks[shardId];
		nodeShardLocks.erase(shardId);
		return NULL;
	}else{
		return nodeShardLocks[shardId]->grantPendingRequest();
	}
}

bool LockManager::lockMetadataInfo(LockRequest * lockRequest, const vector<NodeId> priorIds, bool blocking = true){
	return metadataInfoWriteLock->lock(lockRequest, blocking, priorIds);
}

/*
 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
 */
LockRequest * LockManager::releaseMetadataInfo(const NodeOperationId & releaseOp){
	metadataInfoWriteLock->release(releaseOp);
	return metadataInfoWriteLock->grantPendingRequest();
}

/*
 *	If blocking is true, it returns true
 *	else, it returns true only if (primaryKey,requester) lock pair can be saved.
 *
 *	Note : lockRequest is filled the LockRequest if it returns true;
 */
bool LockManager::lockPrimaryKey(const string & primaryKey,
		LockRequest * lockRequest,
		bool blocking = true){
	if(primaryKeyLocks.find(primaryKey) == primaryKeyLocks.end()){
		primaryKeyLocks[primaryKey] = new SingleResourceLocks();
	}
	return primaryKeyLocks[primaryKey]->lock(lockRequest, blocking);

}
/*
 * If it doesn't return NULL, it returns the LockRequest which was granted as a result of this release
 */
LockRequest * LockManager::releasePrimaryKey(const string & primaryKey, const NodeOperationId & releasedOp){
	if(primaryKeyLocks.find(primaryKey) == primaryKeyLocks.end()){
		return NULL;
	}
	primaryKeyLocks[primaryKey]->release(releasedOp);
	if(! ( primaryKeyLocks[primaryKey]->isLocked() ||
			primaryKeyLocks[primaryKey]->hasPendingRequest() ) ){
		delete primaryKeyLocks[primaryKey];
		primaryKeyLocks.erase(primaryKey);
		return NULL;
	}else{
		return primaryKeyLocks[primaryKey]->grantPendingRequest();
	}
}


bool LockManager::isLocked(const ClusterShardId& id){
	if(clusterShardLocks.find(id) == clusterShardLocks.end()){
		return false;
	}
	return clusterShardLocks[id]->isLocked();
}
bool LockManager::isLocked(const NodeShardId& id){
	if(nodeShardLocks.find(id) == nodeShardLocks.end()){
		return false;
	}
	return nodeShardLocks[id]->isLocked();
}
bool LockManager::isLocked(const string& pk){
	if(primaryKeyLocks.find(pk) == primaryKeyLocks.end()){
		return false;
	}
	return primaryKeyLocks[pk]->isLocked();
}
}
}
