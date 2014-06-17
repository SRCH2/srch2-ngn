#ifndef __SHARDING_MIGRATION_MIGRATIONMANAGER_H__
#define __SHARDING_MIGRATION_MIGRATIONMANAGER_H__

#include "sharding/configuration/ShardingConstants.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/processor/DistributedProcessorInternal.h"
#include "core/util/Assert.h"

/*
 * This class is responsible of load balancing upon
 * arrival of a new node, removal of a node from cluster, or
 * any change in the state of the cluster which makes the cluster unbalanced
 */
namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class MigrationManager{
public:
	MigrationManager(ConfigManager * config, DPInternalRequestHandler * dpInternal){
		this->configManager = config;
		this->dpInternal = dpInternal;
	}
	void initializeLocalShards(){
		// TODO : we ignore replicas at this point, it will be completed in phase 3
		// get the cluster object from Config Manager
		Cluster* cluster = configManager->getCluster();
		// iterate over all shards in the cluster and initialize the srch2Server objects
		// for those which are 1.local 2.primary shard
		// TODO : get a pointer to shardMap to be able to change it when needed
		std::map<ShardId, Shard, ShardIdComparator> * shardMap = NULL;  // = cluster->getShardMap();
		for(std::map<ShardId, Shard, ShardIdComparator>::iterator shardItr = shardMap->begin(); shardItr != shardMap->end(); ++shardItr){
			ShardId shardId = shardItr->first;
			// check to make sure this shard is a local shard
			if(shardId.coreId != cluster->getCurrentNode()->getId()){
				continue;
			}
			// check to make sure this is a primary shard
			if(! shardId.isPrimaryShard()){
				continue;
			}

			ASSERT(shardItr->second.getShardState() == SHARDSTATE_UNALLOCATED);
			// this shard is local and it's a primary shard, so we can initialize the Srch2Server object from DPInternal
			CoreInfo_t * coreInfo = NULL ;// TODO : API must be added to cluster to get id and return coreInfo_t
			Srch2ServerHandle newHandle = this->dpInternal->registerSrch2Server(shardId, coreInfo);
			if(newHandle <= 0){ // error case
				// TODO : handle error, dpInternal couldn't allocate this index.
			}
			/*
			 * change the state of this shard to 'indexing'
			 */
			// 1. TODO : get an X lock on the cluster because we need to change the state of this shard
			// ...
			// 2. change the state of this shardId
			shardItr->second.setShardState(SHARDSTATE_INDEXING);
			// 3. TODO : release the X lock
			// ....

			/*
			 * use DPInternal to bootstrap the index
			 */
			DPInternalAPIStatus status = this->dpInternal->bootstrapSrch2Server(newHandle);
			// 4. check the result and if succeed, change the status
			if(status == DPInternal_Srch2ServerNotFound){ // Error case
				// Why isn't the shard available ?
				this->dpInternal->deleteSrch2Server(newHandle); // it's wrong anyways ...TODO
				ASSERT(false);
			}else if(status == DPInternal_Success){
				/*
				 * change the state of this shard to 'allocated' and save the indexHandle in shard
				 */
				//1. TODO : get an X lock on the cluster
				// ..
				//2. change the state to 'allocated'
				shardItr->second.setShardState(SHARDSTATE_ALLOCATED);
				//3. change the indexHandle
				shardItr->second.setSrch2ServerHandle(newHandle);
				//4. TODO : release the X lock
				// ....
			}
			// and move to the next shard.

		}
	}
private:
	ConfigManager * configManager;
	DPInternalRequestHandler * dpInternal;
};


}
}

#endif // __SHARDING_MIGRATION_MIGRATIONMANAGER_H__
