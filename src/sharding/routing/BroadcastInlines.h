#ifndef __BROADCAST_INLINES_H__
#define __BROADCAST_INLINES_H__

#include "transport/PendingMessages.h"
#include "sharding/routing/RoutingManager.h"


namespace srch2 {
namespace httpwrapper {

template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_wait_for_all_w_cb(RequestType & requestObj,
		ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
		CoreShardInfo & coreInfo) {

	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(requestObj,
												new RMCallback<RequestType, ResponseType>(aggregator),
												RequestType::messageKind,
												false,
												broadcastResolver.size());

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;

		tm.route(unicast->nodeId, msg, 0, cb);
	}

	getMessageAllocator()->deallocateMessage(msg);
}

}
}

#endif /* __BROADCAST_INLINES_H__ */
