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
#include "sharding/metadata_manager/Cluster.h"

namespace srch2 {
namespace httpwrapper {

const unsigned BLOCK_SIZE =  1400;  // MTU size

// various migration message's body types
struct MigrationInitMsgBody{
	ClusterShardId shardId;
	unsigned srcOperationId;
	unsigned dstOperationId;
	unsigned shardComponentCount;
};

struct MigrationInitAckMsgBody{
	ClusterShardId shardId;
	unsigned ipAddress;
	short portnumber;
};

struct ShardComponentInfoMsgBody {
	ClusterShardId shardId;
	unsigned componentSize;
	unsigned componentNameSize;
	char name[0];
};

struct ShardComponentInfoAckMsgBody{
	ClusterShardId shardId;
};

struct MigrationData {
	ClusterShardId shardId;
	unsigned sequenceNumber;
	char data[BLOCK_SIZE];
};

struct MigrationDoneMsgBody{
	ClusterShardId shardId;
};

struct MigrationDoneAckMsgBody{
	ClusterShardId shardId;
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


struct MigrationThreadArguments {
	MigrationManager *mm;
	ClusterShardId shardId;
	unsigned remotedNode;
};

void *receiverThreadEntryPoint(void *arg) {

	MigrationThreadArguments * tArgs = (MigrationThreadArguments *)arg;
	MigrationService *clientService = new MigrationService(tArgs->mm);
	clientService->receiveShard(tArgs->shardId, tArgs->remotedNode);
	delete tArgs;
	return NULL;
}

void *senderThreadEntryPoint(void *arg) {

	MigrationThreadArguments * tArgs = (MigrationThreadArguments *)arg;
	MigrationService *clientService = new MigrationService(tArgs->mm);
	clientService->sendShard(tArgs->shardId, tArgs->remotedNode);
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
		MigrationInitMsgBody *initMsgBody = (MigrationInitMsgBody *)incomingMessage->getMessageBody();

		//ACQUIRE LOCK
		if (!migrationMgr->hasActiveSession(initMsgBody->shardId, remoteNode)) {

			string sessionkey = migrationMgr->initMigrationSession(initMsgBody->shardId,
					initMsgBody->srcOperationId, initMsgBody->dstOperationId, remoteNode,
					initMsgBody->shardComponentCount);

			migrationMgr->migrationSession[sessionkey].status = MM_STATE_INIT_RCVD;

			pthread_t receiverThread;
			MigrationThreadArguments * arg = new MigrationThreadArguments();
			arg->mm = migrationMgr;
			arg->shardId = initMsgBody->shardId;
			arg->remotedNode = remoteNode;
			pthread_create(&receiverThread, NULL, receiverThreadEntryPoint, arg);
			pthread_detach(receiverThread);
		}
		//RELEASE LOCK
		break;
	}
	case MigrationComponentBeginMessage:
	{
		ShardComponentInfoMsgBody *compInfoMsgBody = (ShardComponentInfoMsgBody *)incomingMessage->getMessageBody();
		string sessionkey;
		if (migrationMgr->hasActiveSession(compInfoMsgBody->shardId, remoteNode, sessionkey)) {
			migrationMgr->migrationSession[sessionkey].shardCompSize = compInfoMsgBody->componentSize;
			migrationMgr->migrationSession[sessionkey].shardCompName.assign(compInfoMsgBody->name,
					compInfoMsgBody->componentNameSize);
			migrationMgr->migrationSession[sessionkey].status = MM_STATE_INFO_RCVD;
		}
		break;

	}
	case MigrationInitAckMessage:
	{
		MigrationInitAckMsgBody *initAckMessageBody = (MigrationInitAckMsgBody *)incomingMessage->getMessageBody();
		string sessionkey;
		if (migrationMgr->hasActiveSession(initAckMessageBody->shardId, remoteNode, sessionkey)) {
			migrationMgr->migrationSession[sessionkey].listeningPort = initAckMessageBody->portnumber;
			migrationMgr->migrationSession[sessionkey].remoteAddr = initAckMessageBody->ipAddress;
			migrationMgr->migrationSession[sessionkey].status = MM_STATE_INIT_ACK_RCVD;
		}
		break;
	}
	case MigrationComponentBeginAckMessage:
	{
		//if (migrationMgr->migrationSession[compInfoMsgBody->shardId].status == MM_STATUS_ACK_RECEIVED)
		string sessionkey;
		ShardComponentInfoMsgBody *compInfoMsgBody = (ShardComponentInfoMsgBody *)incomingMessage->getMessageBody();
		if (migrationMgr->hasActiveSession(compInfoMsgBody->shardId, remoteNode, sessionkey)) {
			migrationMgr->migrationSession[sessionkey].status = MM_STATE_INFO_ACK_RCVD;
		}
		break;
	}
	case MigrationComponentEndAckMessage:
	{
		string sessionkey;
		MigrationDoneMsgBody *completeMsgBody = (MigrationDoneMsgBody *)incomingMessage->getMessageBody();
		if (migrationMgr->hasActiveSession(completeMsgBody->shardId, remoteNode, sessionkey)) {
			migrationMgr->migrationSession[sessionkey].status = MM_STATE_COMPONENT_TRANSFERRED_ACK_RCVD;
		}
		break;
	}
	case MigrationCompleteAckMessage:
	{
		string sessionkey;
		MigrationDoneMsgBody *completeMsgBody = (MigrationDoneMsgBody *)incomingMessage->getMessageBody();
		if (migrationMgr->hasActiveSession(completeMsgBody->shardId, remoteNode, sessionkey)) {
			migrationMgr->migrationSession[sessionkey].status = MM_STATE_SHARD_TRANSFERRED_ACK_RCVD;
		}
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

void MigrationService::receiveShard(ClusterShardId shardId, unsigned remoteNode) {
	int receiveSocket;
	short receivePort;
	string sessionKey = migrationMgr->getSessionKey(shardId, remoteNode);
	migrationMgr->migrationSession[sessionKey].beginTimeStamp = time(NULL);

	migrationMgr->openTCPReceiveChannel(receiveSocket, receivePort);

	if (receiveSocket == -1) {
		//close socket
		Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
		migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
		return;
	}

	migrationMgr->migrationSession[sessionKey].listeningPort = receivePort;

	Logger::console("sending init ack to %d", migrationMgr->migrationSession[sessionKey].remoteNode);
	migrationMgr->sendInitMessageAck(sessionKey);

	Logger::console("waiting for sender to connect...");
	int commSocket = migrationMgr->acceptTCPConnection(receiveSocket, receivePort);
	if (commSocket == -1) {
		//close socket
		Logger::console("Migration: Network error while migrating shard %s", shardId.toString().c_str());
		close(receiveSocket);
		migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
		return;
	}

	Logger::console("Done");

	unsigned componentCount = migrationMgr->migrationSession[sessionKey].shardCompCount;

	// Create a path to store this Shard on storage device.
	ConfigManager *configManager = migrationMgr->configManager;
	string directoryPath = "";
//	string directoryPath = configManager->createShardDir(configManager->getClusterWriteView()->getClusterName(),
//			configManager->getClusterWriteView()->getCurrentNode()->getName(),
//			migrationMgr->getIndexConfig(shardId)->getName(), shardId);
	Srch2Server *migratedShard = new Srch2Server(migrationMgr->getIndexConfig(shardId), directoryPath, "");

	unsigned mmapBufferSize = 0;
	void * mmapBuffer = NULL;
	unsigned maxMappedSize = 0;

	for (unsigned i =0; i < componentCount; ++i) {

		Logger::console("waiting for component begin message...");
		//TODO: add timeout
		while(migrationMgr->migrationSession[sessionKey].status !=  MM_STATE_INFO_RCVD) {
			//sleep(1);
		}
		string filePath = directoryPath + "/" + migrationMgr->migrationSession[sessionKey].shardCompName;
		int writeFd = open(filePath.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0000644);
		if (writeFd == -1) {
			//close socket
			perror("");   // TODO: replace perror in SM/TM/MM with strerror_r
			Logger::console("Migration:I/O error for shard %s", shardId.toString().c_str());
			close(commSocket);
			close(receiveSocket);
			migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
			return;
		}

		migrationMgr->sendInfoAckMessage(sessionKey);

		unsigned componentSize = migrationMgr->migrationSession[sessionKey].shardCompSize;
		unsigned sequenceNumber = 0;

		int ftruncStatus = ftruncate(writeFd, componentSize);
		if (ftruncStatus == -1) {
			//close socket
			perror("");   // TODO: replace perror in SM/TM/MM with strerror_r
			Logger::console("Migration:I/O error for shard %s", shardId.toString().c_str());
			close(commSocket);
			close(receiveSocket);
			migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
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
			close(receiveSocket);
			close(commSocket);
			migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
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
				migrationMgr->migrationSession.erase(sessionKey);
				//delete internalBuffer;
				munmap(mmapBuffer, componentSize);
				close(receiveSocket);
				close(commSocket);
				migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
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
				migrationMgr->migrationSession[sessionKey].shardCompName);

		munmap(mmapBuffer, componentSize);
		close(writeFd);
		//Send ACK
		migrationMgr->sendComponentDoneMsg(sessionKey);
	}

	migrationMgr->sendComponentDoneMsg(sessionKey);

	// create an empty shard
	migrationMgr->migrationSession[sessionKey].shard.reset(migratedShard);

	Logger::console("Saving shard to : %s", directoryPath.c_str());

	migrationMgr->migrationSession[sessionKey].endTimeStamp = time(NULL);
	Logger::console("Received shard %s in %d secs", shardId.toString().c_str(),
			migrationMgr->migrationSession[sessionKey].endTimeStamp -
			migrationMgr->migrationSession[sessionKey].beginTimeStamp);

	migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_SUCCESS);

	close(receiveSocket);
	close(commSocket);
}

void MigrationManager::populateStatus(ShardMigrationStatus& status, unsigned srcOperationId,
		unsigned dstOperationId, unsigned destinationNodeId, boost::shared_ptr<Srch2Server> shard,
		MIGRATION_STATUS migrationResult){

	status.srcOperationId = srcOperationId;
	status.dstOperationId = dstOperationId;
	status.destinationNodeId = destinationNodeId;
	status.shard = shard;
	status.status = migrationResult;
}

void MigrationManager::notifySHMAndCleanup(string sessionKey, MIGRATION_STATUS migrationResult) {
	ShardMigrationStatus migrationStatus;
	populateStatus(migrationStatus, migrationSession[sessionKey].srcOperationId,
			migrationSession[sessionKey].dstOperationId, migrationSession[sessionKey].remoteNode,
			migrationSession[sessionKey].shard,
			migrationResult);
	//shardManager->resolveMMNotification(status);
	migrationSession.erase(sessionKey);
}

void MigrationManager::migrateShard_(const ClusterShardId shardId , boost::shared_ptr<Srch2Server> shardPtr,
		const NodeOperationId & currentAddress, const NodeOperationId & requesterAddress){
	//TODO
	Logger::console("Migration manager is called to transfer shard %s", shardId.toString().c_str());
}

void MigrationManager::migrateShard(unsigned uri, boost::shared_ptr<Srch2Server> shard, NodeId destinationNodeId,
			unsigned srcOperationId , unsigned dstOperationId) {
	// TODO: get the correct API for shardManager
	ClusterShardId shardId; // = shard->getShardId();
	Logger::console("Migrating shard %s to node %d", shardId.toString().c_str(), destinationNodeId);

	/*
	 *  Initialize the migration session info
	 */

	if (hasActiveSession(shardId, destinationNodeId)) {
		ShardMigrationStatus status;
		populateStatus(status, srcOperationId, dstOperationId, destinationNodeId, shard,
				MM_STATUS_BUSY);
		//shardManager->resolveMMNotification(status);
		return;
	}

	string sessionKey = this->initMigrationSession(shardId, srcOperationId,
			dstOperationId, destinationNodeId, 0);

	migrationSession[sessionKey].status = MM_STATE_INIT_RCVD;
	migrationSession[sessionKey].shard = shard;
	pthread_t senderThread;
	MigrationThreadArguments * arg = new MigrationThreadArguments();
	arg->mm = this;
	arg->shardId = shardId;
	arg->remotedNode = destinationNodeId;
	pthread_create(&senderThread, NULL, senderThreadEntryPoint, arg);
	pthread_detach(senderThread);

}
void MigrationService::sendShard(ClusterShardId shardId, unsigned destinationNodeId) {


	string sessionKey = migrationMgr->getSessionKey(shardId, destinationNodeId);
	boost::shared_ptr<Srch2Server>& shard = migrationMgr->migrationSession[sessionKey].shard;
	migrationMgr->migrationSession[sessionKey].beginTimeStamp = time(NULL);

	/*
	 *  Serialize indexes and calculate size of serialized indexes.
	 */

	Logger::console("Save shard to disk...");
	shard->getIndexer()->save();

	vector<std::pair<string, long> > indexFilesWithSize;
	int status = shard->getSerializedShardSize(indexFilesWithSize);

	std::sort(indexFilesWithSize.begin(), indexFilesWithSize.end(), IndexSizeComparator);

	if (status == -1) {
		// cannot determine indexes size
		Logger::console("Unable to access shard on disk");
		migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
		return;
	}

	/*
	 *  Initiate handshaking with MM on the destination node by sending init message and waiting
	 *  for the acknowledgment.
	 */
	migrationMgr->migrationSession[sessionKey].shardCompCount = indexFilesWithSize.size();
	migrationMgr->doInitialHandShake(sessionKey);


	if (migrationMgr->migrationSession[sessionKey].status != MM_STATE_INIT_ACK_RCVD ) {
		Logger::console("Unable to migrate shard ..destination node did not respond");
		migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
		return;
	}

	/*
	 *  Make a TCP connection to remote node
	 */

	int sendSocket = migrationMgr->openTCPSendChannel(migrationMgr->migrationSession[sessionKey].remoteAddr,
			migrationMgr->migrationSession[sessionKey].listeningPort);
	if (sendSocket == -1) {
		migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
		return;
	}

	for (unsigned i = 0; i < indexFilesWithSize.size(); ++i) {
		const string& componentName = indexFilesWithSize[i].first;
		unsigned componentSize = indexFilesWithSize[i].second;
		Logger::console("Migrating shard component %s", componentName.c_str());
		migrationMgr->sendComponentInfoAndWaitForAck(sessionKey);

		if (migrationMgr->migrationSession[sessionKey].status != MM_STATE_INFO_ACK_RCVD) {
			Logger::console("Unable to migrate shard ..destination node did not respond to begin");
			migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
			return;
		}

		// read shard from disk and send through.
		string filename = shard->getIndexer()->getStoredIndexDirectory() + "/" + componentName;

		int inputFd = open(filename.c_str(), O_RDONLY);  // TODO: Should lock as well?
		if (inputFd == -1) {
			Logger::console("Shard could not be read from storage device");
			perror("");
			migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
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
				migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
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
				migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_FAILURE);
				return;
			}
			if (offset >= componentSize)
				break;
#endif

		}
		Logger::console("%u/%u sent", offset, componentSize);

