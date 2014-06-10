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


    template<typename RequestType, typename ResponseType>
    RoutingManagerAPIReturnType broadcast(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregator<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            CoreShardInfo & coreInfo);


    template<typename RequestType , typename ReseponseType>
    RoutingManagerAPIReturnType route(RequestType * requestObj,
    		bool withCallback,
            boost::shared_ptr<ResponseAggregator<RequestType , ReseponseType> > aggregator,
            time_t timeoutValue,
            ShardId shardInfo);



    MessageAllocator * getMessageAllocator() ;
    ConfigManager* getConfigurationManager();
    DPInternalRequestHandler* getDpInternal();
    InternalMessageBroker * getInternalMessageBroker();
    PendingRequestsHandler * getPendingRequestsHandler();
    TransportManager& getTransportManager();
    Srch2Server * getShardIndex(ShardId shardId){
        // should we get Serch2Server based one core ID?
        map<unsigned, Srch2Server *>::iterator shardServer = shardServers.find(shardId.coreId);
        if(shardServer == shardServers.end()){
            return NULL;
        }
        return shardServer->second;
    }

    /*
     * This function sends the request object to a shard which resides
     * in the same node.
     */
    template<typename RequestType >
    Message * prepareInternalMessage(ShardId shardId,
            RequestType * requestObjPointer);
    template<typename RequestType >
    Message * prepareExternalMessage(ShardId shardId,
            RequestType * requestObjPointer);
    //    template<typename RequestType >
    //    static RequestType * decodeInternalMessage(Message * message);
    //    template<typename RequestType >
    //    static RequestType * decodeExternalMessage(Message * message);
    //    static SerializableInsertUpdateCommandInput * decodeExternalInsertUpdateMessage(Message * message,const Schema * schema);

private:

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendInternalMessage(Message * msg, RequestType * requestObjPointer,
            ShardId shardId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL);

    // When pendingRequest is NULL, no response is expected for request.
    template<typename RequestType , typename ResponseType>
    void sendExternalMessage(Message * msg, RequestType * requestObjPointer,
            ShardId shardId, time_t timeoutValue, PendingRequest<RequestType, ResponseType> * pendingRequest = NULL);

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

#include "BroadcastInlines.h"

#endif //__SHARDING_ROUTING_ROUTING_MANAGER_H__
