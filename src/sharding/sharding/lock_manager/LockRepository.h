#ifndef __SHARDING_LOCK_MANAGER_LOCK_REPO_H__
#define __SHARDING_LOCK_MANAGER_LOCK_REPO_H__

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

struct LockRequest{
	LockRequest(NodeOperationId lockHolder, LockLevel lockType,
			ShardingNotification * requestAck);
	LockRequest(NodeOperationId lockHolder1, NodeOperationId lockHolder2,
			LockLevel lockType, ShardingNotification * requestAck);

	// used for reservation
	LockRequest(const unsigned id);
	void fill(LockRequest * lockRequest);

	~LockRequest();

	////////////////// Members //////////////////////
	const unsigned id;
	// The address of the requester of this lock request is
	// obtainable through requestAck->getDest();
	ShardingNotification * requestAck; // is not deleted in lock request
	const LockLevel lockType;
	vector<NodeOperationId> lockHolders;
	bool arrived;
	bool granted;


	/*
	 * Returns true if lockHolders list becomes empty,
	 * which means lock request must be removed (it's completely released)
	 */
	bool eraseLockHolders(const NodeOperationId & op);
	/*
	 * Returns true if lockHolders list becomes empty,
	 * which means lock request must be removed (it's completely released)
	 */
	bool eraseLockHolders(const NodeId & node);
};


/*
 * Implements 2 LockLevels, S and X, and a FIFO waiting list for each type.
 *
 */
class WaitingListHandler {
public:
	bool isEmpty();

	void push(LockRequest * lockRequest, const vector<unsigned> priorIds = vector<unsigned>());

	// returns NULL if no lockRequest is waiting in any queue
	// S lock requests are prior to X lock requests
	// priority list is prior to both S and X requesters
	LockRequest * pop();
	LockRequest * top();
private:
	std::vector<LockRequest *> priorityWaitingList;
	std::queue<LockRequest *> SwaitingList;
	std::queue<LockRequest *> XwaitingList;

	LockRequest * insertPriorityLockRequest(LockRequest * priorLockRequest);
};

class SingleResourceLocks{
public:

	SingleResourceLocks();

	/*
	 * returns false if it's not in locked state
	 * returns true if it's locked, and in this case
	 * lockType is set to either 'S' or 'X'
	 */
	bool isLocked();

	/*
	 * Returns NULL if no pendingRequest is waiting in the WaitingList
	 */
	LockRequest * grantPendingRequest();

	bool hasPendingRequest();

	/*
	 * returns true if lockRequest can be granted immediately and false otherwise
	 */
	bool lock(LockRequest * lockRequest, bool blocking = false, const vector<unsigned> priorIds = vector<unsigned>());

	bool canGrant(LockRequest * lockRequest);
	/*
	 * When an operation is released, even if the resource is unlocked
	 * and there are other requests waiting to be granted, we don't
	 * grant any new requests because and we wait for the container of
	 * this lock manager to ask us to do so.
	 */
	void release(const NodeOperationId & releasedOp);

private:
	std::vector<LockRequest *> currentLockHolder_Queue;

	WaitingListHandler * waitingListHandler;

	void resolveLockConflict(LockRequest * lockRequest, bool blocking, const vector<unsigned> priorIds);

	void grantLock(LockRequest * lockRequest);

};



}
}
#endif // __SHARDING_LOCK_MANAGER_LOCK_REPO_H__
