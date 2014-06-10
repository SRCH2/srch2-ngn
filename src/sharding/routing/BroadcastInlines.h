#ifndef __BROADCAST_INLINES_H__
#define __BROADCAST_INLINES_H__

/*
 * Implementation of RoutingManager.h functions.
 */


namespace srch2 {
namespace httpwrapper {


/*
 * The call back function used by thread to handle local requests.
 */
void * routeInternalMessage(void * arg) ;

template<typename RequestType > inline
Message * RoutingManager::prepareInternalMessage(ShardId shardId,
        RequestType * requestObjPointer){
    // prepare a message which is just as big as a pointer
    // allocate the message
    Message * msg = getMessageAllocator()->allocateMessage(sizeof(RequestType *));
    // initialize the message
    // copy the address saved in requestObjPointer in msg->body
    msg->setBodyAndBodySize(&requestObjPointer, sizeof(RequestType *));
    msg->setDestinationShardId(shardId);
    msg->setInternal()->setLocal();
    msg->setType(RequestType::messageKind());
    msg->setMessageId(transportManager.getUniqueMessageIdValue());
    return msg;
}

template<typename RequestType > inline
Message * RoutingManager::prepareExternalMessage(ShardId shardId,
        RequestType * requestObjPointer){
    // create the message from the request object
    // 1. serialize the message and prepate the body
    void * serializeRequestMessageBodyPointer = requestObjPointer->serialize(getMessageAllocator());
    // 2. get the pointer to the Message
    Message * msg = Message::getMessagePointerFromBodyPointer(serializeRequestMessageBodyPointer);


    // initialize the message
    msg->setDestinationShardId(shardId);
    msg->setInternal();
    msg->setType(RequestType::messageKind());
    msg->setMessageId(transportManager.getUniqueMessageIdValue());
    return msg;
}

template<typename RequestType , typename ResponseType> inline
void RoutingManager::sendInternalMessage(Message * msg, RequestType * requestObjPointer,
        ShardId shardId, time_t timeoutValue,  PendingRequest<RequestType, ResponseType> * pendingRequest) {

    // find the node id of destination
    unsigned nodeId = shardId.getNodeId(configurationManager);

    // only messages which expect reply will go to pending messages
    if(pendingRequest != NULL && ! msg->isNoReply()){
        // register a pending message in this pending request
        pendingRequest->registerPendingMessage(nodeId, timeoutValue, msg, requestObjPointer);
    }else{
        // this is the case of message with no reply so we don't have a pending request.
        // we must send the request and delete the request object and message right here
        // msg is in a shared pointer and it's not stored in a pendingMessage,
        // therefor when it's handled it will delete by itself.
        delete requestObjPointer;
        // message will be deleted in routeInternalMessage
    }

    Logger::console("Message is local");
    pthread_t internalMessageRouteThread;

    std::pair<RoutingManager * ,  std::pair< Message * , NodeId> > * routeInternalMessageArgs =
            new std::pair<RoutingManager * , std::pair<Message *, NodeId> >(this,
                    std::make_pair(msg, nodeId));

    // run for the same shard in a separate thread
    if (pthread_create(&internalMessageRouteThread, NULL, routeInternalMessage, routeInternalMessageArgs) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return;
    }
    pthread_detach(internalMessageRouteThread);


    //    // run for local shard in the same thread
    //    routeInternalMessage(routeInternalMessageArgs);

}

template<typename RequestType , typename ResponseType> inline
void RoutingManager::sendExternalMessage(Message * msg, RequestType * requestObjPointer,
        ShardId shardId, time_t timeoutValue,  PendingRequest<RequestType, ResponseType> * pendingRequest){

    // find the node id of destination
    unsigned nodeId = shardId.getNodeId(configurationManager);

    // only messages which expect reply will go to pending messages
    if(pendingRequest != NULL && ! msg->isNoReply()){
        // register a pending message in this pending request
        PendingMessage<RequestType, ResponseType> * pendingMessage =
                pendingRequest->registerPendingMessage(nodeId, timeoutValue, msg, requestObjPointer);
    }else{
        // no reply case, request object and message must be deleted after sending to network.
        delete requestObjPointer;
    }
    // pass the ready message to TM to be sent to nodeId
    transportManager.route(nodeId, msg, timeoutValue);

    if(msg->isNoReply()){
        // deallocate the message here because there is no reply and no pending request
        transportManager.getMessageAllocator()->deallocateByMessagePointer(msg);
    }

}


/*
 *  Transmits a given message to all shards. The broadcast will not wait for
 *  confirmation from each receiving shard.
 */
template<typename RequestType, typename ResponseType> inline
RoutingManagerAPIReturnType RoutingManager::broadcast(RequestType * requestObj, CoreShardInfo &coreInfo) {
    /*
     * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
     */
    Multiplexer broadcastResolver(configurationManager, coreInfo);



    Message * internalMessage = NULL;
    Message * externalMessage = NULL;

    time_t timeValue; // there is no callback so timeout doesn't matter

    // iterate on all destinations and send the message
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (!configurationManager.isValidNode(nodeId)){
            continue;
        }
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
            sendInternalMessage(internalMessage, requestObj, shardIdFromIteration, timeValue);
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
            sendExternalMessage(externalMessage, requestObj, shardIdFromIteration, timeValue);
        }
    }
    // if internal message is created so it means a shard on the same node is
    // going to use requestObj, so we don't delete it.
    // internal message is NULL so requestObj is going to be used only through a message
    // and we can delete it here.
    if(internalMessage == NULL){
        delete requestObj;
    }

