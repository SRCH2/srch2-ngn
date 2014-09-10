#ifndef __SHARDING_SHARDING_STATE_H__
#define __SHARDING_SHARDING_STATE_H__

#include "ShardManager.h"
#include "./notifications/Notification.h"

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

	OperationState(unsigned operationId):operationId(operationId){}

	virtual ~OperationState(){};

	virtual OperationState * entry() = 0;
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	virtual OperationState * handle(Notification * n) = 0;

	virtual string getOperationName() const {return "operation name";};
	virtual string getOperationStatus() const {return "operation status";};

	unsigned getOperationId() const;
	void setOperationId(unsigned operationId) ;

	void send(ShardingNotification * notification, const NodeOperationId & dest) const;

	// what's returned doesn't need to be started but it might be NULL
	static OperationState * startOperation(OperationState * op);

	static void stateTransit(OperationState * & currentState, Notification * notification);
	static unsigned getNextOperationId();
private:
	unsigned operationId;
	static unsigned nextOperationId;
};

}
}

#endif // __SHARDING_SHARDING_STATE_H__
