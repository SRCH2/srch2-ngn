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
    RequestMessageHandler * getInternalMessageHandler();
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


    template<typename RequestType , typename ResponseType>
    RoutingManagerAPIReturnType sendMessageToNode(RequestType * requestObj,
    		bool withCallback,
            boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            NodeId nodeId, NodeId currentNodeId){
        // register a pending request in the handler
        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;

        if(withCallback){
            pendingRequest = this->replyMessageHandler.registerPendingRequest(true, aggregator, 1);
        }

        // if the destination is the current node, we don't serialize the request object
        // instead, we serialize the pointer to the request object
        Message * msg;
        if(currentNodeId == nodeId) {
            msg = prepareInternalMessage<RequestType>(NULL, requestObj, false);
            sendInternalMessage(msg, requestObj, nodeId,timeoutValue,pendingRequest);

        }else{
            msg = prepareExternalMessage<RequestType>(NULL, requestObj, false);
            sendExternalMessage(msg, requestObj, nodeId,timeoutValue,pendingRequest);
        }

        return RoutingManagerAPIReturnTypeSuccess;

    }

private:



    ConfigManager& configurationManager;
    TransportManager& transportManager;
    DPInternalRequestHandler& dpInternal;
    RequestMessageHandler internalMessageHandler;
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
