#ifndef __BROADCAST_INLINES_H__
#define __BROADCAST_INLINES_H__

#include "transport/PendingMessages.h"
#include "sharding/routing/RoutingManager.h"
#include "RMCallback.h"


namespace srch2 {
namespace httpwrapper {

template<typename RequestType > inline
Message * RoutingManager::prepareInternalMessage(ShardId shardId,
		RequestType *requestObjPointer){
	// prepare a message which is just as big as a pointer
	// allocate the message
	Message *msg = getMessageAllocator()->allocateMessage(sizeof(RequestType *));
	// initialize the message
	// copy the address saved in requestObjPointer in msg->body
	msg->setBodyAndBodySize(&requestObjPointer, sizeof(RequestType *));
	msg->setDestinationShardId(shardId);
	msg->setInternal()->setLocal();
	msg->setType(RequestType::messageKind());

	return msg;
}

template<typename RequestType > inline
Message * RoutingManager::prepareExternalMessage(ShardId shardId,
		RequestType *requestObjPointer){
	// create the message from the request object
	// 1. serialize the message and prepate the body
	void * serializeRequestMessageBodyPointer = requestObjPointer->serialize(getMessageAllocator());
	// 2. get the pointer to the Message
	Message * msg = Message::getMessagePointerFromBodyPointer(serializeRequestMessageBodyPointer);


	// initialize the message
	msg->setDestinationShardId(shardId);
	msg->setInternal();
	msg->setType(RequestType::messageKind());
	return msg;
}

inline void RoutingManager::sendInternalMessage(Message * msg,
		ShardId shardId, timeval timeoutValue, CallbackReference cb) {

	unsigned nodeId = shardId.getNodeId(configurationManager);
	transportManager.route(nodeId, msg, timeoutValue.tv_sec, cb);
}

inline void RoutingManager::sendExternalMessage(Message * msg,
		ShardId shardId, timeval timeoutValue, CallbackReference cb){

	unsigned nodeId = shardId.getNodeId(configurationManager);
	transportManager.route(nodeId, msg, timeoutValue.tv_sec, cb);

}


/*
 *  Transmits a given message to all shards. The broadcast will not wait for
 *  confirmation from each receiving shard.
 */
template<typename RequestType> inline void
RoutingManager::broadcast(RequestType * requestObj, CoreShardInfo &coreInfo) {
	/*
	 * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
	 */
	Multiplexer broadcastResolver(configurationManager, coreInfo);


	Message * internalMessage = NULL;
	Message * externalMessage = NULL;

	timeval timeValue;
	timeValue.tv_sec = timeValue.tv_usec = 0;
	// iterate on all destinations and send the message
	for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
		ShardId shardIdFromIteration = broadcastResolver.getNextShardId();
		// this shard is in the current node
		if(shardIdFromIteration.isInCurrentNode(configurationManager)){
			// so that we create the message only once
			if(internalMessage == NULL){
				internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// broadcast with no callback has no response
				// this flag will be used in other places to
				// understand whether we should deallocate this message or not
				internalMessage->setNoReply();
			}
			internalMessage->setDestinationShardId(shardIdFromIteration);
			sendInternalMessage(internalMessage, shardIdFromIteration, timeValue, CallbackReference());
		}else{// this shard is in some other node
			// so that we create the message only once
			if(externalMessage == NULL){
				externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// broadcast with no callback has no response
				// this flag will be used in other places to
				// understand whether we should deallocate this message or not
				externalMessage->setNoReply();
			}
			externalMessage->setDestinationShardId(shardIdFromIteration);
			sendExternalMessage(externalMessage, shardIdFromIteration, timeValue, CallbackReference());
		}
	}
	// if internal message is created so it means a shard on the same node is
	// going to use requestObj, so we don't delete it.
	// internal message is NULL so requestObj is going to be used only through a message
	// and we can delete it here.
	if(internalMessage == NULL){
		delete requestObj;
	}
}


/*
 *  Transmits a given message to all shards. The broadcast block until
 *  confirmation from each shard is received. Returns false iff any
 *  receiving shard confirms with MESSAGE_FAILED message.
 */
template<typename RequestType> inline bool 
RoutingManager::broadcast_wait_for_all_confirmation(RequestType * requestObject,
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
void RoutingManager::broadcast_w_cb(RequestType * requestObj,
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
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			ResponseType::messageKind(),
			false,
			broadcastResolver.size());

	// Callback object is ready from the beginning for responses because this is not a wait for all case
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();

	Message * internalMessage = NULL;
	Message * externalMessage = NULL;

	timeval timeValue;
	timeValue.tv_sec = timeValue.tv_usec = 0;
	// iterate on all destinations and send the message
	for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
		ShardId shardIdFromIteration = broadcastResolver.getNextShardId();
		// this shard is in the current node
		if(shardIdFromIteration.isInCurrentNode(configurationManager)){
			// so that we create the message only once
			if(internalMessage == NULL){
				internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(internalMessage);
			}
			internalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendInternalMessage(internalMessage, shardIdFromIteration, timeValue, cb);
		}else{// this shard is in some other node
			// so that we create the message only once
			if(externalMessage == NULL){
				externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(externalMessage);
			}
			externalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendExternalMessage(externalMessage, shardIdFromIteration, timeValue, cb);
		}
	}


}

