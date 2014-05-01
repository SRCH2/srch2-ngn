#include "TransportManager.h"
#include<map>
#include<sys/socket.h>
#include<sys/types.h>


using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void* startListening(void* arg) {
  RouteMap *const map = (RouteMap*) arg;
  const Node& base =  map->getBase();

  hostent *routeHost = gethostbyname(base.getIpAddress().c_str());
  //  if(routeHost == -1) throw std::exception
  struct sockaddr_in routeAddress;
  int fd;

  memset(&routeAddress, 0, sizeof(routeAddress));
  routeAddress.sin_family = AF_INET;
  memcpy(&routeAddress.sin_addr, routeHost->h_addr, routeHost->h_length);
  routeAddress.sin_port = htons(base.getPortNumber());

  if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("listening socket failed to bind");
    exit(255);
  }

  while(1) {
    struct sockaddr addr;
    socklen_t addrlen;
    int newfd;
    if((newfd = accept(fd, &addr, &addrlen)) != -1) {
      map->acceptRoute(newfd, *((sockaddr_in*) &addr));
     }
  }

  return NULL;
}

#include "callback_functions.h"

TransportManager::TransportManager(EventBases& bases, Nodes& map) {
  for(Nodes::iterator dest = map.begin(); dest!= map.end(); ++dest) {
    if(dest->thisIsMe) 
      routeMap.setBase(*dest);
    else 
      routeMap.addDestination(*dest);
  }
  
  pthread_create(&listeningThread, NULL, startListening, &routeMap);

  for(Connections dest = routeMap.getNeededConnections(); 
        routeMap.isTotallyConnected(); ++dest) {
    if(!(*dest).second) routeMap.initRoute(*dest);
    sleep(10);
  }

  for(RouteMap::iterator route = routeMap.begin(); route != routeMap.end(); 
      ++route) {
    for(EventBases::iterator base = bases.begin(); 
        base != bases.end(); ++base) {
      struct event* ev = event_new(*base, route->second, 
          EV_READ, cb_recieveMessage, this);
      event_add(ev, NULL);
    }
  }

  distributedTime = 0;
}

MessageTime_t TransportManager::route(NodeId node, Message *msg, 
    unsigned timeout, unsigned callback) {
  Connection fd = routeMap.getConnection(node);
  msg->time = __sync_fetch_and_add(&distributedTime, 1);

  time_t timeOfTimeout_time = timeout + time(NULL);
  //pending_message( timeOfTimeout, msg->time, callback #)

  send(fd, msg, msg->bodySize + sizeof(Message), 0);
  //TODO: errors?
  return msg->time;
}
