/*
 * DicoveryManager.h
 *
 *  Created on: Apr 24, 2014
 *      Author: Surendra
 */

#ifndef __SHARDING_DICOVERYMANAGER_H__
#define __SHARDING_DICOVERYMANAGER_H__

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1;

#include <stdexcept>
#include <sys/types.h>
#include <map>
#include <iostream>
#include <string>
#include <vector>

#include "configuration/ShardingConstants.h"
#include "synchronization/SynchronizerManager.h"
#include "core/util/SerializationHelper.h"

#include "boost/asio.hpp"

using namespace boost;

namespace srch2 {
namespace httpwrapper {

typedef asio::ip::udp BoostUDP;
typedef asio::ip::address_v4 IpAddress;

/*
 *  Discovery message flags used for identifying message type
 */
static const int16_t DISCOVERY_JOIN_CLUSTER_REQ = 0x0;
static const int16_t DISCOVERY_JOIN_CLUSTER_ACK = 0x1;
static const int16_t DISCOVERY_JOIN_CLUSTER_YIELD =  0x2;
static const int16_t DISCOVERY_JOIN_CLUSTER_ACK_REPLY = 0x3;

/*
 *  Helper APIs for UDP sockets
 *  Send UDP packet to a destination "destinationAddress" via "sendSocket".
 *  Return status 0 : Success
 *  Return status 1 : Could not send completely because socket was busy. Try again.
 *  Return status -1: Socket error.
 */
int sendUDPPacketToDestination(BoostUDP::socket& sendSocket, const char *buffer, std::size_t bufferSize,
		const BoostUDP::endpoint& destinationAddress);

// specify timeout for initial discovery. If no master is found within timeout
// interval then current node decides itself as master.
static const int DISCOVERY_TIMEOUT = 6;

// default retry count for sending message.
static const int DISCOVERY_RETRY_COUNT = 3;
static const int DISCOVERY_RETRY_TIMEOUT = 2;

// Cluster Identifier Max Size
static const int DISCOVERY_CLUSTER_IDENT_SIZE = 100;

/*
 *  Discovery message structure. Contains information required for initial
 *  discovery of nodes.
 */
struct DiscoveryMessage {
	int16_t type;                        // message identifier
	uint32_t interfaceNumericAddress;    // IP address of sender (unsigned format)
	uint32_t internalCommunicationPort;  // Port of sender used for TM communication.
	NodeId nodeId;                       // New remote node's Id. To be filled by master and sent to remote node on discovery
	NodeId masterNodeId;                 // Master's Id. To be filled by master and sent to remote node on discovery
	uint32_t ackMessageIdentifier;       // To be used for ACK messages only by master (in multicast discovery)
	char _clusterIdent[DISCOVERY_CLUSTER_IDENT_SIZE];             // cluster name ( upto 100 chars)

	DiscoveryMessage(){
		this->type = 0;
		this->interfaceNumericAddress = 0;
		this->internalCommunicationPort = 0;
		this->nodeId = 0;
		this->masterNodeId = 0;
		this->ackMessageIdentifier = 0;
		memset(this->_clusterIdent, 0 , DISCOVERY_CLUSTER_IDENT_SIZE);
	}
	DiscoveryMessage(const DiscoveryMessage & right){
		this->type = right.type;
		this->interfaceNumericAddress = right.interfaceNumericAddress;
		this->internalCommunicationPort = right.internalCommunicationPort;
		this->nodeId = right.nodeId;
		this->masterNodeId = right.masterNodeId;
		this->ackMessageIdentifier = right.ackMessageIdentifier;
		memcpy(this->_clusterIdent, right._clusterIdent , DISCOVERY_CLUSTER_IDENT_SIZE);
	}
	DiscoveryMessage & operator=(const DiscoveryMessage & right){
		if(this == &right){
			return *this;
		}
		new (this) DiscoveryMessage(right);
		return *this;
	}
	// get number of bytes required to serialized this class.
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(type) ;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(interfaceNumericAddress) ;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(internalCommunicationPort) ;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(nodeId) ;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(masterNodeId) ;
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(ackMessageIdentifier) ;
		numberOfBytes += DISCOVERY_CLUSTER_IDENT_SIZE;
		return numberOfBytes;
	}

