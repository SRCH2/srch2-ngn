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
  return UnicastIterator(coreInfo.coreId, coreInfo.shards.begin());
}

inline UnicastIterator Multiplexer::end() {
  return UnicastIterator(coreInfo.coreId, coreInfo.shards.end());
}

inline UnicastIterator::UnicastIterator() {}
inline UnicastIterator::UnicastIterator(const UnicastIterator& cpy) 
  : id(cpy.id), i(cpy.i) {}

inline UnicastIterator::UnicastIterator(unsigned nodeId,
    const std::vector<ShardId>::iterator& i) : i(i) {
  id.nodeId = nodeId;
  id.shardId = *i;
}

inline UnicastIterator& UnicastIterator::operator=(const UnicastIterator& c) {
  new (this) UnicastIterator(c);
  return *this;
}

inline UnicastIterator& UnicastIterator::operator++() {
  ++i;
  id.shardId = *i;
  return *this;
}

inline UnicastIterator UnicastIterator::operator++(int) {
  UnicastIterator rtn(*this);
  ++i;
  id.shardId = *i;
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
