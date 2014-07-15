/*
 * MigrationManager.cpp
 *
 *  Created on: Jul 1, 2014
 *      Author: srch2
 */

#include "MigrationManager.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>

#include "util/Logger.h"
#include "util/Assert.h"
#include "server/Srch2Server.h"
#include "discovery/DiscoveryManager.h"

namespace srch2 {
namespace httpwrapper {

const unsigned BLOCK_SIZE =  1400;  // MTU size

// various migration message's body types
struct MigrationInitMsgBody{
	ShardId shardId;
	unsigned shardSize;
	char checksum[16];
};

struct MigrationInitAckMsgBody{
	ShardId shardId;
	unsigned ipAddress;
	short portnumber;

};

struct MigrationInitNackMsgBody{
	ShardId shardId;
};

struct MigrationData {
	ShardId shardId;
	unsigned sequenceNumber;
	char data[BLOCK_SIZE];
};

struct MigrationTermMsgBody{
	ShardId shardId;
};

struct MigrationTermAckMsgBody{
	ShardId shardId;
	char flag;  // 0 = incomplete or 1 = complete
	unsigned missingPacketCount;
	unsigned arr[0];
};

// various Redistribution message's body types
//TODO:

struct MigrationThreadArguments {
	MigrationManager *mm;
	ShardId shardId;
};

void *receiverMigrationInitThreadEntryPoint(void *arg) {

	MigrationThreadArguments * tArgs = (MigrationThreadArguments *)arg;
	MigrationService *clientService = new MigrationService(tArgs->mm);
	clientService->receiveShard(tArgs->shardId);
	delete tArgs;
	return NULL;
}

MMCallBackForTM::MMCallBackForTM(MigrationManager *mm) {
	this->migrationMgr = mm;
}

bool MMCallBackForTM::resolveMessage(Message * incomingMessage, NodeId remoteNode) {

	switch (incomingMessage->getType()) {
	case MigrationInitMessage:
	{
		cout << "Got Init Message ...." << endl;
		// process incoming message
		MigrationInitMsgBody *initMessageBody = (MigrationInitMsgBody *)incomingMessage->getMessageBody();

		if (migrationMgr->hasActiveSession(initMessageBody->shardId)) {
			break;
		} else {
			migrationMgr->initMigrationSession(remoteNode, initMessageBody->shardSize, initMessageBody->shardId);
			//pthread_t& receiverThread = migrationMgr->migrationSession[initMessageBody->shardId].processingThread;
			pthread_t receiverThread;
			MigrationThreadArguments * arg = new MigrationThreadArguments();
			arg->mm = migrationMgr;
			arg->shardId = initMessageBody->shardId;
			pthread_create(&receiverThread, NULL, receiverMigrationInitThreadEntryPoint, arg);
			pthread_detach(receiverThread);
		}
		break;
	}
	case MigrationTermMessage:
	{
		cout << "Got Term Message ...." << endl;
		MigrationTermMsgBody *terminateMessageBody = (MigrationTermMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[terminateMessageBody->shardId].status = MM_STATUS_TERM_REQ_RECEIVED;
		break;
	}
	case MigrationInitAckMessage:
	{
		cout << "Got Init Ack Message ...." << endl;
		// process incoming message
		MigrationInitAckMsgBody *initAckMessageBody = (MigrationInitAckMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[initAckMessageBody->shardId].listeningPort = initAckMessageBody->portnumber;
		migrationMgr->migrationSession[initAckMessageBody->shardId].remoteAddr = initAckMessageBody->ipAddress;
		migrationMgr->migrationSession[initAckMessageBody->shardId].status = MM_STATUS_INIT_ACK_RECEIVED;
		break;
	}
	case MigrationTermAckMessage:
	{
		cout << "Got Term Ack Message ...." << endl;
		MigrationTermAckMsgBody *termAckMessageBody = (MigrationTermAckMsgBody *)incomingMessage->getMessageBody();

		migrationMgr->migrationSession[termAckMessageBody->shardId].termAckMessage.reset(
				new char[incomingMessage->getBodySize()]);
		memcpy(migrationMgr->migrationSession[termAckMessageBody->shardId].termAckMessage.get(),
				termAckMessageBody, incomingMessage->getBodySize());

		migrationMgr->migrationSession[termAckMessageBody->shardId].status = MM_STATUS_TERM_ACK_RECEIVED;

		break;
	}
	default :
		ASSERT(false);

	}
	return true;
}

MigrationManager::MigrationManager(ShardManager *shardManager, TransportManager *transport,
		ConfigManager *config) {
	this->_sessionLock = MigrationManager::UNLOCKED;
	this->shardManager = shardManager;
	this->transport = transport;
	this->sendSocket = openSendChannel();
	this->configManager = config;
	transportCallback = new MMCallBackForTM(this);
	this->transport->registerCallbackHandlerForMM(transportCallback);
}

MigrationManager::~MigrationManager() {
	delete transportCallback;
}

void MigrationManager::sendMessage(unsigned destinationNodeId, Message *message) {
	message->setMigrationMask();
	transport->sendMessage(destinationNodeId, message);

}

void MigrationService::receiveShard(ShardId shardId) {
	int receiveSocket;
	short receivePort;
	migrationMgr->openReceiveChannel(receiveSocket, receivePort);

	migrationMgr->migrationSession[shardId].listeningPort = receivePort;
	migrationMgr->migrationSession[shardId].listeningSocket = receiveSocket;

	Message * initAckMesssage = MessageAllocator().allocateMessage(sizeof(MigrationInitAckMsgBody));
	initAckMesssage->setType(MigrationInitAckMessage);
	MigrationInitAckMsgBody *body = (MigrationInitAckMsgBody *)initAckMesssage->getMessageBody();
	body->shardId = shardId;
	body->portnumber = receivePort;
	body->ipAddress = migrationMgr->transport->getPublishedInterfaceNumericAddr();
	cout << "sending init ack to " << migrationMgr->migrationSession[shardId].remoteNode << endl;
	migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, initAckMesssage);
	MessageAllocator().deallocateByMessagePointer(initAckMesssage);

	migrationMgr->migrationSession[shardId].status = MM_STATUS_FETCHING_DATA;

	unsigned dataReceived = 0;
	unsigned shardSize = migrationMgr->migrationSession[shardId].shardSize;

	char * internalBuffer = new char[shardSize];
	unsigned payLoadSize = sizeof(MigrationData);
	MigrationData *buffer = new MigrationData();
	std::set<unsigned> missedPacket;
	unsigned expectedSeqNumber = 0;

receiveShard:
	unsigned retryCount = 0;
	while(dataReceived < shardSize) {

		signed packetSize;
		sockaddr_in destinationAddress;

		int status = readUDPPacketWithSenderInfo(receiveSocket, (char *)buffer, payLoadSize, destinationAddress);
		if (status == 0) {
			retryCount = 0;
			if ((shardSize - buffer->sequenceNumber * BLOCK_SIZE) > BLOCK_SIZE) {
				packetSize = BLOCK_SIZE;
			}else {
				packetSize = shardSize - buffer->sequenceNumber * BLOCK_SIZE;
			}

			if (packetSize < 0)
				continue;

			memcpy(internalBuffer + buffer->sequenceNumber * BLOCK_SIZE, buffer->data, packetSize);
			dataReceived += packetSize;

			// track sequence number
			if (buffer->sequenceNumber == expectedSeqNumber) {
				++expectedSeqNumber;
			} else if (buffer->sequenceNumber > expectedSeqNumber){
				// assuming there is smaller gap
				for (unsigned start = expectedSeqNumber; start < buffer->sequenceNumber; ++start) {
					missedPacket.insert(start);
				}
				expectedSeqNumber = buffer->sequenceNumber;
			}  else {
				// unordered packet.
				missedPacket.erase(buffer->sequenceNumber);
			}

		} else if (status == 1) {
			if (retryCount > 6) {
				Logger::console("Shard Reception timed out !!");
				break;
			}
			sleep(1);
			++retryCount;
		} else if (status == -1) {
			//close socket
			migrationMgr->migrationSession.erase(shardId);
			delete buffer;
			delete internalBuffer;
			close(receiveSocket);
			//TODO: Notify SHM
			return;
		}
	}

	while(migrationMgr->migrationSession[shardId].status != MM_STATUS_TERM_REQ_RECEIVED) {
		sleep(1);
	}

	cout << dataReceived <<  ":" << shardSize << endl;

	if (dataReceived < shardSize) {
		if (expectedSeqNumber == 0) {
			Logger::console("Shard Reception error !!");
			delete buffer;
			delete internalBuffer;
			close(receiveSocket);
			// TODO: notify SHM
		}
		unsigned maxSeqNumber = shardSize / BLOCK_SIZE;
		unsigned totalPacketMissed = missedPacket.size();

		if (expectedSeqNumber <= maxSeqNumber)
			totalPacketMissed += (maxSeqNumber - expectedSeqNumber + 1);
		unsigned totalPacketSize = sizeof(MigrationTermAckMsgBody) + totalPacketMissed * sizeof(unsigned);

		Message *termAckMessage = MessageAllocator().allocateMessage(totalPacketSize);
		termAckMessage->setType(MigrationTermAckMessage);
		MigrationTermAckMsgBody  *termAckBody = (MigrationTermAckMsgBody *)termAckMessage->getMessageBody();
		termAckBody->flag = 0;
		termAckBody->missingPacketCount = totalPacketMissed;
		termAckBody->shardId = shardId;
		unsigned seq = 0;
		std::set<unsigned>::iterator iter = missedPacket.begin();
		while(iter != missedPacket.end()) {
			termAckBody->arr[seq] = *iter;
			++seq;
		}
		for (unsigned start = expectedSeqNumber; start < maxSeqNumber + 1; ++start) {
			termAckBody->arr[seq] = start;
			++seq;
		}
		migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, termAckMessage);
		MessageAllocator().deallocateByMessagePointer(termAckMessage);
		migrationMgr->migrationSession[shardId].status = MM_STATUS_FETCHING_DATA;
		retryCount = 0;
		sleep(1);
		goto receiveShard;
	} else {

		// send term Ack

		Message *termAckMessage = MessageAllocator().allocateMessage(sizeof(MigrationTermAckMsgBody));
		termAckMessage->setType(MigrationTermAckMessage);
		MigrationTermAckMsgBody  *termAckBody = (MigrationTermAckMsgBody *)termAckMessage->getMessageBody();
		termAckBody->flag = 1;
		termAckBody->missingPacketCount = 0;
		termAckBody->shardId = shardId;
		migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, termAckMessage);
		MessageAllocator().deallocateByMessagePointer(termAckMessage);

		Logger::console("Received shard : %s", shardId.toString().c_str());
		std::istringstream inputStream(ios::binary);
		inputStream.rdbuf()->pubsetbuf(internalBuffer , shardSize);
		inputStream.seekg(0, ios::beg);

		// create an empty shard
		migrationMgr->migrationSession[shardId].shard.reset(
				new Srch2Server(migrationMgr->getIndexConfig(shardId), shardId, 1));

		// Create a path to store this Shard on storage device.
		ConfigManager *configManager = migrationMgr->configManager;
		string directoryPath = configManager->createShardDir(configManager->getClusterWriteView()->getClusterName(),
							configManager->getClusterWriteView()->getCurrentNode()->getName(),
							migrationMgr->getIndexConfig(shardId)->getName(), shardId);

		Logger::console("Saving shard to : %s", directoryPath.c_str());
		migrationMgr->migrationSession[shardId].shard->bootStrapIndexerFromByteStream(inputStream, directoryPath);
		migrationMgr->migrationSession.erase(shardId);
	}

