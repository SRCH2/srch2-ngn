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
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	this->metadataChange = metadataChange;
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<NodeId> allNodes;
	nodesWriteview->getArrivedNodes(allNodes, true);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(std::find(exceptions.begin(), exceptions.end(), allNodes.at(i)) == exceptions.end()){
			this->participants.push_back(allNodes.at(i));
		}
	}
	nodesWriteview.reset();
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->atomicLock = NULL;
	this->atomicRelease = NULL;
	this->currentAction = "";
	this->skipLock = skipLock;
}

AtomicMetadataCommit::AtomicMetadataCommit(const NodeId & exception,
		MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock): ProducerInterface(consumer){
	ASSERT(metadataChange != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	this->metadataChange = metadataChange;
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<NodeId> allNodes;
	nodesWriteview->getArrivedNodes(allNodes);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(exception != allNodes.at(i)){
			this->participants.push_back(allNodes.at(i));
		}
	}
	nodesWriteview.reset();
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->atomicLock = NULL;
	this->atomicRelease = NULL;
	this->currentAction = "";
	this->skipLock = skipLock;
}

AtomicMetadataCommit::AtomicMetadataCommit(MetadataChange * metadataChange,
		const vector<NodeId> & participants, ConsumerInterface * consumer, const bool skipLock): ProducerInterface(consumer){
	ASSERT(metadataChange != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction());
	this->metadataChange = metadataChange;
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	for(unsigned i = 0 ; i < participants.size(); ++i){
		if(nodesWriteview->isNodeAlive(participants.at(i))){
			this->participants.push_back(participants.at(i));
		}
	}
	nodesWriteview.reset();
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

SP(Transaction) AtomicMetadataCommit::getTransaction(){
	if(this->getConsumer() == NULL){
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}

void AtomicMetadataCommit::produce(){
    Logger::sharding(Logger::Detail, "Atomic Metadata Commit : starting ...");
    if(participants.empty()){
        // no participant exists
    	finalize(false);
    	return ;
    }
    if(metadataChange == NULL){
        ASSERT(false);
        finalize(false);
        return;
    }
    if(skipLock){
        commit();
    }else{
        lock();
    }
}

void AtomicMetadataCommit::lock(){
	/*
	 * NOTE:
	 */
	Logger::sharding(Logger::Warning, "Currently only X shard locks conflict with metadata %s",
				"because we always use S lock on metadata. It's NEW, be careful.");
    atomicLock = new AtomicLock(selfOperationId, this); // X-locks metadata by default
	this->currentAction = "lock";
    Logger::sharding(Logger::Detail, "Atomic Metadata Commit : locking ...");
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

    Logger::sharding(Logger::Detail, "Atomic Metadata Commit : committing the change on all metadatas ...");

	commitNotification = SP(CommitNotification)(new CommitNotification( metadataChange ));

	ConcurrentNotifOperation * committer = new ConcurrentNotifOperation(commitNotification,
			ShardingCommitACKMessageType, participants, this);
	// committer is deallocated in state-machine so we don't have to have the
	// pointer. Before deleting committer, state-machine calls it's getMainTransactionId()
	// which calls lastCallback from its consumer
	this->currentAction = "commit";
	ShardManager::getShardManager()->getStateMachine()->registerOperation(committer);
}

void AtomicMetadataCommit::end_(map<NodeOperationId , SP(ShardingNotification)> & replies){
	// nothing to do with commit acks
	if(skipLock){
	    finalize(true);
	}else{
        release();
	}
}

void AtomicMetadataCommit::release(){
    atomicRelease = new AtomicRelease(selfOperationId, this);
	this->currentAction = "release";
    Logger::sharding(Logger::Detail, "Atomic Metadata Commit : releasing locks ...");
	atomicRelease->produce();
}

void AtomicMetadataCommit::finalize(bool result){
	this->getConsumer()->consume(result);
	return;
}

}
}
