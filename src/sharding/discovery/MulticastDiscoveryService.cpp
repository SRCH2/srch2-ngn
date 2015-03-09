/*
 * MulticastDiscoveryService.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: Surendra
 */
#include "DiscoveryManager.h"
#include <util/Logger.h>
#include <sstream>
#include <util/Assert.h>
#include "transport/Message.h"

using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

MulticastDiscoveryService::MulticastDiscoveryService(const MulticastDiscoveryConfig& config,
        SyncManager *syncManager): DiscoveryService(syncManager, config.clusterName), discoveryConfig(config),
        		timer(networkService), intialDiscoveryPhase(true) {
    // throws exception if validation failed.
    validateConfigSettings(discoveryConfig);

    // If numeric IP address is 0 ( i.e "0.0.0.0"), then get the actual address from the transport Manager.
    if (this->hostIPAddrNumeric == 0) {
        this->hostIPAddrNumeric = getTransport()->getPublishedInterfaceNumericAddr();
    }
}

void MulticastDiscoveryService::validateConfigSettings(MulticastDiscoveryConfig& discoveryConfig) {

    boost::system::error_code errCode;
    IpAddress multiCastAddress = IpAddress::from_string(discoveryConfig.multiCastAddress, errCode);
    if (errCode) {
    	Logger::console("Muticast Address '%s' validation failed", discoveryConfig.multiCastAddress.c_str());
    	throw std::runtime_error(errCode.message());
    }

    this->multiCastIPAddrNumeric = multiCastAddress.to_ulong();

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

    if ((multiCastAddress >= IpAddress::from_string("224.0.0.0") &&
    		multiCastAddress <= IpAddress::from_string("224.0.0.255"))
        ||
        (multiCastAddress >= IpAddress::from_string("239.0.0.0") &&
        multiCastAddress <= IpAddress::from_string("239.255.255.255"))
        ) {
        std::stringstream ss;
        ss << " Reserved Muticast Address = " << discoveryConfig.multiCastAddress << " cannot be used";
        throw std::runtime_error(ss.str());
    }

    /*
     *   Verify that interfaces are mutlicast enabled.
     */

    IpAddress ipAddressOfHost = IpAddress::from_string(discoveryConfig.hostIpAddressForMulticast, errCode);
    if (errCode) {
    	Logger::console(" Host IP Address '%s' validation failed", discoveryConfig.hostIpAddressForMulticast.c_str());
    	throw std::runtime_error(errCode.message());
    }

    this->hostIPAddrNumeric = ipAddressOfHost.to_ulong();
}

void MulticastDiscoveryService::openListeningChannel(){

    /*
     *  Prepare socket data structures.
     */
	listenSocket = new BoostUDP::socket(networkService);
	listenSocket->open(asio::ip::udp::v4());

    /*
     *   Make socket non blocking
     */

	asio::socket_base::non_blocking_io command(true);
	listenSocket->io_control(command);

    /*
     *  set SO_REUSEADDR: This option enables multiple processes on a same host to listen
     *  to multicast packets.
     */
	listenSocket->set_option(BoostUDP::socket::reuse_address(true));

	unsigned short portToBind = discoveryConfig.multicastPort;

	BoostUDP::endpoint listen_endpoint(
			IpAddress::from_string(discoveryConfig.hostIpAddressForMulticast), portToBind);

    /*
     *  Bind the socket to ip:port. If the binding to given port is not successful then the next
     *  port will be tried. listen_endpoint will contain the port to which binding was actually done.
     */

    bindToAvailablePort(*listenSocket, listen_endpoint);

    discoveryConfig.multicastPort = listen_endpoint.port();

    /*
     *  Join multcast group. Both interface address and multicast address are required
     *  to register the application to a multicast group.
     */
	listenSocket->set_option(asio::ip::multicast::join_group(
			IpAddress::from_string(discoveryConfig.multiCastAddress)));


    Logger::console("Multicast Discovery: UDP listen socket binding done on %s : %d", discoveryConfig.hostIpAddressForMulticast.c_str(), portToBind);
    Logger::console("Multicast Discovery: Joining multicast group :  %s", discoveryConfig.multiCastAddress.c_str());

}

