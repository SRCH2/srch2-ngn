/*
 * DicoveryManager.h
 *
 *  Created on: Apr 24, 2014
 *      Author: surendra
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include "configuration/ShardingConstants.h"
#include <iostream>
#ifndef DICOVERYMANAGER_H_
#define DICOVERYMANAGER_H_

#include<string>
#include<vector>

namespace srch2 {
namespace httpwrapper {

const short DISCOVERY_JOIN_CLUSTER_REQ = 0x0;
const short DISCOVERY_JOIN_CLUSTER_ACK = 0x1;

struct DiscoveryMessage {
	short flag;
	unsigned interfaceNumericAddress;
	unsigned internalCommunicationPort;
	unsigned nodeId;
	unsigned masterNodeId;
};

class DiscoveryService{
public:
	virtual void init() = 0;
	virtual bool isCurrentNodeMaster() = 0;
	virtual unsigned getCurrentNodeId() = 0;
	virtual unsigned getMasterNodeId() = 0;
	virtual std::string getInterfaceAddress() = 0;
	virtual bool getDestinatioAddressByNodeId(NodeId id, struct sockaddr_in& addr) = 0;
	virtual unsigned getCommunicationPort() = 0;
	virtual ~DiscoveryService() {}
};

struct MulticastDiscoveryConfig{

	// IP address of Multicast Group. e.g 224.1.1.2
	std::string multiCastAddress;
	// multicast listening port
	unsigned multicastPort;
	// IP address of interface used for sending and receiving multicast packets.
	// Note: this can be same as interfaceAddress used for internal node communication.
	std::string multicastInterface;


	// IP address of NIC for port binding. Could be 0.0.0.0
	std::string interfaceAddress;
	// IP address of NIC to be used for internal communication. Cannot be 0.0.0.0
	std::string publisedInterfaceAddress;
	unsigned internalCommunicationPort;

	// Time To Live for multicast packet
	int ttl;
	// enabled loopback. 0 = enabled or 1 = false
	u_char enableLoop;

	// only for debug
	void print() {
		std::cout << "transport: [" << interfaceAddress << " : " << internalCommunicationPort << "]" << std::endl;
		std::cout << "discovery: [" << multiCastAddress << " : " << multicastPort
				 << " : " << ttl << " : " << (enableLoop == 1 ? "loop enabled" : "loop disabled") << "]" << std::endl;
	}
};
void * multicastListener(void * arg);
class MulticastDiscoveryManager{
	friend void * multicastListener(void * arg);
public:
	MulticastDiscoveryManager(MulticastDiscoveryConfig config);
	std::string getMultiCastAddressStr() {
		return discoveryConfig.multiCastAddress;
	}
	unsigned getMulticastPort() {
		return discoveryConfig.multicastPort;
	}
	unsigned getCommunicationPort() {
		return discoveryConfig.internalCommunicationPort;
	}

	std::string getInterfaceAddress() {
		return discoveryConfig.interfaceAddress;
	}

	int openListeningChannel();
	int openSendingChannel();
	void init();

	bool isLoopbackMessage(DiscoveryMessage &msg);

	void stopDiscovery() {
		shutdown = true;
	}
	bool isCurrentNodeMaster() {
		return currentNodeMaster;
	}
	void isCurrentNodeMaster(bool isMaster) {
		currentNodeMaster = isMaster;
	}
	unsigned getCurrentNodeId() {
		return this->currentNodeId;
	}
	void setCurrentNodeId(unsigned currentNodeId) {
		this->currentNodeId = currentNodeId;
	}

	unsigned getMasterNodeId() {
		return this->masterNodeId;
	}
	void setMasterNodeId(unsigned masterNodeId) {
		this->masterNodeId = masterNodeId;
	}
	/*
	 *   get next Node id
	 */
	unsigned getNextNodeId();

	in_addr_t getInterfaceNumericAddress() { return interfaceNumericAddr; }

	in_addr_t getPublishedInterfaceNumericAddress() { return publishedInterfaceNumericAddr; }

	bool getDestinatioAddressByNodeId(NodeId id, struct sockaddr_in& addr) {
		if (nodeToAddressMap.count(id) > 0) {
			addr = nodeToAddressMap[id];
			return true;
		}
		return false;
	}

