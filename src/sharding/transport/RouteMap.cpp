#include "RouteMap.h"
#include <netdb.h>

void RouteMap::addDestination(const Node& node) {
  hostent *routeHost = gethostbyname(node.getIpAddress().c_str());
  //  if(routeHost == -1) throw std::exception
  struct sockaddr_in routeAddress;

  memset(&routeAddress, 0, sizeof(routeAddress));
  routeAddress.sin_family = AF_INET;
  memcpy(&routeAddress.sin_addr, routeHost->h_addr, routeHost->h_length);
  routeAddress.sin_port = htons(node.getPortNumber());

  destinations.push_back(std::pair<ConnectionId, bool>(
        ConnectionId(routeAddress, node.getId()), false));
}

bool recieveGreeting(int fd) {
  char greetings[sizeof(GREETING_MESSAGE)];
  memset(greetings, 0, sizeof(greetings));

  char *currentPos = greetings;
  int remaining = sizeof(GREETING_MESSAGE);
  while(true) {
    int readSize = read(fd, currentPos, sizeof(GREETING_MESSAGE));
    remaining -= readSize;
    if(readSize == -1) {
      close(fd);
      return false;
    }
    if(remaining == 0) {
     if (!strcmp(greetings, GREETING_MESSAGE)) 
      break;
     close(fd);
     return false;
    }
    currentPos += readSize;
  }
}

void sendGreeting(int fd, bool greeted) {
  char greetings[sizeof(GREETING_MESSAGE)];
  memset(greetings, 0, sizeof(greetings));

  char *currentPos = greetings;
  int remaining = sizeof(GREETING_MESSAGE);
  memcpy(greetings, (greeted) ? GREETING_MESSAGE : FAILED_GREETING_MESSAGE,
      sizeof(GREETING_MESSAGE));
  while(true) {
    int writeSize = send(fd, currentPos, sizeof(GREETING_MESSAGE), 0);
    remaining -= writeSize;
    if(writeSize == -1) {
      close(fd);
      return;
    }
    if(remaining == 0) {
     if (!strcmp(greetings, GREETING_MESSAGE)) 
      break;
     close(fd);
     return;
    }
    currentPos += writeSize;
  }
}

bool RouteMap::initRoute(std::pair<ConnectionId, bool>& route) {
  if(route.second) return true;

  int fd = socket(AF_INET, SOCK_STREAM, SOCK_NONBLOCK);
  if(fd < 0) return false; 

  if(connect(fd, (struct sockaddr*) &route.first.first, 
        sizeof(route.first.first)))
    return false;

  if(!__sync_bool_compare_and_swap(&route.second, false,true)) return false;

  bool greeted = recieveGreeting(fd);

  if(!greeted)
    return route.second = false;

  addNodeConnection(route.first.second, fd);

  return true;
}

void RouteMap::acceptRoute(int fd, struct sockaddr_in addr) {
  std::pair<ConnectionId, bool> * path = NULL;
  for(std::vector<std::pair<ConnectionId, bool> >::iterator 
      p = destinations.begin(); p != destinations.end(); ++p) {
    path = &(*p);
  }
  if(!path) return;
  if(!__sync_bool_compare_and_swap(&path->second, false, true)) {
    sendGreeting(fd, false);
    return;
  }
  sendGreeting(fd, true);
  if(!recieveGreeting(fd)) {
    path->second = false;
    return;
  }

  addNodeConnection(path->first.second, fd);
}

void RouteMap::addNodeConnection(NodeId addr, int fd) {
  //look into map thread safety
  map[addr] = fd;
}

