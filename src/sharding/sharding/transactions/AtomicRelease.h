#ifndef __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__
#define __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__

#include "../state_machine/node_iterators/NodeIteratorOperation.h"
#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../notifications/LockingNotification.h"
#include "../../configuration/ShardingConstants.h"

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
	AtomicRelease(const vector<string> & primaryKeys, const NodeOperationId & writerAgent, const ClusterPID & pid,
			ConsumerInterface * consumer);


	/// general purpose cluster shard releasing
	AtomicRelease(const ClusterShardId & shardId, const NodeOperationId & agent,
			ConsumerInterface * consumer);

	~AtomicRelease();

	SP(Transaction) getTransaction();

	void produce();
	bool updateParticipants();
	/*
	 * Lock request must be successful (or partially successful in case of primarykeys)
	 * if we reach to this function.
	 */
	void end(map<NodeId, SP(ShardingNotification) > & replies);
	string getName() const {return "AtomicRelease";};
private:

	LockRequestType lockType;
	SP(LockingNotification) releaseNotification;
	OrderedNodeIteratorOperation * releaser; // it will be deleted by state-machie when it returns NULL
	vector<NodeId> participants;

	ClusterPID pid;// only valid and meaningful value if primary key lock

	bool finalizeFlag ;
	// TODO if we solve the problem of primary keys, we can remove this API
	void setParticipants(vector<NodeId> & participants);
	void init();

	void finalize();
};


}
}


#endif // __SHARDING_SHARDING_ATOMIC_RELEASE_OPERATION_H__
