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


class ShardManager;
class RoutingManager;

class InternalMessageHandler : public CallBackHandler {
public:

    InternalMessageHandler(RoutingManager& rm, DPInternalRequestHandler& internalDP)
    : internalDP(internalDP), routingManager(rm){
    	shardManager = NULL;
    };


    bool resolveMessage(Message * msg, NodeId node);

    void deleteResponseObjectBasedOnType(Message * reply, void * responseObject);

    MessageAllocator * getMessageAllocator();

    void setShardManager(ShardManager * shardManager){
    	this->shardManager = shardManager;
    }

private:

    DPInternalRequestHandler& internalDP;
    RoutingManager&  routingManager;
    ShardManager * shardManager;

    template<typename RequestType, typename ResponseType>
    std::pair<Message*,ResponseType*> processRequestMessage(Message*, boost::shared_ptr<Srch2Server> srch2Server ,
            ResponseType * (DPInternalRequestHandler::*fn) (boost::shared_ptr<Srch2Server> srch2Server , RequestType*));

    std::pair<Message*,CommandStatus*> processRequestInsertUpdateMessage(Message *msg, boost::shared_ptr<Srch2Server> srch2Server , const Schema * schema);

    /*
     * Gets the internal message and routes it to one of the DPInternal functions
     */
    std::pair<Message*,void*> notifyDPInternalReturnResponse(Message*);

    Message* notifyShardManagerReturnResponse(Message*, NodeId node);
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
