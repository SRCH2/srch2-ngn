#include "sharding/configuration/ConfigManager.h"

using namespace srch2::httpwrapper;

/*
 * This function temporarily fills out the structure
 * std::map<ShardId, Shard, ShardIdComparator> shardMap;
 * which is in Cluster. This structure basically keeps all shards of the system.
 * Originally this structure should be filled by SM
 */
void generateShards(ConfigManager& configurationManager) {
	// iterate over cores
	for(ConfigManager::CoreInfoMap_t::iterator coreInfoMap = configurationManager.coreInfoIterateBegin();
			coreInfoMap != configurationManager.coreInfoIterateEnd(); ++coreInfoMap)  {
		CoreInfo_t * coreInfoPtr = coreInfoMap->second;
		string coreName = coreInfoMap->first;
		//ASSERT(coreName == coreInfoPtr->getName());
		// iterate over nodes
		for(std::vector<Node>::iterator node= configurationManager.getCluster()->getNodes()->begin();
				node != configurationManager.getCluster()->getNodes()->end(); ++node) {

			// add the shard
			// NOTE: we use node id as the partition id temporarily
			configurationManager.getCluster()->shardMap[ShardId(coreInfoPtr->getCoreId(), node->getId())] =
							Shard(node->getId(), coreInfoPtr->getCoreId(), node->getId());

			coreInfoPtr->shards.push_back(ShardId(coreInfoPtr->getCoreId(), node->getId()));
		}
	}
}
