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
#include "core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {

/*
 *  Discovery message flags used for identifying message type
 */
static const int16_t DISCOVERY_JOIN_CLUSTER_REQ = 0x0;
static const int16_t DISCOVERY_JOIN_CLUSTER_ACK = 0x1;
static const int16_t DISCOVERY_JOIN_CLUSTER_YIELD =  0x2;

/*
 *  Read/Write APIs for UDP sockets
 */
int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize, int flag,
		 struct sockaddr_in& senderAddress);
int readUDPPacketWithSenderInfo(int listenSocket, char *buffer, unsigned bufferSize,
		 struct sockaddr_in& senderAddress) ;
int sendUDPPacketToDestination(int sendSocket, char *buffer, unsigned bufferSize,
		struct sockaddr_in& destinationAddress);
int checkSocketIsReady(int socket, bool checkForRead , unsigned waitTime = 1);

static const int DISCOVERY_RETRY_COUNT = 5;
static const int DISCOVERY_YIELD_WAIT_SECONDS = 3;

/*
 *  Discovery messsage structure. Contains information required for initial
 *  discovery of nodes.
 */
struct DiscoveryMessage {
	int16_t flag;
	uint32_t interfaceNumericAddress;
	uint32_t internalCommunicationPort;
	NodeId nodeId;
	NodeId masterNodeId;
	uint32_t ackMessageIdentifier;  // to be used for ACK messsages only
	char _clusterIdent[100];

	DiscoveryMessage(){
		this->flag = 0;
		this->interfaceNumericAddress = 0;
		this->internalCommunicationPort = 0;
		this->nodeId = 0;
		this->masterNodeId = 0;
		this->ackMessageIdentifier = 0;
		memset(this->_clusterIdent, 0 , 100);
	}
	DiscoveryMessage(const DiscoveryMessage & right){
		this->flag = right.flag;
		this->interfaceNumericAddress = right.interfaceNumericAddress;
		this->internalCommunicationPort = right.internalCommunicationPort;
		this->nodeId = right.nodeId;
		this->masterNodeId = right.masterNodeId;
		this->ackMessageIdentifier = right.ackMessageIdentifier;
		memcpy(this->_clusterIdent, right._clusterIdent , 100);
	}
	DiscoveryMessage & operator=(const DiscoveryMessage & right){
		if(this == &right){
			return *this;
		}
		new (this) DiscoveryMessage(right);
		return *this;
	}
	//
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += sizeof(flag) ;
		numberOfBytes += sizeof(interfaceNumericAddress) ;
		numberOfBytes += sizeof(internalCommunicationPort) ;
		numberOfBytes += sizeof(nodeId) ;
		numberOfBytes += sizeof(masterNodeId) ;
		numberOfBytes += sizeof(ackMessageIdentifier) ;
		numberOfBytes += 100;
		return numberOfBytes;
	}

	void * serialize(void * buffer) const{
		buffer = srch2::util::serializeFixedTypes(flag, buffer);
		buffer = srch2::util::serializeFixedTypes(interfaceNumericAddress, buffer);
		buffer = srch2::util::serializeFixedTypes(internalCommunicationPort, buffer);
		buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
		buffer = srch2::util::serializeFixedTypes(masterNodeId, buffer);
		buffer = srch2::util::serializeFixedTypes(ackMessageIdentifier, buffer);
		memcpy(buffer, _clusterIdent, 100);
		buffer = ((char *)buffer) + 100;
		return buffer;
	}
	void * deserialize(void * buffer){
		buffer = srch2::util::deserializeFixedTypes(buffer ,flag);
		buffer = srch2::util::deserializeFixedTypes(buffer ,interfaceNumericAddress);
		buffer = srch2::util::deserializeFixedTypes(buffer ,internalCommunicationPort);
		buffer = srch2::util::deserializeFixedTypes(buffer ,nodeId);
		buffer = srch2::util::deserializeFixedTypes(buffer ,masterNodeId);
		buffer = srch2::util::deserializeFixedTypes(buffer ,ackMessageIdentifier);
		memcpy(_clusterIdent, buffer, 100);
		buffer = ((char *)buffer) + 100;
		return buffer;
	}
};

