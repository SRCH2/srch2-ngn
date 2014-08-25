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
class NewNodeJoinOperation : public OperationState{
public:

	NewNodeJoinOperation();
	// when we reach to this function, we the metadata is locked and we can
	// read and update the information as we want.
	OperationState * entry();

	OperationState * handle(Notification * notification);
	OperationState * handle(NodeFailureNotification * nodeFailure);
	OperationState * handle(NewNodeLockNotification::ACK * ack);
	OperationState * handle(MetadataReport * report);
	OperationState * handle(CommitNotification::ACK * ack);
	OperationState * handle(LockingNotification::ACK * ack);

private:


	OperationState * acquireLocks();
	OperationState * readMetadata();
	OperationState * commit();
	OperationState * releaseLocks();
	OperationState * finalizeJoin();


	OperationState * lockerOperation;
	OperationState * commitOperation;
	OperationState * releaseOperation;
	NodeId randomNodeToReadFrom;


};

}
}

#endif // __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
