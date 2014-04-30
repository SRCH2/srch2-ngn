#include "TransportManager.h"
#include<map>
#include<sys/socket.h>
#include<sys/types.h>


using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void cb_recieveMessage(int fd, short eventType, void *arg);

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

  if(fd = socket(AF_INET, SOCK_STREAM, 0) < 0) {
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
          EV_READ, cb_recieveMessage, NULL);
      event_add(ev, NULL);
    }
  }

  distributedTime = 0;
}

MessageTime_t TransportManager::route(Message *msg) {
  Connection fd = routeMap.getConnection(msg->shard.coreId);
  msg->time = __sync_fetch_and_add(&distributedTime, 1);

  send(fd, msg, msg->bodySize + sizeof(Message), 0);
  //TODO: errors?
  return msg->time;
}

#include<unistd.h>
#include<errno.h>

bool findNextMagicNumberAndReadMessageHeader(Message *const msg,  int fd) {
  while(true) {
    int readRtn = read(fd, (void*) msg, sizeof(Message));

    if(readRtn == -1) {
      if(errno == EAGAIN || errno == EWOULDBLOCK) return false;

      //v1: handle  error
      return false;
    }

    if(readRtn < sizeof(Message)) {
      //v1: broken message boundary 
      continue;
    }

    //TODO:checkMagicNumbers

    return true;
  }
}

Message* readRestOfMessage(std::allocator<char> messageAllocator,
    int fd, Message *const msgHeader) {
  char *buffer= messageAllocator.allocate(msgHeader->bodySize);

  int readReturnValue = read(fd, buffer, msgHeader->bodySize);

  if(readReturnValue != msgHeader->bodySize); //handle error

  Message* msg = (Message*) (buffer - sizeof(msgHeader));
  memcpy(msg, msgHeader, sizeof(Message));

  return msg;
}
  


void cb_recieveMessage(int fd, short eventType, void *arg) {
  TransportManager* tm = (TransportManager*) arg;
  Message msgHeader;
  if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)) return;

  while(true) {
    MessageTime_t time = tm->distributedTime;
    //check if time needs to be incremented
    if(msgHeader.time < time && 
        /*zero break*/ time - msgHeader.time < UINT_MAX/2 ) break;
    //make sure time did not change
    if(__sync_bool_compare_and_swap(
          &tm->distributedTime, time, msgHeader.time)) break;
  }

  Message *msg = 
    readRestOfMessage(tm->messageAllocator, fd, &msgHeader);
/*
  if(msg.isReply()) tm->pendingMessages.resolve(*msg);
  elseif(msg.isSM() {
    sm->notify(*msg);
  } else {
   // tm->routeManager.trampoline(*msg);
  }
*/

  tm->messageAllocator.deallocate(msg);
}
  
