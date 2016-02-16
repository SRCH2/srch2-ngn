/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * UnicastDiscoveryService.cpp
 *
 *  Created on: Jun 26, 2014
 */
#include "DiscoveryManager.h"
#include <vector>
#include <util/Logger.h>
#include <sstream>
#include <util/Assert.h>
#include "transport/Message.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

UnicastDiscoveryService::UnicastDiscoveryService(UnicastDiscoverySetting& config,
        SyncManager *syncManager): DiscoveryService(syncManager, config.clusterName), discoveryConfig(config) {
    matchedKnownHostIp = "";
    // throws exception if validation failed.
    validateSetting(discoveryConfig);
}

void UnicastDiscoveryService::validateSetting(UnicastDiscoverySetting& config) {

    // validate IPs of known Hosts
    std::vector<HostAndPort> knownHostsValidatedAddresses;
    const vector<string>& allIpAddresses = getTransportManager()->getAllInterfacesIpAddress();
    for (unsigned i = 0; i < discoveryConfig.knownHosts.size(); ++i) {
    	boost::system::error_code errCode;
    	IpAddress networkIpAddress = IpAddress::from_string(discoveryConfig.knownHosts[i].ipAddress, errCode);
    	if (errCode) {
    		Logger::console("Invalid Known Host Address '%s'", discoveryConfig.knownHosts[i].ipAddress.c_str());
    		throw std::runtime_error(errCode.message());
    	}

    	// 0.0.0.0 is not allowed in well known hosts setting
        if (networkIpAddress ==  IpAddress::any()) {
            Logger::console("Address 0.0.0.0 is not allowed as a known host");
            continue;
        }

        if (this->discoveryConfig.knownHosts[i].port == -1) {
            this->discoveryConfig.knownHosts[i].port = unicastDiscoveryDefaultPort;
        }

        knownHostsValidatedAddresses.push_back(this->discoveryConfig.knownHosts[i]);

        // verify whether current node is one of the well known hosts.
        if (networkIpAddress ==  IpAddress::loopback() && matchedKnownHostIp == "") {
            // if well-known host is set to 127.0.0.1, then the current node is well known host
            matchedKnownHostIp = this->discoveryConfig.knownHosts[i].ipAddress;
            matchedKnownHostPort = this->discoveryConfig.knownHosts[i].port;
        } else {
            const string& ipAddress = this->discoveryConfig.knownHosts[i].ipAddress;
            if (matchedKnownHostIp == "" &&
                    (std::find(allIpAddresses.begin(), allIpAddresses.end(), ipAddress) != allIpAddresses.end())) {
                //Logger::console("current host is one of the known host");
                matchedKnownHostIp = this->discoveryConfig.knownHosts[i].ipAddress;
                matchedKnownHostPort = this->discoveryConfig.knownHosts[i].port;
            }
        }
    }

    if (this->matchedKnownHostIp == "") {
        matchedKnownHostIp = getTransportManager()->getPublisedInterfaceAddress();
        matchedKnownHostPort = unicastDiscoveryDefaultPort;
    }

    Logger::console("Discovery ip:port = %s:%d", matchedKnownHostIp.c_str(), matchedKnownHostPort);

}

