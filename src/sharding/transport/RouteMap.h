#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H__

#include "sharding/configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>
#include <iterator>
#include <map>
#include "MessageBuffer.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

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
typedef std::pair<sockaddr_in, NodeId> ConnectionInfo;
typedef std::pair<ConnectionInfo, bool> Route;
//typedef ConnectionBuffers std::map<int, Message* msg, Buffer>

using srch2::httpwrapper::Node;

class Connections : public std::iterator<forward_iterator_tag,
Route > {

public:
	typedef std::vector<Route > RoutePool;

	Connections(RoutePool&);
	Connections(const Connections&);

	Connections& operator++();
	Connections operator++(int);

	bool operator==(const Connections&);
	bool operator!=(const Connections&);

	Route& operator*();
	Route* operator->();

private:

	RoutePool& routePool;
	RoutePool::iterator place;
};

#include "ConnectionsInlines.h"

class RouteMap {
	// this map is from nodeID to Connection
	// Connection right now is just the file descriptor
	std::map<NodeId, Connection> nodeConnectionMap;
	// ConnectionId is the information of one connection to another node
	// bool is whether it's been connection or not so initially it's false
	std::vector<Route > destinations;
	const Node* currentNode;
	mutable boost::shared_mutex _access;
	void addNodeConnection(NodeId, int);
public:

	void initRoutes();
	void initRoute(Route&);
	void acceptRoute(int fd, struct sockaddr_in);
	/*
	 * Adds the connection to this node to the destinations map
	 * this function does not actually connect, it only stores the destination information in
	 * destinations map
	 */
	Route& addDestination(const Node&);
	bool isTotallyConnected() const;
	Connections getNeededConnections();
	void setCurrentNode(Node&);
	const Node& getCurrentNode() const;
	Connection getConnection(NodeId);


	typedef std::map<NodeId, Connection>::iterator iterator;
	iterator begin();
	iterator end();

	friend void* ::tryToConnect(void*);
};

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
