/*
 * MigrationManager.h
 *
 *  Created on: Jul 1, 2014
 *      Author: srch2
 */

#ifndef __MIGRATION_MIGRATIONMANAGER_H__
#define __MIGRATION_MIGRATIONMANAGER_H__

#include <vector>
#include <string>
#include <iostream>

#include "configuration/Shard.h"
#include "transport/TransportManager.h"
#include "ShardManager.h"

namespace srch2 {
namespace httpwrapper {

/*
 *   new API for Srch2Server Object
class Srch2Server {
	void loadIndexFromByteStream(const std::istream& binaryStream);
	void loadIndexFromFile(const std::string& filename); // existing code... need refactor
	void createIndexFromFile();                          // existing code ..need refactor
	void createEmptyIndex();

	bool addRecord(string json);
	bool deleteRecord(string pk);
	bool updateRecord(string pk, string jsonRec);
};

*/

// Hash function for key of type ShardId to be used by boost::unordered_map
struct ShardIdHasher{
  std::size_t operator()(const ShardId& key) const
  {
    return key.coreId + key.partitionId + key.replicaId;
  }
};

enum MIGRATION_STATUS{

	MM_STATE_MIGRATION_BEGIN,

	//sender's state
	MM_STATE_INIT_ACK_WAIT,  MM_STATE_INIT_ACK_RCVD,
	MM_STATE_INFO_ACK_WAIT,  MM_STATE_INFO_ACK_RCVD,
	MM_STATE_COMPONENT_TRANSFERRED_ACK_RCVD, MM_STATE_SHARD_TRANSFERRED_ACK_RCVD,

	//Receiver's state
	MM_STATE_INIT_RCVD,  MM_STATE_INFO_WAIT,
	MM_STATE_INFO_RCVD,  MM_STATE_COMPONENT_RECEIVING,
	MM_STATE_SHARD_RCVD,
};

// migration status data structure
struct ShardMigrationStatus{
	ShardId shardId;
	unsigned sourceNodeId;
	unsigned destinationNodeId;
	MIGRATION_STATUS status;
};

// redistribution data structure
struct Stats{
	ShardId shardId;
	unsigned nodeId;
	unsigned recCount;
};

struct ShardDistributionStatus{
	MIGRATION_STATUS status;
	unsigned statsCount;
	// for source this is destination stats and for the destination this is source stats
	Stats statsArray[0];
};

// Forward declarations
class ShardManager;
class TransportManager;
class Partitioner;

const short MM_MIGRATION_PORT_START = 53000;

// MM internal structure to
class MigrationSessionInfo {
public:
	boost::shared_ptr<Srch2Server> shard;
	unsigned shardCompCount;
	unsigned shardCompSize;
	string shardCompName;

	MIGRATION_STATUS status;

	boost::shared_ptr<char> termAckMessage;

	unsigned beginTimeStamp;  // epoch value
	unsigned endTimeStamp;    // epoch value

	unsigned remoteNode;
	unsigned remoteAddr;
	short listeningPort;
};

// callback handler from TM
class MigrationManager;
class MMCallBackForTM : public CallBackHandler {
public:
	MMCallBackForTM(MigrationManager *migrationMgr);
	bool resolveMessage(Message * msg, NodeId node);
private:
	MigrationManager *migrationMgr;
};

class MigrationService {
public:
	MigrationService(MigrationManager *migrationMgr) { this->migrationMgr = migrationMgr; }
	void sendShard(ShardId shardId, std::ostream& bysteStream, unsigned destinationNodeId);
	void receiveShard(ShardId shardId);
private:
	MigrationManager *migrationMgr;
};

class MigrationManager {
	friend class MMCallBackForTM;
	friend class MigrationService;
public:
	/*
	 * 1. Nonblocking API for migrating shard. Notifies ShardManager when migration is done.
	 * 2. ShardManager must expose method "void Notify(ShardMigrationStatus &)"
	 */
	void migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, unsigned destinationNodeId);

