#include "TransportModule.h"
#include<map>
#include<pthread.h>


using srch2::instantsearch;
using srch2::httpwrapper;

void* startListening(void* map) {
  const Node& base =  ((RouteMap*) map)->getBase();

  hostent *routeHost = gethostbyname(base.getIpAddress().c_str());
  //  if(routeHost == -1) throw std::exception
  struct socketaddr_in routeAddress;

  memset(&routeAddress, 0, sizeof(routeAddress));
  routeAddress.sin_family = AF_INET;
  memcpy(&routeAddress.sin_addr, routeHost->h_addr, host->h_length);
  routeAddress.sin_post = htons(base.getPortNumber());

  if(int fd = socket(AF_INET, SOCKET_STREAM, 0) < 0) {
    perror("listening socket failed to bind");
    exit(255);
  }

  while(1) {
    struct sockaddr addr;
    socklen_t addrlen;
    if((newfd = accept(fd, &addr, &addrlen)) != -1) {
      RouteMap::acceptRoute(newfd, addr);
     }
     if(sendMessage(newfd, true));
     recieveMessage 
    }
  }

  return NULL;
}

TransportManager::TransportManger(EventBases& bases, Nodes& map) {
  for(Nodes::iterator dest = map.begin(); dest!= map.end(); ++dest) {
    if(dest->thisIsMe) 
      routeMap.setBase(**dest);
    else 
      routeMap.addDestination(**dest);
  }
  
  pthread_create(&listeningThread, NULL, startListening, routeMap);

  for(Connections::iterator dest = routeMap.getNeeded.begin(); 
        dest != needed.end(); ++dest) {
      if(!(*dest)->second) {
        routeMap.initRoute(*dest);
      } else {
        ++i;
      }
    }

    sleep(10);
  }

  for(RouteMap::iterator route = routeMap.begin; route != routeMap.end(); 
      ++route) {
    for(EventBases::iterator base = bases->begin(); 
        base != bases->end(); ++base) {
      struct event* ev = event_new(*base, route->second, 
          EV_READ, broker_cb, routeManager.broker);
      event_add(ev, NULL);
    }
  }
}
