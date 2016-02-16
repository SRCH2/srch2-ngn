/*
 * DicoveryManager.cpp
 *
 *  Created on: Apr 24, 2014
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

int sendUDPPacketToDestination(BoostUDP::socket& sendSocket, const char *buffer, std::size_t bufferSize,
		const BoostUDP::endpoint& destinationAddress) {

	boost::system::error_code ec;

	std::size_t byteSent = sendSocket.send_to(boost::asio::buffer(buffer, bufferSize), destinationAddress, MSG_DONTWAIT, ec);

	if (ec) {
		if(ec.value() == asio::error::try_again) {
			return 1;
		} else {
			Logger::console("[Discovery]: %s", ec.message().c_str());
			return -1;
		}
	}

	if (byteSent < bufferSize) {
	    Logger::sharding(Logger::Warning,"Discovery | %d bytes was sent instead of expected %d bytes. This datagram is lost.", byteSent, bufferSize );
		// incomplete read : this datagram is lost, return to caller to handle next one
		return 1;
	}
	ASSERT(bufferSize == byteSent);
	return 0;
}

void DiscoveryService::init() {

    openListeningChannel();

    openSendingChannel();

    discoverCluster();

    if (isCurrentNodeMaster()) {
    	// start a new thread to accept new nodes to cluster.
    	startDiscoveryThread();
    }
}

void DiscoveryService::reInit() {

	ASSERT(isCurrentNodeMaster());

	openListeningChannel();

	openSendingChannel();

	// start a new thread to accept new nodes to cluster.
	startDiscoveryThread();
}

bool DiscoveryService::isLoopbackMessage(DiscoveryMessage &msg){
	return (msg.interfaceNumericAddress == getTransportManager()->getPublishedInterfaceNumericAddr() &&
			msg.internalCommunicationPort == getTransportManager()->getCommunicationPort());
}

bool DiscoveryService::isCurrentClusterMessage(DiscoveryMessage &msg) {
	return (string(msg._clusterIdent).compare(this->clusterIdentifier) == 0);
}

/*
 * Bind the socket to ip:port. If port is not available then try the next "scanRange" (default = 100)
 * ports for availability. Throw error if the port binding is unsuccessful.
 */
void DiscoveryService::bindToAvailablePort(BoostUDP::socket& listenSocket, BoostUDP::endpoint& listen_endpoint, short scanRange) {
	short requestedPort = listen_endpoint.port();
	short portToBind = requestedPort;
	while(1) {
		boost::system::error_code errCode;
		listenSocket.bind(listen_endpoint, errCode);
		if(errCode){
			++portToBind;
			if (portToBind <  requestedPort + scanRange) {
				listen_endpoint.port(portToBind);
			} else {

				Logger::console("unable to bind to any port in range [%d : %d]", requestedPort,
						requestedPort + scanRange);
				throw std::runtime_error(errCode.message());
			}
		} else {
			break; // exit while loop when bind is successful
		}
	}
}

} /* namespace sharding */
} /* namespace srch2 */
