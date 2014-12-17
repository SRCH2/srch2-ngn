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
    unsigned sizeOfMessage = message.getNumberOfBytes();
    char * messageTempBuffer = new char[sizeOfMessage];
    memset(messageTempBuffer, 0, sizeOfMessage);
    message.flag = DISCOVERY_JOIN_CLUSTER_REQ;
    message.interfaceNumericAddress = getTransport()->getPublishedInterfaceNumericAddr();
    message.internalCommunicationPort = getTransport()->getCommunicationPort();

    unsigned byteToCopy = this->clusterIdentifier.size() > 99 ? 99 : this->clusterIdentifier.size();
    strncpy(message._clusterIdent, this->clusterIdentifier.c_str(), byteToCopy);
    message._clusterIdent[byteToCopy] = '\0';

    message.serialize(messageTempBuffer);

    struct sockaddr_in destinationAddress;
    memset(&destinationAddress, 0, sizeof(destinationAddress));
    destinationAddress.sin_family = AF_INET;
    inet_aton(knownHost.c_str(), &destinationAddress.sin_addr);
    destinationAddress.sin_port = htons(port);

    Logger::sharding(Logger::Detail, "DM | Sending join request of %d bytes." , sizeOfMessage);
    int retry = DISCOVERY_RETRY_COUNT;
    //Logger::console("sending MC UDP to %s , %d",discoveryConfig.multiCastAddress.c_str(),  getMulticastPort());
    while(retry) {
		sleep(DISCOVERY_RETRY_COUNT - retry);
        int status = sendUDPPacketToDestination(sendSocket, messageTempBuffer, sizeOfMessage, destinationAddress);
        if (status == 1) {
            --retry;
            continue;
        } else {
            break;
        }
    }
    if(retry == 0){
        Logger::sharding(Logger::Warning, "DM | Unicast : Sending join request of %d bytes FAILED" , sizeOfMessage);
    }

    delete [] messageTempBuffer;
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
    unsigned sizeOfMessage = message.getNumberOfBytes();
    char * messageTempBuffer = new char[sizeOfMessage];
    Logger::console("getNumberOfBytes : %d , sizeof : %d", sizeOfMessage , sizeof(DiscoveryMessage) );
    memset(messageTempBuffer, 0, sizeOfMessage);

    int retryCount = DISCOVERY_RETRY_COUNT;
    struct sockaddr_in senderAddress;

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
        unsigned waitTime = 2;
