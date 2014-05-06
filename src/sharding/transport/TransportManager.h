#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>
#include "Message.h"
#include "MessageAllocator.h"
#include "PendingMessages.h"
#include "routing/InternalMessageBroker.h"


#include "CallbackHandler.h"

namespace srch2 {
namespace httpwrapper {

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

struct TransportManager {
  class RouteMap routeMap;
  pthread_t listeningThread;
  MessageTime_t distributedTime;
  MessageAllocator messageAllocator;
  PendingMessages msgs;
  CallBackHandler *smHandler;
  CallBackHandler *internalTrampoline;

  TransportManager(EventBases&, Nodes&);
  
  //third argument is a timeout in seconds
  MessageTime_t route(NodeId, Message*, unsigned=0, CallbackReference= 
      CallbackReference());
  CallbackReference registerCallback(void*,Callback*,
      ShardingMessageType,bool,int = 1);
  void register_callbackhandler_for_sm(CallBackHandler*);

};

inline void TransportManager::register_callbackhandler_for_sm(CallBackHandler
    *callBackHandler) {
  smHandler = callBackHandler;
}

inline CallbackReference TransportManager::registerCallback(void* obj,
    Callback* cb, ShardingMessageType type,bool all,int shards) {
  return msgs.registerCallback(obj, cb, type, all, shards);
}

}}

#endif /* __TRANSPORT_MANAGER_H__ */
