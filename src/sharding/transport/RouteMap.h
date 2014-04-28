#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H_

#include "configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>

const static char GREETING_MESSAGE[] = "GREETING FROM SRCH2";
const static char FAILED_GREETING_MESSAGE[] = "YOU KNOCKED AGAIN? ";

typedef unsigned NodeId;
typedef int Connection;
typedef std::pair<sockaddr_in, NodeId> ConnectionId;

using srch2::httpwrapper::Node;

struct Connections : std::iterator<forward_iterator_tag,
  std::pair<ConnectionId, bool>&> {
    Connections(std::vector<std::pair<ConnectionId, bool> >&);


class RouteMap {
  typedef std::map<NodeId, Connection> Iterator;
  std::map<NodeId, Connection> map; 
  std::vector<std::pair<ConnectionId, bool> > destinations;

public:
  bool initRoute(std::pair<ConnectionId, bool>&);
  void acceptRoute(int fd, struct sockaddr_in);
  void addNodeConnection(const NodeId, const Connection);
  void addDestination(const Node&);
  bool isConnected(const sockaddr_in &addr);
  Connections getNeededConnections();
};


#endif /* __TRANSPORT_ROUTE_MAP_H__ */
