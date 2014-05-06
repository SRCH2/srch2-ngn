#ifndef __RM_CALLBACKS_H__
#define __RM_CALLBACKS_H__

#include "transport/PendingMessages.h"

namespace srch2 {
namespace httpwrapper {

template <typename RequestType, typename ResponseType>
struct RMCallback : Callback {
  ResultAggregatorAndPrint<RequestType, ResponseType>& aggregrate;

  RMCallback(ResultAggregatorAndPrint<RequestType, ResponseType>&);

  void timeout(void*);
  void callback(Message*);
  void callbackAll(std::vector<Message*>&);
};


template <typename RequestType, typename ResponseType>
RMCallback<RequestType, ResponseType>::RMCallback(
    ResultAggregatorAndPrint<RequestType, ResponseType>& a) : aggregrate(a) {}


template <typename RequestType, typename ResponseType>
void RMCallback<RequestType, ResponseType>::timeout(void*) {
//  aggregrate.timeoutProcessing(//shardInfo, requestObject, metadata);
}


template <typename RequestType, typename ResponseType>
void RMCallback<RequestType, ResponseType>::callback(Message* msg) {
  ResponseType& response = ResponseType::deserialize(msg->buffer);
  aggregrate.callBack(&response);

  delete response;
}

template <typename RequestType, typename ResponseType>
void RMCallback<RequestType, ResponseType>::
callbackAll(std::vector<Message*>& msgs) {
  typedef std::vector<Message*> Messages;
  typedef std::vector<ResponseType*> Responses; 

  Responses responses;
  for(Messages::iterator msg = msgs.begin(); msg != msgs.end(); ++msg) {
    responses.push_back(&ResponseType::deserialize((*msg)->buffer));
  }

  aggregrate.callBack(responses);

  for(typename Responses::iterator response = responses.begin();
      response != responses.end(); ++response) {
    delete *response;
  }
}

}}
#endif /* __RM_CALLBACKS_H__ */
