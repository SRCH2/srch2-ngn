/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
	virtual void consume(bool status, map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {};
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
//	// if returns true, operation must stop and return null to state_machine
//	// True is retunrned by the caller of this module if this failedNode must abort
//	// the task. By defaul this method does not need to be overridden because it returns
//	// false by default.
//	virtual bool shouldAbort(const NodeId & failedNode, unsigned callerId){
//		return shouldAbort(failedNode);
//	}
//	virtual bool shouldAbort(const NodeId & failedNode){
//		return false;
//	}
};

class OrderedNodeIteratorListenerInterface: public NodeIteratorListenerInterface{
public:
	virtual ~OrderedNodeIteratorListenerInterface(){};
	// This method, is called after each response is retrieved and before we move to the next participant,
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
