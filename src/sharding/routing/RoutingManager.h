#ifndef __ROUTING_MANAGER_H__
#define __ROUTING_MANAGER_H__

#include<sys/time.h>
#include<server/Configuration.h>

/* All Objects sent and received from the RoutingManager must have the 
 * following functions calls:
 *
 *    //serializes the object to a byte array and places array into the region
 *    //allocated by given allocator
 *    void* serialize(std::allocator<char>);
 *
 *    //given a byte stream recreate the original object
 *    const Object& deserialize(void*);
 *
 *
 *    //Returns the type of message which uses this kind of object as transport
 *    MessageType messsageKind();
 */

/* All Callback Object used for recieving Messages must have the appropriate
 * callback corresponding with their message Type:
 *
 *   All must have:
 *      onFail();
 *
 *   QueryType must have:
 *      void search(ResultType&); //for single call back
 *      void search(MessageIterator<ResultType&>); //for wait all call back
 *   Any other type:
 *      void receive(ResultType&); //for single call back
 *      void receive(MessageIterator<ResultType&>); //for wait all call back
 *
 *   if Callback has a timeout the following method must be implemented:
 *      timeout();
 */

class RoutingManager {
  typedef unsigned CoreId;
  /* 
   *  Transmits a given message to all shards. The broadcast will not wait for
   *  confirmation from each receiving shard.
   */
  template <typename DataType> void broadcast(CoreId, DataType&); 
  
  /* 
   *  Transmits a given message to all shards. The broadcast block until
   *  confirmation from each shard is received. Returns false iff any 
   *  receiving shard confirms with MESSAGE_FAILED message.
   */
 template <typename DataType> 
   bool broadcast_wait_for_all_confirmation(CoreId, DataType&, bool&, timeval); 
  
  /*
   *  Transmits a given message to all shards. Upon receipt of a response from
   *  any shard, the callback is trigger with the corresponding Message. 
   *  The callback will be called for each shard.
   */
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> 
    void broadcast_w_cb(CoreId, DataType&, CallBack<ResultType>);

  /* 
   *  Transmits a given message to all shards. The return messages for each
   *  shard are held until all shard’s return messages received. Then the 
   *  callback is triggers with an array of message results from each shard.
   */
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> 
  void broadcast_wait_for_all_w_cb(CoreId, DataType&, 
      CallBack<ResultType>);

  /*
   *  Timeout version of their corresponding function. So, after a period of
   *  set milliseconds the timeout callback function is called 
   *
   *       *** Potentially could alert sync layer to timed out message 
   *           from shard ***
   */
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> 
  void broadcast_w_cb_n_timeout(DataType&, CallBack<ResultType>, timeval);
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> 
  void broadcast_wait_for_all_w_cb_n_timeout(Core::Id, DataType&, 
      CallBack<ResultType>, timeval);
  /* 
   *  Transmits a given message to a particular shard in a non-blocking fashion
   */
  template <typename DataType> void route(ShardId, DataType&); 
  
  /* 
   *  Transmits a given message to a pariticular shards, and waits for 
   *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED 
   *  message.
   */
 template <typename DataType> 
   bool route_wait_for_confirmation(ShardId, DataType&, bool&, timeval); 
  
  /*
   *  Transmits a given message to a particular shards. Upon receipt of a 
   *  response shard, the appropriate callback is trigger with the
   *  corresponding Message. 
   */
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> 
    void route_w_cb(ShardId, DataType&, CallBack<ResultType>);

  /*
   *  Timeout version of their corresponding function. So, after a period of
   *  set milliseconds the timeout callback function is called 
   *
   *       *** Potentially could alert sync layer to timed out message 
   *           from shard ***
   */
  template <typename DataType, template<class ResultType> class CallBack,
           class ResultType> void
    route_w_cb_n_timeout(ShardId, DataType&, CallBack<ResultType>, timeval);
};

#endif
