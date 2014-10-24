#ifndef __SHARDING_SHARDING_CONSUMER_PRODUCER_H__
#define __SHARDING_SHARDING_CONSUMER_PRODUCER_H__

#include "../notifications/CommandStatusNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class Transaction;

class ConsumerInterface{
public:
	virtual ~ConsumerInterface(){};
	virtual void consume(bool booleanResult){};
	virtual void consume(const vector<string> & rejectedPKs){};
	virtual void consume(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {};
	virtual void consume(const ShardMigrationStatus & migrationStatus){};
	virtual Transaction * getTransaction() = 0;
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

class ProducerConsumerInterface : public ConsumerInterface, public ProducerInterface{
public:
	ProducerConsumerInterface(ConsumerInterface * consumer):ProducerInterface(consumer){	}
	Transaction * getTransaction(){
		return this->getConsumer()->getTransaction();
	}
};

class NodeIteratorListenerInterface : public ConsumerInterface{
public:
	virtual ~NodeIteratorListenerInterface(){};
	virtual void start(){	}
	virtual void end_(map<NodeOperationId, SP(ShardingNotification) > & _replies){
		// by default, the second end is used by implementors of this interface
		map<NodeId, SP(ShardingNotification) > replies;
		for(map<NodeOperationId, SP(ShardingNotification) >::iterator nodeOpItr = _replies.begin();
				nodeOpItr != _replies.end(); ++ nodeOpItr){
			replies[nodeOpItr->first.nodeId] = nodeOpItr->second;
		}
		end(replies);
	}
	virtual void end(){
		map<NodeId, SP(ShardingNotification) > replies;
		end(replies);
	}
	virtual void end(map<NodeId, SP(ShardingNotification) > & replies){

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
	virtual bool condition(SP(ShardingNotification) req, SP(ShardingNotification) res, vector<NodeId> & updatedListOfParticipants){
		return true;
	}
};

}
}

#endif // __SHARDING_SHARDING_CONSUMER_PRODUCER_H__
