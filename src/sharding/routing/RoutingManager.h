// author : jamshid, Jun 10, 2014
#ifndef __SHARDING_ROUTING_ROUTING_MANAGER_H__
#define __SHARDING_ROUTING_ROUTING_MANAGER_H__

#include<sys/time.h>
#include <map>

#include <sharding/configuration/ConfigManager.h>
#include <sharding/transport/TransportManager.h>
#include <sharding/processor/DistributedProcessorInternal.h>
#include "sharding/routing/ReplyMessageHandler.h"
#include <server/Srch2Server.h>
#include <sharding/routing/ResponseAggregator.h>
#include "transport/MessageAllocator.h"
#include "InternalMessageHandler.h"
#include "sharding/configuration/ShardingConstants.h"

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class RoutingManager {

public:

    RoutingManager(ConfigManager&  configurationManager, DPInternalRequestHandler& dpInternal , TransportManager& tm);

    ~RoutingManager(){};

    MessageAllocator * getMessageAllocator() ;
    ConfigManager* getConfigurationManager();
    DPInternalRequestHandler* getDpInternal();
    InternalMessageHandler * getInternalMessageHandler();
    ReplyMessageHandler * getReplyMessageHandler();
    TransportManager& getTransportManager();



    /*
     * this function is provided for ShardManager to broadcast messages to other nodes'
     * ShardManagers.
     */
    template<typename RequestType, typename ResponseType>
    RoutingManagerAPIReturnType broadcastToNodes(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            vector<NodeId> & destination, NodeId currentNodeId){

        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;
        if(withCallback){
            // register a pending request in the handler
            pendingRequest =
                    this->replyMessageHandler.registerPendingRequest(waitForAll, aggregator, destination.size());
        }


        Message * internalMessage = NULL;
        Message * externalMessage = NULL;

        // iterate over destinations and send the message
        for(vector<NodeId>::iterator nodeIdItr = destination.begin() ; nodeIdItr != destination.end(); ++nodeIdItr){

        	NodeId nodeId = *nodeIdItr;
            Logger::debug("sending request to node - %d", nodeId);

            // this shard is in the current node
            if(currentNodeId == nodeId){
                // so that we create the message only once
                if(internalMessage == NULL){
                    internalMessage = prepareInternalMessage<RequestType>(NULL, requestObj, false); // prepare message for ShardManager
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                internalMessage->setDestinationShardId(ShardId()); // shard Id doesn't matter in SHM messages
                // callback should wait for one more reply
                sendInternalMessage(internalMessage, requestObj, nodeId, timeoutValue, pendingRequest);
            }else{// this shard is in some other node
                // so that we create the message only once
                if(externalMessage == NULL){
                    externalMessage = prepareExternalMessage<RequestType>(NULL, requestObj, false); // prepare message for ShardManager
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                externalMessage->setDestinationShardId(ShardId());
                // callback should wait for one more reply
                sendExternalMessage(externalMessage, requestObj, nodeId, timeoutValue, pendingRequest);
            }
        }

        return RoutingManagerAPIReturnTypeSuccess;
    }


    template<typename RequestType, typename ResponseType>
    RoutingManagerAPIReturnType broadcast(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            vector<const Shard *> & destination){

        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;
        if(withCallback){
            // register a pending request in the handler
            pendingRequest =
                    this->replyMessageHandler.registerPendingRequest(waitForAll, aggregator, destination.size());
        }

        Message * internalMessage = NULL;
        Message * externalMessage = NULL;

        // iterate on all destinations and send the message
        for(vector<const Shard *>::iterator shardItr = destination.begin(); shardItr != destination.end(); ++shardItr) {

        	NodeId nodeId = (*shardItr)->getNodeId();
            Logger::debug("sending request to node - %d", nodeId);

            // this shard is in the current node
            if(aggregator->getClusterReadview()->getCurrentNode()->getId() == nodeId){
                // so that we create the message only once
                if(internalMessage == NULL){
                    internalMessage = prepareInternalMessage<RequestType>(*shardItr, requestObj);
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                internalMessage->setDestinationShardId((*shardItr)->getShardId());
                // callback should wait for one more reply
                sendInternalMessage(internalMessage, requestObj, (*shardItr)->getNodeId(), timeoutValue, pendingRequest);
            }else{// this shard is in some other node
                // so that we create the message only once
                if(externalMessage == NULL){
                    externalMessage = prepareExternalMessage<RequestType>(*shardItr, requestObj);
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                externalMessage->setDestinationShardId((*shardItr)->getShardId());
                // callback should wait for one more reply
                sendExternalMessage(externalMessage, requestObj, (*shardItr)->getNodeId(), timeoutValue, pendingRequest);
            }
        }

        return RoutingManagerAPIReturnTypeSuccess;

    }


    template<typename RequestType , typename ResponseType>
    RoutingManagerAPIReturnType sendMessage(RequestType * requestObj,
    		bool withCallback,
            boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            const Shard * shardInfo){
        // register a pending request in the handler
        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;

        if(withCallback){
            pendingRequest = this->replyMessageHandler.registerPendingRequest(true, aggregator, 1);
        }

        // if the destination is the current node, we don't serialize the request object
        // instead, we serialize the pointer to the request object
        Message * msg;
        if(aggregator->getClusterReadview()->getCurrentNode()->getId() == shardInfo->getNodeId()) {
            msg = prepareInternalMessage<RequestType>(shardInfo, requestObj);
            sendInternalMessage(msg, requestObj, shardInfo->getNodeId(),timeoutValue,pendingRequest);

        }else{
            msg = prepareExternalMessage<RequestType>(shardInfo, requestObj);
            sendExternalMessage(msg, requestObj, shardInfo->getNodeId(),timeoutValue,pendingRequest);
        }

        return RoutingManagerAPIReturnTypeSuccess;

    }

    /*
     * This function sends the request object to a shard which resides
     * in the same node.
     */
    template<typename RequestType >
    Message * prepareInternalMessage(const Shard * shard,
            RequestType * requestObjPointer, bool dpOrShm = true){
        // prepare a message that just includes a memory address (pointer)
        // allocate the message
        Message * msg = getMessageAllocator()->allocateMessage(sizeof(RequestType *));
        // initialize the message
        // copy the address saved in requestObjPointer in msg->body
        msg->setBodyAndBodySize(&requestObjPointer, sizeof(RequestType *));
        if(shard == NULL){
			msg->setDestinationShardId(ShardId());
        }else{
			msg->setDestinationShardId(shard->getShardId());
        }
        if(dpOrShm){
        	msg->setDPInternal();
        }else{
        	msg->setSHMInternal();
        }
        msg->setLocal();
        msg->setType(RequestType::messageType());
        msg->setMessageId(transportManager.getUniqueMessageIdValue());
        return msg;
    }


    template<typename RequestType >
    Message * prepareExternalMessage(const Shard * shard,
            RequestType * requestObjPointer, bool dpOrShm = true){
        // create the message from the request object
        // 1. serialize the message and prepare the body
        void * serializeRequestMessageBodyPointer = requestObjPointer->serialize(getMessageAllocator());
        // 2. get the pointer to the Message
        Message * msg = Message::getMessagePointerFromBodyPointer(serializeRequestMessageBodyPointer);


        // initialize the message
        if(shard == NULL){
			msg->setDestinationShardId(ShardId());
        }else{
			msg->setDestinationShardId(shard->getShardId());
        }
        if(dpOrShm == true){ // DP Internal message
			msg->setDPInternal();
        }else{
        	msg->setSHMInternal();
        }
        msg->setType(RequestType::messageType());
        msg->setMessageId(transportManager.getUniqueMessageIdValue());
        return msg;
    }

private:

    /*
     * The call back function used by thread to handle local requests.
     */
    static void * routeInternalMessage(void * arg) ;

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendInternalMessage(Message * msg, RequestType * requestObject,
            NodeId nodeId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL){

    	// only messages which expect reply will go to pending messages
    	if(pendingRequest != NULL){
    		// register a pending message in this pending request
    		PendingMessage<RequestType, ResponseType> * pendingMessage =
    				pendingRequest->registerPendingMessage(nodeId, timeoutValue, msg, requestObject);
    	}else{
    		// no reply case, request object and message must be deleted after sending to network.
    		delete requestObject;
    	}

        Logger::debug("Message is local");
        pthread_t internalMessageRouteThread;

        std::pair<RoutingManager * ,  std::pair< Message * , NodeId> > * routeInternalMessageArgs =
                new std::pair<RoutingManager * , std::pair<Message *, NodeId> >(this,
                        std::make_pair(msg, nodeId));

        // run for the same shard in a separate thread
        // the reason is that even the request message for some external threads are not sent
        // so if we use the same thread those external shards won't start the job until this shard
        // is finished which makes it sequential and wrong.
        if (pthread_create(&internalMessageRouteThread, NULL, routeInternalMessage, routeInternalMessageArgs) != 0){
            //        Logger::console("Cannot create thread for handling local message");
            perror("Cannot create thread for handling local message");
            return;
        }
        // since we don't join this thread it must be detached so that its resource gets deallocated.
        pthread_detach(internalMessageRouteThread);

    }

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendExternalMessage(Message * msg, RequestType * requestObject,
            NodeId nodeId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL){

        // only messages which expect reply will go to pending messages
        if(pendingRequest != NULL){
            // register a pending message in this pending request
            PendingMessage<RequestType, ResponseType> * pendingMessage =
                    pendingRequest->registerPendingMessage(nodeId, timeoutValue, msg, requestObject);
        }else{
            // no reply case, request object and message must be deleted after sending to network.
            delete requestObject;
        }
        // pass the ready message to TM to be sent to nodeId
        transportManager.sendMessage(nodeId, msg, timeoutValue);
    }


    ConfigManager& configurationManager;
    TransportManager& transportManager;
    DPInternalRequestHandler& dpInternal;
    InternalMessageHandler internalMessageHandler;
    // a map from coreId to Srch2Server //TODO : V1 : should it be a map from ShardId to shardServer?
    std::map<unsigned, Srch2Server *> shardServers;

    /*
     * the data structure which stores all pending messages in this node
     * this object keeps all messages which are waiting for response
     * and resolves response messages.
     */
    ReplyMessageHandler replyMessageHandler;
};

}
}

#endif //__SHARDING_ROUTING_ROUTING_MANAGER_H__
