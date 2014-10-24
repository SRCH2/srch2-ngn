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


class StateMachine{
public:

	void registerOperation(OperationState * operation);
	// goes to srcOpId target
	void handle(SP(ShardingNotification) notification);
	// goes to everybody
	void handle(SP(Notification) notification);

	void print() const;

private:
	map<unsigned, OperationState *> activeOperations;

	bool addActiveOperation(OperationState * operation);

	void startOperation(OperationState * operation);
	void stateTransit(OperationState * operation, OperationState * nextState);

};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