	// serialize this class into the "buffer"
	void * serialize(void * buffer) const{
		buffer = srch2::util::serializeFixedTypes(type, buffer);
		buffer = srch2::util::serializeFixedTypes(interfaceNumericAddress, buffer);
		buffer = srch2::util::serializeFixedTypes(internalCommunicationPort, buffer);
		buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
		buffer = srch2::util::serializeFixedTypes(masterNodeId, buffer);
		buffer = srch2::util::serializeFixedTypes(ackMessageIdentifier, buffer);
		memcpy(buffer, _clusterIdent, DISCOVERY_CLUSTER_IDENT_SIZE);
		buffer = ((char *)buffer) + DISCOVERY_CLUSTER_IDENT_SIZE;
		return buffer;
	}
	// deserialize this class from the "buffer"
	void * deserialize(void * buffer){
		buffer = srch2::util::deserializeFixedTypes(buffer ,type);
		buffer = srch2::util::deserializeFixedTypes(buffer ,interfaceNumericAddress);
		buffer = srch2::util::deserializeFixedTypes(buffer ,internalCommunicationPort);
		buffer = srch2::util::deserializeFixedTypes(buffer ,nodeId);
		buffer = srch2::util::deserializeFixedTypes(buffer ,masterNodeId);
		buffer = srch2::util::deserializeFixedTypes(buffer ,ackMessageIdentifier);
		memcpy(_clusterIdent, buffer, DISCOVERY_CLUSTER_IDENT_SIZE);
		buffer = ((char *)buffer) + DISCOVERY_CLUSTER_IDENT_SIZE;
		return buffer;
	}
};

// stores IP (unsigned) and port information of node.
struct NodeEndPoint {
	unsigned long ip;
	unsigned short port;

	NodeEndPoint(unsigned long ip, unsigned short port) : ip(ip) , port(port) {}
	bool operator < (const NodeEndPoint& rhs)  const{
		if (this->ip == rhs.ip) {
			return this->port < rhs.port;
		} else {
			return this->ip < rhs.ip;
		}
	}
};
/*
 *  This is an abstract base class for all the classes which provide discovery services.
 */
class DiscoveryService{

public:
	DiscoveryService(SyncManager *syncManager, const string& clusterIdentifier) {
		this->syncManager = syncManager;
		this->shutdown = false;
		this->clusterIdentifier = clusterIdentifier;
	}
	// Entry point for discovery module. It should be called to initiate discovery.
	void init();

	// Restart discovery in case of master failure.
	void reInit();

	// stop Master node's discovery thread on engine's shutdown.
	void stopDiscovery() { shutdown = true; }

	virtual ~DiscoveryService() {}

protected:

	/* -------------------------------------------------------------------------
	 * APIs provided by base class.
	 * -------------------------------------------------------------------------
	 */

	// Check whether the message was sent by node itself. ( for multicast discovery)
	bool isLoopbackMessage(DiscoveryMessage &msg);

	// Check whether the message belongs to the same cluster as of the current node.
	// it helps to detect and avoid cross cluster discovery on same network.
	bool isCurrentClusterMessage(DiscoveryMessage &msg);

	void setCurrentNodeMasterFlag(bool flag) { syncManager->setNodeIsMaster(flag); }

	bool isCurrentNodeMaster() { return syncManager->isThisNodeMaster(); }

	SyncManager *getSyncManager() { return  syncManager; }

	TransportManager *getTransportManager() { return  syncManager->getTransportManager(); }

	// Bind the socket to ip:port. If port is not available then try the next "scanRange" (default = 100)
	// ports for availability. Throw error if the port binding is unsuccessful.
	void bindToAvailablePort(BoostUDP::socket& listenSocket, BoostUDP::endpoint& listen_endpoint, short scanRange = 100);

	/* -------------------------------------------------------------------------
	 * APIs to be implemented by derived class
	 * -------------------------------------------------------------------------
	 */
	// Create and configure socket for receiving data.
	virtual void openListeningChannel() = 0;

	// Create and configure socket for sending data.
	virtual void openSendingChannel() = 0;

	// Starts the post discovery thread to accept new cluster join request.
	// Only master node should use this.
	virtual void startDiscoveryThread(bool skipInitialDiscovery = false) = 0;

	// Initial discovery function which is called by all the nodes searching for a cluster.
	// All socket calls in this function are asynchronous with callbacks.
	virtual void discoverCluster() = 0;

//	virtual void closeDiscoveryChannels() { }
//
//	virtual void sendJoinRequest() { }

	/* -------------------------------------------------------------------------
	 * Data members
	 * -------------------------------------------------------------------------
	 */
	// Boost network service which does all the low level socket stuff
	asio::io_service networkService;