	delete buffer;
	delete internalBuffer;
	cout << "Migration Done" << endl;

}

void MigrationManager::migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, unsigned destinationNodeId) {
	cout << "MIGRATE SHARD To " << destinationNodeId << "...." << endl;

	/*
	 *  Initialize the migration session info
	 */

	this->initMigrationSession(destinationNodeId, 0, shardId);

	/*
	 *  Serialize indexes and calculate size of serialized indexes.
	 */

	std::ostringstream outputBuffer(std::ios_base::binary);
	cout << "trying to serialize the shard ...." << endl;
	shard->serialize(outputBuffer);
	outputBuffer.seekp(0, ios::end);
	unsigned shardSize = outputBuffer.tellp();
	cout << "ostream size = " << outputBuffer.tellp() << endl;

	/*
	 *  Initiate handshaking with MM on the destination node by sending init message and waiting
	 *  for the acknowledgment.
	 */

	Message *initMessage = MessageAllocator().allocateMessage(sizeof(MigrationInitMsgBody));
	initMessage->setType(MigrationInitMessage);
	MigrationInitMsgBody *initMessageBody = (MigrationInitMsgBody *)initMessage->getMessageBody();
	initMessageBody->shardId = shardId;
	initMessageBody->shardSize = shardSize;
	// TODO: initMessageBody->checksum =

	migrationSession[shardId].status = MM_STATUS_WAITING_FOR_INIT_ACK;
	cout << "waiting for init ack "<< endl;
	int tryCount = 3;
	while(migrationSession[shardId].status != MM_STATUS_INIT_ACK_RECEIVED && tryCount) {
		sendMessage(destinationNodeId, initMessage);
		--tryCount;
		sleep(2);
	}
	MessageAllocator().deallocateByMessagePointer(initMessage);
	if (migrationSession[shardId].status != MM_STATUS_INIT_ACK_RECEIVED ) {
		cout << "Unable to migrate shard ..destination node did not respond"<< endl;
		migrationSession.erase(shardId);
		// callback SHM with failure.
		return;
	}

	/*
	 *  Connect to remote node and start data transfer.
	 */
	unsigned remotePort = migrationSession[shardId].listeningPort;
	struct sockaddr_in destinationAddress;
	memset(&destinationAddress, 0, sizeof(destinationAddress));
	destinationAddress.sin_family = AF_INET;
	destinationAddress.sin_addr.s_addr = migrationSession[shardId].remoteAddr;
	destinationAddress.sin_port = htons(remotePort);

	string binaryStream = outputBuffer.str();
	char * buffer = (char *)binaryStream.c_str();
	cout << "string size = " << binaryStream.size() << endl;

	unsigned dataSent = 0;
	unsigned sequenceNumber = 0;
	MigrationData *payLoad = new MigrationData();
	unsigned payLoadSize = sizeof(MigrationData);
	while(dataSent < shardSize) {
		unsigned dataSize = 0;
		if ((shardSize - dataSent) > BLOCK_SIZE)
			dataSize = BLOCK_SIZE;
		else
			dataSize = shardSize - dataSent;

		payLoad->shardId = shardId;
		payLoad->sequenceNumber = sequenceNumber;
		memcpy(payLoad->data, buffer + dataSent, dataSize);

		int status = sendUDPPacketToDestination(sendSocket, (char *)(payLoad)
				, payLoadSize, destinationAddress);
		if (status == 1) {
			cout <<  "packet not sent " << sequenceNumber << endl;
		} else if (status == 0){
			cout <<  "packet sent " << sequenceNumber << endl;
			dataSent += dataSize;
			++sequenceNumber;
		} else {
			cout << "Unable to migrate shard ..network error"<< endl;
			migrationSession.erase(shardId);
			// callback SHM with failure.
			return;
		}
	}
	// data send done..now send migration terminate request.
	Message *termMessage = MessageAllocator().allocateMessage(sizeof(MigrationTermMsgBody));
	termMessage->setType(MigrationTermMessage);
	MigrationTermMsgBody *termMessageBody = (MigrationTermMsgBody *)termMessage->getMessageBody();
	termMessageBody->shardId = shardId;
	sendMessage(destinationNodeId, termMessage);
	MessageAllocator().deallocateByMessagePointer(termMessage);

	migrationSession[shardId].status = MM_STATUS_WAITING_FOR_TERM_ACK;
	cout << "waiting for terminate ack "<< endl;
	while(migrationSession[shardId].status != MM_STATUS_TERM_ACK_RECEIVED) {
		sleep(1);
	}
	MigrationTermAckMsgBody *termAckMessageBody = (MigrationTermAckMsgBody *) migrationSession[shardId].termAckMessage.get();
	cout << "Term status " << (int)termAckMessageBody->flag <<  endl;

	if (termAckMessageBody->flag == 0) {
		unsigned sentPacket = 0;
		while( sentPacket < termAckMessageBody->missingPacketCount) {
			unsigned missedSeqNumber = termAckMessageBody->arr[sentPacket];
			signed dataSize = 0;
			if ((shardSize - missedSeqNumber * BLOCK_SIZE) > BLOCK_SIZE) {
				dataSize = BLOCK_SIZE;
			}else {
				dataSize = shardSize - missedSeqNumber * BLOCK_SIZE;
			}

			if (dataSize < 0) {
				cout << "Unable to migrate shard ..Invalid packet size"<< endl;
				migrationSession.erase(shardId);
				// callback SHM with failure.
				return;
			}

			payLoad->shardId = shardId;
			payLoad->sequenceNumber = missedSeqNumber;
			memcpy(payLoad->data, buffer + missedSeqNumber * BLOCK_SIZE, dataSize);

			int status = sendUDPPacketToDestination(sendSocket, (char *)(payLoad)
					, payLoadSize, destinationAddress);
			if (status == 1) {
				cout <<  "packet not sent " << missedSeqNumber << endl;
			} else if (status == 0){
				cout <<  "packet sent " << missedSeqNumber << endl;
				++sentPacket;
			} else {
				cout << "Unable to migrate shard ..network error"<< endl;
				migrationSession.erase(shardId);
				// callback SHM with failure.
				return;
			}
		}

	} else {
		migrationSession.erase(shardId);
		// callback SHM with success message.
		Logger::console("Migrated shard %s", shardId.toString().c_str());
	}
}

