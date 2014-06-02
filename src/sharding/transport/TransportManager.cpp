#include "TransportManager.h"
#include<map>
#include<sys/socket.h>
#include<sys/types.h>
#include <core/util/Assert.h>


using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

// TODO : move this code to RouteMap.cpp although it's temporary
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
   const int optVal = 1;
   setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, sizeof(optVal));

	if(bind(fd, (struct sockaddr*) &routeAddress, sizeof(routeAddress)) < 0) {
      close(fd);
		perror("listening socket failed to bind");
		exit(255);
	}

	if(listen(fd, 20) == -1) {
      close(fd);
		perror("listening socket failed start");
		exit(255);
	}

   routeMap->setListeningSocket(fd);

   fd_set checkConnect;
   timeval timeout;
	while(!routeMap->isTotallyConnected()) {
      //prevent infinite hanging in except so listening socket closes
      FD_ZERO(&checkConnect);
      FD_SET(fd, &checkConnect);
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      if(select(fd+1, &checkConnect, NULL, NULL, &timeout) !=1) continue;

		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(sockaddr_in);
		memset(&addr, 0,sizeof(sockaddr_in));
		int newfd;
		if((newfd = accept(fd, (sockaddr*) &addr, &addrlen)) != -1) {
			routeMap->acceptRoute(newfd, *((sockaddr_in*) &addr));
		}
	}

   shutdown(fd, SHUT_RDWR);
   close(fd);
   return NULL;
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
	// TODO in V1, this routeMap moves to SM
	routeMap.initRoutes();

	while(!routeMap.isTotallyConnected()) {
		sleep(3);
	}

	//close(routeMap.getListeningSocket());
	Logger::console("Connected");


	// RouteMap iterates over routes. Routes are std::map<NodeId, Connection>
	// which is basically NodeId to file descriptor
	// We bound the route file descriptors (connection to other nodes) to event bases
	// that are bound to cb_recieveMessage. This way cb_recieveMessage receives all internal messages
	for(RouteMap::iterator route = routeMap.begin(); route != routeMap.end(); ++route) {
		for(EventBases::iterator base = bases.begin(); base != bases.end(); ++base) {
			TransportCallback *cb_ptr = new TransportCallback();
			struct event* ev = event_new(*base, route->second.fd, EV_READ, 	cb_recieveMessage, cb_ptr);
			new (cb_ptr) TransportCallback(this, &route->second, ev, *base);
			event_add(ev, NULL);
		}
	}

	distributedTime = 0;
	synchManagerHandler = 0;
}

MessageID_t TransportManager::route(NodeId node,Message * msg,
		unsigned timeout) {

	if(msg == NULL){
		Logger::console("Trying to send NULL message in TM route(node,msg)");
		return 0;
	}
	if(! msg->isSMRelated()){
		Logger::console("Message is being sent through TM route(node,msg). Msg type is %d", msg->getType());
	}

	// local messages do not reach to this point.
	ASSERT(! msg->isLocal());

	Connection conn = routeMap.getConnection(node);

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	unsigned returnStatus = send(conn.fd, msg, msg->getBodySize() + sizeof(Message), flag);
	//TODO: errors?
	if(returnStatus == -1){
		perror("Message sending fails in route(node,msg)");
	}else if(returnStatus !=  msg->getBodySize() + sizeof(Message)){
		Logger::console("Message not sent completely through TM route(node,msg). Msg type is %d", msg->getType());
	}else{
		if(! msg->isSMRelated()){
			Logger::console("Success");
		}
	}

//	/*
//	 * If we don't wait for reply we should delete the msg here
//	 */
//	if(msg->isNoReply()){
//		getMessageAllocator()->deallocateByMessagePointer(msg);
//	}//TODO for the case of noReply ...

	return msg->getMessageId();
}

MessageID_t TransportManager::route(int fd, Message * msg) {
	if(msg == NULL){
		Logger::console("Trying to send NULL message in TM route(fd,msg)");
		return 0;
	}
	if(! msg->isSMRelated()){
		Logger::console("Message is being sent through TM route(fd,msg). Msg type is %d", msg->getType());
	}

#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	ssize_t returnStatus = send(fd, msg, msg->getBodySize() + sizeof(Message), flag);
	//TODO: errors?
	if(returnStatus == -1){
		perror("Message sending fails in route(fd,msg)");
	}else if(returnStatus !=  msg->getBodySize() + sizeof(Message)){
		Logger::console("Message not sent completely through TM route(fd,msg). Msg type is %d", msg->getType());
	}else{
		if(! msg->isSMRelated()){
			Logger::console("Success");
		}
	}


	return msg->getMessageId();
}

MessageID_t& TransportManager::getDistributedTime() {
	return distributedTime;
}

MessageID_t TransportManager::getUniqueMessageIdValue(){
	return __sync_fetch_and_add(&distributedTime, 1);
}

CallBackHandler* TransportManager::getRmHandler() {
	return routeManagerHandler;
}

pthread_t TransportManager::getListeningThread() const {
	return listeningThread;
}

MessageAllocator * TransportManager::getMessageAllocator() {
	return &messageAllocator;
}

RoutingManager * TransportManager::getRoutingManager(){
	return this->routingManager;
}

void TransportManager::setRoutingManager(RoutingManager * rm){
	this->routingManager = rm;
}

RouteMap * TransportManager::getRouteMap() {
	return &routeMap;
}

CallBackHandler* TransportManager::getSmHandler() {
	return synchManagerHandler;
}


TransportManager::~TransportManager() {
//TODO:bind threads
   for(RouteMap::iterator i = routeMap.begin(); i != routeMap.end(); ++i) {
      close(i->second.fd);
   }
   close(routeMap.getListeningSocket());
}