    return RoutingManagerAPIReturnTypeSuccess;
}


/*
 *  Transmits a given message to all shards. The broadcast block until
 *  confirmation from each shard is received. Returns false iff any
 *  receiving shard confirms with MESSAGE_FAILED message.
 */
template<typename RequestType, typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::broadcast_wait_for_all_confirmation(RequestType * requestObject,
        bool& timedout, time_t timeoutValue , CoreShardInfo & coreInfo){
    //TODO
    return     RoutingManagerAPIReturnTypeAllNodesDown;
}
/*
 *  Transmits a given message to all shards. Upon receipt of a response from
 *  any shard, the callback is trigger with the corresponding Message.
 *  The callback will be called for each shard.
 */
template<typename RequestType , typename ResponseType> inline
RoutingManagerAPIReturnType RoutingManager::broadcast_w_cb(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
        CoreShardInfo & coreInfo){

    /*
     * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
     */
    Multiplexer broadcastResolver(configurationManager, coreInfo);


    // find out how many shards are still within reach
    unsigned totalNumberOfRepliesToExpect = 0;
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (configurationManager.isValidNode(nodeId)){
            totalNumberOfRepliesToExpect ++;
        }
    }

    if(totalNumberOfRepliesToExpect == 0){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }

    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(false, aggregator, totalNumberOfRepliesToExpect);


    Message * internalMessage = NULL;
    Message * externalMessage = NULL;

    time_t timeValue;
    // set timeout to zero which indicates there is no timeout
    timeValue = 0;


    // iterate on all destinations and send the message
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (!configurationManager.isValidNode(nodeId)){
            continue;
        }
        // this shard is in the current node
        if(shardIdFromIteration.isInCurrentNode(configurationManager)){
            // so that we create the message only once
            if(internalMessage == NULL){
                internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            internalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendInternalMessage(internalMessage, requestObj, shardIdFromIteration, timeValue, pendingRequest);
        }else{// this shard is in some other node
            // so that we create the message only once
            if(externalMessage == NULL){
                externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            externalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendExternalMessage(externalMessage, requestObj, shardIdFromIteration, timeValue, pendingRequest);
        }
    }

    return RoutingManagerAPIReturnTypeSuccess;

}

/*
 *  Transmits a given message to all shards. The return messages for each
 *  shard are held until all shard’s return messages received. Then the
 *  callback is triggers with an array of message results from each shard.
 */
