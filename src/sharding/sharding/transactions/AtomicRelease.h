#ifndef __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__
#define __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__

#include "NodeIteratorOperation.h"
#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/LockingNotification.h"
#include "../../metadata_manager/ResourceLocks.h"
#include "../../../configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * LockRequester must provide
 * void lockResult(bool granted);
 * and in case of primaryKey locking
 * void lockResult(vector<string> rejectedPrimaryKeys);
 * which will have rejectedPrimaryKeys empty if all primaryKeys can be locked successfully
 */
class AtomicRelease : public OrderedNodeIteratorListenerInterface, public ProducerInterface{
public:

	/// copy
	AtomicRelease(const ClusterShardId & srcShardId,
			const ClusterShardId & destShardId,
			const NodeOperationId & copyAgent,
			ConsumerInterface * consumer);

	/// node arrival
	AtomicRelease(const NodeOperationId & newNodeOpId,
			ConsumerInterface * consumer);

	/// record releasing
	AtomicRelease(const vector<string> & primaryKeys, const NodeOperationId & writerAgent,
			ConsumerInterface * consumer);


	/// general purpose cluster shard releasing
	AtomicRelease(const ClusterShardId & shardId, const NodeOperationId & agent,
			ConsumerInterface * consumer);

	~AtomicRelease();

	void produce();

	/*
	 * Lock request must be successful (or partially successful in case of primarykeys)
	 * if we reach to this function.
	 */
	void end(map<NodeId, ShardingNotification * > & replies);
private:

	LockingNotification::LockRequestType lockType;
	LockingNotification * releaseNotification;
	OrderedNodeIteratorOperation * releaser; // it will be deleted by state-machie when it returns NULL
	ConsumerInterface * consumer;

	bool finalizeFlag ;
	// TODO if we solve the problem of primary keys, we can remove this API
	void setParticipants(const vector<NodeId> & participants);

	void init();

	void finalize();
};


}
}


#endif // __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__
