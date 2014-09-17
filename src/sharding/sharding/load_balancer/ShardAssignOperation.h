#ifndef __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"
#include "../metadata_manager/Shard.h"
#include "../SerialLockOperation.h"
#include "../CommitOperation.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardAssignOperation: public OperationState {
public:

	ShardAssignOperation(const unsigned operationId,
			const ClusterShardId & unassignedShard);
	~ShardAssignOperation();

	OperationState * entry();

	OperationState * handle(LockingNotification::ACK * ack);
	OperationState * handle(CommitNotification::ACK * ack);
	OperationState * handle(NodeFailureNotification * nodeFailure);

	OperationState * handle(Notification * notification);

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	const ClusterShardId shardId;

	OperationState * lockOperation;
	SerialLockResultStatus * lockOperationResult;
	OperationState * commitOperation;
	OperationState * releaseOperation;

	OperationState * acquireLocks();
	OperationState * commit();
	OperationState * release();

};

}
}

#endif // __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