//        unsigned retryCount
        while(retryCount) {
        	int selectResult = checkSocketIsReady(listenSocket, true, waitTime);
        	if( selectResult == -1){
            	delete [] messageTempBuffer;
                exit(0); // TODO : we exit ?
        	}else if (selectResult == 0){
        		waitTime += 2;
        		retryCount --;
        		continue;
        	}
            int status = readUDPPacketWithSenderInfo(listenSocket, messageTempBuffer, sizeOfMessage, MSG_DONTWAIT, senderAddress);
            if (status == -1) {
                delete [] messageTempBuffer;
                exit(0);
            }
            if (status == 0) {
                message.deserialize(messageTempBuffer);
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
                	waitTime = 2;
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
                                waitTime = 2;
                                for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
                                    if (knownHosts[i].ipAddress.compare(discovery->matchedKnownHostIp) == 0 &&
                                            knownHosts[i].port == discovery->discoveryPort)
                                        continue;
                                    discovery->sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
                                }
                                continue; // go to next try to read a DATA_GRAM which is a discovery message
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
                                strncpy(yeildMessage._clusterIdent, discovery->clusterIdentifier.c_str(), byteToCopy);
                                yeildMessage._clusterIdent[byteToCopy] = '\0';

                                unsigned sizeOfYeildMessage = yeildMessage.getNumberOfBytes();
                                char * yeildMessageTempBuffer = new char[sizeOfYeildMessage];
                                yeildMessage.serialize(yeildMessageTempBuffer);
                                tryYieldMsgAgain:
                                int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, yeildMessageTempBuffer,
                                		sizeOfYeildMessage, senderAddress);
                                if (sendStatus == -1) {
                                    delete [] messageTempBuffer;
                                    delete [] yeildMessageTempBuffer;
                                    exit(-1);
                                }
                                if (sendStatus == 1) {
                                	sleep(1);
                                    goto tryYieldMsgAgain;
                                }
                                ASSERT(sendStatus == 0);
                                delete [] yeildMessageTempBuffer;
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
                        break;
                    } // switch

                    if (masterDetected) {
                        break;  // exit while loop
                    }
                } // else : message is ours
            } // if status == 0
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

            close(listenSocket);
            discovery->_discoveryDone = true;
            delete [] messageTempBuffer;
            return NULL;
        }
        //master was not detected and we had timeout in the discovery loop
        ASSERT(retryCount == 0);
        discovery->isCurrentNodeMaster(true);
        if (!shouldElectItselfAsMaster){
            Logger::console("Cluster may have other masters!.");
        }

        unsigned masterNodeID = discovery->getSyncManager()->getNextNodeId();
        discovery->getSyncManager()->setCurrentNodeId(masterNodeID);
        discovery->getSyncManager()->setMasterNodeId(masterNodeID);
        Logger::console("Current Node (message : %d) is master node and chooses %d as it id.", message.nodeId, masterNodeID);
    }

    // Make the listening socket blocking now.
    int val = fcntl(listenSocket, F_GETFL);
    val &= ~O_NONBLOCK;;
    fcntl(listenSocket, F_SETFL, val);


    // ****** release the execution for other modules ***********************************
    discovery->_discoveryDone = true;
    // ***********************************************************************************

    while(!discovery->shutdown) {
        Logger::console("discovering new nodes at [ >> %d , %d >> ]", listenSocket, discovery->sendSocket);
        memset(&senderAddress, 0, sizeof(senderAddress));
        memset(messageTempBuffer, 0, sizeOfMessage);

        int status = readUDPPacketWithSenderInfo(listenSocket, messageTempBuffer, sizeOfMessage, senderAddress);

        if (status == -1) {
            delete [] messageTempBuffer;
            exit(0);
        }

        if (status == 0) {
            message.deserialize(messageTempBuffer);
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
                    unsigned sizeOfAckMessage = ackMessage.getNumberOfBytes();
                    char * ackMessageTempBuffer = new char[sizeOfAckMessage];
                    ackMessage.flag = DISCOVERY_JOIN_CLUSTER_ACK;
                    ackMessage.interfaceNumericAddress = discovery->getTransport()->getPublishedInterfaceNumericAddr();
                    ackMessage.internalCommunicationPort = discovery->getTransport()->getCommunicationPort();
                    ackMessage.masterNodeId = discovery->getSyncManager()->getCurrentNodeId();
                    ackMessage.nodeId = discovery->getSyncManager()->getNextNodeId();
                    ackMessage.ackMessageIdentifier = message.internalCommunicationPort;

                    unsigned byteToCopy = discovery->clusterIdentifier.size() > 99 ? 99 : discovery->clusterIdentifier.size();
                    strncpy(ackMessage._clusterIdent, discovery->clusterIdentifier.c_str(), byteToCopy);
                    ackMessage._clusterIdent[byteToCopy] = '\0';
                    ackMessage.serialize(ackMessageTempBuffer);

                    tryAckAgain:
                    // send multicast acknowledgment
                    struct sockaddr_in destinationAddress;
                    memset(&destinationAddress, 0, sizeof(destinationAddress));
                    destinationAddress.sin_addr.s_addr = message.interfaceNumericAddress;
                    destinationAddress.sin_family = AF_INET;
                    destinationAddress.sin_port = htons(message.internalCommunicationPort);

                    int sendStatus = sendUDPPacketToDestination(discovery->sendSocket, ackMessageTempBuffer,
                    		sizeOfAckMessage, senderAddress);
                    if (sendStatus == -1) {
                        delete [] messageTempBuffer;
                        delete [] ackMessageTempBuffer;
                        exit(-1);
                    }
                    if (sendStatus == 1) { // may return one in case we miss one corrupted message
                    	sleep(1);
                        goto tryAckAgain;
                    }

                    discovery->getSyncManager()->addNodeToAddressMappping(ackMessage.nodeId, destinationAddress);
                    delete [] ackMessageTempBuffer;
                    break;
                }
                default:
                    ASSERT(false);
                    Logger::console("Invalid flag !!");
                    break;
                }

            }
        } // if status == 0 , message receivied completely.

        if (status == 1) {
            // socket is non-blocking mode. should not get return value 1
            ASSERT(false);
        }
    }
    close(listenSocket);
    delete [] messageTempBuffer;
    return NULL;
}

bool UnicastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
    if (senderIp > getTransport()->getPublishedInterfaceNumericAddr()) {
        return true;
    }
    //    else if ( (senderIp == getTransport()->getPublishedInterfaceNumericAddr() )
    //            && (senderPort > getTransport()->getCommunicationPort())) {
    //        return true;
    //    }
    return false;
}

}}



