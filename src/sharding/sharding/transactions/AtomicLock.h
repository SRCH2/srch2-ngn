/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_SHARDING_ATOMIC_LOCK_OPERATION_H__
#define __SHARDING_SHARDING_ATOMIC_LOCK_OPERATION_H__

#include "../state_machine/State.h"
#include "../state_machine/node_iterators/NodeIteratorOperation.h"
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


class AtomicLock : public OrderedNodeIteratorListenerInterface, public ProducerInterface{
public:

	/// copy
	AtomicLock(const ClusterShardId & srcShardId,
			const ClusterShardId & destShardId,
			const NodeOperationId & copyAgent,
			ConsumerInterface * lockRequester);

	/// move
	AtomicLock(const ClusterShardId & shardId,
			const NodeOperationId & srcMoveAgent,
			const NodeOperationId & destMoveAgent,
			ConsumerInterface * lockRequester);

	/// node arrival , or metadata lock in general
	AtomicLock(const NodeOperationId & newNodeOpId,
			ConsumerInterface * lockRequester,
			const vector<NodeId> & listOfOlderNodes = vector<NodeId>(),
			const LockLevel & lockLevel = LockLevel_X,
			const bool blocking = true);

	/// record locking
	AtomicLock(const vector<string> & primaryKeys,
			const NodeOperationId & writerAgent,
			const ClusterPID & pid,
			ConsumerInterface * lockRequester);


	/// general purpose cluster shard locking
	AtomicLock(const ClusterShardId & shardId,
			const NodeOperationId & agent, const LockLevel & lockLevel,
			ConsumerInterface * lockRequester);

	/// general purpose cluster shard locking 2 ( a list of shards with same lock type and requester)
	AtomicLock(const vector<ClusterShardId> & shardIds,
			const NodeOperationId & agent, const LockLevel & lockLevel,
			ConsumerInterface * consumer);

	~AtomicLock();

	SP(Transaction) getTransaction();

	void produce();

	/*
	 * This method is called after receiving the response from each participant
	 */
	bool condition(SP(ShardingNotification) req, SP(ShardingNotification) res,
			vector<NodeId> & updatedListOfParticipants);


	/*
	 * Lock request must be successful (or partially successful in case of primarykeys)
	 * if we reach to this function.
	 */
	void end(map<NodeId, SP(ShardingNotification) > & replies);


	string getName() const {return "AtomicLock";};

private:

	SP(LockingNotification) lockNotification;
	LockRequestType lockType;

	SP(LockingNotification) releaseNotification;

	vector<NodeId> participants;
	NodeId latestRespondedParticipant;

	ClusterPID pid;// only valid and meaningful value if primary key lock

	void recover();

	void init();


	void finalize(bool result);

	bool getDefaultStatusValue() const;


};


}
}


#endif // __SHARDING_SHARDING_ATOMIC_LOCK_OPERATION_H__
