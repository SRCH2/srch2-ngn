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


#include "State.h"
#include "../transactions/Transaction.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


unsigned OperationState::nextOperationId = 10;
pthread_mutex_t OperationState::operationIdMutex;
const unsigned OperationState::DataRecoveryOperationId = 1;

unsigned OperationState::getOperationId() const{
	return operationId;
}
void OperationState::setOperationId(unsigned operationId) {
	this->operationId = operationId;
}

void OperationState::send(SP(ShardingNotification) notification, const NodeOperationId & dest) const{
	if(! notification){
		ASSERT(false);
		return ;
	}
	notification->setDest(dest);
	notification->setSrc(NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()));
	ShardingNotification::send(notification);
}

SP(Transaction) OperationState::getTransaction(){
	return transaction;
}
void OperationState::setTransaction(SP(Transaction) sp){
    this->transaction.reset();
	this->transaction = sp;
}
void OperationState::lock(){
	this->operationContentLock.lock();
}
void OperationState::unlock(){
	this->operationContentLock.unlock();
}

void OperationState::initOperationStateStaticEnv(){
	pthread_mutex_init(&operationIdMutex, 0);
}

unsigned OperationState::getNextOperationId(){
	pthread_mutex_lock(&operationIdMutex);
	if(nextOperationId + 1 == (unsigned) -1){
		nextOperationId = 0;
		pthread_mutex_unlock(&operationIdMutex);
		return 0;
	}
	unsigned tmp = nextOperationId++;
	pthread_mutex_unlock(&operationIdMutex);
	return tmp;
}

}
}