void MulticastDiscoveryService::openSendingChannel(){

    /*
     *  Prepare send socket's data structures.
     *  Note: We are using a different socket for sending multicast because standard
     *  (RFC 1122 [Braden 1989]) forbids the use of same socket for sending/receiving
     *  IP datagram packets - Richard Steven's UNP book.
     */
	sendSocket = new BoostUDP::socket(networkService);
	sendSocket->open(asio::ip::udp::v4());

    /*
     *   Make socket non blocking
     */

	asio::socket_base::non_blocking_io command(true);
	sendSocket->io_control(command);

    /*
     *   Set TTL ( Time To Live ). Purpose of TTL in multicast packet is to
     *   limit the scope of the packet to avoid flooding connected networks.
     *
     *   TTL     Scope
     *  ----------------------------------------------------------------------
     *  0     Restricted to the same host. Won't be output by any interface.
     *  1     Restricted to the same subnet. Won't be forwarded by a router.
     *  <32   Restricted to the same site, organization or department. (SRCH2 Default = 31)
     *  <64   Restricted to the same region.
     *  <128  Restricted to the same continent.
     *  <255  Unrestricted in scope. Global.
     *
     */

    unsigned optionVal = discoveryConfig.ttl;
    sendSocket->set_option(asio::ip::multicast::hops(discoveryConfig.ttl));

}