/*
 *  Transmits a given message to all shards. The return messages for each
 *  shard are held until all shard’s return messages received. Then the
 *  callback is triggers with an array of message results from each shard.
 */
template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_wait_for_all_w_cb(RequestType * requestObj,
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
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			ResponseType::messageKind(),
			true,
			broadcastResolver.size());

	Message * internalMessage = NULL;
	Message * externalMessage = NULL;

	timeval timeValue;
	timeValue.tv_sec = timeValue.tv_usec = 0;
	// iterate on all destinations and send the message
	for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
		ShardId shardIdFromIteration = broadcastResolver.getNextShardId();
		// this shard is in the current node
		if(shardIdFromIteration.isInCurrentNode(configurationManager)){
			// so that we create the message only once
			if(internalMessage == NULL){
				internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(internalMessage);
			}
			internalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendInternalMessage(internalMessage, shardIdFromIteration, timeValue, cb);
		}else{// this shard is in some other node
			// so that we create the message only once
			if(externalMessage == NULL){
				externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(externalMessage);
			}
			externalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendExternalMessage(externalMessage, shardIdFromIteration, timeValue, cb);
		}
	}

	// callback object is not ready for responses unless all requests are sent.
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();
}


/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline
void RoutingManager::broadcast_w_cb_n_timeout(RequestType * requestObj,
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
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			ResponseType::messageKind(),
			false,
			broadcastResolver.size());

	// Callback object is ready from the beginning for responses because this is not a wait for all case
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();

	Message * internalMessage = NULL;
	Message * externalMessage = NULL;

	// iterate on all destinations and send the message
	for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
		ShardId shardIdFromIteration = broadcastResolver.getNextShardId();
		// this shard is in the current node
		if(shardIdFromIteration.isInCurrentNode(configurationManager)){
			// so that we create the message only once
			if(internalMessage == NULL){
				internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(internalMessage);
			}
			internalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendInternalMessage(internalMessage, shardIdFromIteration, timeoutValue, cb);
		}else{// this shard is in some other node
			// so that we create the message only once
			if(externalMessage == NULL){
				externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(externalMessage);
			}
			externalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendExternalMessage(externalMessage, shardIdFromIteration, timeoutValue, cb);
		}
	}

}


template<typename RequestType , typename ResponseType> inline void
RoutingManager::broadcast_wait_for_all_w_cb_n_timeout(RequestType * requestObj,
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
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			ResponseType::messageKind(),
			true,
			0);

	Message * internalMessage = NULL;
	Message * externalMessage = NULL;

	// iterate on all destinations and send the message
	for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
		ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

		unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
		if (!configurationManager.isValidNode(nodeId))
			continue;
		Logger::debug("sending request to node - %d", nodeId);

		// this shard is in the current node
		if(nodeId == configurationManager.getCurrentNodeId()){
			// so that we create the message only once
			if(internalMessage == NULL){
				internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(internalMessage);
			}
			internalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendInternalMessage(internalMessage, shardIdFromIteration, timeoutValue, cb);
		}else{// this shard is in some other node
			// so that we create the message only once
			if(externalMessage == NULL){
				externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
				// request message is stored in cb object to be deleted when replies are ready
				// and cb object is being destroyed.
				cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(externalMessage);
			}
			externalMessage->setDestinationShardId(shardIdFromIteration);
			// callback should wait for one more reply
			cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
			sendExternalMessage(externalMessage, shardIdFromIteration, timeoutValue, cb);
		}
	}

	// callback object is not ready for responses unless all requests are sent.
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();
}



