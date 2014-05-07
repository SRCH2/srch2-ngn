#include "sharding/routing/Multiplexer.h"


namespace srch2 {
namespace httpwrapper {





Multiplexer::Multiplexer(ConfigManager& cm, CoreShardInfo& info) :
  cm(cm), info(info), coreInfo(*cm.getCoreInfo(info.coreName)) {}

UnicastIterator Multiplexer::begin() {
  return UnicastIterator(*cm.getCluster(),coreInfo.shards.begin());
}

UnicastIterator Multiplexer::end() {
  return UnicastIterator(*cm.getCluster(), coreInfo.shards.end());
}

size_t Multiplexer::size() {
  return coreInfo.shards.size();
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


/*
template<Callback>
void Multiplex::broadcast(Message &msg, Callback cb, timeval time) {
  registerCallBack(msg, cb
*/



/*template<typename CallBack> 
void callBack(CallBack& cb, SerializedQueryResults& qr) {
  cb.internalSearchCommand(qr);
}

template<typename CallBack> void callBack(CallBack& cb, StatusMessageType& s) {
  cb.internalSearchCommand(s);
}
 //specialize 
template<typename DataType, typename ResponseType, 
  template<ResponseType> class CallBack, int WaitCondition>
void Multiplexer<DataType, ResponseType, CallBack, WaitCondition>::
onResultMessage(Message &msg) {

  push_back(msg);

  if(waitingCond == ROUTING_RESPONDED_EVERY) {
    if(msg.islocal()) {
      callBack(cb, (ResponseType) *msg.buffer);
    } else {
      ResponseType& response = ResponseType::deserialize(msg.buffer, msg.size);
      push_back(reponse);
      callBack(cb, response);
    }
  }

  waitingShards.zero(msg.shard.partitionId);

  if(waitingShards.isZero() && !finalized.getAndSet()) {
    if(waitingCond == ROUTING_RESPONSED_AFTER_ALL) {
     callBack(cb, MessageIterator<ResponseType>(msgs));
    }

    cb.finalize();
  }
}
template<typename DataType, typename ResponseType, 
  template<ResponseType> class CallBack, int WaitCondition>
void Multiplexer<DataType, ResponseType, CallBack, WaitCondition>::
onTimeout() {
  cb.timeout();
  if(waitingCond == ROUTING_RESPONDED_AFTER_ALL) {
     callBack(cb, MessageIterator<ResponseType>(msgs));
  }
  cb.finalize();
}
*/
