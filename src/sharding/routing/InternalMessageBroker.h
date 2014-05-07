#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "sharding/processor/DistributedProcessorInternal.h"
#include "sharding/transport/Message.h"
#include "sharding/transport/CallbackHandler.h"
#include "RoutingManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class InternalMessageBroker : CallBackHandler {
public:

	InternalMessageBroker(RoutingManager& rm, DPInternalRequestHandler& internalDP) : internalDP(internalDP), routingManager(rm) {};

	/*
	 * Gets the internal message and routes it to one of the DPInternal functions
	 */
	Message* notify(Message*);

	/*
	 * This function gets the index for a particular shard
	 */
	Srch2Server* getShardIndex(ShardId&);
	MessageAllocator * getMessageAllocator();

private:

	DPInternalRequestHandler& internalDP;
	RoutingManager&  routingManager;
	template<typename InputType, typename Deserializer, typename OutputType>
	Message* broker(Message*, Srch2Server*, OutputType (DPInternalRequestHandler::*fn) (Srch2Server*, InputType*));
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
