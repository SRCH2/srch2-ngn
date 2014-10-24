#ifndef __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__

#include "./AtomicLock.h"
#include "./AtomicRelease.h"
#include "./AtomicMetadataCommit.h"
#include "../state_machine/State.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../notifications/Notification.h"
#include "../notifications/CopyToMeNotification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardCopyOperation : public ProducerInterface, public NodeIteratorListenerInterface{
public:

	ShardCopyOperation(const ClusterShardId & unassignedShard,
			NodeId srcNodeId, const ClusterShardId & shardToReplicate,
			ConsumerInterface * consumer);

	~ShardCopyOperation();

	Transaction * getTransaction() ;

	void produce();

	void lock(); // ** start **
	// for lock
	void consume(bool granted);
	// ** if (granted)
	void transfer(); // : requires receiving a call to our callback registered in state-machine to get MM messages
	void end(map<NodeId, ShardingNotification * > & replies);
	// if returns true, operation must stop and return null to state_machine
	bool shouldAbort(const NodeId & failedNode);
	// coming back from transfer
	void receiveStatus(const ShardMigrationStatus & status);
	void commit();
	// ** end if
	void release();
	void finalize(); // ** return **

private:

	NodeOperationId currentOpId; // used to be able to release locks, and also talk with MM
	const ClusterShardId unassignedShardId;
	const ClusterShardId replicaShardId;
	const NodeId srcNodeId;

	string currentAction;
	AtomicLock * locker;
	AtomicRelease * releaser;
	SP(CopyToMeNotification) copyToMeNotif ;
	AtomicMetadataCommit * committer;

	bool finalizedFlag;
	bool successFlag;

	LocalPhysicalShard physicalShard;

};


}
}

#endif // __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
