#include "TransportManager.h"
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <core/util/Assert.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>
#include "routing/RoutingManager.h"

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
	if(cb->tm->recieveMessage(fd, cb)){
		event_add(cb->ev, NULL);
	}else{
		// Node is either dead or their is some socket error. We will not wait for
		// the data from this node anymore.
		// TODO: V1
		// 1. SM removes dead node from connection map.
		// OR
		// 2. Reinstate the wait for the data on this socket. ( by adding event_back)
		//
		Logger::warn("TM stopped listening to any data from node %d.", cb->conn->nodeId);
	}
}

/*
 *   Write Callback :  This function is called when the socket is available for writing/sending
 *   data.
 *
 *   Note for V0: This is not implemented yet. We may implement this for V1.  Please ignore for
 *   V0 review.
 */

void cb_sendMessage(int fd, short eventType, void *arg) {
	Logger::console("send message callback called");
}


///
///  TM Private member functions
///

/*
 *  The function dispatches message to upstream handlers.
 */
void * TransportManager::notifyUpstreamHandlers(Message *msg, int fd, NodeId  nodeId) {

	if(msg->isReply()) {
		Logger::debug("Reply message is received. Msg type is %d", msg->getType());
		if ( ! getRoutingManager()->getPendingRequestsHandler()->resolveResponseMessage(msg,
				nodeId)){
			getMessageAllocator()->deallocateByMessagePointer(msg);
		}
	} else if(msg->isInternal()) {

		if(msg->isNoReply()){
			Logger::debug("Request message with no reply is received from node %d", nodeId);
			// This msg comes from another node and does not need a reply
			// it comes from a broadcast or route with no callback
			getRmHandler()->notifyNoReply(msg);
			// and delete the msg
			getMessageAllocator()->deallocateByMessagePointer(msg);
		} else {
			Logger::debug("Request message is received with reply form node %d", nodeId);
			std::pair<Message * , void *> resultOfDPInternal = getRmHandler()->notifyWithReply(msg);
			/*
			 *  Comment : TM should not handle the reply here. The RM should
			 *  handle it in
			 *  InternalMessageBroker::notifyWithReply(Message * message)
			 *  and send reply to TM through route() function
			 */
			Message* replyMessage = resultOfDPInternal.first;
			// responseObject must be deleted
			ASSERT(resultOfDPInternal.second == NULL);
			if(replyMessage != NULL) {
				replyMessage->setRequestMessageId(msg->getMessageId());
				replyMessage->setReply()->setInternal();
				Connection& conn = getRouteMap()->getConnection(nodeId);
				while(!__sync_bool_compare_and_swap(&conn.sendLock, false, true));
				_route(fd, replyMessage);
				conn.sendLock = false;
				/*
				 * Request and Reply messages must be deallocated at this time in this case because PendingMessage
				 * structure which is responsible for this is in the source node of this request.
				 */
				getMessageAllocator()->deallocateByMessagePointer(msg);
				getMessageAllocator()->deallocateByMessagePointer(replyMessage);
			}
		}
	} else {
		// Check whether this node has registered SMHandler into TM yet. If not skip the message.
		if (getSmHandler() != NULL){
			getSmHandler()->notifyWithReply(msg);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
	}
	return NULL;
}

/*
 *  This is a simple low level function which reads the data from the supplied socket descriptor
 *  and fill it into the buffer. It also returns the total number of bytes read.
 *
 *  Return status :
 *
 *  0  : success
 *  1  : Partial data read
 *  -1 : error
 */

int TransportManager::readDataFromSocket(int fd, char *buffer, int byteToRead, int *byteReadCount) {

	int readByte = recv(fd, buffer, byteToRead, MSG_DONTWAIT);

	if(readByte == 0) {
		// the connection is closed by peer. return status -1 (error)
		return -1;
	}

	if(readByte == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			// socket is not ready for read. return status 1 ( come again later)
			return 1;
		} else {
			perror("Error while reading data from socket : ");
			//some socket error. return status -1 (error)
			return -1;
		}
	}

	*byteReadCount = readByte;

	if(readByte < byteToRead) {
		// incomplete read. return status 1 ( come again later)
		return 1;
	}

	ASSERT(byteToRead == readByte);
	return 0;
}

/*
 * This function reads a fixed size header from the socket stream. Each message start
 * with message header and then followed by message body.
 *
 * --------------------------------
 * | Message Header | Rest of Body |
 * ---------------------------------
 */
