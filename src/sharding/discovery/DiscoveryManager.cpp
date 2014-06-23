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

using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

MulticastDiscoveryManager::MulticastDiscoveryManager(MulticastConfig config): discoveryConfig(config) {
	// throws exception if validation failed.
	validateConfigSettings(discoveryConfig);
	listenSocket = -1;
	sendSocket = -1;
	_discoveryDone = false;
	currentNodeMaster = false;
	shutdown = false;
	currentNodeId = -1;
	masterNodeId = -1;
	uniqueNodeId = 0;
}

void MulticastDiscoveryManager::validateConfigSettings(MulticastConfig discoveryConfig) {

	struct in_addr ipAddress;
	/*
	 * convert to numerical form in network byte order (big endian).
	 */
	if (inet_aton(discoveryConfig.interfaceAddress.c_str(), &ipAddress) == 0) {
		std::stringstream ss;
		ss << " Invalid Interface Address = " << discoveryConfig.interfaceAddress;
		throw std::runtime_error(ss.str());
	}
	this->interfaceNumericAddr = ipAddress.s_addr;

//	if (this->interfaceNumericAddr == INADDR_ANY) {  // 0.0.0.0
//		this->publishInterfaceAddr =
//	}

	memset(&ipAddress, sizeof(ipAddress), 0);
	if (inet_aton(discoveryConfig.multiCastAddress.c_str(), &ipAddress) == 0) {
		std::stringstream ss;
		ss << " Invalid Muticast Address = " << discoveryConfig.multiCastAddress;
		throw std::runtime_error(ss.str());
	}

	this->multiCastNumericAddr = ipAddress.s_addr;

	/*
	 *  Verify whether it is not a reserved address
	 *  Multicast IP addresses are in range of 224.0.0.0 - 239.255.255.255
	 *
	 *  Reserved address space.
	 *  224.0.0.0 - 224.0.0.255
	 *  239.0.0.0 - 239.255.255.255
	 *  http://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml
	 *  http://www.tldp.org/HOWTO/Multicast-HOWTO-2.html
	 */
	unsigned *ptr = (unsigned *)(&this->multiCastNumericAddr);

	if ((*ptr == 224 && *(ptr + 1) == 0 && *(ptr + 2) == 0) || (*ptr == 239)){
		std::stringstream ss;
		ss << " Reserved Muticast Address = " << discoveryConfig.multiCastAddress << " cannot be used";
		throw std::runtime_error(ss.str());
	}
}

/*
 *   The function is non-blocking and performs the following tasks
 *   1. Creates UDP socket.
 *   2. Setup Multicast parameters for socket at IP layer
 *   3. Bind the socket
 *
 */

