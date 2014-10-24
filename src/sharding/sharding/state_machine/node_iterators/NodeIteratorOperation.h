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
	virtual ~OrderedNodeIteratorOperation();

	Transaction * getTransaction();

	OperationState * entry();
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	OperationState * handle(SP(Notification) n);

	OperationState * handle(SP(ShardingNotification) notif);

	OperationState * handle(SP(NodeFailureNotification) notif);

	void setParticipants(vector<NodeId> & participants);

	bool validateResponse(SP(ShardingNotification) response);

	string getOperationName() const ;

	string getOperationStatus() const ;



private:
	ShardingMessageType resType;
	SP(ShardingNotification) request;

	vector<NodeOperationId> participants;
	unsigned participantsIndex;

	/*
	 * Validator class must provide
	 * bool validateResponse(Request * req, Response * res);
	 * which is called after each response.
	 * it must also provide
	 * void finalize(void *);
	 * which is called when all participants are iterated.
	 * Also, it must provide void abort(int error_code) in case this operation exits due to error
	 */
	OrderedNodeIteratorListenerInterface * validatorObj;

	OperationState * askNode(const unsigned nodeIndex);

	void updateParticipantsList(vector<NodeId> newParticipants);

};

}
}


#endif // __SHARDING_SHARDING_NODE_ITERATOR_OPERATION_H__
