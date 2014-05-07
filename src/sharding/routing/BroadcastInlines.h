#ifndef __BROADCAST_INLINES_H__
#define __BROADCAST_INLINES_H__

#include "transport/PendingMessages.h"
#include "sharding/routing/RoutingManager.h"
#include "RMCallback.h"


namespace srch2 {
namespace httpwrapper {

/*
 *  Transmits a given message to all shards. The broadcast will not wait for
 *  confirmation from each receiving shard.
 */
template<typename RequestType> inline void
RoutingManager::broadcast(RequestType& requestObj, CoreShardInfo &coreInfo) {
	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	// create the message from the request object
	Message* msg = (Message*)
    ((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;
    msg->mask |= INTERNAL_MASK;

		tm.route(unicast->nodeId, msg, 0);
	}

	getMessageAllocator()->deallocateMessage(msg);
}


/*
 *  Transmits a given message to all shards. The broadcast block until
 *  confirmation from each shard is received. Returns false iff any
 *  receiving shard confirms with MESSAGE_FAILED message.
 */
template<typename RequestType> inline bool 
RoutingManager::broadcast_wait_for_all_confirmation(RequestType& requestObject,
			bool& timedout, timeval timeoutValue , CoreShardInfo & coreInfo){
//TODO
  return false;
}
/*
 *  Transmits a given message to all shards. Upon receipt of a response from
 *  any shard, the callback is trigger with the corresponding Message.
 *  The callback will be called for each shard.
 */
template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_w_cb(RequestType& requestObj, 
      ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
      CoreShardInfo & coreInfo){

	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messageKind(),
												true,
												broadcastResolver.size()-1); //hack for missing local

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;
    msg->mask |= INTERNAL_MASK;

		tm.route(unicast->nodeId, msg, 0, cb);
	}

	getMessageAllocator()->deallocateMessage(msg);

}

/*
 *  Transmits a given message to all shards. The return messages for each
 *  shard are held until all shard’s return messages received. Then the
 *  callback is triggers with an array of message results from each shard.
 */
template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_wait_for_all_w_cb(RequestType & requestObj,
			ResultAggregatorAndPrint<RequestType , ResponseType> *aggregator, 
      CoreShardInfo & coreInfo) {

	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messageKind(),
												false,
												broadcastResolver.size()-1); //hack for missing local

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;
    msg->mask |= INTERNAL_MASK;

		tm.route(unicast->nodeId, msg, 0, cb);
	}

	getMessageAllocator()->deallocateMessage(msg);
}


/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_w_cb_n_timeout(RequestType& requestObj,
      ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
			timeval timeoutValue , CoreShardInfo & coreInfo ){

	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messageKind(),
												true,
												broadcastResolver.size()-1); //hack for missing local

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;
    msg->mask |= INTERNAL_MASK;

		tm.route(unicast->nodeId, msg, timeoutValue.tv_sec, cb);
	}

	getMessageAllocator()->deallocateMessage(msg);

}


template<typename RequestType , typename ResponseType> inline void
RoutingManager::broadcast_wait_for_all_w_cb_n_timeout(RequestType& requestObj,
			ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator, 
      timeval timeoutValue, CoreShardInfo & coreInfo){

	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messsageKind(),
												true,
												broadcastResolver.size()-1); //hack for missing local

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


	for(UnicastIterator unicast = broadcastResolver.begin();
			unicast != broadcastResolver.end(); ++unicast) {
		msg->shard = unicast->shardId;
    msg->mask |= INTERNAL_MASK;

		tm.route(unicast->nodeId, msg, timeoutValue.tv_sec, cb);
	}

	getMessageAllocator()->deallocateMessage(msg);

}



/*
*  Transmits a given message to a particular shard in a non-blocking fashion
*/
template<typename RequestType> inline void 
RoutingManager::route(RequestType& requestObj, ShardId & shardInfo) {
  unsigned nodeId = 
    configurationManager.getCluster()->shardMap[shardInfo].getNodeId();

	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


  msg->shard = shardInfo;
  msg->mask |= INTERNAL_MASK;

  tm.route(nodeId, msg);

	getMessageAllocator()->deallocateMessage(msg);
}

/*
 *  Transmits a given message to a pariticular shards, and waits for
 *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED
 *  message.
 */
template<typename RequestType> inline bool
RoutingManager::route_wait_for_confirmation(RequestType& requestObj,
    bool& timedout, timeval timeoutValue , ShardId shardInfo){
  return false;
}

/*
 *  Transmits a given message to a particular shards. Upon receipt of a
 *  response shard, the appropriate callback is trigger with the
 *  corresponding Message.
 */
template<typename RequestType , typename ResponseType> inline void 
RoutingManager::route_w_cb(RequestType& requestObj,
    ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator, 
    ShardId shardInfo) {
  unsigned nodeId = 
    configurationManager.getCluster()->shardMap[shardInfo].getNodeId();

	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messageKind());


	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


  msg->shard = shardInfo;
  msg->mask |= INTERNAL_MASK;

  tm.route(nodeId, msg, 0, cb);

	getMessageAllocator()->deallocateMessage(msg);

}

/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline void 
RoutingManager::route_w_cb_n_timeout(RequestType & requestObj,
    ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
			timeval timeoutValue, ShardId shardInfo) {

  unsigned nodeId = 
    configurationManager.getCluster()->shardMap[shardInfo].getNodeId();

	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 */
	CallbackReference cb = tm.registerCallback(&requestObj,
												new RMCallback<RequestType, ResponseType>(*aggregator),
												RequestType::messageKind());


	// create the message from the request object
	Message* msg = (Message*)
    		((char*) requestObj.serialize(getMessageAllocator()) - sizeof(Message));


  msg->shard = shardInfo;
  msg->mask |= INTERNAL_MASK;

  tm.route(nodeId, msg, timeoutValue.tv_sec, cb);

	getMessageAllocator()->deallocateMessage(msg);


}


}
}

#endif /* __BROADCAST_INLINES_H__ */
