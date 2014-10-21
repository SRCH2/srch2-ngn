#include "AtomicRelease.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Node.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../StateMachine.h"

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
		ConsumerInterface * consumer){

	// prepare the locker and locking notification
	this->releaseNotification = new LockingNotification(srcShardId, destShardId, copyAgent, true);
	this->lockType = LockingNotification::LockRequestType_Copy;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(consumer);
	this->finalizeFlag = false;
	init();
}

/// node arrival
AtomicRelease::AtomicRelease(const NodeOperationId & newNodeOpId,
		ConsumerInterface * consumer){ // releases the metadata

	// prepare the locker and locking notification
	this->releaseNotification = new LockingNotification(newNodeOpId, vector<NodeId>(), LockLevel_X, true, true);
	// LockLevel_X is just a place holder,
	this->lockType = LockingNotification::LockRequestType_Metadata;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(consumer);
	this->finalizeFlag = false;
	init();
}

/// record releasing
AtomicRelease::AtomicRelease(const vector<string> & primaryKeys, const NodeOperationId & writerAgent,
		ConsumerInterface * consumer){

	// prepare the locker and locking notification
	this->releaseNotification = new LockingNotification(primaryKeys, writerAgent, true);
	this->lockType = LockingNotification::LockRequestType_PrimaryKey;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(consumer);
	this->finalizeFlag = false;
	init();
}


/// general purpose cluster shard releasing
AtomicRelease::AtomicRelease(const ClusterShardId & shardId, const NodeOperationId & agent,
		ConsumerInterface * consumer){

	// prepare the locker and locking notification
	this->releaseNotification = new LockingNotification(shardId, agent, true);
	this->lockType = LockingNotification::LockRequestType_GeneralPurpose;
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	ProducerInterface::connectDeletePathToParent(consumer);
	this->finalizeFlag = false;
	init();
}


AtomicRelease::~AtomicRelease(){
	if(releaseNotification == NULL){
		ASSERT(false);
	}else{
		delete releaseNotification;
	}
}

void AtomicRelease::produce(){
	ShardManager::getShardManager()->getStateMachine()->registerOperation(releaser);
}

void AtomicRelease::end(map<NodeId, ShardingNotification * > & replies){
	finalize();
}
void AtomicRelease::finalize(){
	this->finalizeFlag = true;
	this->consumer->consume(true);
}

void AtomicRelease::setParticipants(const vector<NodeId> & participants){
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
