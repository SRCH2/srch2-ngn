#ifndef  __MULTIPLEXER_H__
#define __MULTIPLEXER_H__

#include<vector>
#include "configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

/*
 * Multiplexer is responsibe of using the shard->node map to provide an iterator
 * on nodes which is used by RM methods such as broadcast
 */
class Multiplexer {
public:
    Multiplexer(ConfigManager& cm, CoreShardInfo& info);

    /*
     * Usage of this iterator is like this :

  void Usage(){

      for(initIteration(); hasMore(); nextIteration()){
          ShardId shardId = getNextShardId();
          // ...
      }
  }

     */
    size_t size();
    void initIteration();
    bool hasMore();
    void nextIteration();
    ShardId getNextShardId();
private:
    ConfigManager& configManager;
    CoreShardInfo& coreShardInfo;
    CoreInfo_t& coreInfo;
    vector<ShardId> destinations;
    unsigned destinationsIterator;
};
}}
#endif /* __MULTIPLEXER_H__ */