/*
 *  This is an abstract base class for all the classes which provide discovery services.
 */
class DiscoveryService{
public:
	DiscoveryService(SyncManager *syncManager, const string& clusterIdent) {
		this->syncManager = syncManager;
		this->shutdown = false;
		this->clusterIdentifier = clusterIdent;
	}
	// methods to be implemented by concrete service provider.
	virtual void init() = 0;

	virtual void reInit() = 0;

	virtual void startDiscoveryThread(bool skipInitialDiscovery = false) = 0;

	// APIs provided by base class.
	bool isLoopbackMessage(DiscoveryMessage &msg);
	bool isCurrentClusterMessage(DiscoveryMessage &msg);
	void isCurrentNodeMaster(bool flag) { syncManager->setNodeIsMaster(flag); }
	void stopDiscovery() { shutdown = true; }
	SyncManager *getSyncManager() { return  syncManager; }
	TransportManager *getTransport() { return  syncManager->getTransport(); }
	virtual ~DiscoveryService() {}
protected:
	bool shutdown;
	string clusterIdentifier;
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

	// name of the cluster to distinguish nodes in same network but different cluster.
	std::string clusterName;

	// only for debug
	void print() {
		std::cout << "discovery: [" << clusterName <<  " : " << multiCastAddress << " : " << multicastPort
				 << " : " << ttl << " : " << (enableLoop == 1 ? "loop enabled" : "loop disabled") << "]" << std::endl;
	}
};

void * multicastListener(void * arg);

class MulticastDiscoveryService : public DiscoveryService{
	friend void * multicastListener(void * arg);
public:
	MulticastDiscoveryService(MulticastDiscoveryConfig config, SyncManager *syncManager);

	void init();

	void reInit();

	void startDiscoveryThread(bool skipInitialDiscovery = false) {
		this->skipInitialDiscovery = skipInitialDiscovery;
		pthread_create(&multicastListenerThread, NULL, multicastListener, this);
		pthread_detach(multicastListenerThread);

	}
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

	pthread_t multicastListenerThread;
	bool skipInitialDiscovery;
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
	std::string clusterName;
	// only for debug
	void print() {
		std::cout << "discovery: [ " << clusterName << " : "  << std::endl;
		for (unsigned i = 0; i < knownHosts.size(); ++i) {
			 std::cout << knownHosts[i].ipAddress.c_str() << " : " << knownHosts[i].port << std::endl;
		}
		std::cout << "]" << std::endl;
	}
};

void * unicastListener(void * arg);
const unsigned unicastDiscoveryDefaultPort = 4000;
class UnicastDiscoveryService : public DiscoveryService{

	friend void * unicastListener(void * arg);
public:
	UnicastDiscoveryService(UnicastDiscoveryConfig& config, SyncManager *syncManager);

	void init();

	void reInit();

	void startDiscoveryThread(bool skipInitialDiscovery = false) {
		this->skipInitialDiscovery = skipInitialDiscovery;
		pthread_create(&unicastListenerThread, NULL, unicastListener, this);
		pthread_detach(unicastListenerThread);
	}
private:
	int openListeningChannel();

	void sendJoinRequest(const std::string& knownHost, unsigned port);

	void validateConfigSettings(UnicastDiscoveryConfig& config);

	bool shouldYield(unsigned senderIp, unsigned senderPort);

	UnicastDiscoveryConfig discoveryConfig;

	std::string matchedKnownHostIp;
	unsigned matchedKnownHostPort;
	unsigned discoveryPort;
	bool isWellKnownHost;

	int listenSocket;
	int sendSocket;
	bool _discoveryDone;

	pthread_t unicastListenerThread;
	bool skipInitialDiscovery;
};


} /* namespace sharding */
} /* namespace srch2 */
#endif /* DICOVERYMANAGER_H_ */
