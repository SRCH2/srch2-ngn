#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>
#include "Message.h"
#include "MessageAllocator.h"
#include "PendingMessages.h"

namespace srch2 {
namespace httpwrapper {

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

struct SMCallBackHandler {
  void notify(Message *msg); 
};

struct TransportManager {
  struct RouteMap routeMap;
  pthread_t listeningThread;
  MessageTime_t distributedTime;
  MessageAllocator messageAllocator;
  SMCallBackHandler *smHandler;
  PendingMessages msgs;


  TransportManager(EventBases&, Nodes&);
  
  //third argument is a timeout in seconds
  MessageTime_t route(NodeId, Message*, unsigned=0, CallbackReference= 
      CallbackReference());
  void register_callbackhandler_for_sm(SMCallBackHandler*);
  CallbackReference registerCallback(void*,void*,
      ShardingMessageType,bool,int = 1);
};

inline void TransportManager::register_callbackhandler_for_sm(SMCallBackHandler
    *callBackHandler) {
  smHandler = callBackHandler;
}

inline CallbackReference TransportManager::registerCallback(void* obj,void* cb,
      ShardingMessageType type,bool all,int shards) {
  return msgs.registerCallback(obj, cb, type, all, shards);
}

}}

#endif /* __TRANSPORT_MANAGER_H__ */
