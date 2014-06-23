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
		Cluster * cluster = configManager->getClusterWriteView();

		std::vector<CoreShardContainer * > * currentNodeShardInfo =
				cluster->getNodeShardInformation_Writeview(cluster->getCurrentNode()->getId());

		for(unsigned cid = 0 ; cid < currentNodeShardInfo->size() ; ++cid){
			vector<Shard *> * primaryShards = currentNodeShardInfo->at(cid)->getPrimaryShards();

			for(unsigned sid = 0 ; sid < primaryShards->size() ; ++sid){

				ASSERT(primaryShards->at(sid)->getShardState() == SHARDSTATE_UNALLOCATED);

				primaryShards->at(sid)->setShardState(SHARDSTATE_INDEXING);
				string directoryPath = configManager->getShardDir(cluster->getClusterName(),
						cluster->getCurrentNode()->getName(),
						currentNodeShardInfo->at(cid)->getCore()->getName(), primaryShards->at(sid)->getShardId());
				boost::shared_ptr<Srch2Server> srch2Server =
						this->dpInternal->registerAndInitializeSrch2Server(primaryShards->at(sid)->getShardId(),
								currentNodeShardInfo->at(cid)->getCore(), directoryPath);

				primaryShards->at(sid)->setShardState(SHARDSTATE_ALLOCATED);

				primaryShards->at(sid)->setSrch2Server(srch2Server);
			}

		}

		configManager->commitClusterMetadata();
	}
private:
	ConfigManager * configManager;
	DPInternalRequestHandler * dpInternal;
};


}
}

#endif // __SHARDING_MIGRATION_MIGRATIONMANAGER_H__
