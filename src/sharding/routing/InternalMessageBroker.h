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

	InternalMessageBroker(RoutingManager&  routingManager)
		this->routingManager;
	}
	void processInternalMessage(Message * message);
	Srch2Server * getShardIndex(ShardId & shardId);


private:
  template<InputType, Deserializer, OutputType>
    void broker(Message *msg, Srch2Server* server,
        OutputType* (*DpInternalMessage::fn) (Srch2Server*, InputType*));
  std::allocator<char> getMessageAllocator()
  sendReply(Message*, void*);
 
	RoutingManager&  routingManager;
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
