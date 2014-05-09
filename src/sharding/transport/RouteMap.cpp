#include "RouteMap.h"
#include <netdb.h>
#include "ConnectionsInlines.h"
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>

using namespace srch2::httpwrapper;

Route& RouteMap::addDestination(const Node& node) {
	hostent *routeHost = gethostbyname(node.getIpAddress().c_str());
	//  if(routeHost == -1) throw std::exception
	struct sockaddr_in routeAddress;

	memset(&routeAddress, 0, sizeof(routeAddress));
	routeAddress.sin_family = AF_INET;
	memcpy(&routeAddress.sin_addr, routeHost->h_addr, routeHost->h_length);
	routeAddress.sin_port = htons(node.getPortNumber());

	destinations.push_back(std::pair<ConnectionInfo, bool>(
			ConnectionInfo(routeAddress, node.getId()), false));

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

struct RouteMapAndRouteHandle {
	RouteMap*const routeMap;
	Route*const route;
	pthread_t connectingRouteThread;

	RouteMapAndRouteHandle(RouteMap &routeMap, Route& dest) : routeMap(&routeMap), route(&dest) {}
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

/*
 * This function uses the routeMap to connect to other nodes in the cluster
 */
void* tryToConnect(void *arg) {
	RouteMapAndRouteHandle *routeMapAndRouteHandle = (RouteMapAndRouteHandle*) arg;

	while(!routeMapAndRouteHandle->routeMap->nodeConnectionMap.count(routeMapAndRouteHandle->route->first.second)) {
		sleep(5);

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0) continue;

		if(connect(fd, (struct sockaddr*) &routeMapAndRouteHandle->route->first.first,
				sizeof(routeMapAndRouteHandle->route->first.first)) == -1) {
			close(fd);
			continue;
		}

		while(!routeMapAndRouteHandle->routeMap->nodeConnectionMap.count(routeMapAndRouteHandle->route->first.second)) {
			if(!__sync_bool_compare_and_swap(&routeMapAndRouteHandle->route->second, false,true)) {
				continue;
			}
			break;
		}

		if(routeMapAndRouteHandle->routeMap->nodeConnectionMap.count(routeMapAndRouteHandle->route->first.second)) break;

		if(!sendGreeting(fd, true, routeMapAndRouteHandle->routeMap->getCurrentNode().getId()) ||
				recieveGreeting(fd) == -1) {
			routeMapAndRouteHandle->route->second = false;
			continue;
		}

		fcntl(fd, F_SETFL, O_NONBLOCK);
		routeMapAndRouteHandle->routeMap->addNodeConnection(routeMapAndRouteHandle->route->first.second, fd);
	}
	delete routeMapAndRouteHandle;
	return NULL;
}

void RouteMap::initRoutes() {
	// iterate over all destinations and initialize all routes one by one
	for(std::vector<Route >::iterator
			destination = destinations.begin(); destination != destinations.end();
			++destination) {
		initRoute(*destination);
	}
}

void RouteMap::initRoute(Route& route) {
	// create the handle to routemap and route and pass it to thread
	RouteMapAndRouteHandle *rNm = new RouteMapAndRouteHandle(*this, route);
	// TODO : where should we deallocate this routemap ? never?
	pthread_create(&rNm->connectingRouteThread, NULL, tryToConnect, rNm);
}

void RouteMap::acceptRoute(int fd, struct sockaddr_in addr) {
	Route * path = NULL;
	unsigned nodeId;
	if((nodeId = recieveGreeting(fd)) == -1) {
		close(fd);
		return;
	}
	for(std::vector<Route >::iterator
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
		// if(routeMap.count(path->first.second)) {
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
	//look into routeMap thread safety
	nodeConnectionMap[addr] = Connection(fd);
}

bool RouteMap::isTotallyConnected() const {
	// if it's completely connected, nodeConnectionMap should have the same
	// number of connections as destinations map
	return nodeConnectionMap.size() == destinations.size();
}

Connections RouteMap::getNeededConnections() {
	return Connections(destinations);
}

Connection RouteMap::getConnection(NodeId nodeId) {
	return nodeConnectionMap[nodeId];
}


void RouteMap::setCurrentNode(Node& currentNode) { this->currentNode = &currentNode; }
const Node& RouteMap::getCurrentNode() const { return *currentNode; }

RouteMap::iterator RouteMap::begin() { return nodeConnectionMap.begin(); }
RouteMap::iterator RouteMap::end() { return nodeConnectionMap.end(); }

