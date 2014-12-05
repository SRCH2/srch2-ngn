#include "AtomicLock.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/// copy
AtomicLock::AtomicLock(const ClusterShardId & srcShardId,
		const ClusterShardId & destShardId,
		const NodeOperationId & copyAgent,
		ConsumerInterface * consumer): ProducerInterface(consumer){
	ASSERT(this->getTransaction());
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(srcShardId, destShardId, copyAgent));
	releaseNotification = SP(LockingNotification)(new LockingNotification(srcShardId, destShardId, copyAgent, true));
	lockType = LockRequestType_Copy;
	init();
}

/// move
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & srcMoveAgent,
		const NodeOperationId & destMoveAgent,
		ConsumerInterface * consumer): ProducerInterface(consumer){
	ASSERT(this->getTransaction());
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(shardId, srcMoveAgent, destMoveAgent));
	releaseNotification = SP(LockingNotification)(new LockingNotification(shardId, srcMoveAgent, destMoveAgent, true));
	lockType = LockRequestType_Move;
	init();
}

/// node arrival
AtomicLock::AtomicLock(const NodeOperationId & newNodeOpId,
		ConsumerInterface * consumer,
		const vector<NodeId> & listOfOlderNodes, const LockLevel & lockLevel,
		const bool blocking): ProducerInterface(consumer){
	ASSERT(this->getTransaction());
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel));
	releaseNotification = SP(LockingNotification)(new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel, true,true));
	lockType = LockRequestType_Metadata;
	init();
}

/// record locking
AtomicLock::AtomicLock(const vector<string> & primaryKeys,
		const NodeOperationId & writerAgent,
		const ClusterPID & pid,
		ConsumerInterface * consumer): ProducerInterface(consumer){
	ASSERT(this->getTransaction());

	this->pid = pid;
	if(primaryKeys.empty()){
		lockNotification.reset();
		return;
	}
	/*
	 * list of primary keys must be ascending
	 */
	for(unsigned i = 0 ; i < primaryKeys.size() - 1; ++i){
		if(primaryKeys.at(i).compare(primaryKeys.at(i+1)) > 0){
			ASSERT(false);
			return;
		}
	}
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(primaryKeys, writerAgent, pid));
	releaseNotification = SP(LockingNotification)(new LockingNotification(primaryKeys, writerAgent, pid, true));
	this->lockType = LockRequestType_PrimaryKey;
	init();

}


/// general purpose cluster shard locking
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & agent, const LockLevel & lockLevel,
		ConsumerInterface * consumer): ProducerInterface(consumer){
	ASSERT(this->getTransaction());
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(shardId, agent, lockLevel));
	releaseNotification = SP(LockingNotification)(new LockingNotification(shardId, agent));
	lockType = LockRequestType_GeneralPurpose;
	init();
}


/// general purpose cluster shard locking 2 ( a list of shards with same lock type and requester)
AtomicLock::AtomicLock(const vector<ClusterShardId> & shardIds,
		const NodeOperationId & agent, const LockLevel & lockLevel,
		ConsumerInterface * consumer): ProducerInterface(consumer){
	ASSERT(this->getTransaction());
	if(shardIds.empty()){
		lockNotification.reset();
		return;
	}
	/*
	 * list of primary keys must be ascending
	 */
	for(unsigned i = 0 ; i < shardIds.size() - 1; ++i){
		if(shardIds.at(i) > shardIds.at(i+1)){
			ASSERT(false);
			return;
		}
	}
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(shardIds, agent, lockLevel));
	releaseNotification = SP(LockingNotification)(new LockingNotification(shardIds, agent));
	lockType = LockRequestType_ShardIdList;
	init();
}


AtomicLock::~AtomicLock(){
}

