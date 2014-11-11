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

#define ACTIVE_OPERATINS_GROUP_COUNT 1000
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

	void print() const;
	void lockStateMachine();
	void unlockStateMachine();

private:
	vector<pair<boost::recursive_mutex *, map<unsigned, OperationState *> > > activeOpertationGroups;

	bool addActiveOperation(OperationState * operation);

	void startOperation(OperationState * operation);
	void stateTransit(OperationState * operation, OperationState * nextState);

	void lockOperationGroup(unsigned opid);
	void unlockOperationGroup(unsigned opid);
	map<unsigned, OperationState *> & getOperationGroup(unsigned opid);
};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
