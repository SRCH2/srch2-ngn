#ifndef __SHARDING_CONFIGURATION_SHARD_H__
#define __SHARDING_CONFIGURATION_SHARD_H__

#include <map>
#include <vector>
#include <set>
#include "src/core/util/Assert.h"
#include "CoreInfo.h"
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>




using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;

class ShardId {
public:
	unsigned coreId;
	unsigned partitionId; // ID for a partition, numbered 0, 1, 2, ...

	// ID for a specific primary/replica for a partition; assume #0 is always the primary shard.  For V0, replicaId is always 0
	unsigned replicaId;

	bool isPrimaryShard() const;
	std::string toString() const;

	ShardId() ;
	ShardId(unsigned coreId, unsigned partitionId, unsigned replicaId=0) ;

	bool operator==(const ShardId& rhs) const ;
	bool operator!=(const ShardId& rhs) const ;
	bool operator>(const ShardId& rhs) const ;
	bool operator<(const ShardId& rhs) const ;
	bool operator>=(const ShardId& rhs) const ;
	bool operator<=(const ShardId& rhs) const ;

};

class ShardIdComparator {
public:
	// returns s1 > s2
	bool operator() (const ShardId s1, const ShardId s2);
};



class Shard {
public:
	Shard();
	Shard(const Shard & shard);
	Shard(unsigned nodeId, unsigned coreId, unsigned partitionId = 0,
			unsigned replicaId = 0);
	//Can be used in Migration
	void setPartitionId(int partitionId);
	//Can be used in Migration
	void setReplicaId(int replicaId);
	ShardId getShardId() const;
	void setShardState(ShardState newState);
	void setNodeId(unsigned id);
	ShardState getShardState() const;
	unsigned getNodeId() const;
	void setSrch2Server(boost::shared_ptr<Srch2Server> srch2Server);
	boost::shared_ptr<Srch2Server> getSrch2Server() const;
	std::string toString() const;


private:
	ShardId shardId;
	ShardState shardState;
	unsigned nodeId;
	boost::shared_ptr<Srch2Server> srch2Server;
};

/*
 * A simple container which contains all shards of a core
 */
class CoreShardContainer{
public:
	CoreShardContainer(CoreInfo_t * core);
	CoreShardContainer(const CoreShardContainer & coreShardContainer);
	CoreInfo_t * getCore();
	const CoreInfo_t * getCore() const;
	void setCore(CoreInfo_t * core);

	vector<Shard *> * getPrimaryShards();
	vector<Shard *> * getReplicaShards();
	void addPrimaryShards(vector<const Shard *> & primaryShards) const;
	void addReplicaShards(vector<const Shard *> & replicaShards) const;
	void addPrimaryShardReplicas(const ShardId & primaryShardId, vector<const Shard *> & replicaShards) const;
	unsigned getTotalNumberOfPrimaryShards() const;
	const Shard * getShard(const ShardId & shardId) const;

private:
	CoreInfo_t * core;

	// note: all primary and replica shards in these two sets
	// share the same coreId and nodeId
	vector<Shard *> primaryShards;
	vector<Shard *> replicaShards;
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARD_H__
