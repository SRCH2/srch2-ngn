#ifndef __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__
#define __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__

#include "./LockBatch.h"
#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../../configuration/ShardingConstants.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class LockManager{
public:
	LockManager();


	void resolve(SP(LockingNotification)  notif);

	void resolve(const unsigned readviewReleasedVersion);

	void resolveNodeFailure(const NodeId & failedNode);

	bool canAcquireLock(const ClusterShardId & shardId, const LockLevel & lockLevel);

	void resolveLock(LockBatch * lockBatch);
	bool resolveRelease(LockBatch * lockBatch);
	bool isPartitionLocked(const ClusterPID & pid);
	void getLockedPartitions(vector<ClusterPID> & lockedPartitions, bool shouldLock = true);
	void setNodePassedInitialization(const NodeId & nodeId);
	void initialize();
	void print(JsonResponseHandler * response = NULL);
private:


	void printPendingRequests(const vector<LockBatch *> & pendingLockBatches, const string & tableName, JsonResponseHandler * response = NULL) const;

	void printRVPendingRequests(JsonResponseHandler * response = NULL);

	void printClusterShardIdLocks(JsonResponseHandler * response = NULL);
	void printPrimaryKeyLocks(JsonResponseHandler * response = NULL);
	void printMetadataLocks(JsonResponseHandler * response = NULL);


	bool canAcquireAllBatch(LockBatch * lockBatch);


	void movePendingLockBatchesForward(unsigned pendingLockBatchIdx = 0);
	/*
	 * returns true only of lockBatch is finished and must be removed.
	 * This method moves a pending lock batch forward as much as possible
	 */
	bool moveLockBatchForward(LockBatch * lockBatch);

	void finalize(LockBatch * lockBatch);

	void setPendingForRVRelease(LockBatch * lockBatch);
	bool isNodePassedInitialization(const NodeId & nodeId);

	bool isLockManagerClean() const{
		bool result = true;
		result = result && pendingLockBatches.empty();
		result = result && rvReleasePendingLockBatches.empty();
		result = result && clusterShardLocks.isClean();
		result = result && primaryKeyLocks.isClean();
		result = result && allNodeSharedInfo.isClean();
		return result;
	}


	vector<LockBatch *> pendingLockBatches;
	boost::recursive_mutex lockManagerMutex;

	vector<LockBatch *> rvReleasePendingLockBatches;
	boost::mutex readviewReleaseMutex;

	ItemLockHolder<ClusterShardId> clusterShardLocks;
	ItemLockHolder<string> primaryKeyLocks;

	ItemLockHolder<string> allNodeSharedInfo;
	map<NodeId, bool> passedInitialization;

	static const string metadataResourceName;
};

}
}
#endif // __SHARDING_LOCK_MANAGER_LOCK_MANAGER_H__
