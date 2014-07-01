#ifndef __SHARDING_ROUTING_MESSAGE_RESOLVER_INTERFACE_H__
#define __SHARDING_ROUTING_MESSAGE_RESOLVER_INTERFACE_H__


#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/Message.h"
#include "sharding/routing/ResponseAggregator.h"
#include "core/util/Assert.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "RoutingUtil.h"
#include <vector>
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {


/*
 * We need this class to be father of PendingRequest because
 * PendingRequestHandler needs polymorphism to store all PendingRequest objects.
 */
class MessageResolverInterface{
public:
    // resolves the corresponding PendingMessage with this response
    // if returns true, this pendingRequest is ready to be deleted for finalizing.
    virtual bool resolveMessage(Message * responseMessage, NodeId nodeIdOfResponse, void * responseObject) = 0;
    // this function looks for all messages in this request that have timed out and must be deleted from pendingMessages
    // returns true if it's time for this request to be deleted.
    virtual bool resolveTimedoutMessages() = 0;
    // returns true of it contains a PendingMessage corresponding to this responseMessage
    virtual bool isMessageMine(Message * responseMessage) = 0;
    virtual ~MessageResolverInterface(){};
};


}
}

#endif // __SHARDING_ROUTING_MESSAGE_RESOLVER_INTERFACE_H__
