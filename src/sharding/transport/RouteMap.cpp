#include "RouteMap.h"
#include <netdb.h>
#include "ConnectionsInlines.h"
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>

using namespace srch2::httpwrapper;

std::pair<ConnectionId, bool>& RouteMap::addDestination(const Node& node) {
  hostent *routeHost = gethostbyname(node.getIpAddress().c_str());
  //  if(routeHost == -1) throw std::exception
  struct sockaddr_in routeAddress;

  memset(&routeAddress, 0, sizeof(routeAddress));
  routeAddress.sin_family = AF_INET;
  memcpy(&routeAddress.sin_addr, routeHost->h_addr, routeHost->h_length);
  routeAddress.sin_port = htons(node.getPortNumber());

  destinations.push_back(std::pair<ConnectionId, bool>(
        ConnectionId(routeAddress, node.getId()), false));

  return destinations.back();
}

int recieveGreeting(int fd) {
  char greetings[sizeof(GREETING_MESSAGE)+sizeof(int)+1];
  memset(greetings, 0, sizeof(greetings)+sizeof(int));

  char *currentPos = greetings;
  int remaining = sizeof(GREETING_MESSAGE) + sizeof(int);
  while(true) {
    int readSize = read(fd, currentPos, remaining);
    remaining -= readSize;
    if(readSize == -1) {
      if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
      close(fd);
      return -1;
    }
    if(remaining == 0) {
     if(!memcmp(greetings, GREETING_MESSAGE, sizeof(GREETING_MESSAGE))) 
      break;
     close(fd);
     return -1;
    }
    currentPos += readSize;
  }
  return *((int*)(greetings + sizeof(GREETING_MESSAGE)));
}

typedef std::pair<ConnectionId, bool> Route;

struct RouteAndMap {
  RouteMap*const map;
  Route*const route;
  pthread_t connectingRouteThread;

  RouteAndMap(RouteMap &map, Route& dest) : map(&map), route(&dest) {}
};

bool sendGreeting(int fd, bool greeted, unsigned nodeId) {
  char greetings[sizeof(GREETING_MESSAGE)+sizeof(int)];
  memset(greetings, 0, sizeof(greetings));

  char *currentPos = greetings;
  int remaining = sizeof(GREETING_MESSAGE)+sizeof(int);
  memcpy(greetings, (greeted) ? GREETING_MESSAGE : FAILED_GREETING_MESSAGE,
      sizeof(GREETING_MESSAGE));
  *((int*)(greetings + sizeof(GREETING_MESSAGE))) = nodeId;
  while(true) {
#ifdef __MACH__
	  int flag = SO_NOSIGPIPE;
#else
	  int flag = MSG_NOSIGNAL;
#endif
    int writeSize = send(fd, currentPos, remaining, flag);
    remaining -= writeSize;
    if(writeSize == -1) {
      if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
      close(fd);
      return false;
    }
    if(remaining == 0) {
     return true;
    }
    currentPos += writeSize;
  }
}

void* tryToConnect(void *arg) {
  RouteAndMap *map = (RouteAndMap*) arg;

  while(!map->map->map.count(map->route->first.second)) {
    sleep(5);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) continue; 
    
    if(connect(fd, (struct sockaddr*) &map->route->first.first, 
        sizeof(map->route->first.first)) == -1) {
      close(fd);
      continue;
    }
   
    while(!map->map->map.count(map->route->first.second)) {
      if(!__sync_bool_compare_and_swap(&map->route->second, false,true)) {
        continue;
      }
      break;
    }

    if(map->map->map.count(map->route->first.second)) break;
    
    if(!sendGreeting(fd, true, map->map->getBase().getId()) || 
        recieveGreeting(fd) == -1) {
      map->route->second = false;
      continue;
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);
    map->map->addNodeConnection(map->route->first.second, fd);
  }
  delete map;
  return NULL;
}

void RouteMap::initRoutes() {
  for(std::vector<std::pair<ConnectionId, bool> >::iterator 
      destination = destinations.begin(); destination != destinations.end();
      ++destination) {
    initRoute(*destination);
  }
}

void RouteMap::initRoute(std::pair<ConnectionId, bool>& route) {
  RouteAndMap *rNm = new RouteAndMap(*this, route);
  pthread_create(&rNm->connectingRouteThread, NULL, tryToConnect, rNm);
}

void RouteMap::acceptRoute(int fd, struct sockaddr_in addr) {
  std::pair<ConnectionId, bool> * path = NULL;
  unsigned nodeId;
  if((nodeId = recieveGreeting(fd)) == -1) {
    close(fd);
    return;
  }
  for(std::vector<std::pair<ConnectionId, bool> >::iterator 
      p = destinations.begin(); p != destinations.end(); ++p) {
    if(p->first.second == nodeId) {
      path = &(*p);
      break;
    }
  }
  if(!path) {
    sendGreeting(fd, false, nodeId);
    close(fd);
    return;
  }
  while(!__sync_bool_compare_and_swap(&path->second, false, true)) {
   // if(map.count(path->first.second)) {
      sendGreeting(fd, false, nodeId);
      close(fd);
      return;
   // }
  //  continue;
  }

  if(!sendGreeting(fd, true, nodeId)) {
    path->second = false;
    return;
  }


  addNodeConnection(path->first.second, fd);
}

void RouteMap::addNodeConnection(NodeId addr, int fd) {
  //look into map thread safety
  map[addr] = fd;
}

bool RouteMap::isTotallyConnected() const {
  return map.size() == destinations.size();
}

Connections RouteMap::getNeededConnections() {
  return Connections(destinations);
}

Connection RouteMap::getConnection(NodeId nodeId) {
  return map[nodeId];
}

void RouteMap::setBase(Node& base) { this->base = &base; }
const Node& RouteMap::getBase() const { return *base; }

RouteMap::iterator RouteMap::begin() { return map.begin(); }
RouteMap::iterator RouteMap::end() { return map.end(); }

