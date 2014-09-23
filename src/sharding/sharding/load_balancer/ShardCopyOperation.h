#ifndef __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/CopyToMeNotification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../SerialLockOperation.h"
#include "../CommitOperation.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardCopyOperation : public OperationState{
public:

	ShardCopyOperation(const unsigned operationId, const ClusterShardId & unassignedShard,
			NodeId srcNodeId, const ClusterShardId & shardToReplicate);

	OperationState * entry();


	OperationState * handle(LockingNotification::ACK * ack);
	OperationState * handle(MMNotification * mmStatus);
	OperationState * handle(CommitNotification::ACK * ack);
	OperationState * handle(NodeFailureNotification * nodeFailure);

	OperationState * handle(Notification * notification);


	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	const ClusterShardId unassignedShardId;
	const ClusterShardId replicaShardId;
	const NodeId srcNodeId;

	OperationState * lockOperation;
	SerialLockResultStatus * lockOperationResult;
	OperationState * commitOperation;
	OperationState * releaseOperation;
	LocalPhysicalShard physicalShard;

	OperationState * acquireLocks();
	OperationState * transfer();
	OperationState * commit();
	OperationState * release();
};


}
}

#endif // __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
