#include "TransportModule.h"
/*
// Adds node to list of active connections. Returns false iff node already 
// has an active connection
bool Connections::addNode(struct NodeAddr& node, int fd) {
  const struct NodeList oldNodeList(alloc, nodes);
  
  NodeList::Iterator insertLocation = 
    std::lower_bound(oldNodeList.first(), oldNodeList.end(), node);

  if(*insertLocation == node) return false;

  {
    //single insertion constructor
    struct NodeList newList(alloc, oldList, insertLocation, node);

#ifdef __GNUC__
    if(__sync_bool_compare_and_swap(oldNodeList, nodes, newList)) 
      return; 
  } //deallocate constructor
    addNode(node, fd);
#endif

  return true;
}

bool TransportModule::connect(struct NodeAddr node) {
  if(connections[node]) return true;

  int fd = socket(SOCKET_STREAM, AF_INET, 0);
  if(fd < 0) return false;

  socketaddr_in addressOfNode;
  addressOfNode.sin_family = AF_INET;
  addressOfNode.sin_port = htons(node.port);
  inet_pton(AF_INET, &node.ipaddress, &addressOfNode.sin_addr);

  if( connect(fd, &addressOfNode, sizeof(socketAddr_in)) == -1) return false;

  
  if(connections.addNode(node, fd) == false) 
    close(fd); //already bound to Node no need to open again

  return true;
}
*/
#include<map>
#include<pthread.h>

struct

typedef NodeId unsigned;
typedef Connection int;
typedef ConnectionId std::pair<socketaddr, NodeId>;

using srch2::instantsearch;
using srch2::httpwrapper;

class RouteMap {
  typedef Iterator std::map<NodeId, Connection>;
  std::map<NodeId, Connection> map; 
  std::forward_list<std::pair<ConnectionId>, bool> 
    destinations;

public:
  initRoute(const Node&);
  addNodeConnection(const NodeId, const Connection);
  isConnected(const socketaddr_in &addr);
};

void RouteMap::addDestination(const Node& node) {
  hostent *routeHost = gethostbyname(node.getIpAddress().c_str());
  //  if(routeHost == -1) throw std::exception
  struct socketaddr_in routeAddress;

  memset(&routeAddress, 0, sizeof(routeAddress));
  routeAddress.sin_family = AF_INET;
  memcpy(&routeAddress.sin_addr, routeHost->h_addr, host->h_length);
  routeAddress.sin_post = htons(node.getPortNumber());

  destination.add(std::pair<ConnectionId>, bool>(
        ConnectionId(socketaddr_in, node.getId()), false);
}

bool recieveMessage(int fd) {
  char greetings[sizeof(GREETING_MESSAGE)];
  memset(greeting, 0, sizeof(greetings));

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

void sendMessage(bool greeted) {
  char greetings[sizeof(GREETING_MESSAGE)];
  memset(greeting, 0, sizeof(greetings));

  char *currentPos = greetings;
  int remaining = sizeof(GREETING_MESSAGE);
  memcpy(greeting, (greeted) ? GREETING_MESSAGE : FAILED_GREETING_MESSAGE,
      sizeof(GREETING_MESSAGE));
  while(true) {
    int writeSize = send(fd, currentPos, sizeof(GREETING_MESSAGE));
    remaining -= writeSize;
    if(wrtieSize == -1) {
      close(fd);
      return false;
    }
    if(remaining == 0) {
     if (!strcmp(greetings, GREETING_MESSAGE)) 
      break;
     close(fd);
     return false;
    }
    currentPos += writeSize;
  }
 
}

bool RouteMap::initRoute(std::pair<ConnectionId>, AtomicBool>& route) {
  if(route.second) return true;

  int fd = socket(AF_INSET, SOCK_STREAM, SOCK_NONBLOCK);
  if(fd < 0) return false; 

  if(connect(fd, (struct sockaddr*) &routeAddress, sizeof(routeAddress)))
    return false;

  if(route.second.compareAndSet(false, true)) return false;

  bool greeted = recievingGreeting(fd);

  sendMessage(greeted);
  if(!greeted)
    return route.second = false;

  addNodeConnection(route.first.second, fd);

  return true;
}
 



TransportManager::TransportManger(EventBases& bases, Nodes& map) {
  const Node& basePosition;

  for(Nodes::iterator dest = map.begin(); dest!= map.end(); ++dest) {
    if(dest->thisIsMe) basePosition= **dest;
    else 
      routeMap.addDestination(**dest);
  }

  startListening(basePosition, routeMap);

  typedef std::forward_list<std::pair<ConnectionId, bool>*> Connections;
  Connections needed = routeMap.notConnected();
  while(!routeMap.full()) {
    for(Connections::iterator dest = needed.begin(); 
        dest != needed.end(); ++dest) {
      if(!(*dest)->second) 
        routeMap.initRoute(*dest);
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
