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
#ifndef __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
#define __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__

#include "State.h"
#include "../notifications/Notification.h"
#include "ConsumerProducer.h"

#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

#define ACTIVE_OPERATINS_GROUP_COUNT 100
class StateMachine{
public:

	StateMachine();

	~StateMachine();

	/*
	 * This is NOT a thread entry point
	 */
	void registerOperation(OperationState * operation);

	/*
	 * This is a thread entry point
	 * Goes to dest operation
	 */
	void handle(SP(ShardingNotification) notification);

	/*
	 * This is a thread entry point
	 */
	// goes to everybody
	void handle(SP(Notification) notification);

	void print(JsonResponseHandler * response = NULL) const;
	bool lockStateMachine();
	void unlockStateMachine();

	void clear();

private:

	struct ActiveOperationGroup{
		boost::mutex contentMutex;
		map<unsigned , SP(OperationState)> activeOperations;
		bool addActiveOperation(OperationState * operation);
		bool deleteActiveOperation(const unsigned operationId);
		SP(OperationState) getActiveOperation(const unsigned operationId);
		void getAllActiveOperations(map<unsigned , SP(OperationState)> & activeOperations);
		unsigned size();
		void clear(bool shouldLock = true);

	};

	ActiveOperationGroup activeOperationGroups[ACTIVE_OPERATINS_GROUP_COUNT];


	void lockOperationGroup(unsigned opid);
	void unlockOperationGroup(unsigned opid);
	ActiveOperationGroup & getOperationGroup(unsigned opid);
};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
