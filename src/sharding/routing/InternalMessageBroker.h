#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "sharding/processor/DistributedProcessorInternal.h"
#include "transport/Message.h"
#include "transport/MessageAllocator.h"
#include "sharding/transport/CallbackHandler.h"

namespace srch2is = srch2::instantsearch;
using namespace std;


namespace srch2 {
namespace httpwrapper {

class RoutingManager;

class InternalMessageBroker : public CallBackHandler {
public:

    InternalMessageBroker(RoutingManager& rm, DPInternalRequestHandler& internalDP) : internalDP(internalDP), routingManager(rm) {};


    void resolveMessage(Message * msg, NodeId node);

    void deleteResponseObjectBasedOnType(Message * reply, void * responseObject);

    /*
     * This function gets the index for a particular shard
     */
    Srch2Server* getShardIndex(ShardId&);
    MessageAllocator * getMessageAllocator();

private:

    DPInternalRequestHandler& internalDP;
    RoutingManager&  routingManager;

    template<typename RequestType, typename ResponseType>
    std::pair<Message*,ResponseType*> processRequestMessage(Message*, Srch2Server*,
            ResponseType * (DPInternalRequestHandler::*fn) (Srch2Server*, RequestType*));

    std::pair<Message*,CommandStatus*> processRequestInsertUpdateMessage(Message *msg, Srch2Server* server, const Schema * schema);

    /*
     * Gets the internal message and routes it to one of the DPInternal functions
     */
    std::pair<Message*,void*> notifyReturnResponse(Message*);
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