	// keep a mapping of a host(node) to NodeId.
	std::map<NodeEndPoint, NodeId> nodeToNodeIdMap;

	// flag to indicate whether we should stop discovery service. Used when the engine is exiting.
	bool shutdown;

	// name of the cluster.
	string clusterIdentifier;

	SyncManager *syncManager;
};

/*
 *  Multicast discovery related Data structures and Class
 */
struct MulticastDiscoverySetting{

	// IP address of Multicast Group. e.g 224.1.1.2
	std::string multiCastAddress;

	// multicast listening port
	unsigned multicastPort;

	// IP address of interface used for sending and receiving multicast packets.
	// Note: this can be same as interfaceAddress used for internal node communication.
	std::string hostIpAddressForMulticast;

	// Time To Live for multicast packet
	int ttl;

	// enabled loopback. 0 = enabled or 1 = false
	u_char enableLoop;

	// name of the cluster to distinguish nodes in same network but different cluster.
	std::string clusterName;

	// only for debug
	void print() {
		Logger::console ("Multicast discovery: [Cluster = %s, IP:Port = %s:%d, TTL : %d]", clusterName.c_str(),
				multiCastAddress.c_str(), multicastPort, ttl);
	}
};

// Entry point for multicast Listener thread.
void * multicastListener(void * arg);

/*
 *   Derived class implementing multicast related logic
 */
class MulticastDiscoveryService : public DiscoveryService{
	friend void * multicastListener(void * arg);
public:
	MulticastDiscoveryService(const MulticastDiscoverySetting& config, SyncManager *syncManager);

private:

	std::string getMultiCastAddress() {
		return discoveryConfig.multiCastAddress;
	}
	unsigned getMulticastPort() {
		return discoveryConfig.multicastPort;
	}

	void startDiscoveryThread(bool skipInitialDiscovery = false) {
		pthread_create(&multicastListenerThread, NULL, multicastListener, this);
		pthread_detach(multicastListenerThread);

	}
	// open listen and send channel for multicast discovery. Creates socket and sets
	// multicast options for the socket.
	void openListeningChannel();
	void openSendingChannel();

	// Initial discovery function which is called by all the nodes searching for a cluster.
	// All socket calls in this function are asynchronous with callbacks.
	void discoverCluster();

	// callback handler for discovery phase.
	void discoveryReadHandler(const boost::system::error_code&, std::size_t);

	// callback handler for initial discovery phase.
	void initialDiscoveryHandler(const boost::system::error_code&, std::size_t);

	// callback handler for initial discovery timeout.
	void timeoutHandler(const boost::system::error_code& errCode);

	// Only called by a master node after the node elect itself as a master. This method
	// handles all the discovery requests from the other nodes. It is blocking API
	// and should run in a separate thread.
	void postDiscoveryListener();

	// callback handler for post-discovery phase (after the master is elected).
	void postDiscoveryReadHandler(const boost::system::error_code&, std::size_t);

	// Send multicast request to join the cluster.
	void sendJoinRequest();

	// Verify whether the IP addresses are valid and some other misc checks.
	void validateSetting(MulticastDiscoverySetting& config);

	// Decide whether a current node should yield to another node in a
	// race to become master. (i.e multiple node started simultaneously).
	// Yield rules:
	// if Node1_IP > Node2_IP, Node2 should Yield
	// if Node1_IP == Node2_IP, then if Node1_port > Node2_port then Node 2 should Yield
	//  Node1_IP == Node2_IP && Node1_port == Node2_port is not allowed.
	bool shouldYield(unsigned senderIp, unsigned senderPort);

	/* -------------------------------------------------------------------------
	 * Data members
	 * -------------------------------------------------------------------------
	 */
	MulticastDiscoverySetting discoveryConfig;

	// numeric representation of multicast IP address
	unsigned long multiCastIPAddrNumeric;

	// numeric representation of the host IP address
	unsigned long hostIPAddrNumeric;

	// socket for incoming traffic.
	BoostUDP::socket *listenSocket;

	// socket for outgoing traffic.
	BoostUDP::socket *sendSocket;

	pthread_t multicastListenerThread;

	// Buffer used for storing discovery message
	char * messageBuffer;

	// Flag to indicate whether a master/cluster was found during the discovery.
	bool masterDetected;

	// Flag to indicate whether a current node should yield to another node in a
	// race to become master. (i.e multiple node started simultaneously)
	bool yieldingToAnotherNode;

	// variable stores ip/port info of a sender (remote node)
	BoostUDP::endpoint senderEndPoint;

