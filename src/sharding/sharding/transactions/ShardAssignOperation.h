#ifndef __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__

#include "./AtomicLock.h"
#include "./AtomicMetadataCommit.h"
#include "../metadata_manager/Shard.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommitNotification.h"
#include "../../notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardAssignOperation: public ConsumerInterface,public ProducerInterface {
public:

	ShardAssignOperation(const ClusterShardId & unassignedShard, ConsumerInterface * consumer);
	~ShardAssignOperation();


	void produce();

	void lock(); // ** start **
	// for lock
	void consume(bool granted);
	// ** if (granted)
	void commit();
	// ** end if
	void release();

	void finalize(); // ** return **

private:

	NodeOperationId currentOpId; // used to be able to release locks, and also talk with MM
	const ClusterShardId shardId;

	ConsumerInterface * consumer;
	string currentAction;
	AtomicLock * locker;
	AtomicRelease * releaser;
	AtomicMetadataCommit * committer;
	bool releasingMode;

	bool finalizedFlag;
	bool successFlag;

};

}
}

#endif // __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
