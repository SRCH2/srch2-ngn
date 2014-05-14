#include "sharding/configuration/ConfigManager.h"

using namespace srch2::httpwrapper;

void generateShards(ConfigManager& cm) {
  for(ConfigManager::CoreInfoMap_t::iterator core = cm.coreInfoIterateBegin();
                core != cm.coreInfoIterateEnd(); ++core)  {
    int n = 0;
    for(std::vector<Node>::iterator node= cm.getCluster()->getNodes()->begin(); 
        node != cm.getCluster()->getNodes()->end(); ++node) {
      cm.getCluster()->shardMap
        [ShardId(core->second->getCoreId(), node->getId())] =
          Shard(node->getId(), core->second->getCoreId(), node->getId());
      core->second->shards.push_back(
          ShardId(core->second->getCoreId(), node->getId()));
    }
  }
}
