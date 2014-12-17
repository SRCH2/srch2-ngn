/*
 * MulticastDiscoveryService.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: Surendra
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

MulticastDiscoveryService::MulticastDiscoveryService(MulticastDiscoveryConfig config,
        SyncManager *syncManager): DiscoveryService(syncManager, config.clusterName), discoveryConfig(config) {
    // throws exception if validation failed.
    validateConfigSettings(discoveryConfig);
    listenSocket = -1;
    sendSocket = -1;
    _discoveryDone = false;
    skipInitialDiscovery = false;

    if (this->multiCastInterfaceNumericAddr == 0) {
        Logger::console("setting up MC interface as Published Interface.");
        this->multiCastInterfaceNumericAddr = getTransport()->getPublishedInterfaceNumericAddr();
    }

    memset(&multicastGroupAddress, 0, sizeof(multicastGroupAddress));
    inet_aton(discoveryConfig.multiCastAddress.c_str(), &multicastGroupAddress.sin_addr);
    //multicastGroupAddress.sin_addr.s_addr = multiCastNumericAddr;
    multicastGroupAddress.sin_family = AF_INET;
    multicastGroupAddress.sin_port = htons(discoveryConfig.multicastPort);
}

void MulticastDiscoveryService::validateConfigSettings(MulticastDiscoveryConfig& discoveryConfig) {

    struct in_addr ipAddress;
    memset(&ipAddress, 0, sizeof(ipAddress));
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

    /*
     *   Verify that interfaces are mutlicast enabled.
     */
    // ToDO: defensive check ..not critical at this stage of project.
    // verifyMulticastEnabled(this->discoveryConfig.multicastInterface);

    memset(&ipAddress, 0, sizeof(ipAddress));
    if (inet_aton(discoveryConfig.multicastInterface.c_str(), &ipAddress) == 0) {
        std::stringstream ss;
        ss << " Invalid Muticast Interface Address = " << discoveryConfig.multicastInterface;
        throw std::runtime_error(ss.str());
    }

    this->multiCastInterfaceNumericAddr = ipAddress.s_addr;
}

/*
 *   The function is non-blocking and performs the following tasks
 *   1. Creates UDP socket.
 *   2. Setup Multicast parameters for socket at IP layer
 *   3. Bind the socket
 *
 */

int MulticastDiscoveryService::openListeningChannel(){

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
    int status = setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, (void*) &optionVal, sizeof(optionVal));
    if (status < 0) {
        perror("address reuse failed : ");
        exit(-1);
    }
#ifdef SO_REUSEPORT
    status = setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, (void*) &optionVal, sizeof(optionVal));
    if (status < 0) {
        perror("port reuse failed : ");
        exit(-1);
    }
