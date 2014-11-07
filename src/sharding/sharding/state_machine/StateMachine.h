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
	 * No preProcess or postProcess of transaction will be called in
	 * start.
	 */
	void registerOperation(OperationState * operation);
	// goes to srcOpId target
	void handle(SP(ShardingNotification) notification);
	// goes to everybody
	void handle(SP(Notification) notification);

	void print() const;
	void lockStateMachine();
	void unlockStateMachine();

private:
	vector<pair<boost::mutex *, map<unsigned, OperationState *> > > activeOpertationGroups;

	bool addActiveOperation(OperationState * operation);

	void startOperation(OperationState * operation);
	void stateTransit(OperationState * operation,
			OperationState * nextState, const bool shouldCallPostProcess);

	void lockOperationGroup(unsigned opid);
	void unlockOperationGroup(unsigned opid);
	map<unsigned, OperationState *> & getOperationGroup(unsigned opid);
};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
