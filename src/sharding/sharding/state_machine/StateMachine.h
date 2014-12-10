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