	/*
	 * 1. Nonblocking API for migrating a single shard to multiple destinations.
	 * 2. Notifies ShardManager when migration is done. ShardManager must expose method
	 *    "void Notify(ShardMigrationStatus &)"
	 */
	void migrateShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, std::vector<unsigned> destinationNodeId);

	/*
	 * 1. Non Blocking API. Notifies ShardManager when distribution is done.
	 * 2. ShardManager must expose method "void Notify(ShardDistributionStatus &)"
	 * 3. partitioner is used for deciding shardId/NodeId
	 */
	void reDistributeShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, Partitioner *partitioner, unsigned destination);

	/*
	 * 1. ShardManager on client node provides shardId of the shard that is expected to arrive.
	 * 2. MM return shard object.
	 */
	boost::shared_ptr<Srch2Server> acceptMigratedShard(ShardId shardId);

	/*
	 *  1. ShardManager on remote node provides Shard object to which migrated record should be assigned.
	 *  2. non-blocking API.
	 */
	void registerShardForMigratedRecords(ShardId shardId, boost::shared_ptr<Srch2Server> shard);

	/*
	 *  1. ShardManager should call this to stop accepting new records into the given shard.
	 *  2. MM releases shared pointer to the requested shard.
	 */
	void unRegisterShardForMigratedRecords(ShardId shardId);

	/*
	 * ShardManager should call this API to request aborting the migration of the given shardId.
	 * This is a non blocking API. When migration stops, MM notifies SHM via callback with migration
	 * status as ABORTED
	 */
	void stopMigration(ShardId shardId);

	/*
	 * ShardManager should call this API to request aborting the migration of the given shardId to
	 * the given destinatonNodeId only.
	 * This is a non blocking API. When migration stops, MM notifies SHM via callback with migration
	 * status as ABORTED
	 */
	void stopMigration(ShardId shardId, unsigned destinatonNodeId);

	MigrationManager(ShardManager *shardManager, TransportManager *transport, ConfigManager *config);

	~MigrationManager();

	void receiveShard(ShardId shardId);

private:
	// MM private functions
	int openTCPSendChannel(unsigned addr, short port);
	int openSendChannel();
	void openTCPReceiveChannel(int& , short&);
	void openReceiveChannel(int& , short&);
	void sendComponentInfoAndWaitForAck(ShardId shardId,const string& componentName, unsigned componentSize,
			unsigned destinationNodeId);
	void sendComponentDoneMsg(ShardId shardId, const string& componentName);
	void sendInitMessageAck(ShardId shardId, short receivePort);
	void sendInfoAckMessage(ShardId shardId);
	int acceptTCPConnection(int tcpSocket , short receivePort);
	void doInitialHandShake(ShardId shardId, unsigned componentCount, unsigned destinationNodeId);
	void initMigrationSession(unsigned, unsigned, ShardId);
	bool hasActiveSession(const ShardId& shardId);
	void sendMessage(unsigned destinationNodeId, Message *message);
	const CoreInfo_t *getIndexConfig(ShardId shardId);

	Message * allocateMessage(unsigned size) {
		return MessageAllocator().allocateMessage(size);
	}

	void deAllocateMessage(Message *p) {
		MessageAllocator().deallocateByMessagePointer(p);
	}

	// MM private members
	int sendSocket;
	int receiveSocket;
	short receivePort;
	TransportManager * transport;
	ShardManager* shardManager;
	MMCallBackForTM *transportCallback;
	ConfigManager *configManager;
	boost::unordered_map<ShardId, MigrationSessionInfo, ShardIdHasher> migrationSession;
	enum SessionLockState {
		LOCKED,
		UNLOCKED
	};
	SessionLockState _sessionLock;
};

}
}
#endif /* __MIGRATION_MIGRATIONMANAGER_H__ */
