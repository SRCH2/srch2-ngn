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
#ifndef __MACH__
#include <sys/sendfile.h>
#endif
#include <sys/mman.h>

namespace srch2 {
namespace httpwrapper {

const unsigned BLOCK_SIZE =  1400;  // MTU size

// various migration message's body types
struct MigrationInitMsgBody{
	ShardId shardId;
	unsigned shardComponentCount;
};

struct MigrationInitAckMsgBody{
	ShardId shardId;
	unsigned ipAddress;
	short portnumber;
};

struct ShardComponentInfoMsgBody {
	ShardId shardId;
	unsigned componentSize;
	unsigned componentNameSize;
	char name[0];
};

struct ShardComponentInfoAckMsgBody{
	ShardId shardId;
};

struct MigrationData {
	ShardId shardId;
	unsigned sequenceNumber;
	char data[BLOCK_SIZE];
};

struct MigrationDoneMsgBody{
	ShardId shardId;
};

struct MigrationDoneAckMsgBody{
	ShardId shardId;
	char flag;  // 0 = incomplete or 1 = complete
	unsigned missingPacketCount;
	unsigned arr[0];
};


bool IndexSizeComparator(std::pair<string, long> l,  std::pair<string, long> r) {
	if (l.second > r.second)
		return true;   // put larger index first
	else
		return false;
}

int checkSocketIsReadyForRead(int socket) {
	/*
	 *  Prepare data structure for select system call.
	 *  http://man7.org/linux/man-pages/man2/select.2.html
	 */
	fd_set selectSet;
	timeval waitTimeout;
	waitTimeout.tv_sec = 1;
	waitTimeout.tv_usec = 0;
	FD_ZERO(&selectSet);
	FD_SET(socket, &selectSet);


	/*
	 *   Wait until timeout = 1sec or until socket is ready for read/write. (whichever occurs first)
	 *   see select man page : http://linux.die.net/man/2/select
	 */
	int result = 0;
	// pass select set to read argument
	result = select(socket + 1, &selectSet, NULL, NULL, &waitTimeout);
	if (result == -1) {
		perror("error while waiting for a socket to become available for read/write!");
	}
	return result;
}

int readDataFromSocketWithRetry(int fd, char *buffer, int byteToRead, int retryCount) {

	while(1) {
		int readByte = recv(fd, buffer, byteToRead, MSG_DONTWAIT);
		if(readByte == 0) {
			// the connection is closed by peer. return status -1 (error)
			return -1;
		}

		if(readByte == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				// socket is not ready for read. try again.
				if (retryCount) {
					checkSocketIsReadyForRead(fd);
					--retryCount;
					continue;
				} else {
					Logger::console("read timeout");
					return -1;
				}
			} else {
				perror("Error while reading data from socket : ");
				//some socket error. return status -1 (error)
				return -1;
			}
		}

		if(readByte < byteToRead) {
			buffer += readByte;
			byteToRead -= readByte;
			continue;
		}

		ASSERT(byteToRead == readByte);
		return 0;
	}
	return 0;
}



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

			migrationMgr->initMigrationSession(remoteNode, initMessageBody->shardComponentCount, initMessageBody->shardId);
			migrationMgr->migrationSession[initMessageBody->shardId].status = MM_STATE_INIT_ACK_RCVD;

