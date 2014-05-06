#ifndef __BROADCAST_INLINES_H__
#define __BROADCAST_INLINES_H__

#include "transport/PendingMessages.h"

template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_wait_for_all_w_cb(RequestType & requestObj,
			ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator, 
      CoreShardInfo & coreInfo) {
  Multiplexer broadcastResolver(configurationManager, coreInfo);
  CallbackReference cb =
    tm.registerCallback(requestObj, 
        new RMCallback<RequestType, ResponseType>(aggregator),
        RequestType::messageKind, false, broadcastResolver.size());
  Message* msg = (Message*) 
    ((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));
  for(UnicastIterator unicast = broadcastResolver.begin(); 
      unicast != broadcastResolver.end(); ++unicast) {
    msg->shard = unicast->shardId;
    tm.route(unicast->nodeId, msg, 0, cb);
  }
  getMessageAllocator().deallocateMessage(msg);
}

#endif /* __BROADCAST_INLINES_H__ */
