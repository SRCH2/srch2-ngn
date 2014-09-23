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

UnicastDiscoveryService::UnicastDiscoveryService(UnicastDiscoveryConfig& config,
		SyncManager *syncManager): DiscoveryService(syncManager, config.clusterName), discoveryConfig(config) {
	listenSocket = -1;
	sendSocket = -1;
	_discoveryDone = false;
	matchedKnownHostIp = "";
	// throws exception if validation failed.
	validateConfigSettings(discoveryConfig);
	skipInitialDiscovery = false;
}

void UnicastDiscoveryService::validateConfigSettings(UnicastDiscoveryConfig& config) {

	// validate IPs of known Hosts
	struct in_addr networkIpAddress;
	std::vector<HostAndPort> knownHostsValidatedAddresses;
	const vector<string>& allIpAddresses = getTransport()->getAllInterfacesIpAddress();
	for (unsigned i = 0; i < discoveryConfig.knownHosts.size(); ++i) {
		memset(&networkIpAddress, 0, sizeof(networkIpAddress));
		if (inet_aton(this->discoveryConfig.knownHosts[i].ipAddress.c_str(), &networkIpAddress) == 0) {
			std::stringstream ss;
			ss << " Invalid Known Host Address = " << this->discoveryConfig.knownHosts[i].ipAddress;
			Logger::console(ss.str().c_str());
			throw std::runtime_error(ss.str());
		}
		if (networkIpAddress.s_addr == INADDR_ANY) {
			Logger::console("Address 0.0.0.0 is not allowed as a known host");
			continue;
		}

		if (this->discoveryConfig.knownHosts[i].port == -1) {
			this->discoveryConfig.knownHosts[i].port = unicastDiscoveryDefaultPort;
		}

		knownHostsValidatedAddresses.push_back(this->discoveryConfig.knownHosts[i]);

		if (networkIpAddress.s_addr == htonl(INADDR_LOOPBACK) && matchedKnownHostIp == "") {
			// if well-known host is set to 127.0.0.1, then the current node is well known host
			// this setting should be used only for
			isWellKnownHost = true;
			matchedKnownHostIp = this->discoveryConfig.knownHosts[i].ipAddress;
			matchedKnownHostPort = this->discoveryConfig.knownHosts[i].port;
		} else {
			const string& ipAddress = this->discoveryConfig.knownHosts[i].ipAddress;
			if (matchedKnownHostIp == "" &&
					(std::find(allIpAddresses.begin(), allIpAddresses.end(), ipAddress) != allIpAddresses.end())) {
				//Logger::console("current host is one of the known host");
				isWellKnownHost = true;
				matchedKnownHostIp = this->discoveryConfig.knownHosts[i].ipAddress;
				matchedKnownHostPort = this->discoveryConfig.knownHosts[i].port;
			}
		}
	}
}

void UnicastDiscoveryService::sendJoinRequest(const string& knownHost, unsigned port) {
	DiscoveryMessage message;
	memset(&message, 0, sizeof(message));
	message.flag = DISCOVERY_JOIN_CLUSTER_REQ;
	message.interfaceNumericAddress = getTransport()->getPublishedInterfaceNumericAddr();
	message.internalCommunicationPort = getTransport()->getCommunicationPort();

	unsigned byteToCopy = this->clusterIdentifier.size() > 99 ? 99 : this->clusterIdentifier.size();
	strncpy(message.clusterIdent, this->clusterIdentifier.c_str(), byteToCopy);

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
	/*
	 *   Make socket non blocking
	 */

	fcntl(udpSocket, F_SETFL, O_NONBLOCK);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
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
	this->discoveryPort = portToBind;

	Logger::console("Discovery UDP listen socket binding done on %d", portToBind);

	return udpSocket;
}

void UnicastDiscoveryService::init() {

	if (this->matchedKnownHostIp == "") {
		matchedKnownHostIp = getTransport()->getPublisedInterfaceAddress();
		matchedKnownHostPort = unicastDiscoveryDefaultPort;
	}
	Logger::console("Discovery ip:port = %s:%d", matchedKnownHostIp.c_str(), matchedKnownHostPort);
	listenSocket = openListeningChannel();
	sendSocket = listenSocket;  // we use same socket for sending/receiving.

	_discoveryDone = false;

	// start a thread to listen for incoming data packet.
	startDiscoveryThread();

	while(!_discoveryDone) {
		sleep(1);
	}
}

