#include "TransportManager.h"
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <core/util/Assert.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>

using namespace srch2::instantsearch;
namespace srch2 {
namespace httpwrapper {

////
////  LIBEVENT CALLBACKS
////

/*
 *   Read Callback :  This function is called when there is data available to be read
 *   from the socket.
 */
void cb_recieveMessage(int fd, short eventType, void *arg) {
	TransportCallback* cb = (TransportCallback*) arg;
	if(recieveMessage(fd, cb)){
		event_add(cb->ev, NULL);
	}else{
		Logger::console("SEVERE: TM is not handling any event on socket %d anymore.",fd);
		//TODO: fetch the nodeId for this socket and print that in error.
	}
}

/*
 *   Write Callback :  This function is called when the socket is available for writing/sending
 *   data.
 */

void cb_sendMessage(int fd, short eventType, void *arg) {
	Logger::console("send message callback called");
}

///


/*
 *   Each notification to upstream handlers are dispatched in a new thread.
 */
void * notifyUpstreamHandlers(void *arg) {
	DisptchArguments * dispatchArgument = (DisptchArguments *)arg;
	TransportManager * tm = dispatchArgument->tm;
	Message * msg = dispatchArgument->message;

	if(msg->isReply()) {
		Logger::console("Reply message is received. Msg type is %d", msg->getType());
		tm->getPendingMessagesHandler()->resolveResponseMessage(msg);
		//tm->getMessageAllocator()->deallocateByMessagePointer(msg);

	} else if(msg->isInternal()) { // receiving a message which

		if(msg->isNoReply()){
			Logger::console("Request message with no reply is received. Msg type is %d", msg->getType());
			// This msg comes from another node and does not need a reply
			// it comes from a broadcast or route with no callback
			tm->getInternalTrampoline()->notifyNoReply(msg);
			// and delete the msg
			tm->getMessageAllocator()->deallocateByMessagePointer(msg);
		} else {
			Logger::console("Request message is received. Msg type is %d", msg->getType());
			Message* replyMessage = tm->getInternalTrampoline()->notifyWithReply(msg);
			if(replyMessage != NULL) {
				replyMessage->setRequestMessageId(msg->getMessageId());
				replyMessage->setReply()->setInternal();
				tm->_route(dispatchArgument->fd, replyMessage);
				tm->getMessageAllocator()->deallocateByMessagePointer(msg);
				tm->getMessageAllocator()->deallocateByMessagePointer(replyMessage);
			}
		}
	} else {
		// If one node is up but another one has not registered SmHandler into tm yet,
		// this check will avoid a crash.
		if (tm->getSmHandler() != NULL){
			//Logger::console("SM Request message is received. Msg type is %d", msg->getType());
			tm->getSmHandler()->notifyWithReply(msg);
		}
		tm->getMessageAllocator()->deallocateByMessagePointer(msg);
	}
	return NULL;
}

/*
 * This function reads a fixed size header from the socket stream. Each message start
 * with message header and then followed by message body.
 *
 * --------------------------------
 * | Message Header | Rest of Body |
 * ---------------------------------
 */
int readMessageHeader(Message *const msg,  int fd) {
	while(true) {
		int readRtn = recv(fd, (void*) msg, sizeof(Message), MSG_DONTWAIT);

		if(readRtn == 0) {
			return 1;
		}
		if(readRtn == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				return -1;
			}
			if(errno == ECONNREFUSED || errno == EBADF){
				return 1;
			}
			return false;
		}

		if(readRtn < sizeof(Message)) {
			//v1: broken message boundary == seriously bad
			return -1;
		}

		return 0;
	}

	//ASSERT(false);
	return -1;
}

Message* readRestOfMessage(MessageAllocator& messageAllocator,
		int fd, Message *const msgHeader, int *readCount) {
	Message *msg= messageAllocator.allocateMessage(msgHeader->getBodySize());
	int rv = recv(fd, Message::getBodyPointerFromMessagePointer(msg), msgHeader->getBodySize(), MSG_DONTWAIT);

	if(rv == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			rv = 0;
		} else {
			//v1: handle error
			return NULL;
		}
	}

	*readCount = rv;
	memcpy(msg, msgHeader, sizeof(Message));

	return msg;
}

bool readPartialMessage(int fd, MessageBuffer& buffer) {
	int toRead = buffer.msg->getBodySize() - buffer.readCount;
	if(toRead == 0) {
		//strangely we don't need to read anything;)
		return true;
	}

	int readReturnValue = recv(fd,
			Message::getBodyPointerFromMessagePointer(buffer.msg) + buffer.readCount, toRead, MSG_DONTWAIT);
	if(readReturnValue < 0) {
		//TODO: handle errors
		return false;
	}

	buffer.readCount += readReturnValue;

	return (buffer.msg->getBodySize() == buffer.readCount);
}


