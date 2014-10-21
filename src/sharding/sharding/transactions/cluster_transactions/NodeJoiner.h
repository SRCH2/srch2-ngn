#ifndef __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
#define __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/NewNodeLockNotification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"
#include "../notifications/MetadataReport.h"
#include <stdlib.h>
#include <time.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * This class (which is kind of state for a state machine)
 * joins a new node to the existing cluster. The protocol starts
 * with entry() and continues with a sequence of handle() functions.
 * When this operation finishes, node is in "joined" state (and for example
 * as a result, wouldn't bounce any notifications.
 */
/*
 * NOTE : The mutex of metadata manager and lock manager is locked/unlocked in handle functions of this class.
 */


class NodeJoiner : public Transaction, public ConsumerInterface, public NodeIteratorListenerInterface{
public:

	static void run();


	~NodeJoiner();

private:

	NodeJoiner();

	ShardingTransactionType getTransactionType();
	void join();


	void lock();

	// coming back from lock
	void consume(bool booleanResult);

	void readMetadata();

	// coming back from readMetadata
	void end_(map<NodeOperationId, ShardingNotification * > & replies);
	// if returns true, operation must stop and return null to state_machine
	bool shouldAbort(const NodeId & failedNode);

	void commit();

	void release();

	void finalize(bool result = true);

	void getOlderNodesList(vector<NodeId> & olderNodes);



private:
	NodeOperationId selfOperationId;
	NodeId randomNodeToReadFrom;

	string currentOperation;
	bool finalizedFlag ;
	bool releaseModeFlag;
	AtomicLock * locker;
	AtomicRelease * releaser;
	MetadataReport::REQUEST * readMetadataNotif;
	CommitNotification * commitNotification;
	MetadataChange * metadataChange;
	AtomicMetadataCommit * committer;

};

}
}

#endif // __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
