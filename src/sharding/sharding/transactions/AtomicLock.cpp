#include "AtomicLock.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

	/// copy
AtomicLock::AtomicLock(const ClusterShardId & srcShardId,
		const ClusterShardId & destShardId,
		const NodeOperationId & copyAgent,
		ConsumerInterface * consumer){

	// prepare the locker and locking notification
	lockNotification = new LockingNotification(srcShardId, destShardId, copyAgent);
	releaseNotification = new LockingNotification(srcShardId, destShardId, copyAgent, true);
	lockType = LockingNotification::LockRequestType_Copy;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(this->consumer);
	this->finalzedFlag = false;
	init();
}

/// move
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & srcMoveAgent,
		const NodeOperationId & destMoveAgent,
		ConsumerInterface * lockRequester){

	// prepare the locker and locking notification
	lockNotification = new LockingNotification(shardId, srcMoveAgent, destMoveAgent);
	releaseNotification = new LockingNotification(shardId, srcMoveAgent, destMoveAgent, true);
	lockType = LockingNotification::LockRequestType_Move;
	ASSERT(consumer != NULL);
	this->consumer = lockRequester;
	ProducerInterface::connectDeletePathToParent(this->consumer);
	this->finalzedFlag = false;
	init();
}

/// node arrival
AtomicLock::AtomicLock(const NodeOperationId & newNodeOpId,
		ConsumerInterface * lockRequester,
		const vector<NodeId> & listOfOlderNodes, const LockLevel & lockLevel,
		const bool blocking){

	// prepare the locker and locking notification
	lockNotification = new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel);
	releaseNotification = new LockingNotification(newNodeOpId, listOfOlderNodes, lockLevel, true);
	lockType = LockingNotification::LockRequestType_Metadata;
	ASSERT(consumer != NULL);
	this->consumer = lockRequester;
	ProducerInterface::connectDeletePathToParent(this->consumer);
	this->finalzedFlag = false;
	init();
}

/// record locking
AtomicLock::AtomicLock(const vector<string> & primaryKeys,
		const NodeOperationId & writerAgent,
		ConsumerInterface * lockRequester){

	// prepare the locker and locking notification
	lockNotification = new LockingNotification(primaryKeys, writerAgent);
//	releaseNotification = new LockingNotification(primaryKeys, writerAgent, true);
	releaseNotification = NULL;
	lockType = LockingNotification::LockRequestType_PrimaryKey;
	ASSERT(consumer != NULL);
	this->consumer = lockRequester;
	ProducerInterface::connectDeletePathToParent(this->consumer);
	this->finalzedFlag = false;
	init();
}


/// general purpose cluster shard locking
AtomicLock::AtomicLock(const ClusterShardId & shardId,
		const NodeOperationId & agent, const LockLevel & lockLevel,
		ConsumerInterface * lockRequester){

	// prepare the locker and locking notification
	lockNotification = new LockingNotification(shardId, agent, lockLevel);
	releaseNotification = new LockingNotification(shardId, agent, true);
	lockType = LockingNotification::LockRequestType_GeneralPurpose;
	ASSERT(consumer != NULL);
	this->consumer = lockRequester;
	ProducerInterface::connectDeletePathToParent(this->consumer);
	this->finalzedFlag = false;
	init();
}


AtomicLock::~AtomicLock(){
	if(lockNotification == NULL){
		ASSERT(false);
	}else{
		delete lockNotification;
	}
}

void AtomicLock::produce(){
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
bool AtomicLock::condition(ShardingNotification * reqArg, ShardingNotification * resArg){
	LockingNotification * req = (LockingNotification *)reqArg;
	LockingNotification::ACK * res = (LockingNotification::ACK)resArg;

	if(req == NULL || res == NULL){
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

	if(req->getLockRequestType() == LockingNotification::LockRequestType_PrimaryKey){
		vector<string> & primaryKeys = req->getPrimaryKeys();
		for(unsigned pkIndex = 0 ; pkIndex < primaryKeys.size(); ++pkIndex){
			if(res->isGranted(pkIndex)){
				continue;
			}else{
				// primary key rejected
				// 1. put primary key in rejected PKs map
				if(participantIndex > 0){ // only if any node actually needs recovery
					rejectedPrimaryKeys[primaryKeys.at(pkIndex)] = participantIndex;
				}
				// 2. remove primaryKey from list
				primaryKeys.erase(primaryKeys.begin()+pkIndex);
				pkIndex--;
			}
		}
		if(primaryKeys.size() > 0){
			return true;
		}
		// no primaryKey is left, all are rejected : release successful requests
		recover();
		return false; // stops the locker operation
	}// else : other lock request only need one boolean value to be checked;
	else{
		if(res->isGranted()){
			return true;
		}else{
			recover();
			return false;
		}
	}

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

	if(participantIndex == 0){
		finalize(false);
		return;
	}

	vector<NodeId> releaseParticipants;
	if(lockType == LockingNotification::LockRequestType_PrimaryKey){
		ASSERT(releaseNotification == NULL);
		vector<string> primaryKeysToRelease ;
		unsigned maxParticipantIndex = 0 ;
		for(map<string, unsigned>::iterator pkItr = rejectedPrimaryKeys.begin(); pkItr != rejectedPrimaryKeys.end(); ++pkItr){
			primaryKeysToRelease.push_back(pkItr->first);
			if(pkItr->second > maxParticipantIndex){
				maxParticipantIndex = pkItr->second;
			}
		}
		for(unsigned i = 0 ; i < maxParticipantIndex; ++i){
			releaseParticipants.push_back(participants.at(i));
		}
		if(maxParticipantIndex == 0){ // no need to recover anything
			finalize(false);
			return;
		}
		releaseNotification = new LockingNotification(primaryKeysToRelease, writeAgent, true);

	}else{
		// release from [0 to participantIndex)
		for(unsigned i = 0 ; i < participantIndex; ++i){
			releaseParticipants.push_back(participants.at(i));
		}
		// releaseNotification is prepared in the time of preparing locker
	}
	OrderedNodeIteratorOperation * releaser =
			new OrderedNodeIteratorOperation(releaseNotification, ShardingLockACKMessageType , releaseParticipants);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(releaser);
	finalize(false);
}

/*
 * Lock request must be granted (or partially granted in case of primarykeys)
 * if we reach to this function.
 */
void AtomicLock::end(map<NodeId, ShardingNotification * > & replies){
	if(! replies.empty()){
		ASSERT(false);
		finalize(false);
		return;
	}
	if(lockType == LockingNotification::LockRequestType_PrimaryKey){
		vector<string> rejectedPKs;
		if(rejectedPrimaryKeys.size() > 0){
			for(map<string, unsigned>::iterator pkItr = rejectedPrimaryKeys.begin(); pkItr != rejectedPrimaryKeys.end(); ++pkItr){
				rejectedPKs.push_back(pkItr->first);
			}
			recover();
		}
		finalize(rejectedPKs);
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
	consumer->consume(result);
}
void AtomicLock::finalize(const vector<string> & rejectedPKs){
	this->finalzedFlag = true;
	consumer->consume(rejectedPKs);
}

}}
