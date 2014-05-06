#ifndef  __MULTIPLEXER_H__
#define __MULTIPLEXER_H__

#include<vector>
#include "configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

class UnicastIterator {
  struct Unicast {
    NodeId nodeId;
    ShardId shardId;
  } id;
  std::vector<ShardId>::iterator i;
  Cluster& cluser;

public:
  UnicastIterator(const UnicastIterator&);
  UnicastIterator(unsigned, const std::vector<ShardId>::iterator&);
  UnicastIterator();

  UnicastIterator& operator=(const UnicastIterator&);
  UnicastIterator& operator++();
  UnicastIterator operator++(int);
  bool operator==(const UnicastIterator&);
  bool operator!=(const UnicastIterator&);

  Unicast* operator->();
};

struct Multiplexer {
  ConfigManager& cm;
  CoreShardInfo& info;
  CoreInfo_t& coreInfo;

  Multiplexer(ConfigManager& cm, CoreShardInfo& info);
  size_t size();
  UnicastIterator begin();
  UnicastIterator end();
};

inline
Multiplexer::Multiplexer(ConfigManager& cm, CoreShardInfo& info) :
  cm(cm), info(info), coreInfo(*cm.getCoreInfo(info.coreName)) {}

inline UnicastIterator Multiplexer::begin() {
  return UnicastIterator(coreInfo.shards.begin(), *cm.getCluster());
}

inline UnicastIterator Multiplexer::end() {
  return UnicastIterator(coreInfo.shards.end(), *cm.getCluster());
}

inline UnicastIterator::size() {
  return coreInfo.shards.size();
}

inline UnicastIterator::UnicastIterator() {}
inline UnicastIterator::UnicastIterator(const UnicastIterator& cpy) 
  : id(cpy.id), i(cpy.i), cluster(cluser) {}

inline UnicastIterator::UnicastIterator(Cluster &cluster
    const std::vector<ShardId>::iterator& i) : i(i), cluster(cluster) {
  id.shardId = *i;
  id.nodeId = cluster.shardMap[id.shardId].getNodeId();
}

inline UnicastIterator& UnicastIterator::operator=(const UnicastIterator& c) {
  new (this) UnicastIterator(c);
  return *this;
}

inline UnicastIterator& UnicastIterator::operator++() {
  ++i;
  id.shardId = *i;
  id.nodeId = cluster.shardMap[id.shardId].getNodeId();
  return *this;
}

inline UnicastIterator UnicastIterator::operator++(int) {
  UnicastIterator rtn(*this);
  ++*this;
  return rtn;
}
inline bool UnicastIterator::operator==(const UnicastIterator& rhs) {
  return id.nodeId == rhs.id.nodeId && i == rhs.i;
}
inline bool UnicastIterator::operator!=(const UnicastIterator& rhs) {
  return id.nodeId != rhs.id.nodeId || i != rhs.i;
}
inline UnicastIterator::Unicast* UnicastIterator::operator->() {
  return &this->id;
}

}}
#endif /* __MULTIPLEXER_H__ */
