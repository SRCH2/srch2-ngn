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
#include "Transaction.h"
#include "TransactionSession.h"
#include "../state_machine/State.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../metadata_manager/Cluster.h"
#include "../metadata_manager/Cluster_Writeview.h"

#include "core/util/Assert.h"
#include "core/util/Logger.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

void Transaction::startTransaction(SP(Transaction) trans){
	if(! trans){
		ASSERT(false);
		return;
	}
	if(trans->getTransactionType() != ShardingTransactionType_Loadbalancing){
		Logger::sharding(Logger::Step, "Starting transaction %s", getTransTypeStr(trans->getTransactionType()));
	}
	trans->init();
	trans->threadBegin(trans);
	trans->run();
	trans->threadEnd();
}

Transaction::Transaction():transactionId(OperationState::getNextOperationId()){
	// lock shared manager in S mode to avoid kill command
	// delete it in the middle of an operation.
	ShardManager::getShardManagerGuard().lock_shared();

	session = NULL;
	attachedToThreadFlag = false;
	finalizeArgument = NULL;
}

void Transaction::init(){
	this->initSession();
}

Transaction::~Transaction(){
	delete session;
	finalize();
	ShardManager::getShardManagerGuard().unlock_shared();
}

void Transaction::finalize(){
	finalizeWork(finalizeArgument);
	if(finalizeArgument != NULL){
	    if(finalizeArgument->needWriteviewLock){
	        this->transMutex.unlock();
	    }
		delete finalizeArgument;
		finalizeArgument = NULL;
	}
}
void Transaction::setFinalizeArgument(bool arg, bool needWriteviewLock){
	if(finalizeArgument != NULL){
		ASSERT(false);
		delete finalizeArgument;
	}
	this->finalizeArgument = new Params(arg);
	this->finalizeArgument->needWriteviewLock = needWriteviewLock;
}

Transaction::Params * Transaction::getFinalizeArgument(){
	return this->finalizeArgument;
}

TRANS_ID Transaction::getTID() const {
	return this->transactionId;
}

void Transaction::initSession(){
	this->setSession(new TransactionSession());
	this->getSession()->response = new JsonResponseHandler();
}
TransactionSession * Transaction::getSession(){
	return this->session;
}

SP(ClusterNodes_Writeview) Transaction::getNodesWriteview_write() const{
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getShardManager()->
			getMetadataManager()->getClusterNodesWriteview_write();
	return nodesWriteview;
}

SP(const ClusterNodes_Writeview) Transaction::getNodesWriteview_read() const{
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getShardManager()->
			getMetadataManager()->getClusterNodesWriteview_read();
	return nodesWriteview;
}

void Transaction::setSession(TransactionSession * session){
	ASSERT(session != NULL);
	if(this->session != NULL){
		delete this->session;
		this->session = NULL;
	}
	this->session = session;
}


ReadviewTransaction::ReadviewTransaction(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview):
		Transaction(){
	this->clusterReadview = clusterReadview;
}

boost::shared_ptr<const ClusterResourceMetadata_Readview> ReadviewTransaction::getReadview() const{
	return clusterReadview;
}

WriteviewTransaction::WriteviewTransaction():Transaction(){
	writeviewLock = NULL;
}

void WriteviewTransaction::threadBegin(SP(Transaction) sp){ // sets sharedPointer to sp
	Transaction::threadBegin(sp);
	if(writeviewLock != NULL){
		ASSERT(false);
		delete writeviewLock;
	}
	writeviewLock = new boost::unique_lock<boost::shared_mutex>();
	this->writeview = ShardManager::getWriteview_write(*writeviewLock);
}
void WriteviewTransaction::threadEnd(){ // resets sharedPointer
	if(finalizeArgument != NULL && finalizeArgument->needWriteviewLock){
		finalizeArgument->writeviewLock = writeviewLock;
		writeviewLock = NULL;
	}else{
		if(writeviewLock != NULL){
			delete writeviewLock;
			writeviewLock = NULL;
		}
	}
	Transaction::threadEnd();
}

Cluster_Writeview * WriteviewTransaction::getWriteview(){
	return writeview;
}

void WriteviewTransaction::setWriteview(Cluster_Writeview * newWriteview){
    this->writeview = newWriteview;
}

}
}