		while (migrationMgr->migrationSession[sessionKey].status != MM_STATE_COMPONENT_TRANSFERRED_ACK_RCVD &&
				migrationMgr->migrationSession[sessionKey].status != MM_STATE_SHARD_TRANSFERRED_ACK_RCVD) {
			//sleep(1);
		}
		Logger::console("Data received by remote node...continuing");

	}

	migrationMgr->migrationSession[sessionKey].endTimeStamp = time(NULL);
	Logger::console("Send shard %s in %d secs", shardId.toString().c_str(),
			migrationMgr->migrationSession[sessionKey].endTimeStamp -
			migrationMgr->migrationSession[sessionKey].beginTimeStamp);

	migrationMgr->notifySHMAndCleanup(sessionKey, MM_STATUS_SUCCESS);

}

void MigrationManager::doInitialHandShake(const string& sessionKey) {

	Message *initMessage = MessageAllocator().allocateMessage(sizeof(MigrationInitMsgBody));
	initMessage->setType(MigrationInitMessage);
	MigrationInitMsgBody *initMessageBody = (MigrationInitMsgBody *)initMessage->getMessageBody();
	initMessageBody->shardId = migrationSession[sessionKey].shardId;
	initMessageBody->shardComponentCount = migrationSession[sessionKey].shardCompCount;
	unsigned destinationNodeId = migrationSession[sessionKey].remoteNode;
	migrationSession[sessionKey].status = MM_STATE_INIT_ACK_WAIT;
	int tryCount = 5;
	do {
		sendMessage(destinationNodeId, initMessage);
		--tryCount;
		sleep(2);
	} while(migrationSession[sessionKey].status != MM_STATE_INIT_ACK_RCVD && tryCount);
	deAllocateMessage(initMessage);
}

void MigrationManager::sendInitMessageAck(const string& sessionKey) {

	Message * initAckMesssage = MessageAllocator().allocateMessage(sizeof(MigrationInitAckMsgBody));
	initAckMesssage->setType(MigrationInitAckMessage);
	MigrationInitAckMsgBody *body = (MigrationInitAckMsgBody *)initAckMesssage->getMessageBody();
	body->shardId = migrationSession[sessionKey].shardId;
	body->portnumber = migrationSession[sessionKey].listeningPort;
	body->ipAddress = transport->getPublishedInterfaceNumericAddr();
	migrationSession[sessionKey].status = MM_STATE_INFO_WAIT;
	sendMessage(migrationSession[sessionKey].remoteNode, initAckMesssage);
	MessageAllocator().deallocateByMessagePointer(initAckMesssage);

}

void MigrationManager::sendInfoAckMessage(const string& sessionKey) {
	Message * infoAckMesssage = MessageAllocator().allocateMessage(sizeof(ShardComponentInfoAckMsgBody));
	infoAckMesssage->setType(MigrationComponentBeginAckMessage);
	ShardComponentInfoAckMsgBody *body = (ShardComponentInfoAckMsgBody *)infoAckMesssage->getMessageBody();
	body->shardId = migrationSession[sessionKey].shardId;
	migrationSession[sessionKey].status = MM_STATE_COMPONENT_RECEIVING;
	Logger::console("sending component begin ack to %d ", migrationSession[sessionKey].remoteNode);
	sendMessage(migrationSession[sessionKey].remoteNode, infoAckMesssage);
	MessageAllocator().deallocateByMessagePointer(infoAckMesssage);
}

void MigrationManager::sendComponentInfoAndWaitForAck(const string& sessionKey) {

	unsigned destinationNodeId = migrationSession[sessionKey].remoteNode;
	string componentName = migrationSession[sessionKey].shardCompName;
	unsigned compInfoMessageSize = sizeof(ShardComponentInfoMsgBody) + componentName.size();
	Message *compInfoMessage = allocateMessage(compInfoMessageSize);
	compInfoMessage->setType(MigrationComponentBeginMessage);
	ShardComponentInfoMsgBody *bodyPtr = (ShardComponentInfoMsgBody *)compInfoMessage->getMessageBody();
	bodyPtr->shardId = migrationSession[sessionKey].shardId;
	bodyPtr->componentSize = migrationSession[sessionKey].shardCompSize;
	bodyPtr->componentNameSize = componentName.size();
	memcpy(bodyPtr->name, componentName.c_str(), componentName.size());
	migrationSession[sessionKey].status = MM_STATE_INFO_ACK_WAIT;
	sendMessage(destinationNodeId, compInfoMessage);
	deAllocateMessage(compInfoMessage);
	//int tryCount = 5000000;
	while(migrationSession[sessionKey].status != MM_STATE_INFO_ACK_RCVD) {
		//sleep(1);
		//--tryCount;
	}
}

void MigrationManager::sendComponentDoneMsg(const string& sessionKey) {

	const string& componentName  = migrationSession[sessionKey].shardCompName;
	bool shardDone = componentName.size() > 0 ? false : true;
	Message *compDoneMessage = allocateMessage(sizeof(MigrationDoneMsgBody));

	if (!shardDone) {
		compDoneMessage->setType(MigrationComponentEndAckMessage);
	} else {
		compDoneMessage->setType(MigrationCompleteAckMessage);
	}

	MigrationDoneMsgBody *bodyPtr = (MigrationDoneMsgBody *)compDoneMessage->getMessageBody();
	bodyPtr->shardId = migrationSession[sessionKey].shardId;

	if (!shardDone) {
		migrationSession[sessionKey].status = MM_STATE_INFO_WAIT;  // waiting for next component
	} else {
		migrationSession[sessionKey].status = MM_STATE_SHARD_RCVD;
	}
	sendMessage(migrationSession[sessionKey].remoteNode, compDoneMessage);
	deAllocateMessage(compDoneMessage);

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

	return udpSocket;

}

const CoreInfo_t * MigrationManager::getIndexConfig(ClusterShardId shardId) {
	boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> clusterReadview;
	srch2::httpwrapper::ShardManager::getReadview(clusterReadview);
	return clusterReadview->getCore(shardId.coreId);
}

bool MigrationManager::hasActiveSession(const ClusterShardId& shardId, unsigned remoteNode) {
	string sessionKey;
	return hasActiveSession(shardId, remoteNode, sessionKey);
}
bool MigrationManager::hasActiveSession(const ClusterShardId& shardId, unsigned remoteNode, string& sessionKey) {
	sessionKey = getSessionKey(shardId, remoteNode);
	if (migrationSession.find(sessionKey) == migrationSession.end()) {
		return false;
	} else  {
		return true;
	}
}

string MigrationManager::initMigrationSession(ClusterShardId shardId, unsigned srcOperationId,
		unsigned dstOperationId, unsigned remoteNode, unsigned shardCompCount) {

	string key = getSessionKey(shardId, remoteNode);
	MigrationSessionInfo * info = new MigrationSessionInfo();
	info->shardId = shardId;
	info->srcOperationId = srcOperationId;
	info->dstOperationId = dstOperationId;
	info->beginTimeStamp = 0;
	info->endTimeStamp = 0;
	info->listeningPort = -1;
	info->status = MM_STATE_MIGRATION_BEGIN;
	info->remoteNode = remoteNode;
	info->shardCompCount = shardCompCount;
	//info.shard.reset();
	migrationSession[key] = *info;
	return key;
}

} /* namespace httpwrapper */
} /* namespace srch2 */
