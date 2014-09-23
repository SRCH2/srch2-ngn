/*
 * DicoveryManager.cpp
 *
 *  Created on: Apr 24, 2014
 *      Author: srch2
 */

#include "DiscoveryManager.h"
#include <util/Logger.h>
#include <sstream>
#include <errno.h>
#include <util/Assert.h>
#include <sys/fcntl.h>
#include "transport/Message.h"
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>

using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {


int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize, int flag,
		 struct sockaddr_in& senderAddress) {

	unsigned int senderAddressLen = sizeof(senderAddress);
	while(1) {
		int status = recvfrom(listenSocket, buffer, bufferSize, flag,
				(struct sockaddr *)&senderAddress, &senderAddressLen);

		if (status == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				//perror("Recv");
				return 1;
			} else {
				perror("Discovery : Error while reading data from UDP socket : ");
				return -1;
			}
		}
		if (status < bufferSize) {
			// incomplete read
			//Logger::console("incomplete read ...continuing");
			buffer += status;
			bufferSize -= status;
			continue;
		}
		break;
	}
	return 0;
}

int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize,
		struct sockaddr_in& senderAddress)
{
	return readUDPPacketWithSenderInfo(listenSocket, buffer, bufferSize, 0, senderAddress);
}

int sendUDPPacketToDestination(int sendSocket, char *buffer, unsigned bufferSize,
		struct sockaddr_in& destinationAddress) {

	while(1) {
		int status = sendto(sendSocket, buffer, bufferSize, MSG_DONTWAIT,
				(const sockaddr *)&destinationAddress, sizeof(destinationAddress));

		if (status == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				//perror("Send");
				return 1;
			} else {
				perror("Discovery : Error while sending data from UDP socket : ");
				return -1;
			}
		}

		if (status < bufferSize) {
			// incomplete read
			//Logger::console("incomplete send ...continuing");
			buffer += status;
			bufferSize -= status;
			continue;
		}
		//Logger::console("UDP multicast data send");
		break;
	}
	return 0;

}
int checkSocketIsReady(int socket, bool checkForRead) {
	/*
	 *  Prepare data structure for select system call.
	 *  http://man7.org/linux/man-pages/man2/select.2.html
	 */
	fd_set selectSet;
	timeval waitTimeout;
	waitTimeout.tv_sec = 2;
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
		perror("error while waiting for a socket to become available for read/write!");
	}
	return result;
}

bool DiscoveryService::isLoopbackMessage(DiscoveryMessage &msg){
	return (msg.interfaceNumericAddress == getTransport()->getPublishedInterfaceNumericAddr() &&
			msg.internalCommunicationPort == getTransport()->getCommunicationPort());
}

bool DiscoveryService::isCurrentClusterMessage(DiscoveryMessage &msg) {
	return string(msg.clusterIdent) == this->clusterIdentifier;
}

} /* namespace sharding */
} /* namespace srch2 */