void UnicastDiscoveryService::sendJoinRequest(const string& knownHost, unsigned port) {
    DiscoveryMessage message;
    unsigned sizeOfMessage = message.getNumberOfBytes();
    char * messageTempBuffer = new char[sizeOfMessage];
    memset(messageTempBuffer, 0, sizeOfMessage);
    message.type = DISCOVERY_JOIN_CLUSTER_REQ;
    message.interfaceNumericAddress = getTransportManager()->getPublishedInterfaceNumericAddr();
    message.internalCommunicationPort = getTransportManager()->getCommunicationPort();

    unsigned byteToCopy = this->clusterIdentifier.size() > DISCOVERY_CLUSTER_IDENT_SIZE - 1 ?
    		DISCOVERY_CLUSTER_IDENT_SIZE - 1 : this->clusterIdentifier.size();
    strncpy(message._clusterIdent, this->clusterIdentifier.c_str(), byteToCopy);
    message._clusterIdent[byteToCopy] = '\0';

    message.serialize(messageTempBuffer);

    BoostUDP::endpoint destinationEndPoint(IpAddress::from_string(knownHost), port);

    Logger::sharding(Logger::Detail, "DM | Sending join request of %d bytes." , sizeOfMessage);
    int retry = DISCOVERY_RETRY_COUNT;
    // send data to remote destination with retry on failure
    while(retry) {
		sleep(DISCOVERY_RETRY_COUNT - retry);
        int status = sendUDPPacketToDestination(*discoverySocket, messageTempBuffer, sizeOfMessage, destinationEndPoint);
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

void UnicastDiscoveryService::openListeningChannel(){
    /*
     *  Prepare socket data structures.
     */
	discoverySocket = new BoostUDP::socket(networkService);

	discoverySocket->open(asio::ip::udp::v4());
    /*
     *   Make socket non blocking
     */

	asio::socket_base::non_blocking_io command(true);
	discoverySocket->io_control(command);

	unsigned portToBind = this->matchedKnownHostPort;
	BoostUDP::endpoint hostEndPoint(IpAddress::from_string(this->matchedKnownHostIp), portToBind);

    /*
     *  Bind the socket to ip:port. If the binding to given port is not successful then the next
     *  port will be tried. listen_endpoint will contain the port to which binding was actually done.
     */
    bindToAvailablePort(*discoverySocket, hostEndPoint);

    this->discoveryPort = hostEndPoint.port();

    Logger::console("Discovery UDP listen socket binding done on %d", portToBind);

}

void UnicastDiscoveryService::initialDiscoveryHandler(const boost::system::error_code& errCode,
		std::size_t bytes_transferred) {

	if (errCode) {
		if (errCode == asio::error::try_again)
			return;
		else {
			Logger::console("errCode = %d", errCode.value());
			throw runtime_error(errCode.message());
		}
	}
	DiscoveryMessage message;
	message.deserialize(messageBuffer);
	// ignore looped back messages.
	if (isLoopbackMessage(message)) {
		Logger::debug("Loopback message...continuing");
		return;
	} if (!isCurrentClusterMessage(message)) {
		Logger::debug("message from different network using same unicast setting...continuing");
		return;
	} else {
		switch(message.type)
		{
		case DISCOVERY_JOIN_CLUSTER_ACK:
		{
			/*
			 *   Master node is detected. Stop discovery.
			 */
			if (message.ackMessageIdentifier == getTransportManager()->getCommunicationPort()) {
				masterDetected = true;
				getSyncManager()->storeMasterConnectionInfo(message.interfaceNumericAddress,
						message.internalCommunicationPort);
				getSyncManager()->setMasterNodeId(message.masterNodeId);
				Logger::console("Master detected at node = %d", message.masterNodeId);
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
				if ( shouldYield(message.interfaceNumericAddress, message.internalCommunicationPort)){
					Logger::console("Yielding to other node");
					yieldingToAnotherNode = true;
				} else {
					// if not yielding then ask others to yield.
					DiscoveryMessage yeildMessage;
					yeildMessage.type = DISCOVERY_JOIN_CLUSTER_YIELD;
					yeildMessage.interfaceNumericAddress = getTransportManager()->getPublishedInterfaceNumericAddr();
					yeildMessage.internalCommunicationPort = getTransportManager()->getCommunicationPort();
					yeildMessage.masterNodeId = -1;

					unsigned byteToCopy = clusterIdentifier.size() > DISCOVERY_CLUSTER_IDENT_SIZE - 1 ?
							DISCOVERY_CLUSTER_IDENT_SIZE - 1 : clusterIdentifier.size();
					strncpy(yeildMessage._clusterIdent, clusterIdentifier.c_str(), byteToCopy);
					yeildMessage._clusterIdent[byteToCopy] = '\0';

					unsigned sizeOfYeildMessage = yeildMessage.getNumberOfBytes();
					char * yeildMessageTempBuffer = new char[sizeOfYeildMessage];
					yeildMessage.serialize(yeildMessageTempBuffer);
					tryYieldMsgAgain:
					int sendStatus = sendUDPPacketToDestination(*discoverySocket, yeildMessageTempBuffer,
							sizeOfYeildMessage, senderEndPoint);
					if (sendStatus == 1) {
						goto tryYieldMsgAgain;
					}
					delete [] yeildMessageTempBuffer;
					Logger::console("Yeild message sent !!");
				}

			}
			break;
		}
		case DISCOVERY_JOIN_CLUSTER_YIELD:
		{
			/*
			 *  Got yield message form other node. Obey.
			 */
			Logger::console("Yielding to other node");
			yieldingToAnotherNode = true;
			break;
		}
		default:
			ASSERT(false);
			break;
		} // switch(message.type)
	} // else

	if (yieldingToAnotherNode || masterDetected) {
		networkService.stop();
	}
}

void UnicastDiscoveryService::timeoutHandler(const boost::system::error_code& errCode) {
	networkService.stop();
}

void UnicastDiscoveryService::discoverCluster() {

    DiscoveryMessage message;
    unsigned sizeOfMessage = message.getNumberOfBytes();
    messageBuffer = new char[sizeOfMessage];
    memset(messageBuffer, 0, sizeOfMessage);

    startDiscovery:
    yieldingToAnotherNode = false;
    masterDetected = false;

    /*
     *   1st: Asynchronously broadcast ping to all well known hosts except itself.
     */
    const vector<HostAndPort>& knownHosts = this->discoveryConfig.knownHosts;
    for (unsigned i = 0 ; i < knownHosts.size(); ++i) {
    	if (knownHosts[i].ipAddress.compare(this->matchedKnownHostIp) == 0 &&
    			knownHosts[i].port == this->discoveryPort)
    		continue;
    	sendJoinRequest(knownHosts[i].ipAddress, knownHosts[i].port);
    }

    // setup a timeout timer.
    asio::deadline_timer timer(networkService);
    timer.expires_from_now(boost::posix_time::seconds(DISCOVERY_TIMEOUT));

    while(true) {
    	// Work1: non blocking wait for data on the socket. When the data arrives "initialDiscoveryHandler" will be called
    	// Event: Data arrival on socket
    	// Handler: initialDiscoveryHandler
    	discoverySocket->async_receive_from(asio::buffer(messageBuffer, sizeOfMessage),
    			senderEndPoint, boost::bind(&UnicastDiscoveryService::initialDiscoveryHandler, this, _1, _2));

    	// Work2: non blocking wait for timeout. When the Timeout happens "timeoutHandler" will be called
    	// Event: timeout
    	// Handler: timeoutHandler
    	timer.async_wait(boost::bind(&UnicastDiscoveryService::timeoutHandler, this, _1));

    	// Run the boost network IO service. It blocks till work1 and work2 are both done OR stop() is called.
    	// Work1 and work2 are considered to be done when the "Event" arrives for which they are waiting and their
    	// respective "Handlers" are called.
    	networkService.run();

    	// reset is required by library ..no special logic here.
    	networkService.reset();

    	// If master was detected or Node is yielding to other node in discovery race, exit the loop
    	if (masterDetected || yieldingToAnotherNode)
    		break;

    	// also check whether timeout occurred..if yes then exit.
    	if (timer.expires_at() <= asio::deadline_timer::traits_type::now()) {
    		break;
    	}
    }

    // Restart discovery process after waiting if there are other nodes in discovery race.
    // It is probable that one of the node will become master till then.
	if (yieldingToAnotherNode) {
		sleep(DISCOVERY_TIMEOUT);
		goto startDiscovery;
	}

    if (masterDetected) {
    	// If master detected then exit the discovery.
    	setCurrentNodeMasterFlag(false);
    	discoverySocket->close();
    } else {
    	// If No master is detected then choose itself as master.
    	// Do NOT close the discovery socket because the master node will
    	// use it for processing cluster join request ( done in a separate thread).
    	setCurrentNodeMasterFlag(true);
    	unsigned masterNodeID = getSyncManager()->getNextNodeId();
    	Logger::console("Current Node is master with id = %d", masterNodeID);
    	getSyncManager()->setCurrentNodeId(masterNodeID);
    	getSyncManager()->setMasterNodeId(masterNodeID);
    }

	delete [] messageBuffer;
	return;
}

// Used only for the master node because master node handles all discovery request
void * unicastDiscoveryListner(void *arg) {
	UnicastDiscoveryService *discovery = (UnicastDiscoveryService *) arg;
	discovery->postDiscoveryListener();
	return NULL;
}

void UnicastDiscoveryService::postDiscoveryListener() {

	// Make the listening socket blocking now.
	asio::socket_base::non_blocking_io command(false);
	discoverySocket->io_control(command);

    DiscoveryMessage message;
    unsigned sizeOfMessage = message.getNumberOfBytes();
    messageBuffer = new char[sizeOfMessage];
    memset(messageBuffer, 0, sizeOfMessage);

    boost::system::error_code errCode;

    while(!shutdown) {

    	std::size_t byteRead = discoverySocket->receive_from(
    			boost::asio::buffer(messageBuffer, sizeOfMessage), senderEndPoint, 0, errCode);

    	if (errCode) {
    		Logger::error("%s", errCode.message().c_str());
    		continue;
    	}

        if (byteRead == sizeOfMessage) {
            message.deserialize(messageBuffer);
            if (isLoopbackMessage(message)) {
                continue;
            } if (!isCurrentClusterMessage(message)) {
                Logger::console("message from different network using same unicast setting...continuing");
                continue;
            } else {
                switch(message.type)
                {
                case DISCOVERY_JOIN_CLUSTER_ACK:
                {
                    Logger::console("ERROR:: Multiple masters present in the cluster");
                    break;
                }
                case DISCOVERY_JOIN_CLUSTER_REQ:
                {
                    // TODO: what happens if node has sent join request already ??
                    Logger::console("Got cluster joining request!!");
                    DiscoveryMessage ackMessage;
                    unsigned sizeOfAckMessage = ackMessage.getNumberOfBytes();
                    char * ackMessageTempBuffer = new char[sizeOfAckMessage];
                    ackMessage.type = DISCOVERY_JOIN_CLUSTER_ACK;
                    ackMessage.interfaceNumericAddress = getTransportManager()->getPublishedInterfaceNumericAddr();
                    ackMessage.internalCommunicationPort = getTransportManager()->getCommunicationPort();
                    ackMessage.masterNodeId = getSyncManager()->getCurrentNodeId();

                    ackMessage.ackMessageIdentifier = message.internalCommunicationPort;
                    unsigned byteToCopy = clusterIdentifier.size() > DISCOVERY_CLUSTER_IDENT_SIZE - 1 ?
                    		DISCOVERY_CLUSTER_IDENT_SIZE - 1 : clusterIdentifier.size();
                    strncpy(ackMessage._clusterIdent, clusterIdentifier.c_str(), byteToCopy);
                    ackMessage._clusterIdent[byteToCopy] = '\0';
                    ackMessage.serialize(ackMessageTempBuffer);

                    tryAckAgain:
                    // send unicast acknowledgment

                    int sendStatus = sendUDPPacketToDestination(*discoverySocket, ackMessageTempBuffer,
                    		sizeOfAckMessage, senderEndPoint);

                    if (sendStatus == 1) { // may return one in case we miss one corrupted message
                        goto tryAckAgain;
                    }
                    if (sendStatus == 0) {
                    	//getSyncManager()->addNodeToAddressMappping(ackMessage.nodeId, message.interfaceNumericAddress,
                    	//		message.internalCommunicationPort);
                    }
                    delete [] ackMessageTempBuffer;
                    break;
                }
                default:
                    ASSERT(false);
                    Logger::console("[Discovery] Invalid message received !!");
                    break;
                }

            }
        } // if (byteRead == sizeOfMessage) , message receivied completely.
    }
    discoverySocket->close();
    delete [] messageBuffer;
}

bool UnicastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
    if (senderIp > getTransportManager()->getPublishedInterfaceNumericAddr()) {
        return true;
    }
    return false;
}

}}