int MulticastDiscoveryManager::openListeningChannel(){

	/*
	 *  Prepare socket data structures.
	 */
	int udpSocket;
	if((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("listening socket failed to init");
		exit(255);
	}

	/*
	 *   Make socket non blocking
	 */

	fcntl(udpSocket, F_SETFL, O_NONBLOCK);

	/*
	 *  set SO_REUSEADDR: This option enables multiple processes on a same host to listen
	 *  to multicast packets.
	 */
	int optionVal = 1;  // 1 is used to enable the setting.
	setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, (void*) &optionVal, sizeof(optionVal));

	setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, (void*) &optionVal, sizeof(optionVal));

	/*
	 *   Set TTL ( Time To Live ). Purpose of TTL in multicast packet is to
	 *   limit the scope of the packet to avoid flooding connected networks.
	 *
	 *   TTL     Scope
	 *  ----------------------------------------------------------------------
	 *  0     Restricted to the same host. Won't be output by any interface.
	 *  1     Restricted to the same subnet. Won't be forwarded by a router.
	 *  <32   Restricted to the same site, organization or department. (Our Default)
	 *  <64   Restricted to the same region.
	 *  <128  Restricted to the same continent.
	 *  <255  Unrestricted in scope. Global.
	 *
	 */

	optionVal = discoveryConfig.ttl;
	setsockopt(udpSocket, IPPROTO_IP, IP_MULTICAST_TTL, (void*) &optionVal, sizeof(optionVal));

	/*
	 *  Enable loopback. This enables multiple instances on the same host to receive multicast
	 *  packets. Enabled by default. If disabled, kernel does not send multicast packet to current
	 *  host.
	 */
	u_char loop = discoveryConfig.enableLoop;
	setsockopt(udpSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

	/*
	 *  Select Interface. By default we use system provided default interface. In such case
	 *  interfaceNumericAddress will be 0. If interfaceNumericAddress is a non zero value
	 *  specified by a user then it is used as a multicast interface.
	 *
	 *  Note: this is useful for the hosts which have multiple ip addresses.
	 */
	if (this->interfaceNumericAddr) {  // 0.0.0.0

		struct in_addr interfaceAddr;
		interfaceAddr.s_addr = this->interfaceNumericAddr;

		int status = setsockopt (udpSocket, IPPROTO_IP, IP_MULTICAST_IF, &interfaceAddr,
				sizeof(interfaceAddr));
		if (status == -1) {
			Logger::console("Invalid interface specified. Using system default.");
		}
	}

	/*
	 *  Join multcast group. Both interface address and multicast address are required
	 *  to register the application to a multicast group.
	 */
	struct ip_mreq routeAddress;
	memset(&routeAddress, 0, sizeof(routeAddress));
	routeAddress.imr_multiaddr.s_addr = this->multiCastNumericAddr;
	routeAddress.imr_interface.s_addr = this->interfaceNumericAddr;

	setsockopt (udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &routeAddress, sizeof(routeAddress));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = this->interfaceNumericAddr;
	unsigned portToBind = discoveryConfig.multicastPort;
	addr.sin_port = htons(portToBind);

	/*
	 *  Bind the socket to ip:port
	 */
tryNextPort:
	if( bind(udpSocket, (struct sockaddr *) &addr, sizeof(sockaddr_in)) < 0){
		++portToBind;
		if (portToBind < discoveryConfig.multicastPort + 100) {
			addr.sin_port = htons(portToBind);
			goto tryNextPort;
		} else {
			perror("");
			Logger::console("unable to bind to any port in range [%d : %d]", discoveryConfig.multicastPort,
					discoveryConfig.multicastPort + 100);
			exit(-1);
		}
	}
	discoveryConfig.multicastPort = portToBind;

	//Logger::console("Discovery UDP listen socket binding done on %d", portToBind);

	return udpSocket;
}