int TransportManager::readMessageHeader(Message *const message,  int fd) {

	char *buffer = (char *) message;
	int byteToRead = sizeof(Message);
	int byteReadCount = 0;
	int retryCount = 5;

	while(retryCount) {
		int status = readDataFromSocket(fd, buffer, byteToRead, &byteReadCount);
		if (status != 1) {
			return status;
		}
		// status is 1 which means incomplete read. Because reading the header completely
		// is critical, we should try again once socket is ready for read.
		byteToRead -= byteReadCount;
		buffer += byteReadCount;
		// check socket is ready for write operation
		if (checkSocketIsReadyForRead(fd) == -1) {
			break;
		}
		--retryCount;
	}
	if (byteToRead) {
		// if we still have some bytes to read after max trial, then it is an error
		return -1;
	}
	return 0;
}


/*
 *   The function reads the message body which follows the message header.
 */

int TransportManager::readMessageBody(int fd, MessageBuffer &readBuffer) {
	char *buffer = readBuffer.msg->getMessageBody() + readBuffer.readCount;
	int byteToRead = readBuffer.msg->getBodySize() - readBuffer.readCount;
	int byteReadCount = 0;
	int status = readDataFromSocket(fd, buffer, byteToRead, &byteReadCount);
	readBuffer.readCount += byteReadCount;
	return status;
}


/*
 *    The function for handling received callback for data read.
 *
 *    return status
 *
 *    true : We should keep listening to the event on this socket
 *    false: There was some error and we should not listen to the event on this socket.
 */

bool TransportManager::recieveMessage(int fd, TransportCallback *cb) {
	if( fd != cb->conn->fd) {
		//major error
		Logger::warn("connection mismatch: received data on wrong socket!!");
		return false;
	}

	MessageBuffer& readBuffer = cb->conn->buffer;
	while(!__sync_bool_compare_and_swap(&readBuffer.lock, false, true));

	if(readBuffer.msg == NULL) {
		/*
		 *  readBuffer.msg == NULL means there was no incomplete read in the previous iteration.
		 *  The current read from the socket is for a fresh new message.
		 */
		Message msgHeader;

		/*
		 *  1. read the message header which is a fixed size block.
		 */
		int status = readMessageHeader(&msgHeader, fd);

		if(status != 0){
			// there was an error. We cannot continue to read on this socket.
			readBuffer.lock = false;
			return false;
		}

		/*
		 *  2. sets the distributedTime of TM to the maximum time received by a message
		 *  in a thread safe fashion
		 */

		while(true) {
			MessageID_t time = getDistributedTime();
			//check if time needs to be incremented
			if(msgHeader.getMessageId() <= time &&
					/*zero break*/ time - msgHeader.getMessageId() < UINT_MAX/2 ) break;
			//make sure time did not change
			if(__sync_bool_compare_and_swap(
					&getDistributedTime(), time, msgHeader.getMessageId()+1)) break;
		}

		/*
		 *  3. read the remaining body of the message.
		 *
		 *  Note: we have some types of message like GetInfoCommandInfo that currently don't
		 *	have any information in them and therefore their body size is zero
		 */
		readBuffer.msg = getMessageAllocator()->allocateMessage(msgHeader.getBodySize());
		memcpy(readBuffer.msg, &msgHeader, sizeof(Message));
		if(msgHeader.getBodySize() > 0){

			readBuffer.readCount = 0;
			int status = readMessageBody(fd, readBuffer);

			if(status == 1) {
				// we will come back again for the remaining data. See section 4.
				readBuffer.lock = false;
				return true;
			} else if (status == -1) {
				// there was an error. We cannot continue to read on this socket.
				readBuffer.lock = false;
				return false;
			}
		}

	} else {
		/*
		 *   4. Try to read the remaining part of the incomplete message from
		 *      the previous iteration.
		 */

		int byteToRead = readBuffer.msg->getBodySize() - readBuffer.readCount;
		if(byteToRead > 0) {

			int status = readMessageBody(fd, readBuffer);
			if(status == 1) {
				// we will come back again for the remaining data.
				readBuffer.lock = false;
				return true;
			} else if (status == -1) {
				// there was an error. We cannot continue to read on this socket.
				readBuffer.lock = false;
				return false;
			}

		}
	}

	Message* completeMessage = readBuffer.msg;
	// set to NULL to indicate that the message was read completely.
	readBuffer.msg = NULL;
	readBuffer.lock = false;

	notifyUpstreamHandlers(completeMessage, fd, cb->conn->nodeId);
	return true;
}

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
	pthread_detach(listeningThread);
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
	routeManagerHandler = NULL;
	routingManager = NULL;
}