bool recieveMessage(int fd, TransportCallback *cb) {
	if( fd != cb->conn->fd) {
		//major error
		return false;
	}

	MessageBuffer& b = cb->conn->buffer;
	TransportManager *tm = cb->tm;
	while(!__sync_bool_compare_and_swap(&b.lock, false, true));

	if(b.msg == NULL) {
		Message msgHeader;

		int resultOfFindingNextMagicNumber = readMessageHeader(&msgHeader, fd);
		if(resultOfFindingNextMagicNumber != 0){
			if(resultOfFindingNextMagicNumber == 1){
				return false;
			}
			// there is some sort of error in the stream so we can't
			// get the next message
			b.lock = false;
			return true;
		}

		// sets the distributedTime of TM to the maximum time received by a message
		// in a thread safe fashion
		while(true) {
			MessageID_t time = tm->getDistributedTime();
			//check if time needs to be incremented
			if(msgHeader.getMessageId() <= time &&
					/*zero break*/ time - msgHeader.getMessageId() < UINT_MAX/2 ) break;
			//make sure time did not change
			if(__sync_bool_compare_and_swap(
					&tm->getDistributedTime(), time, msgHeader.getMessageId()+1)) break;
		}

		// we have some types of message like GetInfoCommandInfo that currently don't
		// have any information in them and therefore their body size is zero
		if(msgHeader.getBodySize() != 0){
			if(!(b.msg = readRestOfMessage(*(tm->getMessageAllocator()),
					fd, &msgHeader, &b.readCount))) {
				b.lock = false;
				return true;
			}

			if(b.readCount != b.msg->getBodySize()) {
				b.lock = false;
				return true;
			}
		}else{
			b.msg= tm->getMessageAllocator()->allocateMessage(msgHeader.getBodySize());
			memcpy(b.msg, &msgHeader, sizeof(Message));
		}
	} else {
		if(!readPartialMessage(fd, b)) {
			b.lock = false;
			return true;
		}
	}

	Message* msg = b.msg;
	b.msg = NULL;
	b.lock = false;

	DisptchArguments * arguments = new DisptchArguments(tm, msg, fd);
	pthread_t internalMessageRouteThread;
	if (pthread_create(&internalMessageRouteThread, NULL, notifyUpstreamHandlers, arguments) != 0){
		Logger::console("Cannot create thread for handling local message");
		return 255; // TODO: throw exception.
	}
	return true;
}

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
	synchManagerHandler = NULL;
	internalTrampoline = NULL;
}

void * routeInternalMessage(void * tmAndMsg) {
	DisptchArguments * pointers = (DisptchArguments *)tmAndMsg;
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
		} else {
			Message* reply = tm->getInternalTrampoline()->notifyWithReply(msg);
			ASSERT(reply != NULL);
			if(reply != NULL) {
				reply->setRequestMessageId(msg->getMessageId());
				reply->setReply()->setInternal();
				tm->getPendingMessagesHandler()->resolveResponseMessage(reply);
				//NOTE: Who will the reply message ??
			}
			if(reply == NULL){
				Logger::console("Reply is null");
			}
		}
	} else {
		if (tm->getSmHandler() != NULL) {
			tm->getSmHandler()->notifyWithReply(msg);
		} else {
			Logger::debug( "SM handler is not ready yet");
		}
	}
	// and delete the msg
	tm->getMessageAllocator()->deallocateByMessagePointer(msg);
	delete pointers;
	return NULL;
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
		DisptchArguments * pointers = new DisptchArguments(this, msg);
		pthread_t internalMessageRouteThread;
		if (pthread_create(&internalMessageRouteThread, NULL, routeInternalMessage, pointers) != 0){
			Logger::console("Cannot create thread for handling local message");
			return 255; // TODO: throw exception.
		}
		return msg->getMessageId();
	}

	Connection conn = routeMap.getConnection(node);

	while(!__sync_bool_compare_and_swap(&conn.sendLock, false, true));
	MessageID_t returnValue = _route(conn.fd, msg);
	conn.sendLock = true;
	return returnValue;
}

/*
 *   This function is internal to Transport layer. All external modules should use
 *   <code>route(NodeId node, Message *msg, unsigned timeout, CallbackReference callback)</code>
 *   Note: The function is not thread safe. Caller should ensure thread safety.
 */

MessageID_t TransportManager::_route(int fd, Message *msg) {
	if(msg == NULL){
		Logger::console("Trying to send NULL message in TM route(fd,msg)");
		return 0;
	}
	if(! msg->isSMRelated()){
		Logger::console("Message is being sent through TM route(fd,msg). Msg type is %d", msg->getType());
	}
	//msg->setMessageId( __sync_fetch_and_add(&distributedTime, 1));

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

}}