int MulticastDiscoveryManager::openSendingChannel(){

	/*
	 *  Prepare socket data structures.
	 */
	int udpSocket;
	if((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed !");
		exit(255);
	}

	/*
	 *   Make socket non blocking
	 */

	fcntl(udpSocket, F_SETFL, O_NONBLOCK);

	//Logger::console("Discovery UDP sending socket init done.");

	return udpSocket;

}


int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize, int flag,
		 struct sockaddr_in& senderAddress) {

	unsigned int senderAddressLen;
	while(1) {
		int status = recvfrom(listenSocket, buffer, bufferSize, flag,
				(struct sockaddr *)&senderAddress, &senderAddressLen);

		if (status == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return 1;
			} else {
				perror("Discovery : Error while reading data from UDP socket : ");
				return -1;
			}
		}
		if (status < bufferSize) {
			// incomplete read
			Logger::console("incomplete read ...continuing");
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
			Logger::console("incomplete send ...continuing");
			buffer += status;
			bufferSize -= status;
			continue;
		}
		//Logger::console("UDP multicast data send");
		break;
	}
	return 0;
}
int checkSocketIsReady(int socket, bool checkForRead);
void * multicastListener(void * arg) {

	MulticastDiscoveryManager * discovery = (MulticastDiscoveryManager *) arg;
	int listenSocket = discovery->listenSocket;

	DiscoveryMessage message;
	memset(&message, 0, sizeof(message));

	int retryCount = 10;
	struct sockaddr_in senderAddress;

	char * buffer = (char *)&message;
	unsigned bufferLen = sizeof(message);

	bool shouldElectItselfAsMaster = true;
	bool masterDetected = false;

	// initial discovery loop
	bool stop = false;
	while(retryCount) {
		int status = readUDPPacketWithSenderInfo(listenSocket, buffer, bufferLen, MSG_DONTWAIT,
			 senderAddress);
		if (status == -1) {
			exit(0);
		}
		if (status == 0) {
			// ignore looped back messages.
			if (discovery->isLoopbackMessage(message)) {
				///Logger::console("loopback message ...continuing");
				continue;
			} else {
				switch(message.flag)
				{
				case DISCOVERY_JOIN_CLUSTER_ACK:
				{
					/*
					 *   Master node is detected. Stop listening to discovery.
					 *   Start SM messaging with master to get nodeIds and other cluster related info.
					 */
					shouldElectItselfAsMaster = false;
					masterDetected = true;
					break;
				}
				case DISCOVERY_JOIN_CLUSTER_REQ:
				{
					/*
					 * if we receive this message then their is another node which is also
					 * in process of joining the cluster. If there is already a master for this
					 * cluster then it should be fine. Otherwise, there is a race condition for
					 * becoming the master. How to deal with it? solution. use ip address + port as
					 * a tie breaker. The one with higher ip address + port combination will get
					 * preference in this race.
					 */
					if (!masterDetected) {
						Logger::console("Race to become master detected !!");
						if ( discovery->shouldYield(message.interfaceNumericAddress,
								message.internalCommunicationPort)){
								Logger::console("Yielding to other node");
								shouldElectItselfAsMaster = false;
								retryCount = 10;
								sleep(1);
								discovery->sendJoinRequest();
								continue;
						}

					}
					break;
				}
				default:
					ASSERT(false);
				}

				if (masterDetected) {
					break;  // exit while loop
				}
			}
		}
		--retryCount;
		//if (!shouldElectItselfAsMaster) {
			discovery->sendJoinRequest();
		//}
		//Logger::console(">> %d" , retryCount);
		//Logger::console("no master detected ...waiting...");
		checkSocketIsReady(listenSocket, true);
	}

	discovery->isCurrentNodeMaster(!masterDetected);
	if (masterDetected) {

		Logger::console("Master node = %d ", message.masterNodeId);
		Logger::console("Curernt node = %d ", message.nodeId);
		senderAddress.sin_port = htons(message.internalCommunicationPort);
		discovery->nodeToAddressMap[message.masterNodeId] = senderAddress;
		discovery->setCurrentNodeId(message.nodeId);
		discovery->setMasterNodeId(message.masterNodeId);
		discovery->_discoveryDone = true;
		return NULL;
	}

	unsigned masterNodeID = discovery->getNextNodeId();
	discovery->nodeToAddressMap[masterNodeID] = senderAddress;
	discovery->setCurrentNodeId(masterNodeID);
	discovery->setMasterNodeId(masterNodeID);
	discovery->_discoveryDone = true;

	//Else master was not detected and we had timeout in the discovery loop
	ASSERT(retryCount == 0);
	Logger::console("Current Node is master node");
	if (!shouldElectItselfAsMaster){
		Logger::console("Cluster may have other masters!.");
	}

	// Make the listening socket blocking now.
	int val = fcntl(listenSocket, F_GETFL);
	val &= ~O_NONBLOCK;;
	fcntl(listenSocket, F_SETFL, val);

	while(!discovery->shutdown) {
		memset(&senderAddress, 0, sizeof(senderAddress));
		memset(&message, 0, sizeof(message));

		int status = readUDPPacketWithSenderInfo(listenSocket, buffer, bufferLen, senderAddress);

		if (status == -1) {
			exit(0);
		}

		if (status == 0) {
			// ignore looped back messages.
			if (discovery->isLoopbackMessage(message)) {
				//Logger::console("loopback message ...continuing");
				continue;
			} else {
				switch(message.flag)
				{
				case DISCOVERY_JOIN_CLUSTER_ACK:
				{
					Logger::console("ERROR:: Multiple masters present in the cluster");
					//fatal condition. We have two choice.
					// 1. Bring down cluster
					// 2. Ask all masters to step down.
					// TODO
					break;
				}
				case DISCOVERY_JOIN_CLUSTER_REQ:
				{
					// new node joining the cluster. Notify MM and get a new node
					// id for this cluster.
					//discovery->enqueueNewJoinRequest(senderAddress);
					// send acknowledgment that master exist

					// TODO: check whether same node sends JOIN request again. This logic will
					// assign new id to it. Should we reuse new id or use the new id.
					//Logger::console("Got cluster joining request!!");
					DiscoveryMessage ackMessage;
					ackMessage.flag = DISCOVERY_JOIN_CLUSTER_ACK;
					ackMessage.interfaceNumericAddress = discovery->interfaceNumericAddr;
					ackMessage.internalCommunicationPort = discovery->getCommunicationPort();
					ackMessage.masterNodeId = discovery->getCurrentNodeId();
					ackMessage.nodeId = discovery->getNextNodeId();
					tryAckAgain:
					// send multicast acknowledgment
					struct sockaddr_in destinationAddress;
					memset(&destinationAddress, 0, sizeof(destinationAddress));
					inet_aton(discovery->discoveryConfig.multiCastAddress.c_str(), &destinationAddress.sin_addr);
					destinationAddress.sin_family = AF_INET;
					destinationAddress.sin_port = htons(discovery->getMulticastPort());

					int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, (char *)&ackMessage,
							sizeof(ackMessage), destinationAddress);
					if (sendStatus == -1) {
						exit(-1);
					}
					if (sendStatus == 1) {
						goto tryAckAgain;
					}
					senderAddress.sin_port = htons(message.internalCommunicationPort);
					discovery->nodeToAddressMap[ackMessage.nodeId] = senderAddress;

					break;
				}
				default:
					ASSERT(false);
					Logger::console("Invalid flag !!");
				}

			}
		}

		if (status == 1) {
			// socket is non-blocking mode. should not get return value 1
			ASSERT(false);
		}
	}
	return NULL;
}

