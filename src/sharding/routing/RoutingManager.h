#ifndef __SHARDING_ROUTING_ROUTING_MANAGER_H__
#define __SHARDING_ROUTING_ROUTING_MANAGER_H__

#include<sys/time.h>
#include <map>

#include <sharding/configuration/ConfigManager.h>
#include <sharding/transport/TransportManager.h>
#include <sharding/processor/DistributedProcessorInternal.h>
#include "sharding/routing/PendingMessages.h"
#include <server/Srch2Server.h>
#include <sharding/processor/ResultsAggregatorAndPrint.h>
#include "Multiplexer.h"
#include "transport/MessageAllocator.h"
#include "InternalMessageBroker.h"
#include "sharding/configuration/ShardingConstants.h"

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class RoutingManager {

public:

    RoutingManager(ConfigManager&  configurationManager, TransportManager& tm);

    ~RoutingManager(){
        delete pendingRequestsHandler;
    };

    MessageAllocator * getMessageAllocator() ;
    ConfigManager* getConfigurationManager();
    DPInternalRequestHandler* getDpInternal();
    InternalMessageBroker * getInternalMessageBroker();
    PendingRequestsHandler * getPendingRequestsHandler();
    TransportManager& getTransportManager();
    Srch2Server * getShardIndex(ShardId shardId);

    template<typename RequestType, typename ResponseType>
    RoutingManagerAPIReturnType broadcast(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            vector<ShardId> & destination){

        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;
        if(withCallback){
            // register a pending request in the handler
            pendingRequest =
                    this->pendingRequestsHandler->registerPendingRequest(waitForAll, aggregator, destination.size());
        }

        Message * internalMessage = NULL;
        Message * externalMessage = NULL;

        // iterate on all destinations and send the message
        for(vector<ShardId>::iterator shardIdItr = destination.begin(); shardIdItr != destination.end(); ++shardIdItr) {

        	NodeId nodeId = shardIdItr->getNodeId(configurationManager);
            Logger::debug("sending request to node - %d", nodeId);

            // this shard is in the current node
            if(nodeId == configurationManager.getCurrentNodeId()){
                // so that we create the message only once
                if(internalMessage == NULL){
                    internalMessage = prepareInternalMessage<RequestType>(*shardIdItr, requestObj);
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                internalMessage->setDestinationShardId(*shardIdItr);
                // callback should wait for one more reply
                sendInternalMessage(internalMessage, requestObj, *shardIdItr, timeoutValue, pendingRequest);
            }else{// this shard is in some other node
                // so that we create the message only once
                if(externalMessage == NULL){
                    externalMessage = prepareExternalMessage<RequestType>(*shardIdItr, requestObj);
                    // request message is stored in cb object to be deleted when replies are ready
                    // and cb object is being destroyed.
                }
                externalMessage->setDestinationShardId(*shardIdItr);
                // callback should wait for one more reply
                sendExternalMessage(externalMessage, requestObj, *shardIdItr, timeoutValue, pendingRequest);
            }
        }

        return RoutingManagerAPIReturnTypeSuccess;

    }


    template<typename RequestType , typename ResponseType>
    RoutingManagerAPIReturnType route(RequestType * requestObj,
    		bool withCallback,
            boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            ShardId shardInfo){
        // register a pending request in the handler
        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;

        if(withCallback){
            pendingRequest = this->pendingRequestsHandler->registerPendingRequest(true, aggregator, 1);
        }

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

        MessageAllocator * getMessageAllocator() ;
        ConfigManager* getConfigurationManager();
        DPInternalRequestHandler* getDpInternal();
        InternalMessageBroker * getInternalMessageBroker();
        PendingRequestsHandler * getPendingRequestsHandler();
        TransportManager& getTransportManager();
        Srch2Server * getShardIndex(ShardId shardId);

        return RoutingManagerAPIReturnTypeSuccess;

    }

    /*
     * This function sends the request object to a shard which resides
     * in the same node.
     */
    template<typename RequestType >
    Message * prepareInternalMessage(ShardId shardId,
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


    template<typename RequestType >
    Message * prepareExternalMessage(ShardId shardId,
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

private:

    /*
     * The call back function used by thread to handle local requests.
     */
    static void * routeInternalMessage(void * arg) ;

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendInternalMessage(Message * msg, RequestType * requestObjPointer,
            ShardId shardId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL){

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

    }

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendExternalMessage(Message * msg, RequestType * requestObjPointer,
            ShardId shardId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL){

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
        transportManager.sendMessage(nodeId, msg, timeoutValue);

        if(msg->isNoReply()){
            // deallocate the message here because there is no reply and no pending request
            transportManager.getMessageAllocator()->deallocateByMessagePointer(msg);
        }

    }

    //std::map<ShardId, Srch2Server*> shardToIndex;
    ConfigManager& configurationManager;
    TransportManager& transportManager;
    DPInternalRequestHandler dpInternal;
    InternalMessageBroker internalMessageBroker;
    // a map from coreId to Srch2Server //TODO : should it be a map from ShardId to shardServer?
    std::map<unsigned, Srch2Server *> shardServers;

    /*
     * the data structure which stores all pending messages in this node
     * this object keeps all messages which are waiting for response
     * and resolves response messages.
     */
    PendingRequestsHandler * pendingRequestsHandler;
};

}
}

#endif //__SHARDING_ROUTING_ROUTING_MANAGER_H__