			pthread_t receiverThread;
			MigrationThreadArguments * arg = new MigrationThreadArguments();
			arg->mm = migrationMgr;
			arg->shardId = initMessageBody->shardId;
			pthread_create(&receiverThread, NULL, receiverMigrationInitThreadEntryPoint, arg);
			pthread_detach(receiverThread);
		}
		break;
	}
	case MigrationComponentBeginMessage:
	{
		cout << "Got Begin Message ...." << endl;
		ShardComponentInfoMsgBody *compInfoMsgBody = (ShardComponentInfoMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[compInfoMsgBody->shardId].shardCompSize = compInfoMsgBody->componentSize;
		migrationMgr->migrationSession[compInfoMsgBody->shardId].shardCompName.assign(compInfoMsgBody->name,
				compInfoMsgBody->componentNameSize);
		migrationMgr->migrationSession[compInfoMsgBody->shardId].status = MM_STATE_INFO_RCVD;
		break;

	}
	case MigrationInitAckMessage:
	{
		cout << "Got Init Ack Message ...." << endl;
		// process incoming message
		MigrationInitAckMsgBody *initAckMessageBody = (MigrationInitAckMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[initAckMessageBody->shardId].listeningPort = initAckMessageBody->portnumber;
		migrationMgr->migrationSession[initAckMessageBody->shardId].remoteAddr = initAckMessageBody->ipAddress;
		migrationMgr->migrationSession[initAckMessageBody->shardId].status = MM_STATE_INIT_ACK_RCVD;
		break;
	}
	case MigrationComponentBeginAckMessage:
	{
		//if (migrationMgr->migrationSession[compInfoMsgBody->shardId].status == MM_STATUS_ACK_RECEIVED)
		cout << "Got Begin Ack Message ...." << endl;
		ShardComponentInfoMsgBody *compInfoMsgBody = (ShardComponentInfoMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[compInfoMsgBody->shardId].status = MM_STATE_INFO_ACK_RCVD;
		break;
	}
	case MigrationComponentEndAckMessage:
	{
		cout << "Got Comp Term Message ...." << endl;
		MigrationDoneMsgBody *completeMsgBody = (MigrationDoneMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[completeMsgBody->shardId].status = MM_STATE_COMPONENT_TRANSFERRED_ACK_RCVD;
		break;
	}
	case MigrationCompleteAckMessage:
	{
		cout << "Got Term Ack Message ...." << endl;
		MigrationDoneMsgBody *completeMsgBody = (MigrationDoneMsgBody *)incomingMessage->getMessageBody();
		migrationMgr->migrationSession[completeMsgBody->shardId].status = MM_STATE_SHARD_TRANSFERRED_ACK_RCVD;
		break;
	}
//	case MigrationCompleteAckMessage:
//	{
//		cout << "Got Term Ack Message ...." << endl;
//		MigrationDoneAckMsgBody *termAckMessageBody = (MigrationDoneAckMsgBody *)incomingMessage->getMessageBody();
//
//		migrationMgr->migrationSession[termAckMessageBody->shardId].termAckMessage.reset(
//				new char[incomingMessage->getBodySize()]);
//		memcpy(migrationMgr->migrationSession[termAckMessageBody->shardId].termAckMessage.get(),
//				termAckMessageBody, incomingMessage->getBodySize());
//
//		migrationMgr->migrationSession[termAckMessageBody->shardId].status = MM_STATUS_TERM_ACK_RECEIVED;
//
//		break;
//	}
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

	migrationMgr->migrationSession[shardId].beginTimeStamp = time(NULL);

	migrationMgr->openTCPReceiveChannel(receiveSocket, receivePort);

	if (receiveSocket == -1) {
		//close socket
		Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
		migrationMgr->migrationSession.erase(shardId);
		//TODO: Notify SHM
		return;
	}

	migrationMgr->migrationSession[shardId].listeningPort = receivePort;

	Logger::console("sending init ack to %d", migrationMgr->migrationSession[shardId].remoteNode);
	migrationMgr->sendInitMessageAck(shardId, receivePort);

	Logger::console("waiting for sender to connect...");
	int commSocket = migrationMgr->acceptTCPConnection(receiveSocket, receivePort);
	if (commSocket == -1) {
		//close socket
		Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
		migrationMgr->migrationSession.erase(shardId);
		close(receiveSocket);
		//TODO: Notify SHM
		return;
	}

	Logger::console("Done");

	unsigned componentCount = migrationMgr->migrationSession[shardId].shardCompCount;

	// Create a path to store this Shard on storage device.
	ConfigManager *configManager = migrationMgr->configManager;
	string directoryPath = configManager->createShardDir(configManager->getClusterWriteView()->getClusterName(),
			configManager->getClusterWriteView()->getCurrentNode()->getName(),
			migrationMgr->getIndexConfig(shardId)->getName(), shardId);
	Srch2Server *migratedShard = new Srch2Server(migrationMgr->getIndexConfig(shardId), shardId, directoryPath);

	unsigned mmapBufferSize = 0;
	void * mmapBuffer = NULL;
	unsigned maxMappedSize = 0;

	for (unsigned i =0; i < componentCount; ++i) {

		Logger::console("waiting for component begin message...");
		//TODO: add timeout
		while(migrationMgr->migrationSession[shardId].status !=  MM_STATE_INFO_RCVD) {
			//sleep(1);
		}
		string filePath = directoryPath + "/" + migrationMgr->migrationSession[shardId].shardCompName;
		int writeFd = open(filePath.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0000644);
		if (writeFd == -1) {
			//close socket
			perror("");   // TODO: replace perror in SM/TM/MM with strerror_r
			Logger::console("Migration:I/O error for shard %s", shardId.toString().c_str());
			migrationMgr->migrationSession.erase(shardId);
			close(commSocket);
			close(receiveSocket);
			//TODO: Notify SHM
			return;
		}

		migrationMgr->sendInfoAckMessage(shardId);

		unsigned componentSize = migrationMgr->migrationSession[shardId].shardCompSize;
		unsigned sequenceNumber = 0;

		int ftruncStatus = ftruncate(writeFd, componentSize);
		if (ftruncStatus == -1) {
			//close socket
			perror("");   // TODO: replace perror in SM/TM/MM with strerror_r
			Logger::console("Migration:I/O error for shard %s", shardId.toString().c_str());
			migrationMgr->migrationSession.erase(shardId);
			close(commSocket);
			close(receiveSocket);
			//TODO: Notify SHM
			return;
		}

#ifdef __MACH__
		mmapBuffer = mmap(NULL, componentSize, PROT_WRITE|PROT_READ, MAP_PRIVATE, writeFd, 0);
#else
		mmapBuffer = mmap64(NULL, componentSize, PROT_WRITE|PROT_READ, MAP_SHARED, writeFd, 0);
#endif

		if (mmapBuffer == MAP_FAILED) {
			perror("");   // TODO: replace perror in SM/TM/MM with strerror_r
			Logger::console("Migration error for shard %s", shardId.toString().c_str());
			migrationMgr->migrationSession.erase(shardId);
			close(receiveSocket);
			close(commSocket);
			//TODO: Notify SHM
			return;
		}

		long offset = 0;
		int retryCount = 10;
		while(offset < componentSize) {

			signed packetSize;

			if ((componentSize - sequenceNumber * BLOCK_SIZE) > BLOCK_SIZE) {
				packetSize = BLOCK_SIZE;
			}else {
				packetSize = componentSize - sequenceNumber * BLOCK_SIZE;
			}

			int status = readDataFromSocketWithRetry(commSocket, (char *)mmapBuffer + offset, packetSize, retryCount);
			if (status == -1) {
				//close socket
				perror("");
				Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
				migrationMgr->migrationSession.erase(shardId);
				//delete internalBuffer;
				munmap(mmapBuffer, componentSize);
				close(receiveSocket);
				close(commSocket);
				//TODO: Notify SHM
				return;
			}

			offset += packetSize;
			++sequenceNumber;
		}
		Logger::console("%u/%u Received", offset, componentSize);

		std::istringstream inputStream(ios::binary);
		inputStream.rdbuf()->pubsetbuf((char *)mmapBuffer , componentSize);
		inputStream.seekg(0, ios::beg);

		migratedShard->bootStrapShardComponentFromByteStream(inputStream,
				migrationMgr->migrationSession[shardId].shardCompName);

		munmap(mmapBuffer, componentSize);
		close(writeFd);
		//Send ACK
		migrationMgr->sendComponentDoneMsg(shardId, migrationMgr->migrationSession[shardId].shardCompName);
	}

	migrationMgr->sendComponentDoneMsg(shardId, "");

	// create an empty shard
	migrationMgr->migrationSession[shardId].shard.reset(migratedShard);

	Logger::console("Saving shard to : %s", directoryPath.c_str());
	//migrationMgr->migrationSession[shardId].shard->save();

	migrationMgr->migrationSession[shardId].endTimeStamp = time(NULL);
	Logger::console("Received shard %s in %d secs", shardId.toString().c_str(),
			migrationMgr->migrationSession[shardId].endTimeStamp -
			migrationMgr->migrationSession[shardId].beginTimeStamp);


	//munmap(mmapBuffer, maxMappedSize);
	close(receiveSocket);
	close(commSocket);
	//migrationMgr->migrationSession.erase(shardId);
}

void MigrationManager::migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, unsigned destinationNodeId) {
	Logger::console("Migrating shard %s to node %d", shardId.toString().c_str(), destinationNodeId);

	/*
	 *  Initialize the migration session info
	 */

	this->initMigrationSession(destinationNodeId, 0, shardId);

	migrationSession[shardId].beginTimeStamp = time(NULL);

	/*
	 *  Serialize indexes and calculate size of serialized indexes.
	 */

	Logger::console("Save shard to disk...");
	shard->getIndexer()->save();
	Logger::console("Done");

	Logger::console("Get shard on disk size...");
	vector<std::pair<string, long> > indexFilesWithSize;
	int status = shard->getSerializedShardSize(indexFilesWithSize);

	std::sort(indexFilesWithSize.begin(), indexFilesWithSize.end(), IndexSizeComparator);

	if (status == -1) {
		// cannot determine indexes size
		Logger::console("Unable to access shard on disk");
		// TODO: notify SHM
		return;
	}
	Logger::console("Done.");

	/*
	 *  Initiate handshaking with MM on the destination node by sending init message and waiting
	 *  for the acknowledgment.
	 */
	doInitialHandShake(shardId, indexFilesWithSize.size(), destinationNodeId);
	migrationSession[shardId].shardCompCount = indexFilesWithSize.size();

	if (migrationSession[shardId].status != MM_STATE_INIT_ACK_RCVD ) {
		Logger::console("Unable to migrate shard ..destination node did not respond");
		migrationSession.erase(shardId);
		// callback SHM with failure.
		return;
	}

	/*
	 *  Make a TCP connection to remote node
	 */

	int sendSocket = openTCPSendChannel(migrationSession[shardId].remoteAddr,
			migrationSession[shardId].listeningPort);
	if (sendSocket == -1) {
		migrationSession.erase(shardId);
		// callback SHM with failure.
		return;
	}

	for (unsigned i = 0; i < indexFilesWithSize.size(); ++i) {
		const string& componentName = indexFilesWithSize[i].first;
		unsigned componentSize = indexFilesWithSize[i].second;
		Logger::console("Migrating shard component %s", componentName.c_str());
		sendComponentInfoAndWaitForAck(shardId, componentName, componentSize, destinationNodeId);

		if (migrationSession[shardId].status != MM_STATE_INFO_ACK_RCVD) {
			Logger::console("Unable to migrate shard ..destination node did not respond to begin");
			migrationSession.erase(shardId);
			// callback SHM with failure.
			return;
		}

		// read shard from disk and send through.
		string filename = shard->getIndexer()->getStoredIndexDirectory() + "/" + componentName;

		int inputFd = open(filename.c_str(), O_RDONLY);  // TODO: Should lock as well?
		if (inputFd == -1) {
			Logger::console("Shard could not be read from storage device");
			perror("");
			migrationSession.erase(shardId);
			// callback SHM with failure.
			migrationSession.erase(shardId);
			return;
		}
		Logger::console("Sending data for shard component %s", componentName.c_str());
		off_t offset = 0;

		while(1) {
			off_t byteToSend = BLOCK_SIZE;
#ifdef __MACH__
			int status = sendfile(inputFd, sendSocket, offset, &byteToSend, NULL, 0);
			if (status == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("");
				Logger::console("Network error while sending the shard");
				//CallBack SHM
				migrationSession.erase(shardId);
				return;
			}

			if (status == 0 && byteToSend == 0)
				break;

			offset += byteToSend;
#else
			int status = sendfile(sendSocket, inputFd, &offset, byteToSend);
			if (status == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("");
				Logger::console("Network error while sending the shard");
				//CallBack SHM
				migrationSession.erase(shardId);
				return;
			}
			if (offset >= componentSize)
				break;
#endif

		}
		Logger::console("%u/%u sent", offset, componentSize);

		while (migrationSession[shardId].status != MM_STATE_COMPONENT_TRANSFERRED_ACK_RCVD &&
				migrationSession[shardId].status != MM_STATE_SHARD_TRANSFERRED_ACK_RCVD) {
			//sleep(1);
		}
		Logger::console("Data received by remote node...continuing");
//		Logger::console("Unable to migrate shard ..destination node did not respond to end ");
//					migrationSession.erase(shardId);
//					// callback SHM with failure.
//					return;
	}

//	while (migrationSession[shardId].status != MM_STATE_SHARD_TRANSFERRED_ACK_RCVD) {
//		sleep(1);
//	}

	migrationSession[shardId].endTimeStamp = time(NULL);
	Logger::console("Send shard %s in %d secs", shardId.toString().c_str(),
			migrationSession[shardId].endTimeStamp -
			migrationSession[shardId].beginTimeStamp);

}

void MigrationManager::doInitialHandShake(ShardId shardId, unsigned componentCount, unsigned destinationNodeId) {

	Message *initMessage = MessageAllocator().allocateMessage(sizeof(MigrationInitMsgBody));
	initMessage->setType(MigrationInitMessage);
	MigrationInitMsgBody *initMessageBody = (MigrationInitMsgBody *)initMessage->getMessageBody();
	initMessageBody->shardId = shardId;
	initMessageBody->shardComponentCount = componentCount;

	migrationSession[shardId].status = MM_STATE_INIT_ACK_WAIT;
	int tryCount = 5;
	do {
		sendMessage(destinationNodeId, initMessage);
		--tryCount;
		sleep(2);
	} while(migrationSession[shardId].status != MM_STATE_INIT_ACK_RCVD && tryCount);
	deAllocateMessage(initMessage);
}

void MigrationManager::sendInitMessageAck(ShardId shardId, short receivePort) {

	Message * initAckMesssage = MessageAllocator().allocateMessage(sizeof(MigrationInitAckMsgBody));
	initAckMesssage->setType(MigrationInitAckMessage);
	MigrationInitAckMsgBody *body = (MigrationInitAckMsgBody *)initAckMesssage->getMessageBody();
	body->shardId = shardId;
	body->portnumber = receivePort;
	body->ipAddress = transport->getPublishedInterfaceNumericAddr();
	migrationSession[shardId].status = MM_STATE_INFO_WAIT;
	sendMessage(migrationSession[shardId].remoteNode, initAckMesssage);
	MessageAllocator().deallocateByMessagePointer(initAckMesssage);

}

void MigrationManager::sendInfoAckMessage(ShardId shardId) {
	Message * infoAckMesssage = MessageAllocator().allocateMessage(sizeof(ShardComponentInfoAckMsgBody));
	infoAckMesssage->setType(MigrationComponentBeginAckMessage);
	ShardComponentInfoAckMsgBody *body = (ShardComponentInfoAckMsgBody *)infoAckMesssage->getMessageBody();
	body->shardId = shardId;
	migrationSession[shardId].status = MM_STATE_COMPONENT_RECEIVING;
	Logger::console("sending component begin ack to %d ", migrationSession[shardId].remoteNode);
	sendMessage(migrationSession[shardId].remoteNode, infoAckMesssage);
	MessageAllocator().deallocateByMessagePointer(infoAckMesssage);
}

void MigrationManager::sendComponentInfoAndWaitForAck(ShardId shardId, const string& componentName, unsigned componentSize,
		unsigned destinationNodeId) {

	unsigned compInfoMessageSize = sizeof(ShardComponentInfoMsgBody) + componentName.size();
	Message *compInfoMessage = allocateMessage(compInfoMessageSize);
	compInfoMessage->setType(MigrationComponentBeginMessage);
	ShardComponentInfoMsgBody *bodyPtr = (ShardComponentInfoMsgBody *)compInfoMessage->getMessageBody();
	bodyPtr->shardId = shardId;
	bodyPtr->componentSize = componentSize;
	bodyPtr->componentNameSize = componentName.size();
	memcpy(bodyPtr->name, componentName.c_str(), componentName.size());
	migrationSession[shardId].status = MM_STATE_INFO_ACK_WAIT;
	sendMessage(destinationNodeId, compInfoMessage);
	deAllocateMessage(compInfoMessage);
	//int tryCount = 5000000;
	while(migrationSession[shardId].status != MM_STATE_INFO_ACK_RCVD) {
		//sleep(1);
		//--tryCount;
	}
}

void MigrationManager::sendComponentDoneMsg(ShardId shardId, const string& componentName) {

	bool shardDone = componentName.size() > 0 ? false : true;
	Message *compDoneMessage = allocateMessage(sizeof(MigrationDoneMsgBody));

	if (!shardDone) {
		compDoneMessage->setType(MigrationComponentEndAckMessage);
	} else {
		compDoneMessage->setType(MigrationCompleteAckMessage);
	}

	MigrationDoneMsgBody *bodyPtr = (MigrationDoneMsgBody *)compDoneMessage->getMessageBody();
	bodyPtr->shardId = shardId;

	if (!shardDone) {
		migrationSession[shardId].status = MM_STATE_INFO_WAIT;  // waiting for next component
	} else {
		migrationSession[shardId].status = MM_STATE_SHARD_RCVD;
	}
	sendMessage(migrationSession[shardId].remoteNode, compDoneMessage);
	deAllocateMessage(compDoneMessage);

}

void MigrationManager::migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard,
		std::vector<unsigned> destinationNodeId) {
	//TODO:
}

void MigrationManager::openTCPReceiveChannel(int& tcpSocket , short& receivePort) {
	/*
	 *  Prepare socket data structures.
	 */

	if((tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		tcpSocket = -1;
		perror("listening socket failed to init");
		return;
	}

	const int optVal = 1;
	setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, sizeof(optVal));


	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	inet_aton(transport->getPublisedInterfaceAddress().c_str(), &addr.sin_addr);
	receivePort = MM_MIGRATION_PORT_START;
	addr.sin_port = htons(receivePort);
	tryNextPort:
	if( bind(tcpSocket, (struct sockaddr *) &addr, sizeof(sockaddr_in)) < 0) {
		++receivePort;
		if (receivePort < MM_MIGRATION_PORT_START + 100) {
			addr.sin_port = htons(receivePort);
			goto tryNextPort;
		} else {
			perror("");
			Logger::console("unable to bind to port for shard migration in range = [%d : %d]",
					MM_MIGRATION_PORT_START, MM_MIGRATION_PORT_START + 100);
			close(tcpSocket);
			tcpSocket = -1;
			return;
		}
	}
}
int MigrationManager::acceptTCPConnection(int tcpSocket , short receivePort) {
	int tcpReceiveSocket;
	if(listen(tcpSocket, 20) == -1) {
		close(tcpSocket);
		perror("");
		tcpReceiveSocket = -1;
		return tcpReceiveSocket;
	}

	//TODO: make accept non-blocking
	struct sockaddr_in senderAddress;
	unsigned sizeOfAddr = sizeof(senderAddress);
	tcpReceiveSocket = accept(tcpSocket, (struct sockaddr *) &senderAddress, &sizeOfAddr);
	if (tcpReceiveSocket == -1) {
		close(tcpSocket);
		perror("");
		return tcpReceiveSocket;
	}

	/*
	 *   Make socket non blocking
	 */

	//fcntl(tcpReceiveSocket, F_SETFL, O_NONBLOCK);
	return tcpReceiveSocket;

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

int MigrationManager::openTCPSendChannel(unsigned remoteAddr, short remotePort) {
	/*
	 *  Prepare socket data structures.
	 */
	int tcpSocket;
	if((tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("sending socket failed to init");
		return -1;
	}

	// Todo: Try changing socket buffer size. Ideal buffer size ?

	struct sockaddr_in destinationAddress;
	memset(&destinationAddress, 0, sizeof(destinationAddress));
	destinationAddress.sin_family = AF_INET;
	destinationAddress.sin_addr.s_addr = remoteAddr;
	destinationAddress.sin_port = htons(remotePort);

	int status = connect(tcpSocket, (struct sockaddr *)&destinationAddress, sizeof(destinationAddress));

	if (status == -1) {
		perror("Unable to connect to remote node");
		return -1;
	}
	/*
	 *   Make socket non blocking
	 */
	//fcntl(tcpSocket, F_SETFL, O_NONBLOCK);

	return tcpSocket;

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

void MigrationManager::initMigrationSession(unsigned remoteNode, unsigned shardCompCount, ShardId shardId) {

	MigrationSessionInfo * info = new MigrationSessionInfo();
	info->beginTimeStamp = 0;
	info->endTimeStamp = 0;
	info->listeningPort = -1;
	info->status = MM_STATE_MIGRATION_BEGIN;
	info->remoteNode = remoteNode;
	info->shardCompCount = shardCompCount;
	//info.shard.reset();
	migrationSession[shardId] = *info;
}

//void MigrationManager::migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, unsigned destinationNodeId) {
//
//	Logger::console("Migrating shard %s to node %d", shardId.toString().c_str(), destinationNodeId);
//
//	/*
//	 *  Initialize the migration session info
//	 */
//
//	this->initMigrationSession(destinationNodeId, 0, shardId);
//
//	migrationSession[shardId].beginTimeStamp = time(NULL);
//
//	/*
//	 *  Serialize indexes and calculate size of serialized indexes.
//	 */
//
//	std::ostringstream outputBuffer(std::ios_base::binary);
//	cout << "trying to serialize the shard ...." << endl;
//	shard->serialize(outputBuffer);
//	outputBuffer.seekp(0, ios::end);
//	unsigned shardSize = outputBuffer.tellp();
//	cout << "ostream size = " << outputBuffer.tellp() << endl;
//
//	/*
//	 *  Initiate handshaking with MM on the destination node by sending init message and waiting
//	 *  for the acknowledgment.
//	 */
//
//	Message *initMessage = MessageAllocator().allocateMessage(sizeof(MigrationInitMsgBody));
//	initMessage->setType(MigrationInitMessage);
//	MigrationInitMsgBody *initMessageBody = (MigrationInitMsgBody *)initMessage->getMessageBody();
//	initMessageBody->shardId = shardId;
//	initMessageBody->shardComponentCount = shardSize;
//	// TODO: initMessageBody->checksum =
//
//	migrationSession[shardId].status = MM_STATUS_WAITING_FOR_INIT_ACK;
//	cout << "waiting for init ack "<< endl;
//	int tryCount = 3;
//	while(migrationSession[shardId].status != MM_STATUS_INIT_ACK_RECEIVED && tryCount) {
//		sendMessage(destinationNodeId, initMessage);
//		--tryCount;
//		sleep(2);
//	}
//	MessageAllocator().deallocateByMessagePointer(initMessage);
//	if (migrationSession[shardId].status != MM_STATUS_INIT_ACK_RECEIVED ) {
//		cout << "Unable to migrate shard ..destination node did not respond"<< endl;
//		migrationSession.erase(shardId);
//		// callback SHM with failure.
//		return;
//	}
//
//	/*
//	 *  Connect to remote node and start data transfer.
//	 */
//	unsigned remotePort = migrationSession[shardId].listeningPort;
//	struct sockaddr_in destinationAddress;
//	memset(&destinationAddress, 0, sizeof(destinationAddress));
//	destinationAddress.sin_family = AF_INET;
//	destinationAddress.sin_addr.s_addr = migrationSession[shardId].remoteAddr;
//	destinationAddress.sin_port = htons(remotePort);
//
//	string binaryStream = outputBuffer.str();
//	char * buffer = (char *)binaryStream.c_str();
//	cout << "string size = " << binaryStream.size() << endl;
//
//	unsigned dataSent = 0;
//	unsigned sequenceNumber = 0;
//	MigrationData *payLoad = new MigrationData();
//	unsigned payLoadSize = sizeof(MigrationData);
//	while(dataSent < shardSize) {
//		unsigned dataSize = 0;
//		if ((shardSize - dataSent) > BLOCK_SIZE)
//			dataSize = BLOCK_SIZE;
//		else
//			dataSize = shardSize - dataSent;
//
//		payLoad->shardId = shardId;
//		payLoad->sequenceNumber = sequenceNumber;
//		memcpy(payLoad->data, buffer + dataSent, dataSize);
//
//		int status = sendUDPPacketToDestination(sendSocket, (char *)(payLoad)
//				, payLoadSize, destinationAddress);
//		if (status == 1) {
//			cout <<  "packet not sent " << sequenceNumber << endl;
//		} else if (status == 0){
//			//cout <<  "packet sent " << sequenceNumber << endl;
//			dataSent += dataSize;
//			++sequenceNumber;
//		} else {
//			cout << "Unable to migrate shard ..network error"<< endl;
//			migrationSession.erase(shardId);
//			// callback SHM with failure.
//			return;
//		}
//	}
//	// data send done..now send migration terminate request.
//	Message *termMessage = MessageAllocator().allocateMessage(sizeof(MigrationTermMsgBody));
//	termMessage->setType(MigrationTermMessage);
//	MigrationTermMsgBody *termMessageBody = (MigrationTermMsgBody *)termMessage->getMessageBody();
//	termMessageBody->shardId = shardId;
//	sendMessage(destinationNodeId, termMessage);
//	MessageAllocator().deallocateByMessagePointer(termMessage);
//
//	migrationSession[shardId].status = MM_STATUS_WAITING_FOR_TERM_ACK;
//	cout << "waiting for terminate ack "<< endl;
//	while(migrationSession[shardId].status != MM_STATUS_TERM_ACK_RECEIVED) {
//		sleep(1);
//	}
//	MigrationTermAckMsgBody *termAckMessageBody = (MigrationTermAckMsgBody *) migrationSession[shardId].termAckMessage.get();
//	cout << "Term status " << (int)termAckMessageBody->flag <<  endl;
//
//	if (termAckMessageBody->flag == 0) {
//		unsigned sentPacket = 0;
//		while( sentPacket < termAckMessageBody->missingPacketCount) {
//			unsigned missedSeqNumber = termAckMessageBody->arr[sentPacket];
//			signed dataSize = 0;
//			if ((shardSize - missedSeqNumber * BLOCK_SIZE) > BLOCK_SIZE) {
//				dataSize = BLOCK_SIZE;
//			}else {
//				dataSize = shardSize - missedSeqNumber * BLOCK_SIZE;
//			}
//
//			if (dataSize < 0) {
//				cout << "Unable to migrate shard ..Invalid packet size"<< endl;
//				migrationSession.erase(shardId);
//				// callback SHM with failure.
//				return;
//			}
//
//			payLoad->shardId = shardId;
//			payLoad->sequenceNumber = missedSeqNumber;
//			memcpy(payLoad->data, buffer + missedSeqNumber * BLOCK_SIZE, dataSize);
//
//			int status = sendUDPPacketToDestination(sendSocket, (char *)(payLoad)
//					, payLoadSize, destinationAddress);
//			if (status == 1) {
//				cout <<  "packet not sent " << missedSeqNumber << endl;
//			} else if (status == 0){
//				cout <<  "packet sent " << missedSeqNumber << endl;
//				++sentPacket;
//			} else {
//				cout << "Unable to migrate shard ..network error"<< endl;
//				migrationSession.erase(shardId);
//				// callback SHM with failure.
//				return;
//			}
//		}
//
//	} else {
//		migrationSession[shardId].endTimeStamp = time(NULL);
//		migrationSession.erase(shardId);
//		// callback SHM with success message.
//		Logger::console("Migrated shard %s in %d secs", shardId.toString().c_str(),
//				migrationSession[shardId].endTimeStamp - migrationSession[shardId].beginTimeStamp);
//	}
//}


//void MigrationService::receiveShard(ShardId shardId) {
//	int receiveSocket;
//	short receivePort;
//
//	migrationMgr->migrationSession[shardId].beginTimeStamp = time(NULL);
//
//	migrationMgr->openTCPReceiveChannel(receiveSocket, receivePort);
//
//	migrationMgr->migrationSession[shardId].listeningPort = receivePort;
//	migrationMgr->migrationSession[shardId].listeningSocket = receiveSocket;
//
//	Message * initAckMesssage = MessageAllocator().allocateMessage(sizeof(MigrationInitAckMsgBody));
//	initAckMesssage->setType(MigrationInitAckMessage);
//	MigrationInitAckMsgBody *body = (MigrationInitAckMsgBody *)initAckMesssage->getMessageBody();
//	body->shardId = shardId;
//	body->portnumber = receivePort;
//	body->ipAddress = migrationMgr->transport->getPublishedInterfaceNumericAddr();
//	cout << "sending init ack to " << migrationMgr->migrationSession[shardId].remoteNode << endl;
//	migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, initAckMesssage);
//	MessageAllocator().deallocateByMessagePointer(initAckMesssage);
//
//	migrationMgr->migrationSession[shardId].status = MM_STATUS_FETCHING_DATA;
//
//	int commSocket = migrationMgr->acceptTCPConnection(receiveSocket, receivePort);
//
//	unsigned dataReceived = 0;
//	unsigned shardSize = migrationMgr->migrationSession[shardId].shardSize;
//
//	char * internalBuffer = new char[shardSize];
//	unsigned payLoadSize = sizeof(MigrationData);
//	MigrationData *buffer = new MigrationData();
//	std::set<unsigned> missedPacket;
//	unsigned expectedSeqNumber = 0;
//
//receiveShard:
//	unsigned retryCount = 0;
//	while(dataReceived < shardSize) {
//
//		signed packetSize;
//		sockaddr_in destinationAddress;
//
//		int status = readUDPPacketWithSenderInfo(commSocket, (char *)buffer, payLoadSize, destinationAddress);
//		if (status == 0) {
//			retryCount = 0;
//			if ((shardSize - buffer->sequenceNumber * BLOCK_SIZE) > BLOCK_SIZE) {
//				packetSize = BLOCK_SIZE;
//			}else {
//				packetSize = shardSize - buffer->sequenceNumber * BLOCK_SIZE;
//			}
//
//			if (packetSize < 0)
//				continue;
//
//			memcpy(internalBuffer + buffer->sequenceNumber * BLOCK_SIZE, buffer->data, packetSize);
//			dataReceived += packetSize;
//
//			// track sequence number
//			if (buffer->sequenceNumber == expectedSeqNumber) {
//				++expectedSeqNumber;
//			} else if (buffer->sequenceNumber > expectedSeqNumber){
//				// assuming there is smaller gap
//				for (unsigned start = expectedSeqNumber; start < buffer->sequenceNumber; ++start) {
//					missedPacket.insert(start);
//				}
//				expectedSeqNumber = buffer->sequenceNumber;
//			}  else {
//				// unordered packet.
//				missedPacket.erase(buffer->sequenceNumber);
//			}
//
//		} else if (status == 1) {
//			if (retryCount > 6) {
//				Logger::console("Shard Reception timed out !!");
//				break;
//			}
//			sleep(1);
//			++retryCount;
//		} else if (status == -1) {
//			//close socket
//			Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
//			migrationMgr->migrationSession.erase(shardId);
//			delete buffer;
//			delete internalBuffer;
//			close(receiveSocket);
//			close(commSocket);
//			//TODO: Notify SHM
//			return;
//		}
//	}
//
//	while(migrationMgr->migrationSession[shardId].status != MM_STATUS_TERM_REQ_RECEIVED) {
//		sleep(1);
//	}
//
//	cout << dataReceived <<  ":" << shardSize << endl;
//
//	if (dataReceived < shardSize) {
//		if (expectedSeqNumber == 0) {
//			Logger::console("Migration: Reception error for shard %s.", shardId.toString().c_str());
//			delete buffer;
//			delete internalBuffer;
//			close(receiveSocket);
//			close(commSocket);
//			// TODO: notify SHM
//		}
//		unsigned maxSeqNumber = shardSize / BLOCK_SIZE;
//		unsigned totalPacketMissed = missedPacket.size();
//
//		if (expectedSeqNumber <= maxSeqNumber)
//			totalPacketMissed += (maxSeqNumber - expectedSeqNumber + 1);
//		unsigned totalPacketSize = sizeof(MigrationTermAckMsgBody) + totalPacketMissed * sizeof(unsigned);
//
//		Message *termAckMessage = MessageAllocator().allocateMessage(totalPacketSize);
//		termAckMessage->setType(MigrationTermAckMessage);
//		MigrationTermAckMsgBody  *termAckBody = (MigrationTermAckMsgBody *)termAckMessage->getMessageBody();
//		termAckBody->flag = 0;
//		termAckBody->missingPacketCount = totalPacketMissed;
//		termAckBody->shardId = shardId;
//		unsigned seq = 0;
//		std::set<unsigned>::iterator iter = missedPacket.begin();
//		while(iter != missedPacket.end()) {
//			termAckBody->arr[seq] = *iter;
//			++seq;
//		}
//		for (unsigned start = expectedSeqNumber; start < maxSeqNumber + 1; ++start) {
//			termAckBody->arr[seq] = start;
//			++seq;
//		}
//		migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, termAckMessage);
//		MessageAllocator().deallocateByMessagePointer(termAckMessage);
//		migrationMgr->migrationSession[shardId].status = MM_STATUS_FETCHING_DATA;
//		retryCount = 0;
//		sleep(1);
//		goto receiveShard;
//	} else {
//
//		// send term Ack
//		migrationMgr->migrationSession[shardId].endTimeStamp = time(NULL);
//		Logger::console("Received shard %s in %d secs", shardId.toString().c_str(),
//					migrationMgr->migrationSession[shardId].endTimeStamp -
//					migrationMgr->migrationSession[shardId].beginTimeStamp);
//
//		Message *termAckMessage = MessageAllocator().allocateMessage(sizeof(MigrationTermAckMsgBody));
//		termAckMessage->setType(MigrationTermAckMessage);
//		MigrationTermAckMsgBody  *termAckBody = (MigrationTermAckMsgBody *)termAckMessage->getMessageBody();
//		termAckBody->flag = 1;
//		termAckBody->missingPacketCount = 0;
//		termAckBody->shardId = shardId;
//		migrationMgr->sendMessage(migrationMgr->migrationSession[shardId].remoteNode, termAckMessage);
//		MessageAllocator().deallocateByMessagePointer(termAckMessage);
//
//		//Logger::console("Received shard : %s", shardId.toString().c_str());
//		std::istringstream inputStream(ios::binary);
//		inputStream.rdbuf()->pubsetbuf(internalBuffer , shardSize);
//		inputStream.seekg(0, ios::beg);
//
//		// create an empty shard
//		migrationMgr->migrationSession[shardId].shard.reset(
//				new Srch2Server(migrationMgr->getIndexConfig(shardId), shardId, 1));
//
//		// Create a path to store this Shard on storage device.
//		ConfigManager *configManager = migrationMgr->configManager;
//		string directoryPath = configManager->createShardDir(configManager->getClusterWriteView()->getClusterName(),
//							configManager->getClusterWriteView()->getCurrentNode()->getName(),
//							migrationMgr->getIndexConfig(shardId)->getName(), shardId);
//
//		Logger::console("Saving shard to : %s", directoryPath.c_str());
//		migrationMgr->migrationSession[shardId].shard->bootStrapIndexerFromByteStream(inputStream, directoryPath);
//		migrationMgr->migrationSession.erase(shardId);
//	}
//
//	delete buffer;
//	delete internalBuffer;
//
//}

} /* namespace httpwrapper */
} /* namespace srch2 */
