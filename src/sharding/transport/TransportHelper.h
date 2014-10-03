#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include "ConnectionMap.h"

namespace srch2 {
namespace httpwrapper {

bool sendGreeting(int fd, bool greeted, const NodeInfo& node);
int recieveGreeting(int fd, NodeInfo& nodeInfo, bool noTimeout = false);

void* listenForIncomingConnection(void* arg) {
	TransportManager *transport = (TransportManager *) arg;
	const Node& currentNode =  transport->getConnectionMap().getCurrentNode();

	hostent *host = gethostbyname(currentNode.getIpAddress().c_str());
	if (!host) {
		perror("gethostbyname ");
		exit(-1);
	}

	int fd;
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("listening socket failed to init");
		exit(-1);
	}

	const int optVal = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, sizeof(optVal));

	unsigned listeningPort = currentNode.getPortNumber();

	struct sockaddr_in routeAddress;
	memset(&routeAddress, 0, sizeof(routeAddress));
	routeAddress.sin_family = AF_INET;
	memcpy(&routeAddress.sin_addr, host->h_addr, host->h_length);
	routeAddress.sin_port = htons(listeningPort);

	if(bind(fd, (struct sockaddr*) &routeAddress, sizeof(routeAddress)) < 0) {
		Logger::console("Failed to bind to TCP port [%d]", listeningPort);
		perror("");
		close(fd);
		exit(-1);
	}

	// listen for incoming connection with upto 20 connection in queue.
	if(listen(fd, 20) == -1) {
		close(fd);
		perror("listening socket failed start");
		exit(-1);
	}

	fd_set checkConnect;
	timeval timeout;
	while(!transport->isShuttingDown()) {

		FD_ZERO(&checkConnect);
		FD_SET(fd, &checkConnect);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(select(fd+1, &checkConnect, NULL, NULL, &timeout) !=1)
			continue;

		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(sockaddr_in);
		memset(&addr, 0,sizeof(sockaddr_in));
		int newfd;
		if((newfd = accept(fd, (sockaddr*) &addr, &addrlen)) != -1) {
			NodeId remoteNodeId;
			NodeInfo remoteNodeInfo;
			NodeInfo currentNodeInfo = {currentNode.getId(), 0, currentNode.getPortNumber()};
			memcpy(currentNodeInfo.nodeName, currentNode.getName().c_str(), currentNode.getName().size());
			if((remoteNodeId = recieveGreeting(newfd, remoteNodeInfo)) == -1) {
				sendGreeting(newfd, false, currentNodeInfo);
				close(newfd);
				return false;
			}
			if (transport->getConnectionMap().isConnectionExist(remoteNodeId)) {
				sendGreeting(newfd, false, currentNodeInfo);
				close(newfd);
				return false;
			}
			if(!sendGreeting(newfd, true, currentNodeInfo)) {
				close(newfd);
				return false;
			}

			transport->getConnectionMap().acceptConnection(newfd, remoteNodeId);

			// add event to event base.
			Connection *conn = &(transport->getConnectionMap().getConnection(remoteNodeId));
			transport->registerEventListenerForSocket(newfd, conn);


			char ipbuf[INET_ADDRSTRLEN+1] = {0};
			struct in_addr remoteIpAddr;
			remoteIpAddr.s_addr = remoteNodeInfo.ipaddress;
			inet_ntop(AF_INET, (const void *)&remoteIpAddr, ipbuf, INET_ADDRSTRLEN+1);

			Node node(remoteNodeInfo.nodeName, ipbuf, remoteNodeInfo.communicationPort, false);
			node.setId(remoteNodeId);
			string serlializedNode = node.serialize();
			Message * msg = MessageAllocator().allocateMessage(serlializedNode.size());
			msg->setType(NewNodeNotificationMessageType);
			char * msgBody = msg->getMessageBody();
			memcpy(msgBody,serlializedNode.c_str(), serlializedNode.size());
			transport->getDiscoveryHandler()->resolveMessage(msg, /*not used */0);
			MessageAllocator().deallocateByMessagePointer(msg);
		}
	}

	shutdown(fd, SHUT_RDWR);
	close(fd);
	return NULL;
}

/*
 *   Every time a new node is detected send connection request.
 */
bool sendConnectionRequest(TransportManager *transport, unsigned destinationNodeId,
		Node& currentNode, struct sockaddr_in destinationAddress) {

	if (transport->getConnectionMap().isConnectionExist(destinationNodeId)) {
		Logger::console("Connection to node id = %d already exist", destinationNodeId);
		return false;
	}

	while(1) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0)
			continue;
		if(connect(fd, (struct sockaddr*) &destinationAddress, sizeof (destinationAddress)) == -1) {
			if(errno == ECONNREFUSED) {
				close(fd);
				fd = -1;
			} else {
				Logger::console("Connection to node id = %d failed", destinationNodeId);
				perror("");
				exit(-1);
			}
		}
		if(fd == -1) {
			sleep(1);
			continue;
		}
		struct in_addr currentAddr;
		inet_aton(currentNode.getIpAddress().c_str(), &currentAddr);
		NodeInfo currentNodeInfo = {currentNode.getId(), currentAddr.s_addr, currentNode.getPortNumber()};
		memcpy(currentNodeInfo.nodeName, currentNode.getName().c_str(), currentNode.getName().size());
		NodeInfo remoteNodeInfo;
		trySendingAgain:  // label
		if (!sendGreeting(fd, true, currentNodeInfo)){
			close(fd);
			continue;
		}
		NodeId remoteNodeId = recieveGreeting(fd, remoteNodeInfo, true);

		if (remoteNodeId == -1) {
			goto trySendingAgain;
		}
		if (remoteNodeId != destinationNodeId) {
			close(fd);
			Logger::console("wrong destination address for node %d", destinationNodeId);
			return false;
		}
		/*
		 * On successful connection we use this file descriptor for future communication.
		 * Also make it non-blocking
		 */
		fcntl(fd, F_SETFL, O_NONBLOCK);
		Logger::console("Adding node = %d to known connection", destinationNodeId);
		transport->getConnectionMap().addNodeConnection(destinationNodeId, fd);
		// add event to event base to start listening on this socket
		Connection *conn = &(transport->getConnectionMap().getConnection(destinationNodeId));
		transport->registerEventListenerForSocket(fd, conn);
		return true;
	}

	return true;
}

}}
