/*
 * UnicastDiscoveryService.cpp
 *
 *  Created on: Jun 26, 2014
 *      Author: Surendra
 */
#include "DiscoveryManager.h"
#include <vector>
#include <util/Logger.h>
#include <sstream>
#include <errno.h>
#include <util/Assert.h>
#include <sys/fcntl.h>
#include "transport/Message.h"
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>

using namespace std;

namespace srch2 {
namespace httpwrapper {

int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize, int flag,
		 struct sockaddr_in& senderAddress);
int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize,
		 struct sockaddr_in& senderAddress) ;
int sendUDPPacketToDestination(int sendSocket, char *buffer, unsigned bufferSize,
		struct sockaddr_in& destinationAddress);
int checkSocketIsReady(int socket, bool checkForRead);


UnicastDiscoveryService::UnicastDiscoveryService(UnicastDiscoveryConfig& config): discoveryConfig(config) {
	listenSocket = -1;
	sendSocket = -1;
	_discoveryDone = false;
	shutdown = false;
	currentNodeId = -1;
	masterNodeId = -1;
	uniqueNodeId = 0;
	matchedKnownHostIp = "";
	currentNodeMaster =false;
	// throws exception if validation failed.
	validateConfigSettings(discoveryConfig);
}

void UnicastDiscoveryService::validateConfigSettings(UnicastDiscoveryConfig& config) {

	struct in_addr ipAddress;
	/*
	 * convert to numerical form in network byte order (big endian).
	 */
	Logger::console("validating interface address %s", discoveryConfig.interfaceAddress.c_str());
	if (inet_aton(discoveryConfig.interfaceAddress.c_str(), &ipAddress) == 0) {
		std::stringstream ss;
		ss << " Invalid Interface Address = " << discoveryConfig.interfaceAddress;
		throw std::runtime_error(ss.str());
	}
	unsigned int numericInterfaceAddress = ipAddress.s_addr;
	vector<in_addr_t> allIpAddresses;
	if (numericInterfaceAddress == htonl(INADDR_ANY)) {  // 0.0.0.0
		/*
		 *  If the user has provided only a generic address ( 0.0.0.0) in the config file then
		 *  we should pick an interface address which should be published to other nodes for
		 *  internal communication.
		 *  Note: Binding a port to 0.0.0.0 means bind the port to all the interfaces on a
		 *  local device. Hence, picking one of the interfaces as a published address should
		 *  be fine.
		 */
		struct ifaddrs * interfaceAddresses;
		if (getifaddrs(&interfaceAddresses) == -1) {
			perror("Unable to detect interface information on this local machine : ");
		} else {
			struct ifaddrs *interfaceAddress;
			bool firstInterface = true;
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

						memset(&ipAddress, 0, sizeof(ipAddress));
						if (inet_aton(host, &ipAddress) == 0) {
							continue;
						}
						if (firstInterface) {
							this->discoveryConfig.publisedInterfaceAddress = host;
							this->publishedInterfaceNumericAddr = ipAddress.s_addr;
							firstInterface = false;
						}
						allIpAddresses.push_back(ipAddress.s_addr);
					}
				}
			}
			freeifaddrs(interfaceAddresses);
		}
		if (allIpAddresses.size() == 0) {
			std::stringstream ss;
			ss << "Unable to find valid interface address for this node."
					<< " Please specify non-generic IP address in <transport> tag.\n";
			Logger::console(ss.str().c_str());
			throw std::runtime_error(ss.str());
		}
	} else {
		allIpAddresses.push_back(numericInterfaceAddress);
		this->publishedInterfaceNumericAddr = numericInterfaceAddress;
		this->discoveryConfig.publisedInterfaceAddress = this->discoveryConfig.interfaceAddress;
	}

	// validate IPs of known Hosts

	std::vector<HostAndPort> knownHostsValidatedAddresses;
	for (unsigned i = 0; i < discoveryConfig.knownHosts.size(); ++i) {
		memset(&ipAddress, 0, sizeof(ipAddress));
		if (inet_aton(this->discoveryConfig.knownHosts[i].ipAddress.c_str(), &ipAddress) == 0) {
			std::stringstream ss;
			ss << " Invalid Known Host Address = " << this->discoveryConfig.knownHosts[0].ipAddress;
			Logger::console(ss.str().c_str());
			throw std::runtime_error(ss.str());
		}
		if (ipAddress.s_addr == INADDR_ANY) {
			Logger::console("Address 0.0.0.0 is not allowed as a known host");
			continue;
		}

		knownHostsValidatedAddresses.push_back(this->discoveryConfig.knownHosts[i]);

		if (matchedKnownHostIp == "" &&
		    (std::find(allIpAddresses.begin(), allIpAddresses.end(), ipAddress.s_addr)
		    != allIpAddresses.end())) {
			Logger::console("current host is one of the known host");
			isWellKnownHost = true;
			matchedKnownHostIp = this->discoveryConfig.knownHosts[i].ipAddress;
			matchedKnownHostPort = this->discoveryConfig.knownHosts[i].port;
		}
	}
}