void MigrationManager::migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard,
		std::vector<unsigned> destinationNodeId) {
	//TODO:
}

void MigrationManager::openReceiveChannel(int& udpSocket , short& receivePort) {
	/*
	 *  Prepare socket data structures.
	 */
	if((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("listening socket failed to init");
		exit(255);
	}
	/*
	 *   Make socket non blocking
	 */

	fcntl(udpSocket, F_SETFL, O_NONBLOCK);

	// Todo: Try changing socket buffer size. Ideal buffer size ?

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	inet_aton(transport->getPublisedInterfaceAddress().c_str(), &addr.sin_addr);
	receivePort = MM_MIGRATION_PORT_START;
	addr.sin_port = htons(receivePort);
tryNextPort:
	if( bind(udpSocket, (struct sockaddr *) &addr, sizeof(sockaddr_in)) < 0){
		++receivePort;
		if (receivePort < MM_MIGRATION_PORT_START + 100) {
			addr.sin_port = htons(receivePort);
			goto tryNextPort;
		} else {
			perror("");
			Logger::console("unable to bind to port for shard migration in range = [%d : %d]",
					MM_MIGRATION_PORT_START, MM_MIGRATION_PORT_START + 100);
			exit(-1);
		}
	}

}

int MigrationManager::openSendChannel() {
	/*
	 *  Prepare socket data structures.
	 */
	int udpSocket;
	if((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("sending socket failed to init");
		exit(255);
	}
	/*
	 *   Make socket non blocking
	 */
	//fcntl(udpSocket, F_SETFL, O_NONBLOCK);


	// Todo: Try changing socket buffer size. Ideal buffer size ?

	return udpSocket;

}

const CoreInfo_t * MigrationManager::getIndexConfig(ShardId shardId) {
	boost::shared_ptr<const srch2::httpwrapper::Cluster> clusterReadview;
	this->configManager->getClusterReadView(clusterReadview);
	return clusterReadview->getCoreById(shardId.coreId);
}

bool MigrationManager::hasActiveSession(const ShardId& shardId) {
	if (migrationSession.find(shardId) == migrationSession.end()) {
		return false;
	} else  {
		return true;
	}
}

void MigrationManager::initMigrationSession(unsigned remoteNode, unsigned shardSize, ShardId shardId) {

	MigrationSessionInfo * info = new MigrationSessionInfo();
	info->beginTimeStamp = 0;
	info->endTimeStamp = 0;
	info->listeningSocket = -1;
	info->listeningPort = -1;
	info->status = MM_STATUS_MIGRATION_BEGIN;
	info->remoteNode = remoteNode;
	info->shardSize = shardSize;
	//info.shard.reset();
	migrationSession[shardId] = *info;
}
} /* namespace httpwrapper */
} /* namespace srch2 */
