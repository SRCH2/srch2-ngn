#include "AtomicRelease.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Node.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../state_machine/StateMachine.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/// copy
AtomicRelease::AtomicRelease(const ClusterShardId & srcShardId,
		const ClusterShardId & destShardId,
		const NodeOperationId & copyAgent,
		ConsumerInterface * consumer) : ProducerInterface(consumer){

	// prepare the locker and locking notification
	this->releaseNotification = SP(LockingNotification)(new LockingNotification(srcShardId, destShardId, copyAgent, true));
	this->lockType = LockRequestType_Copy;
	this->finalizeFlag = false;
	init();
}

/// node arrival
AtomicRelease::AtomicRelease(const NodeOperationId & newNodeOpId,
		ConsumerInterface * consumer): ProducerInterface(consumer){ // releases the metadata

	// prepare the locker and locking notification
	this->releaseNotification = SP(LockingNotification)(new LockingNotification(newNodeOpId, vector<NodeId>(), LockLevel_X, true, true));
	// LockLevel_X is just a place holder,
	this->lockType = LockRequestType_Metadata;
	this->finalizeFlag = false;
	init();
}

/// record releasing
AtomicRelease::AtomicRelease(const vector<string> & primaryKeys, const NodeOperationId & writerAgent, const ClusterPID & pid,
		ConsumerInterface * consumer): ProducerInterface(consumer){

	// prepare the locker and locking notification
	this->releaseNotification = SP(LockingNotification)(new LockingNotification(primaryKeys, writerAgent, pid, true));
	this->lockType = LockRequestType_PrimaryKey;
	this->finalizeFlag = false;
	init();
}


/// general purpose cluster shard releasing
AtomicRelease::AtomicRelease(const ClusterShardId & shardId, const NodeOperationId & agent,
		ConsumerInterface * consumer): ProducerInterface(consumer){

	// prepare the locker and locking notification
	this->releaseNotification = SP(LockingNotification)(new LockingNotification(shardId, agent, true));
	this->lockType = LockRequestType_GeneralPurpose;
	this->finalizeFlag = false;
	init();
}


AtomicRelease::~AtomicRelease(){
}

Transaction * AtomicRelease::getTransaction(){
	return this->getConsumer()->getTransaction();
}

void AtomicRelease::produce(){
	ShardManager::getShardManager()->getStateMachine()->registerOperation(releaser);
}

void AtomicRelease::end(map<NodeId, SP(ShardingNotification) > & replies){
	finalize();
}
void AtomicRelease::finalize(){
	this->finalizeFlag = true;
	this->getConsumer()->consume(true);
}

void AtomicRelease::setParticipants(vector<NodeId> & participants){
	if(releaser == NULL){
		ASSERT(false);
		return;
	}
	releaser->setParticipants(participants);
}

void AtomicRelease::init(){
	// participants are all node
	vector<NodeId> participants;
	releaseNotification->getInvolvedNodes(participants);
	releaser = new OrderedNodeIteratorOperation(releaseNotification, ShardingLockACKMessageType , participants, this);
}


}
}
