#ifndef  __MULTIPLEXER_H__
#define __MULTIPLEXER_H__

#include<vector>
#include "configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

/*
 * This is used for iterating over nodes/shards for broadcast
 */
typedef unsigned NodeId;
class UnicastIterator {
  struct Unicast {
    ShardId shardId;
  } id;
  std::vector<ShardId>::iterator i;
  /*
   * We need cluster to map shardIds to nodes
   */
  Cluster& cluster;

public:
  UnicastIterator(const UnicastIterator&);
  UnicastIterator(Cluster& cluser, const std::vector<ShardId>::iterator&);
  UnicastIterator();

  UnicastIterator& operator=(const UnicastIterator&);
  UnicastIterator& operator++();
  UnicastIterator operator++(int);
  bool operator==(const UnicastIterator&);
  bool operator!=(const UnicastIterator&);

  Unicast* operator->();
};

/*
 * Multiplexer is responsibe of using the shard->node map to provide an iterator
 * on nodes which is used by RM methods such as broadcast
 */
struct Multiplexer {
  ConfigManager& cm;
  CoreShardInfo& info;
  CoreInfo_t& coreInfo;

  Multiplexer(ConfigManager& cm, CoreShardInfo& info);
  size_t size();
  UnicastIterator begin();
  UnicastIterator end();
};
}}
#endif /* __MULTIPLEXER_H__ */
