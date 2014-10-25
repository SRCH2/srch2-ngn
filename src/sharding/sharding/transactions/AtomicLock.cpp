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

	// prepare the locker and locking notification
	lockNotification = SP(LockingNotification)(new LockingNotification(primaryKeys, writerAgent, pid));
//	releaseNotification = new LockingNotification(primaryKeys, writerAgent, true);
//	releaseNotification = NULL;
	lockType = LockRequestType_PrimaryKey;
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
    Logger::debug("STEP : Atomic lock starts ...");
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
//		vector<string> & primaryKeys = req->getPrimaryKeys();
//		for(unsigned pkIndex = 0 ; pkIndex < primaryKeys.size(); ++pkIndex){
//			if(res->isGranted(pkIndex)){
//				continue;
//			}else{
//				// primary key rejected
//				// 1. put primary key in rejected PKs map
//				if(participantIndex > 0){ // only if any node actually needs recovery
//					rejectedPrimaryKeys[primaryKeys.at(pkIndex)] = participantIndex;
//				}
//				// 2. remove primaryKey from list
//				primaryKeys.erase(primaryKeys.begin()+pkIndex);
//				pkIndex--;
//			}
//		}
//		if(primaryKeys.size() > 0){
//			return true;
//		}
//		// no primaryKey is left, all are rejected : release successful requests
//		recover();
//		return false; // stops the locker operation
	}// else : other lock request only need one boolean value to be checked;
	else{
		if(res->isGranted()){
			return true;
		}else{
			recover();
			return false;
		}
	}

	return false;
}

bool AtomicLock::shouldAbort(const NodeId & failedNode){
	if ( std::find(this->participants.begin(), this->participants.end(), failedNode) == this->participants.end()){
		return false;
	}
	unsigned failedNodeIndex = 0 ;
	for(; failedNodeIndex < this->participants.size(); ++failedNodeIndex){
		if(this->participants.at(failedNodeIndex) == failedNode){
			break;
		}
	}
	this->participants.erase(this->participants.begin() + failedNodeIndex);
	if(this->participantIndex >= failedNodeIndex ){
		this->participantIndex --;
	}
	return false;
}

// if not granted :
void AtomicLock::recover(){

    Logger::debug("STEP : Atomic lock, going to recovery state ...");
	if(participantIndex == 0){
		finalize(false);
		return;
	}

	vector<NodeId> releaseParticipants;
	if(lockType == LockRequestType_PrimaryKey){
//		ASSERT(releaseNotification == NULL);
//		vector<string> primaryKeysToRelease ;
//		unsigned maxParticipantIndex = 0 ;
//		for(map<string, unsigned>::iterator pkItr = rejectedPrimaryKeys.begin(); pkItr != rejectedPrimaryKeys.end(); ++pkItr){
//			primaryKeysToRelease.push_back(pkItr->first);
//			if(pkItr->second > maxParticipantIndex){
//				maxParticipantIndex = pkItr->second;
//			}
//		}
//		for(unsigned i = 0 ; i < maxParticipantIndex; ++i){
//			releaseParticipants.push_back(participants.at(i));
//		}
//		if(maxParticipantIndex == 0){ // no need to recover anything
//			finalize(false);
//			return;
//		}
//		releaseNotification = new LockingNotification(primaryKeysToRelease, writeAgent, pid, true);

	}else{
		// release from [0 to participantIndex)
		for(unsigned i = 0 ; i < participantIndex; ++i){
			releaseParticipants.push_back(participants.at(i));
		}
		// releaseNotification is prepared in the time of preparing locker
	}
	Logger::debug("STEP : Atomic lock making the release operation for recovery...");
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
		finalize(false);
		return;
	}
	if(lockType == LockRequestType_PrimaryKey){
//		vector<string> rejectedPKs;
//		if(rejectedPrimaryKeys.size() > 0){
//			for(map<string, unsigned>::iterator pkItr = rejectedPrimaryKeys.begin(); pkItr != rejectedPrimaryKeys.end(); ++pkItr){
//				rejectedPKs.push_back(pkItr->first);
//			}
//			recover();
//		}
//		finalize(rejectedPKs);
	}else{
		finalize(true);
	}
}

void AtomicLock::setParticipants(const vector<NodeId> & participants){
	if(participants.empty()){
		return;
	}
	this->participants.clear();
	this->participants.insert(this->participants.begin(), participants.begin(), participants.end());
	participantIndex = -1;
	if(locker == NULL){
		ASSERT(false);
		return;
	}
	locker->setParticipants(this->participants);
}


void AtomicLock::finalize(bool result){
	this->finalzedFlag = true;
	Logger::debug("STEP : Atomic lock ends ...");
	this->getConsumer()->consume(result);
}
void AtomicLock::finalize(const vector<string> & rejectedPKs){
	this->finalzedFlag = true;
	this->getConsumer()->consume(rejectedPKs);
}

}}
