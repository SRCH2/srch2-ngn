#ifdef  __MULTIPLEXER_H__
#define __MULTIPLEXER_H__
template<typename DataType, typename ResponseType, 
  template<typename ResponseType> class CallBack, WaitCondition>
class Multiplexer {
  AtomicBitMask<maxNumberOfShards> waitingShards;
  CallBack& cb;
  public:
    void onReturnMessage(Message*);
    void onTimeout();
};

#endif /* __MULTIPLEXER_H__ */
