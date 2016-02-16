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
#ifndef __SHARDING_SHARDING_CONCURRENT_NOTIF_OPERATION_H__
#define __SHARDING_SHARDING_CONCURRENT_NOTIF_OPERATION_H__

#include "../State.h"
#include "../../notifications/Notification.h"
#include "../../../configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


/*
 * Notes :
 * 1. Does not deallocate requests
 * 2. Only deallocates responses if there is no Consumer
 * 3. If a node dies after returning the response, Consumer still receives the response
 */
/*
 * Consumer class must provide :
 * 1.
 * void receiveReplies(const map<NodeOperationId, Response *> & replies);
 * 2.
 * void abort(int error_code);
 */
class ConcurrentNotifOperation : public OperationState {
public:

	// used for single round trip communication
	ConcurrentNotifOperation(SP(ShardingNotification) request,
			ShardingMessageType resType,
			NodeId participant,
			NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true);
	ConcurrentNotifOperation(SP(ShardingNotification) request,
			ShardingMessageType resType,
			NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true);
	ConcurrentNotifOperation(SP(ShardingNotification) request,
			ShardingMessageType resType,
			vector<NodeId> participants,
			NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true);
	ConcurrentNotifOperation(ShardingMessageType resType,
			vector<std::pair<SP(ShardingNotification) , NodeId> > participants,
			NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true);

	virtual ~ConcurrentNotifOperation();

	OperationState * entry();
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	OperationState * handle(SP(Notification) n);

	OperationState * handle(SP(NodeFailureNotification)  notif);

	OperationState * handle(SP(TimeoutNotification)  notif);

	OperationState * handle(SP(ShardingNotification) response);

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	const ShardingMessageType resType;
	const bool expectResponse;
	vector<SP(ShardingNotification)> requests;
	vector<NodeOperationId> participants;

	// response pointers are NULL if it's a no-reply notification
	// and if this is the case, we insert this NULL value right when we sent the request
	map<NodeOperationId , SP(ShardingNotification)> targetResponsesMap;

	/*
	 * Consumer class must provide :
	 * 1.
	 * void receiveReplies(const map<NodeOperationId, Response *> & replies);
	 * 2.
	 * void abort(int error_code);
	 */
	NodeIteratorListenerInterface * consumer;

	OperationState * finalize();

	bool checkFinished();

};


}
}

#endif // __SHARDING_SHARDING_CONCURRENT_NOTIF_OPERATION_H__
