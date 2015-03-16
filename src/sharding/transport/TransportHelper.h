#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include "ConnectionMap.h"

namespace srch2 {
namespace httpwrapper {

class TransportUtil {
public:
	static void getIpAddressFromNumber(unsigned ipNumber, string& ipAddress) {
		struct in_addr address;
		address.s_addr = htonl(ipNumber);
		ipAddress = string(inet_ntoa((address)));
	}

};

class TCPConnectHandler
{
public:

	TCPConnectHandler(BoostNetworkService& ios,
			BoostTCP::socket& socket)
    : io_service_(ios),
      timer_(ios),
      socket_(socket), cancelTimeout(false) { }

	void connectWithTimeout(IpAddress remoteAddress, short remotePort, short timeout = 30) {
	    timer_.expires_from_now(boost::posix_time::seconds(timeout));
		socket_.async_connect(
	        BoostTCP::endpoint(remoteAddress, remotePort),
	        boost::bind(&TCPConnectHandler::handleConnect, this, _1, remoteAddress, remotePort));
	    timer_.async_wait(boost::bind(&TCPConnectHandler::handleTimeout, this, remoteAddress, remotePort));
	    io_service_.run_one();
	    io_service_.reset();
	    timer_.cancel();
	    io_service_.run_one();
	    io_service_.reset();
	}
	void handleConnect(const boost::system::error_code& err, IpAddress& remoteAddress, short remotePort)
	{
		io_service_.stop();
		cancelTimeout = true;
		if (err)
		{
			Logger::error("Could not connect to %s:%d", remoteAddress.to_string().c_str(), remotePort);
			throw runtime_error("");
		}
	}

	void handleTimeout(IpAddress& remoteAddress, short remotePort)
	{
		if (!cancelTimeout) {
			io_service_.stop();
			socket_.close();
			Logger::error("Could not connect to %s:%d", remoteAddress.to_string().c_str(), remotePort);
			throw runtime_error("");
		}
	}

private:
	BoostNetworkService& io_service_;
	boost::asio::deadline_timer timer_;
	BoostTCP::socket& socket_;
	bool cancelTimeout;
};

class TCPBlockingReceiver {

public:
	TCPBlockingReceiver(BoostNetworkService& ios,
			BoostTCP::socket& socket)
	: _netIoService(ios), _sendSocket(socket), _messageBuffer(NULL), _messageSize(0), cancelTimeOut(false) { }

	void receiveData(char *messageBuffer, unsigned messageSize, unsigned timeout = 30) {

		_messageBuffer= messageBuffer;
		_messageSize = messageSize;

		BoostNetworkService timerIoService;
		boost::asio::deadline_timer _timer(_netIoService);
		_timer.expires_from_now(boost::posix_time::seconds(timeout));
		_timer.async_wait(boost::bind(&TCPBlockingReceiver::handleTimeout, this));

		_sendSocket.async_receive(boost::asio::buffer(_messageBuffer, _messageSize),
				boost::bind(&TCPBlockingReceiver::handleReceive, this, _1, _2));
		_netIoService.run_one();
		_netIoService.reset();
		_timer.cancel();
		_netIoService.run_one();  // let the timeout handler run.
		_netIoService.reset();
	}

	void handleReceive(const boost::system::error_code& error, std::size_t bytesReceived)  {

		if (bytesReceived  == 0 && error && boost::asio::error::try_again != error.value()) {
			_netIoService.stop();
			Logger::console("%s", error.message().c_str());
			throw runtime_error("");
		} else if (bytesReceived < _messageSize){
			Logger::console("TCPBlockingReceiver:  incomplete message received..retrying %d/%d", bytesReceived, _messageSize);
			_messageSize -= bytesReceived;
			_messageBuffer += bytesReceived;
			_sendSocket.async_receive(boost::asio::buffer(_messageBuffer, _messageSize),
							boost::bind(&TCPBlockingReceiver::handleReceive, this, _1, _2));
		} else {
			ASSERT(bytesReceived == _messageSize);
			Logger::error("receive complete data on a TCP socket");
			cancelTimeOut = true;
			_netIoService.stop();
		}
	}

	void handleTimeout() {
		if (!cancelTimeOut) {
			_netIoService.stop();
			Logger::error("Could not completely receive data on a TCP socket");
			throw runtime_error("");
		}
	}

	BoostNetworkService& _netIoService;
	BoostTCP::socket& _sendSocket;
	char *_messageBuffer;
	unsigned _messageSize;
	bool cancelTimeOut;
};

class TCPBlockingSender {

public:
	TCPBlockingSender(BoostNetworkService& ios,
			BoostTCP::socket& socket)
	: _netIoService(ios), _sendSocket(socket), _messageBuffer(NULL), _messageSize(0), cancelTimeOut(false) { }