template<typename RequestType , typename ResponseType> inline
RoutingManagerAPIReturnType RoutingManager::broadcast_wait_for_all_w_cb(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> >aggregator,
        CoreShardInfo & coreInfo) {
    /*
     * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
     */
    Multiplexer broadcastResolver(configurationManager, coreInfo);

    // find out how many shards are still within reach
    unsigned totalNumberOfRepliesToExpect = 0;
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (configurationManager.isValidNode(nodeId)){
            totalNumberOfRepliesToExpect ++;
        }
    }

    if(totalNumberOfRepliesToExpect == 0){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }

    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(true, aggregator, totalNumberOfRepliesToExpect);


    Message * internalMessage = NULL;
    Message * externalMessage = NULL;

    time_t timeValue;
    // set timeout to zero which indicates there is no timeout
    timeValue = 0;

    // iterate on all destinations and send the message
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (!configurationManager.isValidNode(nodeId)){
            continue;
        }
        // this shard is in the current node
        if(shardIdFromIteration.isInCurrentNode(configurationManager)){
            // so that we create the message only once
            if(internalMessage == NULL){
                internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            internalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendInternalMessage(internalMessage, requestObj, shardIdFromIteration, timeValue, pendingRequest);
        }else{// this shard is in some other node
            // so that we create the message only once
            if(externalMessage == NULL){
                externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            externalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendExternalMessage(externalMessage, requestObj, shardIdFromIteration, timeValue, pendingRequest);
        }
    }

    return RoutingManagerAPIReturnTypeSuccess;
}


/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline
RoutingManagerAPIReturnType RoutingManager::broadcast_w_cb_n_timeout(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
        time_t timeoutValue , CoreShardInfo & coreInfo ){
    /*
     * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
     */
    Multiplexer broadcastResolver(configurationManager, coreInfo);


    // find out how many shards are still within reach
    unsigned totalNumberOfRepliesToExpect = 0;
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (configurationManager.isValidNode(nodeId)){
            totalNumberOfRepliesToExpect ++;
        }
    }

    if(totalNumberOfRepliesToExpect == 0){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }



    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(false, aggregator, totalNumberOfRepliesToExpect);


    Message * internalMessage = NULL;
    Message * externalMessage = NULL;


    // iterate on all destinations and send the message
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (configurationManager.isValidNode(nodeId)){
            continue;
        }
        // this shard is in the current node
        if(shardIdFromIteration.isInCurrentNode(configurationManager)){
            // so that we create the message only once
            if(internalMessage == NULL){
                internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            internalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendInternalMessage(internalMessage, requestObj, shardIdFromIteration, timeoutValue, pendingRequest);
        }else{// this shard is in some other node
            // so that we create the message only once
            if(externalMessage == NULL){
                externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            externalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendExternalMessage(externalMessage, requestObj, shardIdFromIteration, timeoutValue, pendingRequest);
        }
    }

    return RoutingManagerAPIReturnTypeSuccess;

}


template<typename RequestType , typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::broadcast_wait_for_all_w_cb_n_timeout(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
        time_t timeoutValue, CoreShardInfo & coreInfo){

    /*
     * Multiplexer reads coreInfo object to understand which nodes we need to send this broadcast to
     */
    Multiplexer broadcastResolver(configurationManager, coreInfo);


    // find out how many shards are still within reach
    unsigned totalNumberOfRepliesToExpect = 0;
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (configurationManager.isValidNode(nodeId)){
            totalNumberOfRepliesToExpect ++;
        }
    }

    if(totalNumberOfRepliesToExpect == 0){
        delete requestObj;
        // since aggregator is in a shared pointer, it gets deleted automatically.
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }

    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(true, aggregator, totalNumberOfRepliesToExpect);

    Message * internalMessage = NULL;
    Message * externalMessage = NULL;


    // iterate on all destinations and send the message
    for(broadcastResolver.initIteration(); broadcastResolver.hasMore(); broadcastResolver.nextIteration()) {
        ShardId shardIdFromIteration = broadcastResolver.getNextShardId();

        unsigned nodeId = shardIdFromIteration.getNodeId(configurationManager);
        if (!configurationManager.isValidNode(nodeId)){
            continue;
        }
        Logger::debug("sending request to node - %d", nodeId);

        // this shard is in the current node
        if(nodeId == configurationManager.getCurrentNodeId()){
            // so that we create the message only once
            if(internalMessage == NULL){
                internalMessage = prepareInternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            internalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendInternalMessage(internalMessage, requestObj, shardIdFromIteration, timeoutValue, pendingRequest);
        }else{// this shard is in some other node
            // so that we create the message only once
            if(externalMessage == NULL){
                externalMessage = prepareExternalMessage<RequestType>(shardIdFromIteration, requestObj);
                // request message is stored in cb object to be deleted when replies are ready
                // and cb object is being destroyed.
            }
            externalMessage->setDestinationShardId(shardIdFromIteration);
            // callback should wait for one more reply
            sendExternalMessage(externalMessage, requestObj, shardIdFromIteration, timeoutValue, pendingRequest);
        }
    }

    return RoutingManagerAPIReturnTypeSuccess;
}



