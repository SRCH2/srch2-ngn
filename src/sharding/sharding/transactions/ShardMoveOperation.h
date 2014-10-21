#ifndef __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__

#include "./AtomicLock.h"
#include "./AtomicRelease.h"
#include "./AtomicMetadataCommit.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/MoveToMeNotification.h"
#include "../../notifications/CommitNotification.h"
#include "../../notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class ShardMoveOperation : public ConsumerInterface, public ProducerInterface, public NodeIteratorListenerInterface{
public:

	ShardMoveOperation(const NodeId & srcNodeId, const ClusterShardId & moveShardId, ConsumerInterface * consumer);

	~ShardMoveOperation();

	void produce(){
		lock();
	}


	void lock(){ // **** START ****
		this->locker = new AtomicLock(shardId, srcAddress, currentOpId, this);
		// locker calls all methods of LockResultCallbackInterface from us
		this->releaser = new AtomicRelease(shardId, currentOpId, this); // we only release out lock, srcAddress lock is released when we ask for cleanup
		// releaser calls all methods of BooleanCallbackInterface from us
		this->currentOp = Lock;
		this->locker->lock();
	}
	void consume(bool granted){
		switch (currentOp) {
			case PreStart:
				ASSERT(false);
				break;
			case Lock:
			{
				if(granted){
					transfer();
				}else{
					finalize();
				}
				break;
			}
			case Transfer:
				break;
			case Commit:
				break;
			case Release:
				break;
			default:
				break;
		}
	}
	// **** If lock granted
	void transfer(){
		// transfer data by ordering MM
		// 1. register this transaction in shard manager to receive MM notification
		ShardManager::getShardManager()->registerMMSessionListener(currentOpId.operationId, this);
		// 2. send copyToMe notification to the srcNode to start transferring the data
		this->moveToMeNotif = new MoveToMeNotification(shardId);

		// NOTE : this is deallocated by the state machine
		ConcurrentNotifOperation * copyer = new ConcurrentNotifOperation(moveToMeNotif, NULLType, srcAddress.nodeId , NULL, false);
		this->currentOp = Transfer;
		ShardManager::getShardManager()->getStateMachine()->registerOperation(copyer);
	}
	// for transfer
	void receiveStatus(const ShardMigrationStatus & status){
		// failed or succeed?
		if(status == MM_STATUS_FAILURE){
			this->successFlag = false;
			release();
		}else if(status == MM_STATUS_SUCCESS){
			Cluster_Writeview * writeview = ShardManager::getWriteview();

			string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
					writeview->cores[shardId.coreId]->getName(), &shardId);
			if(indexDirectory.compare("") == 0){
				indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
						writeview->cores[shardId.coreId]->getName(), &shardId);
			}

			physicalShard = LocalPhysicalShard(status.shard, indexDirectory, "");
			return commit();
		}
	}

	// **** If transfer was successful
	void commit(){
		// prepare the shard change
		ShardMoveChange * shardMoveChange = new ShardMoveChange(shardId, srcAddress.nodeId, ShardManager::getCurrentNodeId());
		shardMoveChange->setPhysicalShard(physicalShard);
		committer = new AtomicMetadataCommit(this->getOperationId(), vector<NodeId>(), shardMoveChange);
		commitOperation = OperationState::startOperation(commitOperation);
		if(commitOperation == NULL){
			return release();
		}
		return this;
		//TODO : we must commit here, and destination (or src of shard) must keep a backup whenever a localshard is getting overwritten
		// TODO
		// TODO
		// TODO
		// TODO
		// TODO
	}
	// decide to do the cleanup
    // **** end if

	// **** end if
	void release(){
		// release the locks
		ASSERT(! this->releasingMode);
		this->releasingMode = true;
		this->releaser->release();
	}
	// for commit and release
	void finish(bool done) {
		if(this->releasingMode){ // release result
			ASSERT(done);
			cleanup();
		}else{ // commit result
			ASSERT(done);
			release();
		}
	}
	// if data transfer was successful
	void cleanup(){
		// 1. prepare cleanup command
		MoveToMeNotification::CleanUp * cleaupNotif = new MoveToMeNotification::CleanUp(shardId);

		// NOTE : this is deallocated by the state machine
		ConcurrentNotifOperation * cleaner = new ConcurrentNotifOperation(cleaupNotif, NULLType, srcAddress.nodeId , NULL, false);
		// 2. send the cleanup command to the srcNode to remove the backed up data shard
		ShardManager::getShardManager()->getStateMachine()->registerOperation(cleaner);
	}

	void finalize(){ // ***** END *****
		this->finalizedFlag = true;
		this->consumer->finish(this->successFlag);
	}

	TRANS_ID lastCallback(void * args){
		if(! finalizedFlag ){
			return TRANS_ID_NULL;
		}else{
			return consumer->lastCallback(args);
		}
	}

private:

	enum CurrentOperation{
		PreStart,
		Lock,
		Transfer,
		Commit,
		Release
	};
	const ClusterShardId shardId;
	NodeOperationId currentOpId; // used to be able to release locks, and also talk with MM
	NodeOperationId srcAddress;

	CurrentOperation currentOp;

	bool successFlag;
	bool finalizedFlag;
	bool releasingMode;

	LocalPhysicalShard physicalShard;

	ConsumerInterface * consumer;

	AtomicLock * locker;
	MoveToMeNotification * moveToMeNotif ;
	AtomicMetadataCommit * committer;
	AtomicRelease * releaser;
	MoveToMeNotification::CleanUp * cleaupNotif;

};









/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	const ClusterShardId shardId;
	const NodeOperationId destination;
	LocalPhysicalShard physicalShard;

};


}
}


#endif // __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