SP(Transaction) AtomicLock::getTransaction(){
	if(this->getConsumer() == NULL){
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}

void AtomicLock::produce(){
    Logger::sharding(Logger::Detail, "AtomicLock| starts.");

    if(! lockNotification){
        Logger::sharding(Logger::Detail, "AtomicLock| ends at the beginning, primary key input list was empty.");
        finalize(getDefaultStatusValue());
    	return;
    }

    if(participants.empty()){
        Logger::sharding(Logger::Detail, "AtomicLock| ends at the beginning, no participant found.");
        finalize(getDefaultStatusValue());
    	return;
    }

    OrderedNodeIteratorOperation * locker =
            new OrderedNodeIteratorOperation(lockNotification, ShardingLockACKMessageType , participants, this);
    if(participants.empty()){
        Logger::sharding(Logger::Detail, "AtomicLock| ends unattached, no participant found.");
        delete locker;
        finalize(getDefaultStatusValue());
    	return;
    }

	// register and run the operation state in the state-machine
	ShardManager::getShardManager()->getStateMachine()->registerOperation(locker);
}


// TODO : using writeview for primaryKey locking is WRONG!!!!
void AtomicLock::init(){
	lockNotification->getInvolvedNodes(this->getTransaction(), participants);
	latestRespondedParticipant = 0;
}

/*
 * This method is called after receiving the response from each participant
 */
bool AtomicLock::condition(SP(ShardingNotification) reqArg,
		SP(ShardingNotification) resArg,
		vector<NodeId> & updatedListOfParticipants){

	// get updated list of arrived nodes
	lockNotification->getInvolvedNodes(this->getTransaction(), this->participants );
	updatedListOfParticipants = this->participants;

	SP(LockingNotification) req = boost::dynamic_pointer_cast<LockingNotification>(reqArg);
	SP(LockingNotification::ACK) res = boost::dynamic_pointer_cast<LockingNotification::ACK>(resArg);

	if(! req || ! res){
		ASSERT(false);
		recover();
		return false;
	}
	if(lockType != req->getLockRequestType()){
		ASSERT(false);
		recover();
		return false;
	}
	latestRespondedParticipant = res->getSrc().nodeId;

	if(req->getLockRequestType() == LockRequestType_PrimaryKey){
		ASSERT(res->isGranted());
	}
	if(res->isGranted()){
		return true;
	}
	Logger::sharding(Logger::Detail, "AtomicLock| node %s rejected lock.", resArg->getSrc().toString().c_str());
	recover();
	return false;
}

// if not granted :
void AtomicLock::recover(){

    Logger::sharding(Logger::Detail, "AtomicLock| In case we could acquite some locks we must release them here.");
	if(latestRespondedParticipant <= 0){
		finalize(false);
		return;
	}

	vector<NodeId> releaseParticipants;
	if(lockType == LockRequestType_PrimaryKey){
		ASSERT(false);
	}
	// release from [0 to participantIndex)
	for(int i = 0 ; i < participants.size(); ++i){
		if(participants.at(i) >= latestRespondedParticipant){
			break;
		}
		releaseParticipants.push_back(participants.at(i));
	}
	if(releaseParticipants.empty()){
		finalize(false);
		return;
	}
	Logger::sharding(Logger::Detail, "AtomicLock| making the release operation for recovery.");
	// NOTE: releaseNotification is prepared in the time of preparing locker
	OrderedNodeIteratorOperation * releaser =
			new OrderedNodeIteratorOperation(releaseNotification, ShardingLockACKMessageType , releaseParticipants);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(releaser);
	/// NOTE : not an exit point
	finalize(false);
}

/*
 * Lock request must be granted (or partially granted in case of primarykeys)
 * if we reach to this function.
 */
void AtomicLock::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.empty()){
		__FUNC_LINE__
		Logger::sharding(Logger::Error, "AtomicLock| end must be empty but has %d elements.", replies.size());
		finalize(getDefaultStatusValue());
		return;
	}

	finalize(true);
}

void AtomicLock::finalize(bool result){
	Logger::sharding(Logger::Detail, "AtomicLock| lock : %s", result ? "successfull" : "failed");
	if(lockType == LockRequestType_PrimaryKey){
		this->getConsumer()->consume(result, pid);
	}else{
		this->getConsumer()->consume(result);
	}
}

bool AtomicLock::getDefaultStatusValue() const{
	return false;
}

}}
