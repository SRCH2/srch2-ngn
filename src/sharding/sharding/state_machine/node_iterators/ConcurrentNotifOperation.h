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
