#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H__

#include "sharding/configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>
#include <iterator>
#include <map>
#include "MessageBuffer.h"

void* tryToConnect(void*);

namespace srch2 {
namespace httpwrapper {

const static char GREETING_MESSAGE[] = "GREETING FROM SRCH2";
const static char FAILED_GREETING_MESSAGE[] = "YOU KNOCKED AGAIN? ";

typedef unsigned NodeId;

class Connection {
public:
  int fd;
  MessageBuffer buffer;

  Connection(int fd) : fd(fd) {}
  Connection() {}
};

typedef std::pair<sockaddr_in, NodeId> ConnectionId;
//typedef ConnectionBuffers std::map<int, Message* msg, Buffer>

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

#include "ConnectionsInlines.h"

class RouteMap {
  std::map<NodeId, Connection> map; 
  std::vector<std::pair<ConnectionId, bool> > destinations;
  const Node* base;

  void addNodeConnection(NodeId, int);
public:

  void initRoutes();
  void initRoute(std::pair<ConnectionId, bool>&);
  void acceptRoute(int fd, struct sockaddr_in);
  std::pair<ConnectionId, bool>& addDestination(const Node&);
  bool isTotallyConnected() const;
  Connections getNeededConnections();
  void setBase(Node&);
  const Node& getBase() const;
  Connection getConnection(NodeId);

  typedef std::map<NodeId, Connection>::iterator iterator;
  iterator begin();
  iterator end();

  friend void* ::tryToConnect(void*);
};

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
