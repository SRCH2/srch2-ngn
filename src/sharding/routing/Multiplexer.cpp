
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
