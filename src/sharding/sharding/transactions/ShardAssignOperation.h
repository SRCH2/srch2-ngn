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
#ifndef __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__

#include "./AtomicLock.h"
#include "./AtomicMetadataCommit.h"
#include "../metadata_manager/Shard.h"
#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardAssignOperation: public ProducerInterface, public ConsumerInterface {
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

	string getName() const {return "shard-assign";};

	SP(Transaction) getTransaction(){
		if(this->getConsumer() == NULL){
			return SP(Transaction)();
		}
		return this->getConsumer()->getTransaction();
	}

private:
	// shardId must still be unassigned
	bool checkStillValid();

	enum CurrentAction{
		PreStart,
		Lock,
		Release,
		Commit
	};
	NodeOperationId currentOpId; // used to be able to release locks, and also talk with MM
	const ClusterShardId shardId;

	CurrentAction currentAction;
	AtomicLock * locker;
	AtomicRelease * releaser;
	AtomicMetadataCommit * committer;
	bool successFlag;

};

}
}

#endif // __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