/*
 *  Transmits a given message to a particular shard in a non-blocking fashion
 */
template<typename RequestType> inline void 
RoutingManager::route(RequestType * requestObj, ShardId & shardInfo) {

	// if the destination is the current node, we don't serialize the request object
	// instead, we serialize the pointer to the request object
	Message * msg =  NULL;
	timeval timeValue;
	timeValue.tv_sec = timeValue.tv_usec = 0;
	if(shardInfo.isInCurrentNode(configurationManager)) {
		msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
		msg->setNoReply();
		sendInternalMessage(msg, shardInfo,timeValue,CallbackReference());
		// local NO_REPLY messages and request objects get deleted in InternalMessageBroker
	}else{
		msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
		msg->setNoReply();
		sendExternalMessage(msg,shardInfo,timeValue,CallbackReference());
		// request object of NO_REPLAY non-local messages get deleted here because
		// we don't need it anymore
		delete requestObj;
		// message will be deleted in transportManager.route
	}
}

/*
 *  Transmits a given message to a pariticular shards, and waits for
 *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED
 *  message.
 */
template<typename RequestType> inline bool
RoutingManager::route_wait_for_confirmation(RequestType * requestObj,
		bool& timedout, timeval timeoutValue , ShardId shardInfo){
	//TODO
	return false;
}

/*
 *  Transmits a given message to a particular shards. Upon receipt of a
 *  response shard, the appropriate callback is trigger with the
 *  corresponding Message.
 */
template<typename RequestType , typename ResponseType> inline void 
RoutingManager::route_w_cb(RequestType * requestObj,
		ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
		ShardId shardInfo) {

	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 * we only pass 3 arguments to registerCallback because we are not going to wait for all
	 * (false although it doesn't matter) and the number of shards to wait for is 1 by default
	 */
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			RequestType::messageKind());

	// callback object is always ready because there is only one request
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();

	// if the destination is the current node, we don't serialize the request object
	// instead, we serialize the pointer to the request object
	Message * msg =  NULL;
	timeval timeValue;
	timeValue.tv_sec = timeValue.tv_usec = 0;
	if(shardInfo.isInCurrentNode(configurationManager)) {
		msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
		// request message is stored in cb object to be deleted when replies are ready
		// and cb object is being destroyed.
		cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(msg);
		// callback should wait for one more reply
		cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
		sendInternalMessage(msg, shardInfo,timeValue,cb);

	}else{
		msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
		// request message is stored in cb object to be deleted when replies are ready
		// and cb object is being destroyed.
		cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(msg);
		// callback should wait for one more reply
		cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
		sendExternalMessage(msg,shardInfo,timeValue,cb);
	}

}

/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline void 
RoutingManager::route_w_cb_n_timeout(RequestType * requestObj,
		ResultAggregatorAndPrint<RequestType , ResponseType> * aggregator,
		timeval timeoutValue, ShardId shardInfo) {

	/*
	 * We need to register callback functions to TM so that it calls them upon receiving a message
	 * Here, we register a method around aggregator callback into TM.
	 * we only pass 3 arguments to registerCallback because we are not going to wait for all
	 * (false although it doesn't matter) and the number of shards to wait for is 1 by default
	 */
	CallbackReference cb = transportManager.prepareCallback(requestObj,
			new RMCallback<RequestType, ResponseType>(*aggregator),
			RequestType::messageKind());

	// callback object is always ready because there is only one request
	cb.getRegisteredCallbackPtr()->setReadyForCallBack();

	// if the destination is the current node, we don't serialize the request object
	// instead, we serialize the pointer to the request object
	Message * msg =  NULL;
	if(shardInfo.isInCurrentNode(configurationManager)) {
		msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
		// request message is stored in cb object to be deleted when replies are ready
		// and cb object is being destroyed.
		cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(msg);
		// callback should wait for one more reply
		cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
		sendInternalMessage(msg, shardInfo,timeoutValue,cb);

	}else{
		msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
		// request message is stored in cb object to be deleted when replies are ready
		// and cb object is being destroyed.
		cb.getRegisteredCallbackPtr()->getRequestMessages().push_back(msg);
		// callback should wait for one more reply
		cb.getRegisteredCallbackPtr()->incrementNumberOfRepliesToWaitFor();
		sendExternalMessage(msg,shardInfo,timeoutValue,cb);
	}

}


}
}

#endif /* __BROADCAST_INLINES_H__ */
