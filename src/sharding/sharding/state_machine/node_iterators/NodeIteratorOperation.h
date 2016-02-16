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
#ifndef __SHARDING_SHARDING_NODE_ITERATOR_OPERATION_H__
#define __SHARDING_SHARDING_NODE_ITERATOR_OPERATION_H__

#include "../State.h"
#include "../../notifications/Notification.h"
#include "../../../configuration/ShardingConstants.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Note : Whatever request is chosen, ShardManager must provide
 * void resolve(Request * req);
 */
class OrderedNodeIteratorOperation : public OperationState {
public:

	OrderedNodeIteratorOperation(SP(ShardingNotification) request, ShardingMessageType resType,
			vector<NodeId> & participants, OrderedNodeIteratorListenerInterface * validatorObj = NULL);
	virtual ~OrderedNodeIteratorOperation(){};

	OperationState * entry();
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	OperationState * handle(SP(Notification) n);

	OperationState * handle(SP(ShardingNotification) notif);

	OperationState * handle(SP(NodeFailureNotification) notif);

	OperationState * handle(SP(TimeoutNotification) notif);


	string getOperationName() const ;

	string getOperationStatus() const ;



private:
	ShardingMessageType resType;
	SP(ShardingNotification) request;

	map<NodeOperationId , SP(ShardingNotification)> targetResponsesMap;
	vector<NodeOperationId> participants;
	unsigned participantsIndex;

	/*
	 * Validator class must provide
	 * bool condition(Request * req, Response * res);
	 * which is called after each response.
	 * it must also provide
	 * void finalize(void *);
	 * which is called when all participants are iterated.
	 * Also, it must provide void abort(int error_code) in case this operation exits due to error
	 */
	OrderedNodeIteratorListenerInterface * validatorObj;

	OperationState * finalize();

	OperationState * askNode(const unsigned nodeIndex);

	void appendParticipants(vector<NodeId> newParticipants);
	void setParticipants(vector<NodeId> & participants);

};

}
}


#endif // __SHARDING_SHARDING_NODE_ITERATOR_OPERATION_H__
