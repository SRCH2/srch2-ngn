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
#ifndef __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__

#include "./AtomicLock.h"
#include "./AtomicRelease.h"
#include "./AtomicMetadataCommit.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../notifications/MoveToMeNotification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class ShardMoveOperation : public ProducerInterface, public NodeIteratorListenerInterface{
public:

	ShardMoveOperation(const NodeId & srcNodeId, const ClusterShardId & moveShardId, ConsumerInterface * consumer);

	~ShardMoveOperation();

	SP(Transaction) getTransaction() ;

	void produce();


	void lock();
	void consume(bool granted);
	// **** If lock granted
	void transfer();

	// for transfer
	void consume(const ShardMigrationStatus & status);

	// **** If transfer was successful
	void commit();
	// decide to do the cleanup
    // **** end if

	// **** end if
	void release();

	void end(map<NodeId, SP(ShardingNotification) > & replies);

	void finalize();
	string getName() const {return "shard-move";};

private:
	// shardId must still be read and on the srcAddress
	bool checkStillValid();

	enum CurrentOperation{
		PreStart,
		Lock,
		Transfer,
		Commit,
		Release,
		Cleanup
	};
	const ClusterShardId shardId;
	NodeOperationId currentOpId; // used to be able to release locks, and also talk with MM
	NodeOperationId srcAddress;

	CurrentOperation currentOp;

	bool successFlag;

	bool transferAckReceived;
	ShardMigrationStatus transferStatus;

	LocalPhysicalShard physicalShard;

	AtomicLock * locker;
	SP(MoveToMeNotification) moveToMeNotif ;
	AtomicMetadataCommit * committer;
	AtomicRelease * releaser;
	SP(MoveToMeNotification::CleanUp) cleaupNotif;

};

}
}


#endif // __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