void MulticastDiscoveryService::discoveryReadHandler(const boost::system::error_code& errCode,
		std::size_t bytes_transferred) {
	if (intialDiscoveryPhase) {
		initialDiscoveryHandler(errCode, bytes_transferred);
	} else {
		postDiscoveryReadHandler(errCode, bytes_transferred);
	}

}
void MulticastDiscoveryService::initialDiscoveryHandler(const boost::system::error_code& errCode,
		std::size_t bytes_transferred) {

	DiscoveryMessage message;
	if (errCode) {
		if (errCode == asio::error::try_again) {
			// register again for async read
			listenSocket->async_receive_from(asio::buffer(messageTempBuffer, message.getNumberOfBytes()),
				    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
			return;
		}
		else {
			Logger::console("[Mulitcast Discovery] error = %d", errCode.value());
			throw runtime_error(errCode.message());
		}
	}

	if (bytes_transferred < message.getNumberOfBytes()) {
		// register again for async read
		listenSocket->async_receive_from(asio::buffer(messageTempBuffer, message.getNumberOfBytes()),
						    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
		return;
	}

	message.deserialize(messageTempBuffer);
	// ignore looped back messages.
	if (isLoopbackMessage(message)) {
		Logger::debug("loopback message ...continuing");
	} else if (!isCurrentClusterMessage(message)) {
		Logger::debug("message from different network using same multicast setting...continuing");
	} else {
		switch(message.type)
		{
		case DISCOVERY_JOIN_CLUSTER_ACK:
		{
			/*
			 *   Master node is detected. Stop listening to discovery.
			 *   Start SM messaging with master to get nodeIds and other cluster related info.
			 */
			if (message.ackMessageIdentifier == getTransport()->getCommunicationPort()) {
				masterDetected = true;
				Logger::console("Master node = %d found !", message.masterNodeId);
	            syncManager->addNodeToAddressMappping(message.masterNodeId, message.interfaceNumericAddress, message.internalCommunicationPort);
	            syncManager->setCurrentNodeId(message.nodeId);
	            syncManager->setMasterNodeId(message.masterNodeId);
			}
			break;
		}
		case DISCOVERY_JOIN_CLUSTER_REQ:
		{
			/*
			 * if we receive this message then there is another node which is also
			 * in process of joining the cluster. If there is already a master for this
			 * cluster then it should be fine. Otherwise, there is a race condition for
			 * becoming the master. How to deal with it? solution. use ip address + port as
			 * a tie breaker. The one with higher ip address + port combination will get
			 * preference in this race.
			 */
			if (!masterDetected) {
				Logger::debug("Race to become master detected !!");
				if ( shouldYield(message.interfaceNumericAddress, message.internalCommunicationPort)){
						Logger::debug("Yielding to other node");
						yieldingToAnotherNode = true;
						yieldNodeSet.insert(NodeEndPoint(message.interfaceNumericAddress, message.internalCommunicationPort));
				}
			}
			break;
		}
		default:
			Logger::console("[Multicast Discovery] :  Invalid message flag");
			ASSERT(false);
			break;
		}

	}

	if (!masterDetected && !yieldingToAnotherNode) {
		// register again for async read
		listenSocket->async_receive_from(asio::buffer(messageTempBuffer, message.getNumberOfBytes()),
	    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
	} else {
		networkService.stop();
	}
}

void MulticastDiscoveryService::timeoutHandler(const boost::system::error_code& errCode) {
	if (retryCount) {
		// if retry count is not 0 then decrement the current retry count and reset the timer
		// to expire in DISCOVERY_RETRY_TIMEOUT time from now.
		--retryCount;
		sendJoinRequest();
	    timer.expires_from_now(boost::posix_time::seconds(DISCOVERY_RETRY_TIMEOUT));
    	timer.async_wait(boost::bind(&MulticastDiscoveryService::timeoutHandler, this, _1));
	} else {
		// After max retries stop the discovery service.
		networkService.stop();
	}
}

void MulticastDiscoveryService::discoverCluster() {

    DiscoveryMessage message;
    // number of bytes needed for message (sizeof(message) is wrong because of padding.)
    unsigned sizeOfMessage = message.getNumberOfBytes();
    // temp buffer of this message, we first read bytes into
    // this buffer and then deserialize it into message
    messageTempBuffer = new char[sizeOfMessage];
    memset(messageTempBuffer, 0, sizeOfMessage);

    startDiscovery:
    yieldingToAnotherNode = false;
    masterDetected = false;

    /*
     *  First send a broadcast.
     */
    sendJoinRequest();

    retryCount = DISCOVERY_RETRY_COUNT;
    timer.expires_from_now(boost::posix_time::seconds(DISCOVERY_RETRY_TIMEOUT));

    while(true) {
    	listenSocket->async_receive_from(asio::buffer(messageTempBuffer, sizeOfMessage),
    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
    	timer.async_wait(boost::bind(&MulticastDiscoveryService::timeoutHandler, this, _1));
    	networkService.run();
    	networkService.reset();

    	if (masterDetected || yieldingToAnotherNode) {
    		break;  // exit while loop
    	}

    	if (timer.expires_at() <= asio::deadline_timer::traits_type::now()) {
    		break;
    	}
    }

    if (yieldingToAnotherNode) {
    	sleep(DISCOVERY_TIMEOUT);
    	goto startDiscovery;
    }

    if (masterDetected) {
    	//message.deserialize(messageTempBuffer);
    	setCurrentNodeMasterFlag(false);
    	// remember master's address for further communication.
    	listenSocket->close();
    	sendSocket->close();
    } else {
    	intialDiscoveryPhase = false;
    	setCurrentNodeMasterFlag(true);
    	unsigned masterNodeID = getSyncManager()->getNextNodeId();
    	Logger::console("Current Node is master with id = %d", masterNodeID);
    	getSyncManager()->setCurrentNodeId(masterNodeID);
    	getSyncManager()->setMasterNodeId(masterNodeID);
    }

    return;

}

// Entry point for post discovery thread.
void * multicastListener(void * arg) {

    MulticastDiscoveryService * discovery = (MulticastDiscoveryService *) arg;
    discovery->postDiscoveryListener();
    return NULL;
}

void MulticastDiscoveryService::postDiscoveryReadHandler(const boost::system::error_code& errCode,
		std::size_t byteRead) {

	DiscoveryMessage message;
	std::size_t sizeOfMessage = message.getNumberOfBytes();

	if (errCode) {
		Logger::error("%s", errCode.message().c_str());
		listenSocket->async_receive_from(asio::buffer(messageTempBuffer, message.getNumberOfBytes()),
	    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
		return;
	}

	if (byteRead == sizeOfMessage) {
		message.deserialize(messageTempBuffer);
		// ignore looped back messages.
		if (isLoopbackMessage(message)) {
			Logger::debug("loopback message ...continuing");;
		}else if (!isCurrentClusterMessage(message)) {
			Logger::debug("message from different network using same multicast setting...continuing");
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
				Logger::console("Got cluster joining request from %s : %d", senderEndPoint.address().to_string().c_str(), senderEndPoint.port());
				/*
				 *  Prepare ack message by populating master's information and send it to multicast address.
				 */
				DiscoveryMessage ackMessage;
				ackMessage.type = DISCOVERY_JOIN_CLUSTER_ACK;
				ackMessage.interfaceNumericAddress = getTransport()->getPublishedInterfaceNumericAddr();
				ackMessage.internalCommunicationPort = getTransport()->getCommunicationPort();
				ackMessage.masterNodeId = syncManager->getCurrentNodeId();

				short remoteNodePort = message.internalCommunicationPort;
				ackMessage.ackMessageIdentifier = remoteNodePort;

				unsigned byteToCopy = clusterIdentifier.size() > DISCOVERY_CLUSTER_IDENT_SIZE - 1 ?
						DISCOVERY_CLUSTER_IDENT_SIZE - 1 : clusterIdentifier.size();
				strncpy(ackMessage._clusterIdent, clusterIdentifier.c_str(), byteToCopy);
				ackMessage._clusterIdent[byteToCopy] = '\0';

				NodeEndPoint hostEndPoint = NodeEndPoint(message.interfaceNumericAddress, remoteNodePort);
				if (nodeToNodeIdMap.find(hostEndPoint) != nodeToNodeIdMap.end()) {
					ackMessage.nodeId = nodeToNodeIdMap[hostEndPoint];
				} else {
					ackMessage.nodeId = syncManager->getNextNodeId();
					nodeToNodeIdMap.insert(make_pair(hostEndPoint, ackMessage.nodeId));
					// also write this node to local nodes copy.
					//        				Node node("", , remoteNodePort, true);
					//        				syncManager->addNewNodeToLocalCopy(node);
				}

				char * ackMessageTempBuffer = new char[sizeOfMessage];
				memset(ackMessageTempBuffer, 0, sizeOfMessage);

				ackMessage.serialize(ackMessageTempBuffer);
				tryAckAgain:

				// send multicast acknowledgment
				BoostUDP::endpoint multicastEndPoint(IpAddress::from_string(discoveryConfig.multiCastAddress),
						discoveryConfig.multicastPort);
				int sendStatus = sendUDPPacketToDestination(*sendSocket, ackMessageTempBuffer,
						sizeOfMessage, multicastEndPoint);

				if (sendStatus == 1) {
					goto tryAckAgain;
				}
				syncManager->addNodeToAddressMappping(ackMessage.nodeId, message.interfaceNumericAddress, message.internalCommunicationPort);

				delete [] ackMessageTempBuffer;
				Logger::debug("Ack Sent !!");
				break;
			}
			default:
				ASSERT(false);
				Logger::console("[Multicast Discovery] Invalid message received !!");
				break;
			}
		}
	}

	listenSocket->async_receive_from(asio::buffer(messageTempBuffer, message.getNumberOfBytes()),
    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));

}

void MulticastDiscoveryService::postDiscoveryListener() {

    DiscoveryMessage message;
    std::size_t sizeOfMessage = message.getNumberOfBytes();

    while(!shutdown) {
    	networkService.run();
    	networkService.reset();
    	listenSocket->async_receive_from(asio::buffer(messageTempBuffer, sizeOfMessage),
    			senderEndPoint, boost::bind(&MulticastDiscoveryService::discoveryReadHandler, this, _1, _2));
    	// _1, _2 indicates that the callback function that we are binding to takes two arguments.
    	// Later on the boost's internal layer will call the callback with two arguments.
    	// read about boost::bind at http://www.radmangames.com/programming/how-to-use-boost-bind
    }

    listenSocket->close();
    sendSocket->close();
	delete [] messageTempBuffer;

}

void MulticastDiscoveryService::sendJoinRequest() {
    DiscoveryMessage message;
    unsigned sizeOfMessage = message.getNumberOfBytes();
    char * messageTempBuffer = new char[sizeOfMessage];
    memset(messageTempBuffer, 0, sizeOfMessage);
    message.type = DISCOVERY_JOIN_CLUSTER_REQ;
    message.interfaceNumericAddress =  getTransport()->getPublishedInterfaceNumericAddr();
    message.internalCommunicationPort = getTransport()->getCommunicationPort();

    unsigned byteToCopy = this->clusterIdentifier.size() > DISCOVERY_CLUSTER_IDENT_SIZE - 1 ?
    		DISCOVERY_CLUSTER_IDENT_SIZE -1 : this->clusterIdentifier.size();
    strncpy(message._clusterIdent, this->clusterIdentifier.c_str(), byteToCopy);
    message._clusterIdent[byteToCopy] = '\0';

    message.serialize(messageTempBuffer);

    BoostUDP::endpoint multicastEndPoint(IpAddress::from_string(discoveryConfig.multiCastAddress),
            					discoveryConfig.multicastPort);

    int retry = DISCOVERY_RETRY_COUNT;
    //Logger::console("sending MC UDP to %s , %d",discoveryConfig.multiCastAddress.c_str(),  getMulticastPort());
    while(retry) {
		sleep(DISCOVERY_RETRY_COUNT - retry);
        int status = sendUDPPacketToDestination(*sendSocket, messageTempBuffer, sizeOfMessage, multicastEndPoint);
        if (status == 1) {
            --retry;
            continue;
        } else {
            break;
        }
    }
    if(retry == 0){
        Logger::sharding(Logger::Warning, "DM | Multicast : Sending join request of %d bytes FAILED" , sizeOfMessage);
    }
	delete [] messageTempBuffer;
}

bool MulticastDiscoveryService::shouldYield(unsigned senderIp, unsigned senderPort) {
	Logger::debug("S %u:%u, C %u:%u", senderIp, senderPort, getTransport()->getPublishedInterfaceNumericAddr(), getTransport()->getCommunicationPort());
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
