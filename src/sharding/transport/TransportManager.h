#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

struct TransportManager {
  struct RouteMap routeMap;
  TransportManager(EventBases&, Nodes&);
  pthread_t listeningThread;
};

#endif /* __TRANSPORT_MANAGER_H__ */