	// Stores information for tracking the nodes to which current node
	// yielded in discovery race.
	std::set<NodeEndPoint> yieldNodeSet;

	// timer object to set/handle timeout
	asio::deadline_timer timer;

	// keeps track of retry done to detect a master in a cluster.
	unsigned retryCount;

	// It is a flag to indicate whether the node is in a initial discovery phase ( detecting master)
	// or in a post discovery phase ( detecting incoming nodes as a master).
	bool intialDiscoveryPhase;


};

/*
 * Unicast discovery related Data structures and Class
 */
struct HostAndPort {
	HostAndPort(std::string ip, unsigned port) {
		this->ipAddress = ip;
		this->port = port;
	}
	std::string ipAddress;
	unsigned port;
};
struct UnicastDiscoverySetting {
	std::vector<HostAndPort> knownHosts;
	std::string clusterName;
	// only for debug
	void print() {
		Logger::console("discovery: [ %s : ", clusterName.c_str());
		for (unsigned i = 0; i < knownHosts.size(); ++i) {
			Logger::console("%s : %d", knownHosts[i].ipAddress.c_str(), knownHosts[i].port);
		}
		Logger::console("]");
	}
};

void * unicastDiscoveryListner(void *arg);

// default port for unicast discovery if not specified by user.
const unsigned unicastDiscoveryDefaultPort = 4000;

/*
 *   Derived class implementing unicast related logic
 */
class UnicastDiscoveryService : public DiscoveryService{
	friend void * unicastDiscoveryListner(void * arg);
public:
	UnicastDiscoveryService(UnicastDiscoverySetting& config, SyncManager *syncManager);

private:

	void startDiscoveryThread(bool skipInitialDiscovery = false) {
		pthread_create(&unicastListenerThread, NULL, unicastDiscoveryListner, this);
		pthread_detach(unicastListenerThread);
	}

	// Initial discovery function which is called by all the nodes searching for a cluster.
	// All socket calls in this function are asynchronous with callbacks.
	void discoverCluster();

	// callback handler for initial discovery phase.
	void initialDiscoveryHandler(const boost::system::error_code&, std::size_t);

	// callback handler for initial discovery timeout.
	void timeoutHandler(const boost::system::error_code& errCode);

	// Only called by a master node after the node elect itself as a master. This method
	// handles all the discovery request from the other nodes. It is blocking API and
	// should run in a separate thread.
	void postDiscoveryListener();

	// open listen and send channel for unicast discovery. Basically creates socket
	void openListeningChannel();

	void openSendingChannel() { /*empty because same socket is used for listening and sending */ }

	// send a join request to a known Host (possibly having master node)
	void sendJoinRequest(const std::string& knownHost, unsigned port);

	// Validate IP addresses and other misc checks
	void validateSetting(UnicastDiscoverySetting& config);

	// Decide whether a current node should yield to another node in a
	// race to become master. (i.e multiple node started simultaneously).
	// Yield rules:
	// if Node1_IP > Node2_IP, Node2 should Yield
	// if Node1_IP == Node2_IP, then if Node1_port > Node2_port then Node 2 should Yield
	//  Node1_IP == Node2_IP && Node1_port == Node2_port is not allowed.
	bool shouldYield(unsigned senderIp, unsigned senderPort);

	/* -------------------------------------------------------------------------
	 * Data members
	 * -------------------------------------------------------------------------
	 */
	// store Unicast discovery configuration.
	UnicastDiscoverySetting discoveryConfig;

	// if the current node is one of the known hosts mentioned in the config file
	// then these variables store IP and port information to be used in discovery
	// by the current node.
	std::string matchedKnownHostIp;
	unsigned matchedKnownHostPort;

	// Actual port to which the discovery socket is bound. It can be
	// different from matchedKnownHostPort when there are multiple nodes
	// running on a same machine (host).
	unsigned discoveryPort;

	// send/receive socket
	BoostUDP::socket *discoverySocket;

	// Buffer used for storing discovery message
	char * messageBuffer;

	pthread_t unicastListenerThread;

	// Flag to indicate whether a master/cluster was found during the discovery.
	bool masterDetected;

	// Flag to indicate whether a current node should yield to another node in a
	// race to become master. (i.e multiple nodes started simultaneously)
	bool yieldingToAnotherNode;

	// variable stores ip/port info of a sender (remote node)
	BoostUDP::endpoint senderEndPoint;

};


} /* namespace sharding */
} /* namespace srch2 */
#endif /* __SHARDING_DICOVERYMANAGER_H__ */
