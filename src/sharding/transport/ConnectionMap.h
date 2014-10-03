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

namespace srch2 {
namespace httpwrapper {

const static char GREETING_MESSAGE[] = "SUCCESS";
const static char FAILED_GREETING_MESSAGE[] = "FAILURE";

struct NodeInfo {
	unsigned nodeId;
	unsigned ipaddress;
	unsigned communicationPort;
	char nodeName[1024];
};

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
  Connection():fd(-1), sendLock(false), nodeId(-1){}
};

typedef std::pair<sockaddr_in, NodeId> ConnectionInfo;

class ConnectionMap {
	// this map is from nodeID to Connection class
	std::map<NodeId, Connection> nodeConnectionMap;
	Node currentNode;
	// destinations and nodeConnectionMap are changed by multiple threads
	mutable boost::shared_mutex _access;
public:
	void setCurrentNode(Node& node) { currentNode = node; }
	Node& getCurrentNode() { return currentNode; }
	void addNodeConnection(NodeId, int);
	Connection& getConnection(NodeId);
	bool isConnectionExist(NodeId);
	void acceptConnection(int fd, NodeId destinationNodeId);
	typedef std::map<NodeId, Connection>::iterator iterator;
	iterator begin();
	iterator end();
};

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