	void sendData(char *messageBuffer, unsigned messageSize, unsigned timeout = 30) {

		_messageBuffer= messageBuffer;
		_messageSize = messageSize;

		boost::asio::deadline_timer _timer(_netIoService);
		_timer.expires_from_now(boost::posix_time::seconds(timeout));
		_timer.async_wait(boost::bind(&TCPBlockingSender::handleTimeout, this));
		_sendSocket.async_send(boost::asio::buffer(_messageBuffer, _messageSize),
				boost::bind(&TCPBlockingSender::handleSend, this, _1, _2));
		_netIoService.run_one();
		_netIoService.reset();
		_timer.cancel();
		_netIoService.run_one(); // let the timeout handler run.
		_netIoService.reset();
	}

private:
	void handleSend(const boost::system::error_code& error, std::size_t bytesTransferred)  {

		if (bytesTransferred  == 0 && error && boost::asio::error::try_again != error.value()) {
			_netIoService.stop();
			Logger::console("%s", error.message().c_str());
			throw runtime_error("");
		} else if (bytesTransferred < _messageSize){
			Logger::console("TCPBlockingSender:  incomplete message sent..retrying %d/%d", bytesTransferred, _messageSize);
			_messageSize -= bytesTransferred;
			_messageBuffer += bytesTransferred;
			_sendSocket.async_send(boost::asio::buffer(_messageBuffer, _messageSize),
							boost::bind(&TCPBlockingSender::handleSend, this, _1, _2));
		} else {
			ASSERT(bytesTransferred == _messageSize);
			Logger::error("sent complete data on a TCP socket");
			cancelTimeOut = true;
			_netIoService.stop();
		}
	}

	void handleTimeout() {
		if (!cancelTimeOut) {
			_netIoService.stop();
			Logger::error("Could not completely send data on a TCP socket");
			throw runtime_error("");
		}
	}

	BoostNetworkService& _netIoService;
	BoostTCP::socket& _sendSocket;
	char *_messageBuffer;
	unsigned _messageSize;
	bool cancelTimeOut;
};

//class Transportv2 {
//
//public:
//	Transportv2() {
//
//	}
//
//	void send(NodeId nodeId, Message *message) {
//		// copy message
//		unsigned bodySize = message->getBodySize();
//		Message * copyMessage = MessageAllocator().allocateMessage(bodySize);
//		memcpy(copyMessage, message, sizeof(Message) + bodySize);
//
//		senderLock.lock();
//		// get socket for this nodeId
//		BoostTCP::socket * endpoint = getSocketForNode(nodeId);
//		if (endpoint == NULL) {
//			MessageAllocator().deallocateByMessagePointer(copyMessage);
//			senderLock.unlock();
//			return;
//		}
//		senderLock.unlock();
//	//	sendMessageQueue.put(copyMessage, endpoint, nodeId);
//
//	}
//
//	BoostTCP::socket * getSocketForNode(unsigned nodeId) {
//		map<unsigned, BoostTCP::socket *>::iterator iter =  nodeToSocketMap.find(nodeId);
//		if (iter != nodeToSocketMap.end()){
//			return iter->second;
//		} else {
//			return NULL;
//		}
//	}
//
//private:
//	void flushMessages() {
//		if (sendMessageQueue.size()) {
//			MessageInfo msgInfo = sendMessageQueue.pop();
//			if (msgInfo.message != NULL && msgInfo.endPoint != NULL) {
//
//			}
//		}
//	}
//	BoostNetworkService& _netIoService;
//	BoostTCP::socket& _sendSocket;
//	char *_messageBuffer;
//	unsigned _messageSize;
//	bool cancelTimeOut;
//
//	struct MessageInfo {
//		Message * message;
//		BoostTCP::socket * endPoint;
//		unsigned nodeId;
//	};
//	struct MessageQueue{
//		std::queue<MessageInfo> _mesgQueue;
//		boost::mutex _queueGuard;
//		void put(Message *message, BoostTCP::socket *endPoint, NodeId nodeId) {
//			_queueGuard.lock();
//			MessageInfo msgInfo = {message, endPoint, nodeId};
//			_mesgQueue.push(msgInfo);
//			_queueGuard.unlock();
//		}
//		MessageInfo pop() {
//			_queueGuard.lock();
//			MessageInfo msgInfo = { NULL, NULL, 0};
//			if (_mesgQueue.size())
//				msgInfo = _mesgQueue.front();
//			_mesgQueue.pop();
//			_queueGuard.unlock();
//			return msgInfo;
//		}
//
//		unsigned size() {
//			_queueGuard.lock();
//			unsigned size = _mesgQueue.size();
//			_mesgQueue.pop();
//			_queueGuard.unlock();
//			return size;
//		}
//	} sendMessageQueueArray[MSG_QUEUE_ARRAY_SIZE];
//
//	map<unsigned, BoostTCP::socket *> nodeToSocketMap;
//	boost::mutex senderLock;
//};

}}