void UnicastDiscoveryService::reInit() {

	listenSocket = openListeningChannel();
	sendSocket = listenSocket;  // we use same socket for sending/receiving.
	startDiscoveryThread(true); // start a thread to listen for incoming data packet.
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

	if (!discovery->skipInitialDiscovery) {

	bool shouldElectItselfAsMaster = true;//discovery->isWellKnownHost;
	bool masterDetected = false;

	const vector<HostAndPort>& knownHosts = discovery->discoveryConfig.knownHosts;
InitialDiscovery:
	for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
		if (knownHosts[i].ipAddress.compare(discovery->matchedKnownHostIp) == 0 &&
			knownHosts[i].port == discovery->discoveryPort)
			continue;
		Logger::console("sending joing request");
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
				//checkSocketIsReady(listenSocket, true);
				//Logger::console("loopback message ...continuing");
				continue;
			} if (!discovery->isCurrentClusterMessage(message)) {
				Logger::console("message from different network using same multicast setting...continuing");
				continue;
			} else {
				switch(message.flag)
				{
				case DISCOVERY_JOIN_CLUSTER_ACK:
				{
					/*
					 *   Master node is detected. Stop discovery.
					 */
					if (message.ackMessageIdentifier == discovery->getTransport()->getCommunicationPort()) {
						shouldElectItselfAsMaster = false;
						masterDetected = true;
					}
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
										knownHosts[i].port == discovery->discoveryPort)
									continue;
								discovery->sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
							}
							continue;
						} else {
							// if not yielding then ask others to yield.
							Logger::console("Send Yield message");
							DiscoveryMessage yeildMessage;
							yeildMessage.flag = DISCOVERY_JOIN_CLUSTER_YIELD;
							yeildMessage.interfaceNumericAddress = discovery->getTransport()->getPublishedInterfaceNumericAddr();
							yeildMessage.internalCommunicationPort = discovery->getTransport()->getCommunicationPort();
							yeildMessage.masterNodeId = -1;
							yeildMessage.nodeId = -1;

							unsigned byteToCopy = discovery->clusterIdentifier.size() > 99 ? 99 : discovery->clusterIdentifier.size();
							strncpy(yeildMessage.clusterIdent, discovery->clusterIdentifier.c_str(), byteToCopy);

							tryYieldMsgAgain:
							int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, (char *)&yeildMessage,
									sizeof(yeildMessage), senderAddress);
							if (sendStatus == -1) {
								exit(-1);
							}
							if (sendStatus == 1) {
								goto tryYieldMsgAgain;
							}
						}

					}
					break;
				}
				case DISCOVERY_JOIN_CLUSTER_YIELD:
				{
					Logger::console("Yielding to other node");
					shouldElectItselfAsMaster = false;
					retryCount = DISCOVERY_RETRY_COUNT;
					sleep(DISCOVERY_YIELD_WAIT_SECONDS);
					for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
						if (knownHosts[i].ipAddress.compare(discovery->matchedKnownHostIp) == 0 &&
								knownHosts[i].port == discovery->discoveryPort)
							continue;
						discovery->sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
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

	if (masterDetected) {

		discovery->isCurrentNodeMaster(false);

		Logger::console("Master node = %d ", message.masterNodeId);
		Logger::console("Curernt node = %d ", message.nodeId);

		// remember master's address for further communication.
		senderAddress.sin_addr.s_addr = message.interfaceNumericAddress;
		senderAddress.sin_port = htons(message.internalCommunicationPort);
		discovery->getSyncManager()->addNodeToAddressMappping(message.masterNodeId, senderAddress);

		discovery->getSyncManager()->setCurrentNodeId(message.nodeId);
		discovery->getSyncManager()->setMasterNodeId(message.masterNodeId);
		discovery->_discoveryDone = true;

		close(listenSocket);
		return NULL;
	}
	//master was not detected and we had timeout in the discovery loop
	ASSERT(retryCount == 0);
	discovery->isCurrentNodeMaster(true);
	Logger::console("Current Node is master node");
	if (!shouldElectItselfAsMaster){
		Logger::console("Cluster may have other masters!.");
	}

	unsigned masterNodeID = discovery->getSyncManager()->getNextNodeId();
	discovery->getSyncManager()->setCurrentNodeId(masterNodeID);
	discovery->getSyncManager()->setMasterNodeId(masterNodeID);
    }
	discovery->_discoveryDone = true;

	// Make the listening socket blocking now.
	int val = fcntl(listenSocket, F_GETFL);
	val &= ~O_NONBLOCK;;
	fcntl(listenSocket, F_SETFL, val);

	while(!discovery->shutdown) {
		Logger::console("discovering new nodes");
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
			} if (!discovery->isCurrentClusterMessage(message)) {
				Logger::console("message from different network using same multicast setting...continuing");
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
					ackMessage.interfaceNumericAddress = discovery->getTransport()->getPublishedInterfaceNumericAddr();
					ackMessage.internalCommunicationPort = discovery->getTransport()->getCommunicationPort();
					ackMessage.masterNodeId = discovery->getSyncManager()->getCurrentNodeId();
					ackMessage.nodeId = discovery->getSyncManager()->getNextNodeId();
					ackMessage.ackMessageIdentifier = message.internalCommunicationPort;

					unsigned byteToCopy = discovery->clusterIdentifier.size() > 99 ? 99 : discovery->clusterIdentifier.size();
					strncpy(ackMessage.clusterIdent, discovery->clusterIdentifier.c_str(), byteToCopy);

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

					discovery->getSyncManager()->addNodeToAddressMappping(ackMessage.nodeId, destinationAddress);
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
	close(listenSocket);
	return NULL;
}

bool UnicastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
	if (senderIp > getTransport()->getPublishedInterfaceNumericAddr()) {
		return true;
	}
//	else if ( (senderIp == getTransport()->getPublishedInterfaceNumericAddr() )
//			&& (senderPort > getTransport()->getCommunicationPort())) {
//		return true;
//	}
	return false;
}

}}