void UnicastDiscoveryService::sendJoinRequest(const string& knownHost, unsigned port) {
	DiscoveryMessage message;
	memset(&message, 0, sizeof(message));
	message.flag = DISCOVERY_JOIN_CLUSTER_REQ;
	message.interfaceNumericAddress = this->publishedInterfaceNumericAddr;
	message.internalCommunicationPort = getCommunicationPort();

	struct sockaddr_in destinationAddress;
	memset(&destinationAddress, 0, sizeof(destinationAddress));
	destinationAddress.sin_family = AF_INET;
	inet_aton(knownHost.c_str(), &destinationAddress.sin_addr);
	destinationAddress.sin_port = htons(port);
	int retry = 3;
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

int UnicastDiscoveryService::openListeningChannel(){
	/*
	 *  Prepare socket data structures.
	 */
	int udpSocket;
	if((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("listening socket failed to init");
		exit(255);
	}
#ifdef __MACH__
	int optionVal = 1;  // 1 is used to enable the setting.
	int status = setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, (void*) &optionVal, sizeof(optionVal));
	if (status < 0) {
		perror("address reuse failed : ");
		exit(-1);
	}
#endif
	/*
	 *   Make socket non blocking
	 */

	fcntl(udpSocket, F_SETFL, O_NONBLOCK);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	inet_aton(this->matchedKnownHostIp.c_str(), &addr.sin_addr);
	unsigned portToBind = this->matchedKnownHostPort;
	addr.sin_port = htons(portToBind);

	/*
	 *  Bind the socket to ip:port
	 */
tryNextPort:
	if( bind(udpSocket, (struct sockaddr *) &addr, sizeof(sockaddr_in)) < 0){
		this->isWellKnownHost = false;
		++portToBind;
		if (portToBind < this->matchedKnownHostPort + 100) {
			addr.sin_port = htons(portToBind);
			goto tryNextPort;
		} else {
			perror("");
			Logger::console("unable to bind to any port in range [%d : %d]", this->matchedKnownHostPort,
					this->matchedKnownHostPort + 100);
			exit(-1);
		}
	}
	this->matchedKnownHostPort = portToBind;

	Logger::console("Discovery UDP listen socket binding done on %d", portToBind);

	return udpSocket;
}

int UnicastDiscoveryService::openSendingChannel(){
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

	return udpSocket;
}

void UnicastDiscoveryService::init() {

	if (this->matchedKnownHostIp == "") {
		matchedKnownHostIp = discoveryConfig.publisedInterfaceAddress;
		matchedKnownHostPort = discoveryConfig.internalCommunicationPort;
		Logger::console("setting discovery ip:port = %s:%d", discoveryConfig.publisedInterfaceAddress.c_str(),
				discoveryConfig.internalCommunicationPort);
	}
	listenSocket = openListeningChannel();
	sendSocket = listenSocket;  // we use same socket for sending/receiving.

	pthread_t unicastListenerThread;
	// start a thread to listen for incoming data packet.
	pthread_create(&unicastListenerThread, NULL, unicastListener, this);
	pthread_detach(unicastListenerThread);

	while(!_discoveryDone) {
		sleep(1);
	}
}

void * unicastListener(void * arg) {
	UnicastDiscoveryService * discovery = (UnicastDiscoveryService *) arg;
	int listenSocket = discovery->listenSocket;

	DiscoveryMessage message;
	memset(&message, 0, sizeof(message));

	int retryCount = DISCOVERY_RETRY_COUNT;
	struct sockaddr_in senderAddress;

	char * buffer = (char *)&message;
	unsigned bufferLen = sizeof(message);

	// This node can be master only if is one of the well known host specified in config file.
	bool shouldElectItselfAsMaster = discovery->isWellKnownHost;
	bool masterDetected = false;

	const vector<HostAndPort>& knownHosts = discovery->discoveryConfig.knownHosts;
InitialDiscovery:
	for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
		if (knownHosts[i].ipAddress.compare(discovery->matchedKnownHostIp) == 0 &&
			knownHosts[i].port == discovery->matchedKnownHostPort)
			continue;
		discovery->sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
	}
	// initial discovery loop
	while(retryCount) {
		checkSocketIsReady(listenSocket, true);
		int status = readUDPPacketWithSenderInfo(listenSocket, buffer, bufferLen, MSG_DONTWAIT,
				senderAddress);
		if (status == -1) {
			exit(0);
		}
		if (status == 0) {
			// ignore looped back messages.
			if (discovery->isLoopbackMessage(message)) {
				ASSERT(false);
				Logger::console("loopback message ...continuing");
				continue;
			} else {
				switch(message.flag)
				{
				case DISCOVERY_JOIN_CLUSTER_ACK:
				{
					/*
					 *   Master node is detected. Stop discovery.
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
							retryCount = DISCOVERY_RETRY_COUNT;
							sleep(DISCOVERY_YIELD_WAIT_SECONDS);

							for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
								if (knownHosts[i].ipAddress.compare(discovery->matchedKnownHostIp) == 0 &&
										knownHosts[i].port == discovery->matchedKnownHostPort)
									continue;
								discovery->sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
							}
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
	}

	if (!masterDetected && !discovery->isWellKnownHost) {
		Logger::console("Master node is not up in any of the given well Known hosts list !!. Please see <WellKnownHosts> in the config file.");
		Logger::console("Current node is not a well known host.");
		Logger::console("Will retry again in 25 secs....");
		sleep(25);
		goto InitialDiscovery;
	}

	if (masterDetected) {

		discovery->isCurrentNodeMaster(false);

		Logger::console("Master node = %d ", message.masterNodeId);
		Logger::console("Curernt node = %d ", message.nodeId);

		// remember master's address for further communication.
		senderAddress.sin_addr.s_addr = message.interfaceNumericAddress;
		senderAddress.sin_port = htons(message.internalCommunicationPort);
		discovery->nodeToAddressMap[message.masterNodeId] = senderAddress;

		discovery->setCurrentNodeId(message.nodeId);
		discovery->setMasterNodeId(message.masterNodeId);
		discovery->_discoveryDone = true;
		return NULL;
	}
	//master was not detected and we had timeout in the discovery loop
	ASSERT(retryCount == 0);
	discovery->isCurrentNodeMaster(true);
	Logger::console("Current Node is master node");
	if (!shouldElectItselfAsMaster){
		Logger::console("Cluster may have other masters!.");
	}

	unsigned masterNodeID = discovery->getNextNodeId();
	discovery->setCurrentNodeId(masterNodeID);
	discovery->setMasterNodeId(masterNodeID);
	discovery->_discoveryDone = true;

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
			if (discovery->isLoopbackMessage(message)) {
				ASSERT(false);
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
					// TODO V1 phase 4
					break;
				}
				case DISCOVERY_JOIN_CLUSTER_REQ:
				{
					// TODO: check whether same node sends JOIN request again.
					Logger::console("Got cluster joining request!!");
					DiscoveryMessage ackMessage;
					ackMessage.flag = DISCOVERY_JOIN_CLUSTER_ACK;
					ackMessage.interfaceNumericAddress = discovery->publishedInterfaceNumericAddr;
					ackMessage.internalCommunicationPort = discovery->getCommunicationPort();
					ackMessage.masterNodeId = discovery->getCurrentNodeId();
					ackMessage.nodeId = discovery->getNextNodeId();
					tryAckAgain:
					// send multicast acknowledgment
					struct sockaddr_in destinationAddress;
					memset(&destinationAddress, 0, sizeof(destinationAddress));
					destinationAddress.sin_addr.s_addr = message.interfaceNumericAddress;
					destinationAddress.sin_family = AF_INET;
					destinationAddress.sin_port = htons(message.internalCommunicationPort);

					int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, (char *)&ackMessage,
							sizeof(ackMessage), senderAddress);
					if (sendStatus == -1) {
						exit(-1);
					}
					if (sendStatus == 1) {
						goto tryAckAgain;
					}

					discovery->nodeToAddressMap[ackMessage.nodeId] = destinationAddress;
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


bool UnicastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
	//Logger::console("[%d, %d] [%d, %d]", senderIp, senderPort, interfaceNumericAddr, getCommunicationPort());
	if (senderIp > publishedInterfaceNumericAddr) {
		return true;
	} else if ( (senderIp == publishedInterfaceNumericAddr ) && (senderPort > getCommunicationPort())) {
		return true;
	}
	return false;
}

bool UnicastDiscoveryService::isLoopbackMessage(DiscoveryMessage &msg){
	return (msg.interfaceNumericAddress == this->publishedInterfaceNumericAddr &&
			msg.internalCommunicationPort == this->getCommunicationPort());
}

unsigned UnicastDiscoveryService::getNextNodeId() {
	if (currentNodeMaster)
		return __sync_fetch_and_add(&uniqueNodeId, 1);
	else {
		ASSERT(false);
		return uniqueNodeId;
	}
}

}}



