#ifndef __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
#define __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommitNotification.h"
#include "../../notifications/LockingNotification.h"
#include "../../notifications/MetadataReport.h"
#include "../Transaction.h"
#include "../TransactionSession.h"
#include <stdlib.h>
#include <time.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class AtomicLock;
class AtomicRelease;
class AtomicMetadataCommit;

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


class NodeJoiner : public Transaction, public NodeIteratorListenerInterface{
public:

	static void join();


	~NodeJoiner();

private:

	NodeJoiner();

	Transaction * getTransaction() ;
	void initSession();


	ShardingTransactionType getTransactionType();
	bool run();

	void lock();

	// coming back from lock
	void consume(bool booleanResult);

	void readMetadata();

	// coming back from readMetadata
	void end_(map<NodeOperationId, SP(ShardingNotification) > & replies);
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
	SP(MetadataReport::REQUEST) readMetadataNotif;
	SP(CommitNotification) commitNotification;
	MetadataChange * metadataChange;
	AtomicMetadataCommit * committer;

};

}
}

#endif // __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
