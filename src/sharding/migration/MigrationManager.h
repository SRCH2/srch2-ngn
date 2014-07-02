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

unsigned char MM_SUCCESS = 0x0;                 // successful migration
unsigned char MM_NETWORK_FAILURE = 0x01;        // transport failure
unsigned char MM_SERIALIZATION_FAILURE = 0x02;  // data got corrupted

// migration data structure
struct ShardMigrationStatus{
	unsigned shardId;
	unsigned sourceNodeId;
	unsigned destinationNodeId;
	char status;
};

// redistribution data structure
struct {
	unsigned shardId;
	unsigned nodeId;
	unsigned recCount;
} Stats;

struct ShardDistributionStatus{
	char status;
	unsigned statsCount;
	// for source this is destination stats and for the destination this is source stats
	Stats statsArray[0];
};

// MM internal data structure goes here...

// Forward declarations
class ShardManager;
class TransportManager;
class Partitioner;

class MigrationManager {
public:
	// 1. Nonblocking API. Notifies ShardManager when migration is done.
	// 2. ShardManager must expose method "void Notify(ShardMigrationStatus &)"
	//
	void migrateShard(boost::shared_ptr<Srch2Server> shard, unsigned destinationNodeId);

	// 1. Non Blocking API. Notifies ShardManager when distribution is done.
	// 2. ShardManager must expose method "void Notify(ShardDistributionStatus &)"
	// 3. partitioner is used for deciding shardId/NodeId
	void reDestributeShard(boost::shared_ptr<Srch2Server> shard, Partitioner *partitioner);

	// 1. ShardManager on client node provides shardId of the shard that is expected to arrive.
	// 2. MM return shard object.
	boost::shared_ptr<Srch2Server> acceptMigratedShard(unsigned shardId);

	// 1. ShardManager on remote node provides Shard object to which migrated record should be assigned.
	// 2. non-blocking API.
	void registerShardForMigratedRecords(boost::shared_ptr<Srch2Server> shard);

	// 1. ShardManager should call this to stop accepting new records into the give shard.
	// 2. MM releases shared pointer to the requested shard.
	void unRegisterShardForMigratedRecords(unsigned shardId);

	MigrationManager(ShardManager *shardManager, TransportManager *transport);

	virtual ~MigrationManager();

private:
	 // MM private members

};

} /* namespace httpwrapper */
} /* namespace srch2 */
#endif /* __MIGRATION_MIGRATIONMANAGER_H__ */
