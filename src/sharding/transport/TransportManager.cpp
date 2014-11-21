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

	if(msg->isDPRequest() || msg->isDPReply()) {
		Logger::debug("Reply message is received. Msg type is %d", msg->getType());
		if(getDPMessageHandler() != NULL){
			getDPMessageHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);

	} else if(msg->isDiscovery()) {
		if (getDiscoveryHandler() != NULL) {
			getDiscoveryHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
	} else if(msg->isMigration()) {

		if (getMMHandler() != NULL) {
			getMMHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
	} else if(msg->isSharding()){
//		Logger::debug("Sharding message is received. Msg type is %d", msg->getType());
		if(getShardManagerHandler() != NULL){
			getShardManagerHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
	}else{

		// Check whether this node has registered SMHandler into TM yet. If not skip the message.
		if (getSmHandler() != NULL){
			getSmHandler()->resolveMessage(msg, nodeId);
		}
		getMessageAllocator()->deallocateByMessagePointer(msg);
	}
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
		return 0;
	}

	if(readByte == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			// socket is not ready for read. return status 1 ( come again later)
			Logger::sharding(Logger::Detail, "TM | readDataFromSocket socket not ready. readByte = %d , byteToRead = %d"
					, readByte, byteToRead);
			return 1;
		} else {
			perror("Error while reading data from socket : ");
			Logger::sharding(Logger::Error, "TM | readDataFromSocket Error while reading data from socket");
			//some socket error. return status -1 (error)
			return -1;
		}
	}

	*byteReadCount = readByte;

	if(readByte < byteToRead) {
		// incomplete read. return status 1 ( come again later)
		Logger::sharding(Logger::Detail, "TM | readDataFromSocket incomplete read, will come back later. readByte = %d , byteToRead = %d"
				, readByte, byteToRead);
		return 1;
	}
	if(byteToRead != readByte){
		Logger::sharding(Logger::Error, "TM | readDataFromSocket readByte is larger than byteToRead. readByte = %d , byteToRead = %d"
				, readByte, byteToRead);
		ASSERT(false);
		return -1;
	}

	return 2;
}

/*
 * This function reads a fixed size header from the socket stream. Each message starts
 * with a message header and then followed by message body.
 *
 * --------------------------------
 * | Message Header | Rest of Body |
 * ---------------------------------
 */
int TransportManager::readMessageHeader(Message * message,  int fd) {

	char *buffer = (char *) message;
	int byteToRead = sizeof(Message);
	int byteReadCount = 0;
	int retryCount = 10;

	while(retryCount) {
		int status = readDataFromSocket(fd, buffer, byteToRead, &byteReadCount);
		if (status != 1) {
			return status;
		}
		// status is 1 which means incomplete read. Because reading the header completely
		// is critical, we should try again once socket is ready for read.
		byteToRead -= byteReadCount;
		buffer += byteReadCount;
		// check socket is ready for read operation
		if (checkSocketIsReadyForRead(fd) == -1) {
			Logger::sharding(Logger::Error, "TM | Read. Msg. Hdr. Socket is not ready.");
			break;
		}
		--retryCount;
	}
	if (byteToRead) {
		// if we still have some bytes to read after max trial, then it is an error
		Logger::sharding(Logger::Error, "TM | Read. Msg. Hdr. There is still some bytes to read (%d) even after max trial. Error.", byteToRead);
		return -1;
	}
	return 2;
}


/*
 *   The function reads the message body which follows the message header.
 */

