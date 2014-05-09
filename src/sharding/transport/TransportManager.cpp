#include "TransportManager.h"
#include<map>
#include<sys/socket.h>
#include<sys/types.h>


using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void* startListening(void* arg) {
	RouteMap *const map = (RouteMap*) arg;
	const Node& currentNode =  map->getCurrentNode();

	hostent *routeHost = gethostbyname(currentNode.getIpAddress().c_str());
	//  if(routeHost == -1) throw std::exception
	struct sockaddr_in routeAddress;
	int fd;

	memset(&routeAddress, 0, sizeof(routeAddress));
	routeAddress.sin_family = AF_INET;
	memcpy(&routeAddress.sin_addr, routeHost->h_addr, routeHost->h_length);
	routeAddress.sin_port = htons(currentNode.getPortNumber());

	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("listening socket failed to init");
		exit(255);
	}

	if(bind(fd, (struct sockaddr*) &routeAddress, sizeof(routeAddress)) < 0) {
		perror("listening socket failed to bind");
		exit(255);
	}

	if(listen(fd, 20) == -1) {
		perror("listening socket failed start");
		exit(255);
	}

	while(!map->isTotallyConnected()) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(sockaddr_in);
		memset(&addr, 0,sizeof(sockaddr_in));
		int newfd;
		if((newfd = accept(fd, (sockaddr*) &addr, &addrlen)) != -1) {
			map->acceptRoute(newfd, *((sockaddr_in*) &addr));
		}
	}

  close(fd);
  Logger::console("Connected");
}

#include "callback_functions.h"

TransportManager::TransportManager(EventBases& bases, Nodes& nodes) {
	// for each node we have, if it's us just store out event base
	// otherwise it stores the node as a destination
	for(Nodes::iterator dest = nodes.begin(); dest!= nodes.end(); ++dest) {
		if(dest->thisIsMe)
			routeMap.setCurrentNode(*dest);
		else
			routeMap.addDestination(*dest);
	}

	pthread_create(&listeningThread, NULL, startListening, &routeMap);

	// initializes the connections to other nodes using tryToConnect
	routeMap.initRoutes();

	while(!routeMap.isTotallyConnected()) {
		sleep(10);
	}

	// RouteMap iterates over routes. Routes are std::map<NodeId, Connection>
	// which is basically NodeId to file descriptor
	// We bound the route file descriptors (connection to other nodes) to event bases
	// that are bound to cb_recieveMessage. This way cb_recieveMessage receives all internal messages
	for(RouteMap::iterator route = routeMap.begin(); route != routeMap.end(); ++route) {
		for(EventBases::iterator base = bases.begin(); base != bases.end(); ++base) {
			struct event* ev = event_new(*base, route->second.fd,
					EV_READ|EV_PERSIST, cb_recieveMessage, new TransportCallback(this, &route->second));
			event_add(ev, NULL);
		}
	}

	distributedTime = 0;
}

MessageTime_t TransportManager::route(NodeId node, Message *msg, 
		unsigned timeout, CallbackReference callback) {
	Connection conn = routeMap.getConnection(node);
	msg->time = __sync_fetch_and_add(&distributedTime, 1);

	time_t timeOfTimeout_time = timeout + time(NULL);
	pendingMessages.addMessage(timeout, msg->time, callback);

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	send(conn.fd, msg, msg->bodySize + sizeof(Message), flag);
	//TODO: errors?

	return msg->time;
}

MessageTime_t TransportManager::route(int fd, Message *msg) {
	msg->time = __sync_fetch_and_add(&distributedTime, 1);

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	send(fd, msg, msg->bodySize + sizeof(Message), flag);
	//TODO: errors?

	return msg->time;
}

MessageTime_t& TransportManager::getDistributedTime() {
	return distributedTime;
}

CallBackHandler* TransportManager::getInternalTrampoline() {
	return internalTrampoline;
}

pthread_t TransportManager::getListeningThread() const {
	return listeningThread;
}

MessageAllocator * TransportManager::getMessageAllocator() {
	return &messageAllocator;
}

PendingMessages * TransportManager::getMsgs() {
	return &pendingMessages;
}

RouteMap * TransportManager::getRouteMap() {
	return &routeMap;
}

CallBackHandler* TransportManager::getSmHandler() {
	return synchManagerHandler;
}


//TODO:: TransportManager::~TransportManager() {
  //bind threads
  //close ports
//}
