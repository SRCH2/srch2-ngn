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
	MM_STATUS_MIGRATION_BEGIN,

	MM_STATUS_WAITING_FOR_INIT_ACK,
	MM_STATUS_INIT_ACK_RECEIVED,
	MM_STATUS_INIT_NACK_RECEIVED,
	MM_STATUS_SENDING_DATA,
	MM_STATUS_WAITING_FOR_TERM_ACK,
	MM_STATUS_TERM_ACK_RECEIVED,

	MM_STATUS_WAITING_FOR_INIT_REQ,
	MM_STATUS_INIT_REQ_RECEIVED,
	MM_STATUS_FETCHING_DATA,
	MM_STATUS_TERM_REQ_RECEIVED,

	MM_STATUS_MIGRATION_SUCCESS,
	MM_STATUS_SERIALIZATION_FAILURE,
	MM_STATUS_NETWORK_FAILURE,
	MM_STATUS_MIGRATION_ABORTED
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
	unsigned shardSize;

	MIGRATION_STATUS status;

	boost::shared_ptr<char> termAckMessage;

	unsigned beginTimeStamp;  // epoch value
	unsigned endTimeStamp;    // epoch value

	unsigned remoteNode;
	unsigned remoteAddr;
	signed listeningSocket;
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
	void reDistributeShard(ShardId shardId, boost::shared_ptr<Srch2Server> shard, Partitioner *partitioner);

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
	int openSendChannel();
	void openReceiveChannel(int& , short&);
	void initMigrationSession(unsigned, unsigned, ShardId);
	bool hasActiveSession(const ShardId& shardId);
	void sendMessage(unsigned destinationNodeId, Message *message);
	const CoreInfo_t *getIndexConfig(ShardId shardId);

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