/*
 *  Transmits a given message to a particular shard in a non-blocking fashion
 */
template<typename RequestType, typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::route(RequestType * requestObj, ShardId & shardInfo) {


    // find out whether shard is still within reach
    unsigned nodeId = shardInfo.getNodeId(configurationManager);
    if (!configurationManager.isValidNode(nodeId)){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }

    // if the destination is the current node, we don't serialize the request object
    // instead, we serialize the pointer to the request object

    Message * msg;

    time_t timeValue;
    timeValue = 0; // 0 indicates there is no timeout

    if(shardInfo.isInCurrentNode(configurationManager)) {
        msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
        msg->setNoReply();
        sendInternalMessage(msg, requestObj, shardInfo,timeValue);
        // local NO_REPLY messages and request objects get deleted in InternalMessageBroker
    }else{
        msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
        msg->setNoReply();
        sendExternalMessage(msg, requestObj, shardInfo,timeValue);
        // request object of NO_REPLAY non-local messages get deleted here because
        // we don't need it anymore
        delete requestObj;
        // message will be deleted in transportManager.route
    }

    return RoutingManagerAPIReturnTypeSuccess;
}

/*
 *  Transmits a given message to a pariticular shards, and waits for
 *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED
 *  message.
 */
template<typename RequestType, typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::route_wait_for_confirmation(RequestType * requestObj,
        bool& timedout, time_t timeoutValue , ShardId shardInfo){
    //TODO
    return     RoutingManagerAPIReturnTypeAllNodesDown;
}

/*
 *  Transmits a given message to a particular shards. Upon receipt of a
 *  response shard, the appropriate callback is trigger with the
 *  corresponding Message.
 */
template<typename RequestType , typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::route_w_cb(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
        ShardId shardInfo) {

    // find out whether shard is still within reach
    unsigned nodeId = shardInfo.getNodeId(configurationManager);
    if (!configurationManager.isValidNode(nodeId)){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }
    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(true, aggregator, 1);

    // if the destination is the current node, we don't serialize the request object
    // instead, we serialize the pointer to the request object
    Message * msg;

    time_t timeValue;
    timeValue = 0; // zero indicates there is no timeout

    if(shardInfo.isInCurrentNode(configurationManager)) {
        msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
        sendInternalMessage(msg, requestObj, shardInfo,timeValue,pendingRequest);

    }else{
        msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
        sendExternalMessage(msg, requestObj, shardInfo,timeValue,pendingRequest);
    }

    return RoutingManagerAPIReturnTypeSuccess;

}

/*
 *  Timeout version of their corresponding function. So, after a period of
 *  set milliseconds the timeout callback function is called
 *
 *       *** Potentially could alert sync layer to timed out message
 *           from shard ***
 */
template<typename RequestType , typename ResponseType> inline RoutingManagerAPIReturnType
RoutingManager::route_w_cb_n_timeout(RequestType * requestObj,
        boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
        time_t timeoutValue, ShardId shardInfo) {

    // find out whether shard is still within reach
    unsigned nodeId = shardInfo.getNodeId(configurationManager);
    if (!configurationManager.isValidNode(nodeId)){
        delete requestObj;
        return RoutingManagerAPIReturnTypeAllNodesDown;
    }

    // register a pending request in the handler
    PendingRequest<RequestType, ResponseType> * pendingRequest =
            this->pendingRequestsHandler->registerPendingRequest(true, aggregator, 1);

    // if the destination is the current node, we don't serialize the request object
    // instead, we serialize the pointer to the request object
    Message * msg;
    if(shardInfo.isInCurrentNode(configurationManager)) {
        msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
        sendInternalMessage(msg, requestObj, shardInfo,timeoutValue,pendingRequest);

    }else{
        msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
        sendExternalMessage(msg, requestObj, shardInfo,timeoutValue,pendingRequest);
    }

    return RoutingManagerAPIReturnTypeSuccess;

}


}
}

#endif /* __BROADCAST_INLINES_H__ */
