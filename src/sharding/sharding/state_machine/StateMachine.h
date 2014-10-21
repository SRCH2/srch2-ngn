#ifndef __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
#define __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__

#include "State.h"
#include "../notifications/Notification.h"

#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class NodeIteratorListenerInterface : public BottomUpDeleteInterface{
public:
	virtual ~NodeIteratorListenerInterface(){};
	virtual void start(){	}
	virtual void end_(map<NodeOperationId, ShardingNotification * > & _replies = map<NodeOperationId, ShardingNotification * >){
		// by default, the second end is used by implementors of this interface
		map<NodeId, ShardingNotification * > replies;
		for(map<NodeOperationId, ShardingNotification * >::iterator nodeOpItr = _replies.begin();
				nodeOpItr != _replies.end(); ++ nodeOpItr){
			replies[nodeOpItr->first.nodeId] = nodeOpItr->second;
		}
		end(replies);
	}
	virtual void end(map<NodeId, ShardingNotification * > & replies){

	}
	// if returns true, operation must stop and return null to state_machine
	virtual bool shouldAbort(const NodeId & failedNode){
		return false;
	}
};

class OrderedNodeIteratorListenerInterface: public NodeIteratorListenerInterface{
public:
	virtual ~OrderedNodeIteratorListenerInterface(){};
	// This method, is called after each response is retreived and before we move to the next participant,
	// if it returns false, the iterator will abort. (still, getTransIdToDelete() will be called after.)
	virtual bool condition(ShardingNotification * req, ShardingNotification * res){
		return true;
	}
};

class StateMachine{
public:

	void registerOperation(OperationState * operation);
	// if TID already exists in the map, it returns false.
	// otherwise it saves it and returns true
	bool registerTransaction(Transaction * transaction);
	void removeTransaction(TRANS_ID);
	// goes to srcOpId target
	void handle(ShardingNotification * notification);
	// goes to everybody
	void handle(Notification * notification);

	void print() const;

private:
	map<unsigned, OperationState *> activeOperations;

	//
	map<TRANS_ID, Transaction * > activeTransactions;

	bool addActiveOperation(OperationState * operation);

	void startOperation(OperationState * operation);
	void stateTransit(OperationState * operation, OperationState * nextState);

};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_OPERATION_CONTAINER_H__
