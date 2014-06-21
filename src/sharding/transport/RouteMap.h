#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H__

#include "sharding/configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>
#include <iterator>
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "Message.h"

void* tryToConnect(void*);
void* startListening(void*);

namespace srch2 {
namespace httpwrapper {

const static char GREETING_MESSAGE[] = "GREETING FROM SRCH2";
const static char FAILED_GREETING_MESSAGE[] = "YOU KNOCKED AGAIN? ";

class MessageBuffer {
  public:
    Message* msg;
    bool lock;
    int readCount;
    MessageBuffer() : msg(NULL), lock(false), readCount(0) {}
};

class Connection {
public:
  int fd;
  MessageBuffer buffer;
  bool sendLock;
  NodeId nodeId;

  Connection(int fd, NodeId nodeId) : fd(fd), sendLock(false), nodeId(nodeId) {}
  Connection():fd(-1),sendLock(false) {}

};
typedef std::pair<sockaddr_in, NodeId> ConnectionInfo;
typedef std::pair<ConnectionInfo, bool> Route;

using srch2::httpwrapper::Node;

// TODO : This code will change for V1.
//  a) - Change RouteMap to Discovery
//  b) - Move to SM
class RouteMap {
	// this map is from nodeID to Connection
	// Connection right now is just the file descriptor
	std::map<NodeId, Connection> nodeConnectionMap;
	// ConnectionId is the information of one connection to another node
	// bool is whether it's been connection or not so initially it's false
	std::vector<Route > destinations;
	const Node* currentNode;

	// destinations and nodeConnectionMap are changed by multiple threads
	mutable boost::shared_mutex _access;
	void addNodeConnection(NodeId, int);
	int listeningSocket;
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
	void setCurrentNode(const Node *);
	const Node& getCurrentNode() const;
	Connection& getConnection(NodeId);
	bool checkInMap(NodeId);


	typedef std::map<NodeId, Connection>::iterator iterator;
	iterator begin();
	iterator end();
	int getListeningSocket() const;
	void setListeningSocket(int);

	friend void* ::tryToConnect(void*);
};

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
