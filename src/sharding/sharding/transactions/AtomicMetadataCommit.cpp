#include "AtomicMetadataCommit.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Node.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "./AtomicLock.h"
#include "./AtomicRelease.h"
#include "../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


AtomicMetadataCommit::AtomicMetadataCommit(const vector<NodeId> & exceptions,
		MetadataChange * metadataChange,
		ConsumerInterface * consumer, const bool skipLock): ProducerInterface(consumer){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes, true);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(std::find(exceptions.begin(), exceptions.end(), allNodes.at(i)) == exceptions.end()){
			this->participants.push_back(allNodes.at(i));
		}
	}
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->atomicLock = NULL;
	this->atomicRelease = NULL;
	this->finalizedFlag = false;
	this->currentAction = "";
	this->skipLock = skipLock;
}

AtomicMetadataCommit::AtomicMetadataCommit(const NodeId & exception,
		MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock): ProducerInterface(consumer){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(exception != allNodes.at(i)){
			this->participants.push_back(allNodes.at(i));
		}
	}
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->atomicLock = NULL;
	this->atomicRelease = NULL;
	this->currentAction = "";
	this->skipLock = skipLock;
}

AtomicMetadataCommit::AtomicMetadataCommit(MetadataChange * metadataChange,
		const vector<NodeId> & participants, ConsumerInterface * consumer, const bool skipLock): ProducerInterface(consumer){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	this->participants = participants;
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->atomicLock = NULL;
	this->atomicRelease = NULL;
	this->currentAction = "";
	this->skipLock = skipLock;
}

AtomicMetadataCommit::~AtomicMetadataCommit(){
	if(metadataChange != NULL){
		delete metadataChange;
	}
	if(atomicLock != NULL){
		delete atomicLock;
	}
	if(atomicRelease != NULL){
		delete atomicRelease;
	}
}

Transaction * AtomicMetadataCommit::getTransaction(){
	return this->getConsumer()->getTransaction();
}

void AtomicMetadataCommit::produce(){
    if(participants.size() == 0){
        // no participant exists
        finalize(true);
        return ;
    }
    if(metadataChange == NULL){
        ASSERT(false);
        finalize(false);
        return;
    }
    //lock should be acquired on all nodes
    atomicLock = new AtomicLock(selfOperationId, this); // X-locks metadata by default
    atomicRelease = new AtomicRelease(selfOperationId, this);
    if(skipLock){
        commit();
    }else{
        lock();
    }
}

void AtomicMetadataCommit::lock(){
	this->currentAction = "lock";
	atomicLock->produce();
}

void AtomicMetadataCommit::consume(bool granted){
	if(currentAction.compare("lock") == 0){
		if(! granted){
			finalize(false);
		}else{ // lock granted, we can proceed to commit
			commit();
		}
	}else if(currentAction.compare("release") == 0){
		ASSERT(granted);
		finalize(granted); // done.
	}else {
		ASSERT(false);
		finalize(false);
	}
}

void AtomicMetadataCommit::commit(){
	commitNotification = SP(CommitNotification)(new CommitNotification( metadataChange ));

	ConcurrentNotifOperation * committer = new ConcurrentNotifOperation(commitNotification,
			ShardingCommitACKMessageType, participants, this);
	// committer is deallocated in state-machine so we don't have to have the
	// pointer. Before deleting committer, state-machine calls it's getMainTransactionId()
	// which calls lastCallback from its consumer
	this->currentAction = "commit";
	ShardManager::getShardManager()->getStateMachine()->registerOperation(committer);
}

bool AtomicMetadataCommit::shouldAbort(const NodeId & failedNode){
	if(std::find(participants.begin(), participants.end(), failedNode) == participants.end()){
		return false;
	}
	participants.erase(std::find(participants.begin(), participants.end(), failedNode));
	if(participants.empty()){
		finalize(false);
		return true;
	}
	return false;
}

void AtomicMetadataCommit::end_(map<NodeOperationId , SP(ShardingNotification)> & replies){
	// nothing to do with commit acks
	if(replies.size() != participants.size()){
		ASSERT(false);
		finalize(false);
		return;
	}
	if(skipLock){
	    finalize(true);
	}else{
        release();
	}
}

void AtomicMetadataCommit::release(){
	if(atomicRelease == NULL){
		ASSERT(false);
		return;
	}
	this->currentAction = "release";
	atomicRelease->produce();
}

void AtomicMetadataCommit::finalize(bool result){
	this->getConsumer()->consume(result);
	this->finalizedFlag = true;
	return;
}

}
}
