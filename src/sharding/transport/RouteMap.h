#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H__

#include "configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>
#include <iterator>

namespace srch2 {
namespace httpwrapper {

const static char GREETING_MESSAGE[] = "GREETING FROM SRCH2";
const static char FAILED_GREETING_MESSAGE[] = "YOU KNOCKED AGAIN? ";

typedef unsigned NodeId;
typedef int Connection;
typedef std::pair<sockaddr_in, NodeId> ConnectionId;

using srch2::httpwrapper::Node;

class Connections : public std::iterator<forward_iterator_tag,
  std::pair<ConnectionId, bool> > {

public:
  typedef std::vector<std::pair<ConnectionId, bool> > Pool;

  Connections(Pool&);
  Connections(const Connections&);

  Connections& operator++();
  Connections operator++(int);

  bool operator==(const Connections&);
  bool operator!=(const Connections&);

  std::pair<ConnectionId, bool>& operator*();
  std::pair<ConnectionId, bool>* operator->();

private:

  Pool& pool;
  Pool::iterator place;
};


class RouteMap {
  std::map<NodeId, Connection> map; 
  std::vector<std::pair<ConnectionId, bool> > destinations;
  const Node* base;

  void addNodeConnection(NodeId, int);
public:

  bool initRoute(std::pair<ConnectionId, bool>&);
  void acceptRoute(int fd, struct sockaddr_in);
  void addDestination(const Node&);
  bool isTotallyConnected() const;
  Connections getNeededConnections();
  void setBase(Node&);
  const Node& getBase() const;
  Connection getConnection(NodeId);

  typedef std::map<NodeId, Connection>::iterator iterator;
  iterator begin();
  iterator end();
};

#include "ConnectionsInlines.h"

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

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
