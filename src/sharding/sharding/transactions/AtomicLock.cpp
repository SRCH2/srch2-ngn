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
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(srcShardId, destShardId, copyAgent));
	releaseNotification = SP(LockingNotification)(new LockingNotification(srcShardId, destShardId, copyAgent, true));
	lockType = LockRequestType_Copy;
	this->finalzedFlag = false;
	init();
}

/// move
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & srcMoveAgent,
		const NodeOperationId & destMoveAgent,
		ConsumerInterface * consumer): ProducerInterface(consumer){

	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(shardId, srcMoveAgent, destMoveAgent));
	releaseNotification = SP(LockingNotification)(new LockingNotification(shardId, srcMoveAgent, destMoveAgent, true));
	lockType = LockRequestType_Move;
	this->finalzedFlag = false;
	init();
}

/// node arrival
AtomicLock::AtomicLock(const NodeOperationId & newNodeOpId,
		ConsumerInterface * consumer,
		const vector<NodeId> & listOfOlderNodes, const LockLevel & lockLevel,
		const bool blocking): ProducerInterface(consumer){

	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel));
	releaseNotification = SP(LockingNotification)(new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel, true,true));
	lockType = LockRequestType_Metadata;
	this->finalzedFlag = false;
	init();
}

/// record locking
AtomicLock::AtomicLock(const vector<string> & primaryKeys,
		const NodeOperationId & writerAgent,
		const ClusterPID & pid,
		ConsumerInterface * consumer): ProducerInterface(consumer){

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
	this->pid = pid;
	this->lockType = LockRequestType_PrimaryKey;
	this->finalzedFlag = false;
	init();

}


/// general purpose cluster shard locking
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & agent, const LockLevel & lockLevel,
		ConsumerInterface * consumer): ProducerInterface(consumer){

	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(shardId, agent, lockLevel));
	releaseNotification = SP(LockingNotification)(new LockingNotification(shardId, agent));
	lockType = LockRequestType_GeneralPurpose;
	this->finalzedFlag = false;
	init();
}


AtomicLock::~AtomicLock(){
}

Transaction * AtomicLock::getTransaction(){
	return this->getConsumer()->getTransaction();
}

void AtomicLock::produce(){
    Logger::sharding(Logger::Detail, "AtomicLock| starts.");
	boost::shared_lock<boost::shared_mutex> writeviewSLock;
    const Cluster_Writeview * writeview = ShardManager::getWriteview_read(writeviewSLock);
    bool participantsChangedFlag = false;
    for(int nodeIdx = 0; nodeIdx < participants.size(); ++nodeIdx){
    	if(! writeview->isNodeAlive(participants.at(nodeIdx))){
    		participants.erase(participants.begin() + nodeIdx);
    		nodeIdx --;
    		participantsChangedFlag = true;
    	}
    }
    writeviewSLock.unlock();
    if(participants.empty()){
    	this->getTransaction()->setUnattached();
        Logger::sharding(Logger::Detail, "AtomicLock| ends unattached, no participant found.");
    	return;
    }else if(participantsChangedFlag){
    	locker->setParticipants(participants);
    }

	// register and run the operation state in the state-machine
	ShardManager::getShardManager()->getStateMachine()->registerOperation(locker);
}


// TODO : using writeview for primaryKey locking is WRONG!!!!
void AtomicLock::init(){
	lockNotification->getInvolvedNodes(participants);
	participantIndex = -1;
	locker = new OrderedNodeIteratorOperation(lockNotification, ShardingLockACKMessageType , participants, this);
}

/*
 * This method is called after receiving the response from each participant
 */
bool AtomicLock::condition(SP(ShardingNotification) reqArg, SP(ShardingNotification) resArg, vector<NodeId> & updatedListOfParticipants){

	// get updated list of arrived nodes
	lockNotification->getInvolvedNodes(updatedListOfParticipants);

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

	participantIndex ++;

	if(req->getLockRequestType() == LockRequestType_PrimaryKey){
		ASSERT(res->isGranted());
	}
	if(res->isGranted()){
		return true;
	}else{
		Logger::sharding(Logger::Detail, "AtomicLock| node %s rejected lock.", resArg->getSrc().toString().c_str());
		recover();
		return false;
	}
	return false;
}

bool AtomicLock::shouldAbort(const NodeId & failedNode){
	unsigned failedNodeIndex = this->participants.size() ;
	for(; failedNodeIndex < this->participants.size(); ++failedNodeIndex){
		if(this->participants.at(failedNodeIndex) == failedNode){
			break;
		}
	}
	if(failedNodeIndex == this->participants.size()){
		return false;
	}

	this->participants.erase(this->participants.begin() + failedNodeIndex);

	if(this->participants.empty()){
		// all nodes that are involved in lock are now dead, so we should return the
		// default status.
		finalize(getDefaultStatusValue());
		return true;
	}
	if(this->participantIndex >= failedNodeIndex ){
		this->participantIndex --;
	}
	return false;
}

// if not granted :
void AtomicLock::recover(){

    Logger::sharding(Logger::Detail, "AtomicLock| Recovery state caused by node failure.");
	if(participantIndex == 0){
		finalize(false);
		return;
	}

	vector<NodeId> releaseParticipants;
	if(lockType == LockRequestType_PrimaryKey){
		ASSERT(false);
	}
	// release from [0 to participantIndex)
	for(int i = 0 ; i < participantIndex; ++i){
		releaseParticipants.push_back(participants.at(i));
	}
	if(releaseParticipants.empty()){
		ASSERT(false);
		finalize(false);
		return;
	}
	Logger::sharding(Logger::Detail, "AtomicLock| making the release operation for recovery.");
	// NOTE: releaseNotification is prepared in the time of preparing locker
	OrderedNodeIteratorOperation * releaser =
			new OrderedNodeIteratorOperation(releaseNotification, ShardingLockACKMessageType , releaseParticipants);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(releaser);
	finalize(false);
}

/*
 * Lock request must be granted (or partially granted in case of primarykeys)
 * if we reach to this function.
 */
void AtomicLock::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(! replies.empty()){
		ASSERT(false);
		__FUNC_LINE__
		Logger::sharding(Logger::Error, "AtomicLock| end must be empty but has %d elements.", replies.size());
		finalize(false);
		return;
	}
	/*
	 * Only the fact that we reached here shows that lock was successful
	 */
	if(lockType == LockRequestType_PrimaryKey){
		if(participants.empty()){
			ASSERT(false);
			// record change must be stopped because there is no shard
			// anymore
			Logger::sharding(Logger::Detail, "AtomicLock| empty list of participants in primaryKey lock : abort and return false");
			finalize(false);
		}
		finalize(true);
	}else{
		finalize(true);
	}
}

void AtomicLock::finalize(bool result){
	this->finalzedFlag = true;
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
