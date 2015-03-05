/*
 * DicoveryManager.cpp
 *
 *  Created on: Apr 24, 2014
 *      Author: srch2
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
		return 1; // come back
	}
	ASSERT(bufferSize == byteSent);
	return 0;
}

void DiscoveryService::init() {

    openListeningChannel();

    openSendingChannel();

    // start a thread to listen for incoming data packet.
    discoverCluster();

    if (isCurrentNodeMaster())
    	startDiscoveryThread();
}

void DiscoveryService::reInit() {
	openListeningChannel();

	openSendingChannel();

	startDiscoveryThread();
}

bool DiscoveryService::isLoopbackMessage(DiscoveryMessage &msg){
	return (msg.interfaceNumericAddress == getTransport()->getPublishedInterfaceNumericAddr() &&
			msg.internalCommunicationPort == getTransport()->getCommunicationPort());
}

bool DiscoveryService::isCurrentClusterMessage(DiscoveryMessage &msg) {
	return (string(msg._clusterIdent).compare(this->clusterIdentifier) == 0);
}

} /* namespace sharding */
} /* namespace srch2 */
