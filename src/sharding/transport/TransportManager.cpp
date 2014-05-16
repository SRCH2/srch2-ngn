#include "TransportManager.h"
#include<map>
#include<sys/socket.h>
#include<sys/types.h>
#include <core/util/Assert.h>


using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void* startListening(void* arg) {
	RouteMap *const routeMap = (RouteMap*) arg;
	const Node& currentNode =  routeMap->getCurrentNode();

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

        routeMap->setListeningSocket(fd);

	while(!routeMap->isTotallyConnected()) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(sockaddr_in);
		memset(&addr, 0,sizeof(sockaddr_in));
		int newfd;
		if((newfd = accept(fd, (sockaddr*) &addr, &addrlen)) != -1) {
			routeMap->acceptRoute(newfd, *((sockaddr_in*) &addr));
		}
	}

        return NULL;
}

#include "callback_functions.h"

TransportManager::TransportManager(EventBases& bases, Nodes& nodes) {
	pendingMessages.setTransportManager(this);
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
		sleep(3);
	}

	close(routeMap.getListeningSocket());
	Logger::console("Connected");


	// RouteMap iterates over routes. Routes are std::map<NodeId, Connection>
	// which is basically NodeId to file descriptor
	// We bound the route file descriptors (connection to other nodes) to event bases
	// that are bound to cb_recieveMessage. This way cb_recieveMessage receives all internal messages
	for(RouteMap::iterator route = routeMap.begin(); route != routeMap.end(); ++route) {
		for(EventBases::iterator base = bases.begin(); base != bases.end(); ++base) {
      TransportCallback *cb_ptr = new TransportCallback();
			struct event* ev = event_new(*base, route->second.fd, EV_READ, 
          cb_recieveMessage, cb_ptr);
      new (cb_ptr) TransportCallback(this, &route->second, ev, *base);
			event_add(ev, NULL);
		}
	}

	distributedTime = 0;
}

#define USE_SAME_THREAD_FOR_CURRENT_NODE_PROCESS

MessageTime_t TransportManager::route(NodeId node, Message *msg, 
		unsigned timeout, CallbackReference callback) {
	msg->setTime( __sync_fetch_and_add(&distributedTime, 1));

	time_t timeOfTimeout_time = timeout + time(NULL);
	// only messages which expect reply will go to pending messages
	if(! msg->isNoReply()){
		pendingMessages.addMessage(timeout, msg->getTime(), callback);
	}

#ifdef USE_SAME_THREAD_FOR_CURRENT_NODE_PROCESS
	if(msg->isLocal()) {
		MessageTime_t rtn = msg->getTime();
		if(msg->isInternal()) {
			if(msg->isNoReply()){
				// this msg comes from a local broadcast or route with no call back
				// so there is no reply for this msg.
				// notifyNoReply should deallocate the obj in msg
				// because msg just keeps a pointer to that object
				// and that object itself needs to be deleted.
				getInternalTrampoline()->notifyNoReply(msg);
				// and delete the msg
				getMessageAllocator()->deallocateByMessagePointer(msg);
				return rtn;
			}
			Message* reply = getInternalTrampoline()->notifyWithReply(msg);
			ASSERT(reply != NULL);
			if(reply != NULL) {
				reply->setInitialTime(msg->getTime());
				reply->setReply()->setInternal();
				getMsgs()->resolve(reply);
			}
			// what if resolve returns NULL for something?
		} else {
			getSmHandler()->notifyWithReply(msg);
		}
		return rtn;
	}
#endif


	Connection conn = routeMap.getConnection(node);

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	send(conn.fd, msg, msg->getBodySize() + sizeof(Message), flag);
	//TODO: errors?

	/*
	 * If we don't wait for reply we should delete the msg here
	 */
	if(msg->isNoReply()){
		getMessageAllocator()->deallocateByMessagePointer(msg);
	}
	return msg->getTime();
}

MessageTime_t TransportManager::route(int fd, Message *msg) {
	msg->setTime( __sync_fetch_and_add(&distributedTime, 1));

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	send(fd, msg, msg->getBodySize() + sizeof(Message), flag);
	//TODO: errors?

	return msg->getTime();
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
