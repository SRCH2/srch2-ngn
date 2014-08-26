#ifndef __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/MoveToMeNotification.h"
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


class ShardMoveOperation : public OperationState{
public:

	ShardMoveOperation(const unsigned operationId, const NodeId & srcNodeId, const ClusterShardId & moveShardId);

	OperationState * entry();


	OperationState * handle(MoveToMeNotification::ACK * ack);
	OperationState * handle(MoveToMeNotification::ABORT * ack);
	OperationState * handle(LockingNotification::ACK * ack);
	OperationState * handle(MMNotification * mmStatus);
	OperationState * handle(CommitNotification::ACK * ack);
	OperationState * handle(NodeFailureNotification * nodeFailure);

	OperationState * handle(Notification * notification);
	string getOperationName() const ;
	string getOperationStatus() const ;
private:
	const ClusterShardId shardId;
	NodeOperationId srcAddress;

	OperationState * lockOperation;
	SerialLockResultStatus * lockOperationResult;
	OperationState * commitOperation;
	OperationState * releaseOperation;
	LocalPhysicalShard physicalShard;


	OperationState * connect();
	OperationState * acquireLocks();
	OperationState * transfer();
	OperationState * commit();
	OperationState * release();

	void start();
	OperationState * abort();
	OperationState * finish();
};


class ShardMoveSrcOperation : public OperationState{
public:

	ShardMoveSrcOperation(const NodeOperationId & destination, const ClusterShardId & moveShardId);

	OperationState * entry();

	OperationState * handle(MoveToMeNotification::ABORT * ack);
	OperationState * handle(MoveToMeNotification::FINISH * ack);
	OperationState * handle(MMNotification * mmStatus);
	OperationState * handle(CommitNotification::ACK * ack);
	OperationState * handle(LockingNotification::ACK * ack);

	OperationState * handle(NodeFailureNotification * nodeFailure);


	OperationState * handle(Notification * notification);

private:
	const ClusterShardId shardId;
	const NodeOperationId destination;
	LocalPhysicalShard physicalShard;

	OperationState * compensateOperation;
	OperationState * releaseOperation;

	OperationState * compensate();
	OperationState * release();
	OperationState * connect();
	OperationState * abort();


};


}
}


#endif // __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
