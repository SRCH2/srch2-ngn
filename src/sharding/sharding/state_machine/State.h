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
#ifndef __SHARDING_SHARDING_STATE_H__
#define __SHARDING_SHARDING_STATE_H__

#include "../ShardManager.h"
#include "../notifications/Notification.h"
#include "server/HTTPJsonResponse.h"
#include "ConsumerProducer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * This class provides the interface of the state of one operation
 */
class OperationState{
public:

	OperationState(unsigned operationId):operationId(operationId){
		transaction.reset();
		finalizedFlag = false;
	}

	// NOTE : OperationState must always be in locked state before destruction
	//        so that destruction starts in safe
	virtual ~OperationState(){
	};

	virtual OperationState * entry() = 0;
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	virtual OperationState * handle(SP(Notification) n) = 0;


	virtual string getOperationName() const {return "operation name";};
	virtual string getOperationStatus() const {return "operation status";};

	unsigned getOperationId() const;
	void setOperationId(unsigned operationId) ;
	void setFinalized(){finalizedFlag = true;};
	bool isFinalized() const {return finalizedFlag;};
	void send(SP(ShardingNotification) notification, const NodeOperationId & dest) const;

	SP(Transaction) getTransaction();
	void setTransaction(SP(Transaction) sp);
	void lock();
	void unlock();
	static void initOperationStateStaticEnv();
	static unsigned getNextOperationId();
	static const unsigned DataRecoveryOperationId;

private:
	unsigned operationId;
	static unsigned nextOperationId;
	static pthread_mutex_t operationIdMutex;
	SP(Transaction) transaction;
	boost::mutex operationContentLock;
	bool finalizedFlag;
};

}
}

#endif // __SHARDING_SHARDING_STATE_H__
