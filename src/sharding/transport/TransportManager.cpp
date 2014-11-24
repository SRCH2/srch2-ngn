#include "TransportManager.h"
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <core/util/Assert.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>

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
void cb_receiveMessage(int fd, short eventType, void *arg) {
	TransportCallback* cb = (TransportCallback*) arg;
	if(cb->tm->receiveMessage(fd, cb)){
		event_add(cb->eventPtr, NULL);
	}else{
		// Node is either dead or there is some socket error. We will not wait for
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

}


///
///  TM Private member functions
///

/*
 *  The function dispatches messages to upstream handlers.
 */
void * TransportManager::notifyUpstreamHandlers(Message *msg, int fd, NodeId  nodeId) {

	unsigned messageTotalSize = msg->getTotalSize();
	MessageID_t messageId = msg->getMessageId();
	ShardingMessageType messageType = msg->getType();
	string messageDestinationName = "";
	if(msg->isDPRequest() || msg->isDPReply()) {
		if(getDPMessageHandler() != NULL){
			getDPMessageHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
		messageDestinationName = "DP-Internal";
	} else if(msg->isDiscovery()) {
		if (getDiscoveryHandler() != NULL) {
			getDiscoveryHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
		messageDestinationName = "Discovery";
	} else if(msg->isMigration()) {
		if (getMMHandler() != NULL) {
			getMMHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
		messageDestinationName = "MigrationManager";
	} else if(msg->isSharding()){
		if(getShardManagerHandler() != NULL){
			getShardManagerHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
		messageDestinationName = "ShardManager";
	}else{
		// Check whether this node has registered SMHandler into TM yet. If not skip the message.
		if (getSmHandler() != NULL){
			getSmHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
		messageDestinationName = "SynchManager";
	}
	Logger::sharding(Logger::Detail, "TM | recv : (%d -> here), msg = [%s, %s, %d, %d]",
			nodeId, messageDestinationName.c_str(), getShardingMessageTypeStr(messageType), messageId, messageTotalSize);
	return NULL;
}

/*
 *  This is a simple low level function which reads the data from the supplied socket descriptor
 *  and fills it into the buffer. It also returns the total number of bytes read.
 *
 *  Return status :
 *
 *  0  : success
 *  1  : Partial data read
 *  -1 : error
 */

int TransportManager::readDataFromSocket(int fd, char *buffer, const int byteToRead, int *byteReadCount) {

	int readByte = recv(fd, buffer, byteToRead, MSG_DONTWAIT);

	if(readByte == 0) {
		// the connection is closed by peer. return status -1 (error)
//		Logger::sharding(Logger::Error, "TM | readDataFromSocket the connection is closed by peer. return status -1 (error)");
		Logger::sharding(Logger::Warning, "TM | readDataFromSocket could read zero bytes from the connection.");
		return -1;
	}

	if(readByte == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			// socket is not ready for read. return status 1 ( come again later)
			Logger::sharding(Logger::Detail, "TM | readDataFromSocket socket is NOT READY. Come back later. byteToRead = %d"
					, byteToRead);
			return 1;
		} else {
			perror("Error while reading data from socket : ");
			Logger::sharding(Logger::Error, "TM | readDataFromSocket Error while reading data from socket, byteToRead = %d", byteToRead);
			//some socket error. return status -1 (error)
			return -1;
		}
	}

	*byteReadCount = readByte;

	if(readByte < byteToRead) {
		// incomplete read. return status 1 ( come again later)
		Logger::sharding(Logger::Detail, "TM | readDataFromSocket Incomplete read. Come back later. readByte = %d , byteToRead = %d"
				, readByte, byteToRead);
		return 1;
	}
	if(byteToRead != readByte){
		Logger::sharding(Logger::Error, "TM | readDataFromSocket readByte is larger than byteToRead. readByte = %d , byteToRead = %d"
				, readByte, byteToRead);
		Logger::sharding(Logger::Warning, "TM | !!!!!!!!!! Possible memory corruption !!!!!!!!!!!!");
		ASSERT(false);
		return -1;
	}

	return 0;
}

/*
 * This function reads a fixed size header from the socket stream. Each message starts
 * with a message header and then followed by message body.
 *
 * --------------------------------
 * | Message Header | Rest of Body |
 * ---------------------------------
 */
int TransportManager::readMessageInterrupted(bool isMessageHeader, int fd, MessageBuffer & __messageBuffer, Message ** newCompleteMessage) {

	int byteToRead = 0;
	char * readBuffer;
	if(isMessageHeader){ // reading message header
		if(__messageBuffer.msg != NULL){
			ASSERT(false);
			return 1;
		}
		ASSERT(sizeof(Message) > __messageBuffer.sizeOfPartialMsgHrd);
		byteToRead = sizeof(Message) - __messageBuffer.sizeOfPartialMsgHrd;
		readBuffer = __messageBuffer.partialMessageHeader+__messageBuffer.sizeOfPartialMsgHrd;
//		Logger::sharding(Logger::Detail, "TM | recv : going to read msg header : %d bytes into %p which is the %d index of partial message buffer.",
//				byteToRead, readBuffer, __messageBuffer.sizeOfPartialMsgHrd);
	} else{ // reading message body
		if(__messageBuffer.msg == NULL){
			ASSERT(false);
			return 1;
		}
		byteToRead = __messageBuffer.msg->getBodySize() - __messageBuffer.getReadCount();
		readBuffer = __messageBuffer.msg->getMessageBody() + __messageBuffer.getReadCount();
//		Logger::sharding(Logger::Detail, "TM | recv : going to read body : %d bytes into %p which is the %d index of body of message.",
//				byteToRead, readBuffer, __messageBuffer.getReadCount());
	}
	if(byteToRead == 0){
		ASSERT(false);
		__messageBuffer.timeToWait = 1;
		return 1;
	}
//	char *buffer = (char *) message + __messageBuffer.sizeOfPartialMsgHrd;
	int byteReadCount = 0;
	int status = readDataFromSocket(fd, readBuffer, byteToRead, &byteReadCount);
	if(status == 1){ // either socket wasn't ready or partial read of the message header
		if(byteReadCount == 0){ // socket not ready
			__messageBuffer.numberOfRetriesWithZeroRead ++;
			__messageBuffer.timeToWait += 2;
			if(__messageBuffer.numberOfRetriesWithZeroRead >= 10){
				// if we still have some bytes to read after max trial, then it is an error
				Logger::sharding(Logger::Error, "TM | Read. Msg. Intrptd. There is still some bytes to read (%d) even after max trial. Error.", byteToRead);
				return -1;
			}
			return 1; // returning 1 means come back later.
		}
		ASSERT(byteReadCount > 0);
//		Logger::sharding(Logger::Detail, "TM | recv : number of bytes read from recv is (unsigned long int)%d or %d",
//				(unsigned long int)byteReadCount, byteReadCount);
		// we could at least read one character
		__messageBuffer.numberOfRetriesWithZeroRead = 0;
		__messageBuffer.timeToWait = 1;
		// because message is not complete we don't write it in message, instead we write it in partial copy in buffer
		ASSERT(byteReadCount < byteToRead);
		if(byteReadCount >= byteToRead){
			Logger::sharding(Logger::Error, "TM | Read. Msg. : unexpected number of bytes read.");
			return -1;
		}
		if(isMessageHeader){
			__messageBuffer.sizeOfPartialMsgHrd += byteReadCount;
//			Logger::sharding(Logger::Detail, "TM | recv : size of partial message header is : %d", __messageBuffer.sizeOfPartialMsgHrd);
			return 1; // returning 1 means come back later.
		}else{
			__messageBuffer.setReadCount(__messageBuffer.getReadCount() + byteReadCount);
			Logger::sharding(Logger::Detail, "TM | recv : size of partial body is : %d", __messageBuffer.getReadCount());
			return 1; // returning 1 means come back later.
		}
	}else if (status == 0){ // exactly this amount of data is read
		ASSERT(byteReadCount == byteToRead);
		__messageBuffer.numberOfRetriesWithZeroRead = 0;
		__messageBuffer.timeToWait = 1;
		if(isMessageHeader){
			__messageBuffer.finalizeMessageHeader();
//			Logger::sharding(Logger::Detail, "TM | recv : msg header read completely. its body size is %d",
//					__messageBuffer.msg->getBodySize());
			return 0;
		}else{
			*newCompleteMessage = __messageBuffer.finalizeMessage();
//			Logger::sharding(Logger::Detail, "TM | recv : msg body read completely. Body size was %d", (*newCompleteMessage)->getBodySize());
			return 0;
		}
		return 1;
	}else if (status == -1){
		__messageBuffer.timeToWait = 1;
		return -1;
	}
	ASSERT(false);
	__messageBuffer.timeToWait = 1;
	return -1;


//	int retryCount = 10;
//
//	while(retryCount) {
//
//		if (status != 1) {
//			return status;
//		}
//		// status is 1 which means incomplete read. Because reading the header completely
//		// is critical, we should try again once socket is ready for read.
//		byteToRead -= byteReadCount;
//		buffer += byteReadCount;
//		// check socket is ready for read operation
//		// MAY WAIT FOR 1 SECOND
//		if (checkSocketIsReadyForRead(fd) == -1) {
//			Logger::sharding(Logger::Error, "TM | Read. Msg. Hdr. Socket is not ready.");
//			break;
//		}
//		--retryCount;
//	}
}


///*
// *   The function reads the message body which follows the message header.
// */
//
//int TransportManager::readMessageBody(int fd, MessageBuffer &readBuffer) {
//	char *buffer = readBuffer.msg->getMessageBody() + readBuffer.readCount;
//	int byteToRead = readBuffer.msg->getBodySize() - readBuffer.readCount;
//	int byteReadCount = 0;
//	int status = readDataFromSocket(fd, buffer, byteToRead, &byteReadCount);
//	if(status == 0 || status == 1){
//        readBuffer.readCount += byteReadCount;
//	}
//	return status;
//}
/*
 *    The function for handling received callback for data read.
 *
 *    return status
 *
 *    true : We should keep listening to the event on this socket
 *    false: There was some error and we should not listen to the event on this socket.
 */
/*
 * Note :
 * comingBack value is :
 * 1. == 0 is it's coming from lib event
 * 2. == -1, when the caller is asking not to call recursively at all.
 * 3. > 0 , this variable gives the stack depth
 */
bool TransportManager::receiveMessage(int fd, TransportCallback *cb, int comingBack) {
	if( fd != cb->conn->fd) {
		//major error
		Logger::warn("connection mismatch: received data on wrong socket!!");
		return false;
	}

	MessageBuffer& readBuffer = cb->conn->buffer;

	// acquire lock to avoid interleaved message written to a current socket
//	readBuffer.lockForRead();
	cb->conn->lockRead();

	Message* completeMessage = NULL;
	bool readBody = false;
	if(readBuffer.msg == NULL){ // so message header is being read partially
		/*
		 *  1. read the message header which is a fixed size block (or continue reading it)
		 */
		int rv = 1;
		if(readBuffer.timeToWait > 1){
			rv = checkSocketIsReadyForRead(cb->conn->fd, readBuffer.timeToWait);
		}
		if(rv == -1){
			Logger::sharding(Logger::Error, "TM | recv : select returning -1");
			cb->conn->unlockRead();
			return false;
		}else if(rv == 0){ // socket is not ready yet.
			readBuffer.timeToWait += 2;
			if(readBuffer.timeToWait > 10){
				readBuffer.timeToWait = 10;
			}
			cb->conn->unlockRead();
			return true;
		}
		int status = readMessageInterrupted(true, cb->conn->fd, readBuffer);
		if(status != 0){
			if(status == 1){ // come back later
				cb->conn->unlockRead();
				return true;
			}else if (status == -1){
				Logger::sharding(Logger::Error, "TM | Rec.Msg. Failed to read message header, status %d", status);
			}
			cb->conn->unlockRead();
			return false;
		}
		/*
		 *  2. sets the distributedMessageId of TM to the maximum messageId received by a message
		 *  in a thread safe fashion
		 */
		MessageID_t msgHeaderMsgId = readBuffer.msg->getMessageId();
		while(true) {
			MessageID_t messageID = getCurrentMessageId();
			//check if message Id needs to be incremented
			if(msgHeaderMsgId <= messageID &&
					/*zero break*/ messageID - msgHeaderMsgId < UINT_MAX/2 ) break;
			//make sure id did not change
			if(__sync_bool_compare_and_swap(
					&getCurrentMessageId(), messageID, msgHeaderMsgId+1)) break;
		}


		/*
		 *  3. read the remaining body of the message.
		 *
		 *  Note: we have some types of message like GetInfoCommandInfo that currently don't
		 *	have any information in them and therefore their body size is zero
		 */
		if(readBuffer.msg->getBodySize() > 18000 ){
		    Logger::sharding(Logger::Error, "Too large body size. body size = %d ", readBuffer.msg->getBodySize() );
		}
		readBuffer.sizeOfPartialMsgHrd = 0;
		memset(readBuffer.partialMessageHeader, 0, sizeof(Message));
		if(readBuffer.msg->getBodySize() != 0){
			readBody = true;
		}else{
			completeMessage = readBuffer.finalizeMessage();
			readBuffer.setReadCount(0);
		}
	}else{
		readBody = true;
	}

	if(readBody){
		Message * newCompleteMessage = NULL;
		int status = readMessageInterrupted(NULL, cb->conn->fd, readBuffer, &newCompleteMessage);
		if(status != 0){
			if(status == 1){ // come back later
				cb->conn->unlockRead();
//				usleep(50);
//				if(comingBack != -1){
//					return receiveMessage(fd, cb, comingBack + 1);
//				}else{
//					return true;
//				}
				return true;
			}else if (status == -1){
				Logger::sharding(Logger::Error, "TM | Rec.Msg. Failed to read message body, status %d", status);
			}
			cb->conn->unlockRead();
			return false;
		}else{ // status == 0
			// message is complete, let's give it to upstream
			completeMessage = newCompleteMessage;
		}
	}



	if(completeMessage != NULL){
		readBuffer.timeToWait = 1;
		cb->conn->unlockRead();
		notifyUpstreamHandlers(completeMessage, fd, cb->conn->nodeId);

//		// we could read a message, if it's possible to have data, try
//		if(possibleDataForReadCount > 0){
//			return receiveMessage(fd, cb, -1);
//		}
	}else{
		cb->conn->unlockRead();
	}

	return true;
}

TransportManager::TransportManager(vector<struct event_base *>& bases, TransportConfig& config): evbases(bases) {


	// TODO : There is a bug in this architecture:
	// callback handlers like synchManagerHandler, dpMessageHandler and ... are set after transport manager
	// starts working. Setting these pointers is not protected from reading them (which happens upon message arrival)
	distributedUniqueId = 0;
	synchManagerHandler = NULL;
	dpMessageHandler = NULL;
	shutDown = false;
	discoveryHandler = NULL;
	migrationManagerHandler = NULL;
	this->eventAddedFlag = false;
	validateTransportConfig(config);
	transportConfig = config;
	transportConfig.print();

}

void TransportManager::validateTransportConfig(TransportConfig& config) {

	struct in_addr ipAddress;
	/*
	 * convert to numerical form in network byte order (big endian).
	 */
	if (inet_aton(config.interfaceAddress.c_str(), &ipAddress) == 0) {
		std::stringstream ss;
		ss << " Invalid Interface Address = " << config.interfaceAddress;
		throw std::runtime_error(ss.str());
	}
	in_addr_t interfaceNumericAddr = ipAddress.s_addr;

	fetchAllInterfacesIpAddress(allInterfaceIpAddresses);

	if (interfaceNumericAddr == INADDR_ANY) {  // 0.0.0.0
		/*
		 *  If the user has provided only a generic address ( 0.0.0.0) in the config file then
		 *  we should pick an interface address which should be published to other nodes for
		 *  internal communication.
		 *  Note: Binding a port to 0.0.0.0 means bind the port to all the interfaces on a
		 *  local device. Hence, picking one of the interfaces as a published address should
		 *  be fine.
		 */

		if (allInterfaceIpAddresses.size() > 0) {
			// pick the first interface which is up for internal communication
			this->publisedInterfaceAddress = allInterfaceIpAddresses[0];
		}
		memset(&ipAddress, 0, sizeof(ipAddress));
		if (inet_aton(this->publisedInterfaceAddress.c_str(), &ipAddress) == 0) {
			std::stringstream ss;
			ss << "Unable to find valid interface address for this node."
					<< " Please specify non-generic IP address in <transport> tag.\n";
			Logger::console(ss.str().c_str());
			throw std::runtime_error(ss.str());
		}
		this->publishedInterfaceNumericAddr = ipAddress.s_addr;

//<<<<<<< HEAD
	} else {
		this->publisedInterfaceAddress = config.interfaceAddress;
		this->publishedInterfaceNumericAddr = interfaceNumericAddr;
//=======
//	if (routeMap.begin()!= routeMap.end()) {
//		unsigned currNodeSocketReadBuffer;
//		socklen_t size = sizeof(unsigned);
//		getsockopt(routeMap.begin()->second.fd, SOL_SOCKET, SO_RCVBUF, &socketReadBuffer,
//				&size);
//		getsockopt(routeMap.begin()->second.fd, SOL_SOCKET, SO_SNDBUF, &socketSendBuffer,
//						&size);
//		Logger::console("SO_RCVBUF = %d, SO_SNDBUF = %d", socketReadBuffer, socketSendBuffer);
//>>>>>>> sharding-v0-integration
	}
}

void TransportManager::fetchAllInterfacesIpAddress(vector<string>& ipAddresses) {
	ipAddresses.clear();
	struct ifaddrs * interfaceAddresses;
	if (getifaddrs(&interfaceAddresses) == -1) {
		perror("Unable to detect interface information on this local machine : ");
	} else {
		struct ifaddrs *interfaceAddress;
		// traverse linked list
		for (interfaceAddress = interfaceAddresses; interfaceAddress != NULL;
				interfaceAddress = interfaceAddress->ifa_next) {

			int family = interfaceAddress->ifa_addr->sa_family;
			if (family == AF_INET) { // IPv4
				char host[NI_MAXHOST];
				int status = getnameinfo(interfaceAddress->ifa_addr, sizeof(struct sockaddr_in),
								host, NI_MAXHOST,
								NULL, 0, NI_NUMERICHOST);

				if (status == -1)
					continue;

				unsigned int flags = interfaceAddress->ifa_flags;
				if ((flags & IFF_UP) && !(flags & IFF_LOOPBACK)) {
					ipAddresses.push_back(host);
				}
			}
		}
		freeifaddrs(interfaceAddresses);
	}
}

MessageID_t TransportManager::sendMessage(NodeId node, Message * msg, unsigned timeout) {

	if(msg == NULL){
		Logger::debug("Trying to send NULL message in TM route(node,msg)");
		return 0;
	}

	if (!routeMap.isConnectionExist(node))
		return -1;

	Connection& conn = routeMap.getConnection(node);

	while(!__sync_bool_compare_and_swap(&conn.sendLock, false, true));

	/****
	 * Log info
	 */
	unsigned messageTotalSize = msg->getTotalSize();
	MessageID_t messageId = msg->getMessageId();
	ShardingMessageType messageType = msg->getType();
	string messageDestinationName = "";
	if(msg->isDPRequest() || msg->isDPReply()) {
		messageDestinationName = "DP-Internal";
	} else if(msg->isDiscovery()) {
		messageDestinationName = "Discovery";
	} else if(msg->isMigration()) {
		messageDestinationName = "MigrationManager";
	} else if(msg->isSharding()){
		messageDestinationName = "ShardManager";
	}else{
		messageDestinationName = "SynchManager";
	}
	Logger::sharding(Logger::Detail, "TM | send : (here -> %d), msg = [%s, %s, %d, %d]",
			node, messageDestinationName.c_str(), getShardingMessageTypeStr(messageType), messageId, messageTotalSize);

	/**********/

	MessageID_t returnValue = _sendMessage(conn.fd, msg);
	conn.sendLock = false;
	return returnValue;
}

/*
 *   This function is internal to Transport layer. All external modules should use
 *   <code>route(NodeId node, Message *msg, unsigned timeout, CallbackReference callback)</code>
 *   Note: The function is not thread safe. Caller should ensure thread safety.
 */

MessageID_t TransportManager::_sendMessage(int fd, Message *message) {

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

	// check, we shouldn't, defensive policy, transmit all kind of trash to other nodes.
	// check the mask to be one of valid masks
	if(! message->isValidMask()){
		Logger::sharding(Logger::Error, "TM | Send : Message rejected from send because mask is not valid, mask is : %8x",
				message->getMask());
		return 0;
	}
	if(! (message->getType() >= ShardingMessageTypeFirst && message->getType() <= ShardingMessageTypeLast) ){
		Logger::sharding(Logger::Error, "TM | Send : Message rejected from send because type is not valid, type is : %32x", message->getType());
		return 0;
	}

	unsigned totalbufferSize = message->getTotalSize();
	char * bufferToWrite = (char * ) message;
	unsigned retryCount = 5;  //TODO v1: change to accomodate large data transfer.

	while(retryCount) {

		ssize_t writeSize = send(fd, bufferToWrite, (size_t)totalbufferSize, flag);

		if(writeSize == -1){
			if(errno != EAGAIN && errno != EWOULDBLOCK) {
				/*
				 *  Some socket error occurred. Print error and exit from the loop
				 */
				perror("Message sending failed !!");
				// Todo V1:  remove socket from connection map.
				return 0;
			}else{ // come back later
				// check socket is ready for write operation
				int rv = 0;
				bool shouldWrite = false;
				unsigned waitCount = 10;
				while((rv = checkSocketIsReadyForWrite(fd, 100)) != -1){
					if(rv == 1){
						shouldWrite = true;
						break;
					}else{ // rv == 0
						ASSERT(rv == 0);
						if(! --waitCount){
							break;
						}
						continue;
					}
				}
				if(shouldWrite){
					continue;
				}else{
					retryCount = 0;
					break;
				}
			}
		}

		ASSERT(writeSize >= 0);

		if ((unsigned)writeSize ==  totalbufferSize) {
			break;
		}
		ASSERT(totalbufferSize > (unsigned)writeSize);
		/*
		 *  If we reached here, it means the write was not complete. Try to write again
		 *  when socket is ready.
		 */

		totalbufferSize -= (unsigned)writeSize;
		bufferToWrite += (unsigned)writeSize;

		Logger::console("Message not sent completely through TM route(fd,msg). Msg type is %d and size is %d",
				message->getType(), message->getBodySize());

		--retryCount;
	}

	if (retryCount == 0) {
		Logger::console("TM was unable to write complete message with max try. "
				"Remaining size of the message is %d", totalbufferSize);
		// this should not happen often.
		// Todo : V1: Send an error packet to notify that the message cannot be sent.
		return 0;
	}

	return message->getMessageId();
}

int TransportManager::checkSocketIsReady(int socket, bool checkForRead, int timeToWait) {
	/*
	 *  Prepare data structure for select system call.
	 *  http://man7.org/linux/man-pages/man2/select.2.html
	 */
	fd_set selectSet;
	timeval waitTimeout;
	waitTimeout.tv_sec = timeToWait;
	waitTimeout.tv_usec = 0;
	if(timeToWait <= 0){
		waitTimeout.tv_sec = 1;
	}else if (timeToWait > 10 && timeToWait < 100){
		waitTimeout.tv_sec = 10;
	}else if(timeToWait >= 100){
		waitTimeout.tv_sec = 0;
		waitTimeout.tv_usec = timeToWait;
	}
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
		return -1;
	}
	return FD_ISSET(socket,&selectSet) ? 1 : 0;
}

void TransportManager::registerEventListenerForSocket(int fd, Connection *conn) {
	for(EventBases::iterator base = evbases.begin(); base != evbases.end(); ++base) {
		TransportCallback *cb_ptr = new TransportCallback();
		struct event* ev = event_new(*base, fd, EV_READ, cb_receiveMessage, cb_ptr);
		new (cb_ptr) TransportCallback(this, conn, ev, *base);
		event_add(ev, NULL);
	}
	this->eventAddedFlag = true;
}

MessageID_t& TransportManager::getCurrentMessageId() {
	return distributedUniqueId;
}

MessageID_t TransportManager::getUniqueMessageIdValue(){
	return __sync_fetch_and_add(&distributedUniqueId, 1);
}

CallBackHandler* TransportManager::getDPMessageHandler(){
	return dpMessageHandler;
}

pthread_t TransportManager::getListeningThread() const {
	return listeningThread;
}

MessageAllocator * TransportManager::getMessageAllocator() {
	return &messageAllocator;
}

CallBackHandler* TransportManager::getSmHandler() {
	return synchManagerHandler;
}

CallBackHandler* TransportManager::getMMHandler() {
	return migrationManagerHandler;
}

CallBackHandler* TransportManager::getDiscoveryHandler() {
	return discoveryHandler;
}

CallBackHandler* TransportManager::getShardManagerHandler() {
	return shardManagerHandler;
}

void TransportManager::registerCallbackHandlerForSynchronizeManager(CallBackHandler
		*callBackHandler) {
	synchManagerHandler = callBackHandler;
}

void TransportManager::registerCallbackHandlerForMM(CallBackHandler
		*callBackHandler) {
	migrationManagerHandler = callBackHandler;
}

void TransportManager::registerCallbackHandlerForDiscovery(CallBackHandler
		*callBackHandler) {
	discoveryHandler = callBackHandler;
}

void TransportManager::registerCallbackForDPMessageHandler(CallBackHandler* cbh){
	dpMessageHandler = cbh;
}

void TransportManager::registerCallbackForShardingMessageHandler(CallBackHandler * cbh){
	shardManagerHandler = cbh;
}

TransportManager::~TransportManager() {
	shutDown = true;
	pthread_join(listeningThread, NULL);
	for(ConnectionMap::iterator i = routeMap.begin(); i != routeMap.end(); ++i) {
		close(i->second.fd);
	}
}

}}