private:

	void sendJoinRequest();

	bool shouldYield(unsigned senderIp, unsigned senderPort);

	void validateConfigSettings(MulticastDiscoveryConfig& config);

	MulticastDiscoveryConfig discoveryConfig;

	in_addr_t interfaceNumericAddr;
	in_addr_t publishedInterfaceNumericAddr;

	in_addr_t multiCastNumericAddr;
	in_addr_t multiCastInterfaceNumericAddr;

	int listenSocket;
	int sendSocket;
	bool _discoveryDone;
	bool currentNodeMaster;
	bool shutdown;
	unsigned uniqueNodeId;  // Continuously increasing number
	unsigned currentNodeId;
	unsigned masterNodeId;
	std::map<NodeId, struct sockaddr_in>  nodeToAddressMap;
};

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

	// User provided IP address of NIC for port binding. Could be 0.0.0.0
	std::string interfaceAddress;

	// Derived IP address of NIC to be used for internal communication. Cannot be 0.0.0.0
	std::string publisedInterfaceAddress;

	// User provided port for internal communication.
	unsigned internalCommunicationPort;

	// only for debug
	void print() {
		std::cout << "transport: [" << interfaceAddress << " : " << internalCommunicationPort << "]" << std::endl;
		std::cout << "discovery: [" << std::endl;
		for (unsigned i = 0; i < knownHosts.size(); ++i) {
			 std::cout << knownHosts[i].ipAddress.c_str() << " : " << knownHosts[i].port << std::endl;
		}
		std::cout << "]" << std::endl;
	}
};

static const int DISCOVERY_RETRY_COUNT = 5;
static const int DISCOVERY_YIELD_WAIT_SECONDS = 3;

void * unicastListener(void * arg);
class UnicastDiscoveryService : public DiscoveryService{

	friend void * unicastListener(void * arg);
public:
	UnicastDiscoveryService(UnicastDiscoveryConfig& config);

	int openListeningChannel();

	int openSendingChannel();

	void init();

	void sendJoinRequest(const std::string& knownHost, unsigned port);

	bool shouldYield(unsigned senderIp, unsigned senderPort);

	void validateConfigSettings(UnicastDiscoveryConfig& config);

	unsigned getCommunicationPort() {
		return discoveryConfig.internalCommunicationPort;
	}

	std::string getInterfaceAddress() {
		return discoveryConfig.interfaceAddress;
	}

	bool isLoopbackMessage(DiscoveryMessage &msg);

	void stopDiscovery() {
		shutdown = true;
	}
	bool isCurrentNodeMaster() {
		return currentNodeMaster;
	}
	void isCurrentNodeMaster(bool isMaster) {
		currentNodeMaster = isMaster;
	}
	unsigned getCurrentNodeId() {
		return this->currentNodeId;
	}
	void setCurrentNodeId(unsigned currentNodeId) {
		this->currentNodeId = currentNodeId;
	}

	unsigned getMasterNodeId() {
		return this->masterNodeId;
	}
	void setMasterNodeId(unsigned masterNodeId) {
		this->masterNodeId = masterNodeId;
	}

	unsigned getNextNodeId();

	in_addr_t getPublishedInterfaceNumericAddress() { return publishedInterfaceNumericAddr; }

	bool getDestinatioAddressByNodeId(NodeId id, struct sockaddr_in& addr) {
		if (nodeToAddressMap.count(id) > 0) {
			addr = nodeToAddressMap[id];
			return true;
		}
		return false;
	}
private:

	UnicastDiscoveryConfig discoveryConfig;

	unsigned int publishedInterfaceNumericAddr;

	std::string matchedKnownHostIp;
	unsigned matchedKnownHostPort;
	bool isWellKnownHost;

	int listenSocket;
	int sendSocket;

	bool _discoveryDone;
	bool currentNodeMaster;
	bool shutdown;
	unsigned uniqueNodeId;  // Continuously increasing number
	unsigned currentNodeId;
	unsigned masterNodeId;
	std::map<NodeId, struct sockaddr_in>  nodeToAddressMap;
};


} /* namespace sharding */
} /* namespace srch2 */
#endif /* DICOVERYMANAGER_H_ */
