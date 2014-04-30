#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "sharding/processor/DistributedProcessorInternal.h"
#include "Message.h"
#include "RoutingManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class InternalMessageBroker{
public:

	InternalMessageBroker(RoutingManager * routingManager){
		this->routingManager;
	}
	void processInternalMessage(Message * message);
	Srch2Server * getShardIndex(ShardId & shardId);


	RoutingManager * getRoutingManager(){
		return routingManager;
	}

private:

	RoutingManager * routingManager;
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
