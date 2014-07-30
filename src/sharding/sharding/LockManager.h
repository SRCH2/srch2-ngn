#ifndef __SHARDING_SHARDING_LOCK_MANAGER_H__
#define __SHARDING_SHARDING_LOCK_MANAGER_H__

#include "State.h"
#include "Notification.h"


#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class GlobalLockOperation : public OperationState{
public:
	GlobalLockOperation(unsigned operationId, const LockChange * request);
	~GlobalLockOperation();
	OperationStateType getType();
	// returns false if it's done and called should delete this operation
	void entry(map<NodeId, unsigned> specialTargets);
	void handle(LockingNotification::GRANTED * inputNotification);
	void handle(LockingNotification::REJECTED * inputNotification);
	void handle(LockingNotification::RELEASED * inputNotification);
	void handle(LockingNotification::RV_RELEASED * inputNotification);
	bool doesExpect(LockingNotification::GRANTED * inputNotification) const;
	bool doesExpect(LockingNotification::REJECTED * inputNotification) const;
	bool doesExpect(LockingNotification::RELEASED * inputNotification) const;
	bool doesExpect(LockingNotification::RV_RELEASED * inputNotification) const;
	// returns true when it's done and the called can delete this operation
	// success will contain the status of this lock request
	bool isDone(bool & success) const;
	// TODO adjustToNodeFailure ???
private:
	LockChange * request;
	map<NodeId, bool> haveReplied;
	bool waitForReadviewReleaseFlag;
	unsigned versionToBeReleased;
	bool success;
	bool needToFlush;
	bool hasAllNodesReplied() const;
};

class LockManager{
public:

	static LockManager * createLockManager();
	static LockManager * getLockManager();




	/*
	* the first method called when a notification comes for LockManager,
	* this method uses other functions and the buffer to take care of this notification.
	*/

	void resolve(LockingNotification * notification);
	void resolve(LockingNotification::RV_RELEASED * inputNotification);
	// Local lock request coming from external node
	bool localLock(LockChange * lockRequest, bool & waitForReadViewRelease,  unsigned & metadataVersionToBeReleased);

private:

	static LockManager * singleInstance;
	// ClusterResourceLocks
	ClusterResourceLocks clusterLock;

	// list of (metadataVersionId, operation waiting for ack)
	vector<std::pair<unsigned, NodeOperationId> > pendingLockNotifications;
};

}
}

#endif // __SHARDING_SHARDING_LOCK_MANAGER_H__