#endif

    /*
     *  Join multcast group. Both interface address and multicast address are required
     *  to register the application to a multicast group.
     */
    struct ip_mreq routeAddress;
    memset(&routeAddress, 0, sizeof(routeAddress));
    routeAddress.imr_multiaddr.s_addr = this->multiCastNumericAddr;
    routeAddress.imr_interface.s_addr = this->multiCastInterfaceNumericAddr;

    status = setsockopt (udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &routeAddress, sizeof(routeAddress));
    if (status < 0) {
        perror("join group failed : ");
        exit(-1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // this->multiCastNumericAddr;
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

    Logger::console("Discovery UDP listen socket binding done on %d", portToBind);

    return udpSocket;
}

int MulticastDiscoveryService::openSendingChannel(){

    /*
     *  Prepare send socket's data structures.
     *  Note: We are using different socket for sending multicast because standard
     *  (RFC 1122 [Braden 1989]) forbids the use of same socket for sending/receiving
     *  IP datagram packets - Richard Steven's UNP book.
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

    unsigned optionVal = discoveryConfig.ttl;
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
//    if (this->multiCastInterfaceNumericAddr == 0) {  // 0.0.0.0
//        this->multiCastInterfaceNumericAddr = getTransport()->getPublishedInterfaceNumericAddr();
//    }

    struct in_addr interfaceAddr;
    interfaceAddr.s_addr = this->multiCastInterfaceNumericAddr;

    int status = setsockopt (udpSocket, IPPROTO_IP, IP_MULTICAST_IF, &interfaceAddr,
            sizeof(interfaceAddr));
    if (status == -1) {
        Logger::console("Invalid interface specified. Using system default.");
    }
    //Logger::console("Discovery UDP sending socket init done.");

    return udpSocket;

}

void * multicastListener(void * arg) {

    MulticastDiscoveryService * discovery = (MulticastDiscoveryService *) arg;
    int listenSocket = discovery->listenSocket;

    DiscoveryMessage message;
    // number of bytes needed for message (sizeof(message) is wrong because of padding.)
    unsigned sizeOfMessage = message.getNumberOfBytes();
    // temp buffer of this message, we first read bytes into
    // this buffer and then deserialize it into message
    char * tempMessageBuffer = new char[sizeOfMessage];
    memset(tempMessageBuffer, 0, sizeOfMessage);

    int retryCount = DISCOVERY_RETRY_COUNT;
    struct sockaddr_in senderAddress;

    // two variables for buffer and buffer index that are copies and it's ok to move them

    if (!discovery->skipInitialDiscovery) {

        bool shouldElectItselfAsMaster = true;
        bool masterDetected = false;

        // initial discovery loop
        bool stop = false;
        unsigned waitTime = 1;
        while(retryCount) {
        	int selectResult = checkSocketIsReady(listenSocket, true, waitTime);
			waitTime += 2;
        	if( selectResult == -1){
            	delete [] tempMessageBuffer;
                exit(0); // TODO : we exit ?
        	}else if (selectResult == 0){
        		continue;
        	}
        	int status = readUDPPacketWithSenderInfo(listenSocket, tempMessageBuffer, sizeOfMessage, MSG_DONTWAIT, senderAddress);
            if (status == -1) {
            	delete [] tempMessageBuffer;
                exit(0); // TODO : we exit ?
            }
            if (status == 0) {
            	message.deserialize(tempMessageBuffer);
            	// ignore looped back messages.
                if (discovery->isLoopbackMessage(message)) {
                    Logger::console("loopback message ...continuing");
                    continue;
                } if (!discovery->isCurrentClusterMessage(message)) {
                	Logger::console("message from different network using same multicast setting...continuing");
                	continue;
                } else {
                	switch(message.flag)
                	{
                	case DISCOVERY_JOIN_CLUSTER_ACK:
                	{
                		Logger::console("Ack received !");
                		/*
                		 *   Master node is detected. Stop listening to discovery.
                		 *   Start SM messaging with master to get nodeIds and other cluster related info.
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
                				sleep((DISCOVERY_RETRY_COUNT + 2) * 2);
                				retryCount = DISCOVERY_RETRY_COUNT;
                				discovery->sendJoinRequest();
                				continue;
                			}

                		}
                		break;
                	}
                	default:
                		Logger::console("Invalid flag");
                		ASSERT(false);
                		break;
                	}

                	if (masterDetected) {
                        break;  // exit while loop
                    }
                }
            } // status == 0
            --retryCount;
        }
        discovery->isCurrentNodeMaster(!masterDetected);
        if (masterDetected) {

            Logger::console("Master node = %d ", message.masterNodeId);
            Logger::console("Curernt node = %d ", message.nodeId);
            senderAddress.sin_port = htons(message.internalCommunicationPort);
            discovery->syncManager->addNodeToAddressMappping(message.masterNodeId, senderAddress);
            discovery->syncManager->setCurrentNodeId(message.nodeId);
            discovery->syncManager->setMasterNodeId(message.masterNodeId);
            discovery->_discoveryDone = true;
            close(discovery->listenSocket);
            close(discovery->sendSocket);
            delete [] tempMessageBuffer;
            return NULL;
        }

        uint32_t masterNodeID = discovery->syncManager->getNextNodeId();
        discovery->syncManager->addNodeToAddressMappping(masterNodeID, senderAddress);
        discovery->syncManager->setCurrentNodeId(masterNodeID);
        discovery->syncManager->setMasterNodeId(masterNodeID);
        discovery->_discoveryDone = true;

        //Else master was not detected and we had timeout in the discovery loop
        ASSERT(retryCount == 0);
        Logger::console("Current Node is master node");
        if (!shouldElectItselfAsMaster){
            Logger::console("Cluster may have other masters!.");
        }
    }
    // Make the listening socket blocking now.
    int val = fcntl(listenSocket, F_GETFL);
    val &= ~O_NONBLOCK;;
    fcntl(listenSocket, F_SETFL, val);

    while(!discovery->shutdown) {
        memset(&senderAddress, 0, sizeof(senderAddress));
        memset(tempMessageBuffer, 0, sizeOfMessage);

        int status = readUDPPacketWithSenderInfo(listenSocket, tempMessageBuffer, sizeOfMessage, senderAddress);

        if (status == -1) {
        	delete [] tempMessageBuffer;
        	exit(0); // TODO just exit ?
        }

        if (status == 0) {
            message.deserialize(tempMessageBuffer);
        	// ignore looped back messages.
        	if (discovery->isLoopbackMessage(message)) {
        		Logger::console("loopback message ...continuing");
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
        			// TODO
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
        			ackMessage.masterNodeId = discovery->syncManager->getCurrentNodeId();
        			ackMessage.nodeId = discovery->syncManager->getNextNodeId();
        			ackMessage.ackMessageIdentifier = message.internalCommunicationPort;

        			unsigned byteToCopy = discovery->clusterIdentifier.size() > 99 ? 99 : discovery->clusterIdentifier.size();
        			strncpy(ackMessage._clusterIdent, discovery->clusterIdentifier.c_str(), byteToCopy);
        			ackMessage._clusterIdent[byteToCopy] = '\0';
        			// size of this message in bytes
        			unsigned sizeOfAckMessage = ackMessage.getNumberOfBytes();
        			// temp buffer that we write this message on and then write it to socket
        			char * ackMessageTempBuffer = new char[sizeOfAckMessage];
        			// write message on buffer
        			ackMessage.serialize(ackMessageTempBuffer);
        			tryAckAgain:
        			// send multicast acknowledgment
        			int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, ackMessageTempBuffer,
        					sizeOfAckMessage, discovery->multicastGroupAddress);
        			if (sendStatus == -1) {
        	        	delete [] tempMessageBuffer;
            			delete [] ackMessageTempBuffer;
        				exit(-1); // TODO : just exit ?
        			}
        			if (sendStatus == 1) {
        				goto tryAckAgain;
        			}
        			senderAddress.sin_port = htons(message.internalCommunicationPort);
        			discovery->syncManager->addNodeToAddressMappping(ackMessage.nodeId, senderAddress);
        			delete [] ackMessageTempBuffer;
        			break;
        		}
        		default:
        			ASSERT(false);
        			Logger::console("Invalid flag !!");
        			break;
        		}

        	}
        }

        if (status == 1) {
        	// socket is non-blocking mode. should not get return value 1
        	ASSERT(false);
        }
    }
    close(discovery->listenSocket);
    close(discovery->sendSocket);
	delete [] tempMessageBuffer;
    return NULL;
}

void MulticastDiscoveryService::init() {

    _discoveryDone = false;

    listenSocket = openListeningChannel();
    sendSocket = openSendingChannel();

    // start a thread to listen for incoming data packet.
    startDiscoveryThread();

    // send request to
    sendJoinRequest();

    while(!_discoveryDone) {
        sleep(1);
    }
}

void MulticastDiscoveryService::reInit() {
    listenSocket = openListeningChannel();
    sendSocket = openSendingChannel();
    startDiscoveryThread(true);
}

void MulticastDiscoveryService::sendJoinRequest() {
    DiscoveryMessage message;
    unsigned sizeOfMessage = message.getNumberOfBytes();
    char * messageTempBuffer = new char[sizeOfMessage];
    memset(messageTempBuffer, 0, sizeOfMessage);
    message.flag = DISCOVERY_JOIN_CLUSTER_REQ;
    message.interfaceNumericAddress =  getTransport()->getPublishedInterfaceNumericAddr();
    message.internalCommunicationPort = getTransport()->getCommunicationPort();

    unsigned byteToCopy = this->clusterIdentifier.size() > 99 ? 99 : this->clusterIdentifier.size();
    strncpy(message._clusterIdent, this->clusterIdentifier.c_str(), byteToCopy);
    message._clusterIdent[byteToCopy] = '\0';

    message.serialize(messageTempBuffer);

    int retry = 3;
    //Logger::console("sending MC UDP to %s , %d",discoveryConfig.multiCastAddress.c_str(),  getMulticastPort());
    while(retry) {
        int status = sendUDPPacketToDestination(sendSocket, messageTempBuffer,
        		sizeOfMessage, multicastGroupAddress);
        if (status == 1) {
            --retry;
            continue;
        } else {
            break;
        }
    }
    if(retry == 0){
        Logger::sharding(Logger::Warning, "Multicast : Sending join request of %d bytes FAILED" , sizeOfMessage);
    }
	delete [] messageTempBuffer;
}

bool MulticastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
    if (senderIp > getTransport()->getPublishedInterfaceNumericAddr()) {
        return true;
    }else if ( (senderIp == getTransport()->getPublishedInterfaceNumericAddr() )
            && (senderPort > getTransport()->getCommunicationPort())) {
        return true;
    }
    return false;
}

} /* namespace sharding */
} /* namespace srch2 */
