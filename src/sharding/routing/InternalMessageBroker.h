#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "processor/DistributedProcessorInternal.h"
#include "transport/Message.h"
#include "RoutingManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class InternalMessageBroker{
public:

	InternalMessageBroker(RoutingManager&);
	void processInternalMessage(Message*);
	Srch2Server* getShardIndex(ShardId&);


private:
  template<typename InputType, typename Deserializer, typename OutputType>
    void broker(Message*, Srch2Server*,
        OutputType (DPInternalRequestHandler::*fn) (Srch2Server*, InputType*));
  MessageAllocator * getMessageAllocator();
  void sendReply(Message*, void*);
  
  DPInternalRequestHandler& internalDP;
	RoutingManager&  routingManager;
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
