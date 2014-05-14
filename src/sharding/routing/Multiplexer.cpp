#include "sharding/routing/Multiplexer.h"


namespace srch2 {
namespace httpwrapper {

Multiplexer::Multiplexer(ConfigManager& cm, CoreShardInfo& info) :
  cm(cm), info(info), coreInfo(*cm.getCoreInfo(info.coreName)) {}

UnicastIterator Multiplexer::begin() {
  return UnicastIterator(*cm.getCluster(),coreInfo.getShardsVector().begin());
}

UnicastIterator Multiplexer::end() {
  return UnicastIterator(*cm.getCluster(), coreInfo.getShardsVector().end());
}

size_t Multiplexer::size() {
  return coreInfo.getShardsVector().size();
}

UnicastIterator::UnicastIterator(const UnicastIterator& cpy)
  : id(cpy.id), i(cpy.i), cluster(cpy.cluster) {}

UnicastIterator::UnicastIterator(Cluster &cluster, const std::vector<ShardId>::iterator& i) : i(i), cluster(cluster) {
  id.shardId = *i;
  id.nodeId = cluster.shardMap[id.shardId].getNodeId();
}

UnicastIterator& UnicastIterator::operator=(const UnicastIterator& c) {
  new (this) UnicastIterator(c);
  return *this;
}

UnicastIterator& UnicastIterator::operator++() {
  ++i;
  id.shardId = *i;
  id.nodeId = cluster.shardMap[id.shardId].getNodeId();
  return *this;
}

UnicastIterator UnicastIterator::operator++(int) {
  UnicastIterator rtn(*this);
  ++*this;
  return rtn;
}
bool UnicastIterator::operator==(const UnicastIterator& rhs) {
  return id.nodeId == rhs.id.nodeId && i == rhs.i;
}
bool UnicastIterator::operator!=(const UnicastIterator& rhs) {
  return id.nodeId != rhs.id.nodeId || i != rhs.i;
}
UnicastIterator::Unicast* UnicastIterator::operator->() {
  return &this->id;
}

}}