int TransportManager::readMessageBody(int fd, MessageBuffer &readBuffer) {
	char *buffer = readBuffer.msg->getMessageBody() + readBuffer.readCount;
	int byteToRead = readBuffer.msg->getBodySize() - readBuffer.readCount;
	int byteReadCount = 0;
	int status = readDataFromSocket(fd, buffer, byteToRead, &byteReadCount);
	if(status == 2 || status == 1){
        readBuffer.readCount += byteReadCount;
	}
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

bool TransportManager::receiveMessage(int fd, TransportCallback *cb) {
	if( fd != cb->conn->fd) {
		//major error
		Logger::warn("connection mismatch: received data on wrong socket!!");
		return false;
	}

	MessageBuffer& readBuffer = cb->conn->buffer;

	// acquire lock to avoid interleaved message written to a current socket
	while(!__sync_bool_compare_and_swap(&readBuffer.lock, false, true));

	bool inputMustExist = true;
	vector<Message *> completeMessages;

	while(true){
		if(readBuffer.msg == NULL) {
		    Logger::sharding(Logger::Detail, "TM | receiveMessage : going to read a new message.");
				/*
				 *  readBuffer.msg == NULL means there was no incomplete read in the previous iteration.
				 *  The current read from the socket is for a fresh new message.
				 */
				Message msgHeader;

				/*
				 *  1. read the message header which is a fixed size block.
				 */
				int status = readMessageHeader(&msgHeader, fd);
				// status can be -1, 0 or 2
				if(status == 0){
					if(inputMustExist){
						Logger::sharding(Logger::Error, "TM | readDataFromSocket the connection is closed by peer. return status -1 (error)");
						readBuffer.lock = false;
						return false;
					}else{
						break;
					}
				}
				if(status == -1){
					// there was an error. We cannot continue to read on this socket.
					Logger::sharding(Logger::Error, "TM | Rec.Msg. Failed to read message header, status %d", status);
					readBuffer.lock = false;
					return false;
				}

				inputMustExist = false;
				/*
				 *  2. sets the distributedMessageId of TM to the maximum messageId received by a message
				 *  in a thread safe fashion
				 */

				while(true) {
					MessageID_t messageID = getCurrentMessageId();
					//check if message Id needs to be incremented
					if(msgHeader.getMessageId() <= messageID &&
							/*zero break*/ messageID - msgHeader.getMessageId() < UINT_MAX/2 ) break;
					//make sure id did not change
					if(__sync_bool_compare_and_swap(
							&getCurrentMessageId(), messageID, msgHeader.getMessageId()+1)) break;
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
						// we will come back again for the remaining data. See else section below.
						Logger::sharding(Logger::Detail, "TM | Rec.Msg. Message body is read partially. It was going to be %d bytes. %d bytes read so far."
								, msgHeader.getBodySize(), readBuffer.readCount);
						break;
					} else if (status == 2){
//						continue;
					} else if (status == 0){
						break;
					} else if (status == -1) {
						// there was an error. We cannot continue to read on this socket.
						Logger::sharding(Logger::Error, "TM | Rec.Msg. Failed to read message body, status %d. It was going to be %d bytes. %d bytes read so far."
								, msgHeader.getBodySize(), readBuffer.readCount);
						readBuffer.lock = false;
						return false;
					}
				}

			} else {
	            Logger::sharding(Logger::Detail, "TM | receiveMessage : going continue reading message with body size %d", readBuffer.msg->getBodySize());
				/*
				 *   4. Try to read the remaining part of the incomplete message from
				 *      the previous libevent iteration.
				 */

				int byteToRead = readBuffer.msg->getBodySize() - readBuffer.readCount;
				if(readBuffer.msg->getBodySize() < readBuffer.readCount){
					ASSERT(false);
					Logger::sharding(Logger::Error, "TM | Rec.Msg. Read count %d is larger than message body size %d. Returning false",
							readBuffer.msg->getBodySize() , readBuffer.readCount);
					readBuffer.lock = false;
					return false;
				}
				if(byteToRead > 0) {

					int status = readMessageBody(fd, readBuffer);
					if(status == 1) {
						// we will come back again for the remaining data.
						Logger::sharding(Logger::Detail, "TM | Rec.Msg. Message body is read partially. It was going to be %d bytes. %d bytes read so far."
								, readBuffer.msg->getBodySize(), readBuffer.readCount);
						break;
					}else if (status == 2){
//						continue;
					} else if (status == 0){
						break;
					} else if (status == -1) {
						// there was an error. We cannot continue to read on this socket.
						Logger::sharding(Logger::Error, "TM | Rec.Msg. Failed to read message body, status %d. It was going to be %d bytes. %d bytes read so far."
								, readBuffer.msg->getBodySize(), readBuffer.readCount);
						readBuffer.lock = false;
						return false;
					}

				}
			}

			completeMessages.push_back(readBuffer.msg);
			// set to NULL to indicate that the message was read completely.
			readBuffer.msg = NULL;
	}

	readBuffer.lock = false;
	for(unsigned msgIdx = 0 ; msgIdx < completeMessages.size(); ++msgIdx){
		notifyUpstreamHandlers(completeMessages.at(msgIdx), fd, cb->conn->nodeId);
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

	unsigned totalbufferSize = message->getBodySize() + sizeof(Message);
	char * bufferToWrite = (char * ) message;
	unsigned retryCount = 5;  //TODO v1: change to accomodate large data transfer.

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
		return 0;
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
