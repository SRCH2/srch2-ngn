/*
 * DicoveryManager.h
 *
 *  Created on: Apr 24, 2014
 *      Author: Surendra
 */

#ifndef DICOVERYMANAGER_H_
#define DICOVERYMANAGER_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include <iostream>
#include <string>
#include <vector>

#include "configuration/ShardingConstants.h"
#include "synchronization/SynchronizerManager.h"

namespace srch2 {
namespace httpwrapper {

/*
 *  Discovery message flags used for identifying message type
 */
static const short DISCOVERY_JOIN_CLUSTER_REQ = 0x0;
static const short DISCOVERY_JOIN_CLUSTER_ACK = 0x1;
static const short DISCOVERY_JOIN_CLUSTER_YIELD =  0x2;

/*
 *  Read/Write APIs for UDP sockets
 */
int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize, int flag,
		 struct sockaddr_in& senderAddress);
int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize,
		 struct sockaddr_in& senderAddress) ;
int sendUDPPacketToDestination(int sendSocket, char *buffer, unsigned bufferSize,
		struct sockaddr_in& destinationAddress);
int checkSocketIsReady(int socket, bool checkForRead);

static const int DISCOVERY_RETRY_COUNT = 5;
static const int DISCOVERY_YIELD_WAIT_SECONDS = 3;

/*
 *  Discovery messsage structure. Contains information required for initial
 *  discovery of nodes.
 */
struct DiscoveryMessage {
	short flag;
	unsigned interfaceNumericAddress;
	unsigned internalCommunicationPort;
	unsigned nodeId;
	unsigned masterNodeId;
	unsigned ackMessageIdentifier;  // to be used for ACK messsages only
};

/*
 *  This is an abstract base class for all the classes which provide discovery services.
 */
class DiscoveryService{
public:
	DiscoveryService(SyncManager *syncManager) {
		this->syncManager = syncManager;
		this->shutdown = false;
	}
	// methods to be implemented by concrete service provider.
	virtual void init() = 0;

	// APIs provided by base class.
	bool isLoopbackMessage(DiscoveryMessage &msg);
	void isCurrentNodeMaster(bool flag) { syncManager->setNodeIsMaster(flag); }
	void stopDiscovery() { shutdown = true; }
	SyncManager *getSyncManager() { return  syncManager; }
	TransportManager *getTransport() { return  syncManager->getTransport(); }
	virtual ~DiscoveryService() {}
protected:
	bool shutdown;
	SyncManager *syncManager;
};

///
///  Multicast discovery related Data structures and Class
///
struct MulticastDiscoveryConfig{

	// IP address of Multicast Group. e.g 224.1.1.2
	std::string multiCastAddress;

	// multicast listening port
	unsigned multicastPort;

	// IP address of interface used for sending and receiving multicast packets.
	// Note: this can be same as interfaceAddress used for internal node communication.
	std::string multicastInterface;

	// Time To Live for multicast packet
	int ttl;

	// enabled loopback. 0 = enabled or 1 = false
	u_char enableLoop;

	// only for debug
	void print() {
		std::cout << "discovery: [" << multiCastAddress << " : " << multicastPort
				 << " : " << ttl << " : " << (enableLoop == 1 ? "loop enabled" : "loop disabled") << "]" << std::endl;
	}
};

void * multicastListener(void * arg);

class MulticastDiscoveryService : public DiscoveryService{
	friend void * multicastListener(void * arg);
public:
	MulticastDiscoveryService(MulticastDiscoveryConfig config, SyncManager *syncManager);

	void init();

private:

	std::string getMultiCastAddressStr() {
		return discoveryConfig.multiCastAddress;
	}
	unsigned getMulticastPort() {
		return discoveryConfig.multicastPort;
	}

	int openListeningChannel();

	int openSendingChannel();

	void sendJoinRequest();

	void validateConfigSettings(MulticastDiscoveryConfig& config);

	bool shouldYield(unsigned senderIp, unsigned senderPort);

	MulticastDiscoveryConfig discoveryConfig;

	in_addr_t multiCastNumericAddr;
	in_addr_t multiCastInterfaceNumericAddr;

	int listenSocket;
	int sendSocket;
	bool _discoveryDone;
	struct sockaddr_in multicastGroupAddress;
};

///
/// Unicast discovery related Data structures and Class
///
struct HostAndPort {
	HostAndPort(std::string ip, unsigned port) {
		this->ipAddress = ip;
		this->port = port;
	}
	std::string ipAddress;
	unsigned port;
};
struct UnicastDiscoveryConfig {
	std::vector<HostAndPort> knownHosts;
	// only for debug
	void print() {
		std::cout << "discovery: [" << std::endl;
		for (unsigned i = 0; i < knownHosts.size(); ++i) {
			 std::cout << knownHosts[i].ipAddress.c_str() << " : " << knownHosts[i].port << std::endl;
		}
		std::cout << "]" << std::endl;
	}
};

void * unicastListener(void * arg);
class UnicastDiscoveryService : public DiscoveryService{

	friend void * unicastListener(void * arg);
public:
	UnicastDiscoveryService(UnicastDiscoveryConfig& config, SyncManager *syncManager);

	void init();

private:
	int openListeningChannel();

	void sendJoinRequest(const std::string& knownHost, unsigned port);

	void validateConfigSettings(UnicastDiscoveryConfig& config);

	bool shouldYield(unsigned senderIp, unsigned senderPort);

	UnicastDiscoveryConfig discoveryConfig;

	std::string matchedKnownHostIp;
	unsigned matchedKnownHostPort;
	bool isWellKnownHost;

	int listenSocket;
	int sendSocket;
	bool _discoveryDone;
};


} /* namespace sharding */
} /* namespace srch2 */
#endif /* DICOVERYMANAGER_H_ */