MessageID_t TransportManager::route(NodeId node,Message * msg,
		unsigned timeout) {

	if(msg == NULL){
		Logger::debug("Trying to send NULL message in TM route(node,msg)");
		return 0;
	}

	Connection& conn = routeMap.getConnection(node);

	while(!__sync_bool_compare_and_swap(&conn.sendLock, false, true));
	MessageID_t returnValue = _route(conn.fd, msg);
	conn.sendLock = false;
	return returnValue;
}

/*
 *   This function is internal to Transport layer. All external modules should use
 *   <code>route(NodeId node, Message *msg, unsigned timeout, CallbackReference callback)</code>
 *   Note: The function is not thread safe. Caller should ensure thread safety.
 */

MessageID_t TransportManager::_route(int fd, Message *message) {

	if(message == NULL){
		Logger::debug("Trying to send NULL message in TM route(fd,msg)");
		return 0;
	}

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

	unsigned totalbufferSize = message->getBodySize() + sizeof(Message);
	char * bufferToWrite = (char * ) message;
	unsigned retryCount = 5;

	while(retryCount) {

		ssize_t writeSize = send(fd, bufferToWrite, totalbufferSize, flag);

		if(writeSize == -1){
			if(errno != EAGAIN && errno != EWOULDBLOCK) {
				/*
				 *  Some socket error occurred. Print error and exit from the loop
				 */
				perror("Message sending failed !!");
				// Todo V1:  remove socket from connection map.
				break;
			}
		}

		if (writeSize ==  totalbufferSize) {
			// write completed.
			if(! message->isSMRelated()){
				Logger::console("Success");
			}
			break;
		}

		/*
		 *  If we reached here, it means the write was not complete. Try to write again
		 *  when socket is ready.
		 */

		totalbufferSize -= writeSize;
		bufferToWrite += writeSize;

		Logger::console("Message not sent completely through TM route(fd,msg). Msg type is %d and size is %d",
				message->getType(), message->getBodySize());

		// check socket is ready for write operation
		if (checkSocketIsReadyForWrite(fd) == -1) {
			break;
		}

		--retryCount;
	}

	if (retryCount == 0) {
		Logger::console("TM was unable to write complete message with max try. "
				"Remaining size of the message is %d", totalbufferSize);
		// this should not happen often.
		// Todo : V1: Send an error packet to notify that the message cannot be sent.
	}

	return message->getMessageId();
}

int TransportManager::checkSocketIsReady(int socket, bool checkForRead) {
	/*
	 *  Prepare data structure for select system call.
	 *  http://man7.org/linux/man-pages/man2/select.2.html
	 */
	fd_set selectSet;
	timeval waitTimeout;
	waitTimeout.tv_sec = 1;
	waitTimeout.tv_usec = 0;
	FD_ZERO(&selectSet);
	FD_SET(socket, &selectSet);

	/*
	 *   Wait until timeout = 1sec or until socket is ready for read/write. (whichever occurs first)
	 *   see select man page : http://linux.die.net/man/2/select
	 */
	int result = 0;
	if (checkForRead) {
		// pass select set to read argument
		result = select(socket + 1, &selectSet, NULL, NULL, &waitTimeout);
	} else {
		// pass select set to write argument
		result = select(socket + 1, NULL, &selectSet, NULL, &waitTimeout);
	}
	if (result == -1) {
		perror("error while waiting for a socket to become available for write!");
	}
	return result;
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

void TransportManager::registerCallbackHandlerForSynchronizeManager(CallBackHandler
		*callBackHandler) {
	synchManagerHandler = callBackHandler;
}

void TransportManager::setInternalMessageBroker(CallBackHandler* cbh) {
  routeManagerHandler = cbh;
}

TransportManager::~TransportManager() {

   for(RouteMap::iterator i = routeMap.begin(); i != routeMap.end(); ++i) {
      close(i->second.fd);
   }
   close(routeMap.getListeningSocket());
}

}}
