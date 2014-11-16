#ifndef __SHARDING_SHARDING_CONSUMER_PRODUCER_H__
#define __SHARDING_SHARDING_CONSUMER_PRODUCER_H__

#include "../notifications/CommandStatusNotification.h"
#include "../transactions/Transaction.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class Transaction;

class ConsumerInterface{
public:
	virtual ~ConsumerInterface(){};
	virtual void consume(){};
	virtual void consume(const ClusterPID & pid){};
	virtual void consume(bool booleanResult){};
	virtual void consume(const vector<string> & rejectedPKs){};
	virtual void consume(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {};
	virtual void consume(const ShardMigrationStatus & migrationStatus){};
	virtual void consume(bool booleanResult, const ClusterPID & pid){};
	virtual void consume(const map<string, bool> & recordResults){};
	virtual void consume(const map<string, bool> & results,
			map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){};
	virtual void consume(bool status, const vector<unsigned> & searchableAttributeIds,
			const vector<unsigned> & refiningAttributeIds,
			const vector<JsonMessageCode> & messages){};
	virtual void consume(bool booleanResult, vector<JsonMessageCode> & messageCodes){};
	virtual void consume(unsigned coreId, bool booleanResult, SP(Json::Value) jsonResponse ,
			const vector<JsonMessageCode> & messageCodes, const vector<string> & customMessageStrings){};
	virtual SP(Transaction) getTransaction() = 0;
	virtual string getName() const = 0;
};

class ProducerInterface{
public:
	ProducerInterface(ConsumerInterface * consumer){
		ASSERT(consumer != NULL);
		this->consumer = consumer;
	}
	virtual ~ProducerInterface(){};

	virtual void produce() = 0;

	ConsumerInterface * getConsumer(){
		return consumer;
	}

private:
	ConsumerInterface * consumer;
};

class NodeIteratorListenerInterface : public ConsumerInterface{
public:
	virtual ~NodeIteratorListenerInterface(){};
	virtual void start(){	}


	/*
	 * end functions are callback functions that consumer can override to receive the
	 * result of node iteration.
	 * NOTE : each class which implements this interface must override one and exactly one
	 *        end function from this list.
	 */
	virtual void end_(map<NodeOperationId, SP(ShardingNotification) > & _replies, unsigned callerId){
		// by default, the second end is used by implementors of this interface
		map<NodeId, SP(ShardingNotification) > replies;
		for(map<NodeOperationId, SP(ShardingNotification) >::iterator nodeOpItr = _replies.begin();
				nodeOpItr != _replies.end(); ++ nodeOpItr){
			replies[nodeOpItr->first.nodeId] = nodeOpItr->second;
		}
		end_(_replies);
		end(callerId);
		end();
		end(replies, callerId);
		end(replies);
	}
	virtual void end_(map<NodeOperationId, SP(ShardingNotification) > & _replies){
	}
	virtual void end(unsigned callerId){
	}
	virtual void end(){
	}
	virtual void end(map<NodeId, SP(ShardingNotification) > & replies, unsigned callerId){

	}
	virtual void end(map<NodeId, SP(ShardingNotification) > & replies){

	}
	// if returns true, operation must stop and return null to state_machine
	// True is retunrned by the caller of this module if this failedNode must abort
	// the task. By defaul this method does not need to be overridden because it returns
	// false by default.
	virtual bool shouldAbort(const NodeId & failedNode, unsigned callerId){
		return shouldAbort(failedNode);
	}
	virtual bool shouldAbort(const NodeId & failedNode){
		return false;
	}
};

class OrderedNodeIteratorListenerInterface: public NodeIteratorListenerInterface{
public:
	virtual ~OrderedNodeIteratorListenerInterface(){};
	// This method, is called after each response is retreived and before we move to the next participant,
	// if it returns false, the iterator will abort. (still, getTransIdToDelete() will be called after.)
	virtual bool condition(SP(ShardingNotification) req,
			SP(ShardingNotification) res,
			vector<NodeId> & updatedListOfParticipants){
		return true;
	}
};

}
}

#endif // __SHARDING_SHARDING_CONSUMER_PRODUCER_H__
