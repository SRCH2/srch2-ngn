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
	pendingMessagesHandler.setTransportManager(this);
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

	if (routeMap.begin()!= routeMap.end()) {
		unsigned currNodeSocketReadBuffer;
		unsigned size = sizeof(unsigned);
		getsockopt(routeMap.begin()->second.fd, SOL_SOCKET, SO_RCVBUF, &socketReadBuffer,
				&size);
		getsockopt(routeMap.begin()->second.fd, SOL_SOCKET, SO_SNDBUF, &socketSendBuffer,
						&size);
		Logger::console("SO_RCVBUF = %d, SO_SNDBUF = %d", socketReadBuffer, socketSendBuffer);
	}

	distributedTime = 0;
	synchManagerHandler = 0;
}

void * routeInternalMessage(void * tmAndMsg) {
	MessageAndTMPointers * pointers = (MessageAndTMPointers *)tmAndMsg;
	TransportManager * tm = pointers->tm;
	Message * msg = pointers->message;

	if(msg->isInternal()) {
		if(msg->isNoReply()){
			// this msg comes from a local broadcast or route with no call back
			// so there is no reply for this msg.
			// notifyNoReply should deallocate the obj in msg
			// because msg just keeps a pointer to that object
			// and that object itself needs to be deleted.
			tm->getInternalTrampoline()->notifyNoReply(msg);
			// and delete the msg
			tm->getMessageAllocator()->deallocateByMessagePointer(msg);
		}
		Message* reply = tm->getInternalTrampoline()->notifyWithReply(msg);
		ASSERT(reply != NULL);
		if(reply != NULL) {
			reply->setRequestMessageId(msg->getMessageId());
			reply->setReply()->setInternal();
			tm->getPendingMessagesHandler()->resolveResponseMessage(reply);
		}
		if(reply == NULL){
			Logger::console("Reply is null");
		}
		// what if resolve returns NULL for something?
	} else {
		tm->getSmHandler()->notifyWithReply(msg);
	}
	delete pointers;
}

MessageID_t TransportManager::route(NodeId node, Message *msg, 
		unsigned timeout, CallbackReference callback) {

	if(msg == NULL){
		Logger::console("Trying to send NULL message in TM route(node,msg)");
		return 0;
	}
	if(! msg->isSMRelated()){
		Logger::console("Message is being sent through TM route(node,msg). Msg type is %d", msg->getType());
	}

	// we use a clock for the IDs of Messages
	msg->setMessageId( __sync_fetch_and_add(&distributedTime, 1));

	// TODO VARIABLE IS NOT USED
	time_t timeOfTimeout_time = timeout + time(NULL);
	// only messages which expect reply will go to pending messages
	if(! msg->isNoReply()){
		pendingMessagesHandler.addPendingMessage(timeout, msg->getMessageId(), callback);
	}

	if(msg->isLocal()) {
		Logger::console("Message is local");
		MessageAndTMPointers * pointers = new MessageAndTMPointers(this, msg);
		pthread_t internalMessageRouteThread;
		if (pthread_create(&internalMessageRouteThread, NULL, routeInternalMessage, pointers) != 0){
			Logger::console("Cannot create thread for handling local message");
			return 255; // TODO: throw exception.
		}
		return msg->getMessageId();
	}

	Connection conn = routeMap.getConnection(node);

	while(!__sync_bool_compare_and_swap(&conn.sendLock, false, true));
	MessageID_t returnValue = route(conn.fd, msg);
	conn.sendLock = true;
	return returnValue;
}

/*
 *   This function is internal to Transport layer. All external modules should use
 *   <code>route(NodeId node, Message *msg, unsigned timeout, CallbackReference callback)</code>
 *   Note: The function is not thread safe. Caller should ensure thread safety.
 */

MessageID_t TransportManager::route(int fd, Message *msg) {
	if(msg == NULL){
		Logger::console("Trying to send NULL message in TM route(fd,msg)");
		return 0;
	}
	if(! msg->isSMRelated()){
		Logger::console("Message is being sent through TM route(fd,msg). Msg type is %d", msg->getType());
	}
	msg->setMessageId( __sync_fetch_and_add(&distributedTime, 1));

	/*
	 *  This flag makes sure that we do get SIGPIPE signal when other end
	 *  of the socket is closed. Use return value of send function to
	 *  handle the error.
	 */
#ifdef __MACH__
	int flag = SO_NOSIGPIPE;
#else
	int flag = MSG_NOSIGNAL;
#endif

	unsigned totalMessageSize = msg->getBodySize() + sizeof(Message);
	char * buffer = (char * ) msg;
	unsigned retryCount = 5;  // TODO : retryCount
	while(retryCount) {
		ssize_t writeSize = send(fd, buffer, totalMessageSize, flag);
		if(writeSize == -1){
			perror("Message sending fails in route(fd,msg)");
			break;  // TODO: V1 take more actions
		}else if(writeSize !=  totalMessageSize){
			totalMessageSize -= writeSize;
			buffer += writeSize;
			Logger::console("Message not sent completely through TM route(fd,msg). Msg type is %d and size is %d",
								msg->getType(), msg->getBodySize());
			/*
			 *  Prepare data structure for select system call.
			 *  http://man7.org/linux/man-pages/man2/select.2.html
			 */
			fd_set writeSet;
			timeval waitTimeout;
			waitTimeout.tv_sec = 1;
			waitTimeout.tv_usec = 0;
			FD_ZERO(&writeSet);
			FD_SET(fd, &writeSet);
			/*
			 *   Wait until timeout= 1sec or Socket is ready for write. (whichever occurs first)
			 */
			int result = select(fd+1, NULL, &writeSet, NULL, &waitTimeout);
			if (result == -1) {
				perror("error while waiting for write to socket");
			}
			// TODO: temporary for debugging...remove later
			if (result == 1) {
				Logger::console("socket ready for write");
			}
		}else{
			if(! msg->isSMRelated()){
				Logger::console("Success");
			}
			break;
		}
		--retryCount;
	}

	if (retryCount == 0)
		Logger::console("TM was unable to write complete message with max try. "
				"Remaining size of the message is %d", totalMessageSize);

	return msg->getMessageId();
}

MessageID_t& TransportManager::getDistributedTime() {
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

PendingMessagesHandler * TransportManager::getPendingMessagesHandler() {
	return &pendingMessagesHandler;
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