void MulticastDiscoveryManager::init() {

	listenSocket = openListeningChannel();
	sendSocket = openSendingChannel();

	pthread_t multicastListenerThread;
	// start a thread to listen for incoming data packet.
	pthread_create(&multicastListenerThread, NULL, multicastListener, this);
	pthread_detach(multicastListenerThread);
	// send request to
	sendJoinRequest();

	while(!_discoveryDone) {
		sleep(1);
	}
}

void MulticastDiscoveryManager::sendJoinRequest() {
	DiscoveryMessage message;
	memset(&message, 0, sizeof(message));
	message.flag = DISCOVERY_JOIN_CLUSTER_REQ;
	message.interfaceNumericAddress = interfaceNumericAddr;
	message.internalCommunicationPort = getCommunicationPort();

	struct sockaddr_in destinationAddress;
	memset(&destinationAddress, 0, sizeof(destinationAddress));
	inet_aton(discoveryConfig.multiCastAddress.c_str(), &destinationAddress.sin_addr);
	//destinationAddress.sin_addr.s_addr = multiCastNumericAddr;
	destinationAddress.sin_family = AF_INET;
	destinationAddress.sin_port = htons(getMulticastPort());
	int retry = 3;
	//Logger::console("sending MC UDP to %s , %d",discoveryConfig.multiCastAddress.c_str(),  getMulticastPort());
	while(retry) {

		int status = sendUDPPacketToDestination(sendSocket, (char *)&message,
				sizeof(message), destinationAddress);
		if (status == 1) {
			--retry;
			continue;
		} else {
			break;
		}
	}
}
bool MulticastDiscoveryManager::shouldYield(unsigned senderIp, unsigned senderPort) {
	//Logger::console("[%d, %d] [%d, %d]", senderIp, senderPort, interfaceNumericAddr, getCommunicationPort());
	if (senderIp > interfaceNumericAddr) {
		return true;
	} else if ( (senderIp == interfaceNumericAddr ) && (senderPort > getCommunicationPort())) {
		return true;
	}
	return false;
}

bool MulticastDiscoveryManager::isLoopbackMessage(DiscoveryMessage &msg){
	return (msg.interfaceNumericAddress == this->interfaceNumericAddr &&
			msg.internalCommunicationPort == this->getCommunicationPort());
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
		perror("error while waiting for a socket to become available for write!");
	}
	return result;
}

unsigned MulticastDiscoveryManager::getNextNodeId() {
	if (currentNodeMaster)
		return __sync_fetch_and_add(&uniqueNodeId, 1);
	else {
		ASSERT(false);
		return uniqueNodeId;
	}
}

} /* namespace sharding */
} /* namespace srch2 */
