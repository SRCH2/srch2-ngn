#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_MESSAGE_HANDLER_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_MESSAGE_HANDLER_H_

#include "aggregators/ResponseAggregator.h"
#include "sharding/transport/CallbackHandler.h"
#include "sharding/sharding/metadata_manager/Cluster.h"
#include "sharding/transport/TransportManager.h"
#include "DistributedProcessorInternal.h"
#include "RequestMessageHandler.h"
#include "ReplyMessageHandler.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

/*
 * This class takes care of all messaging of DP modules
 */
class DPMessageHandler : public CallBackHandler{
public:

	DPMessageHandler(ConfigManager& configurationManager,
			TransportManager& transportManager, DPInternalRequestHandler& dpInternal):
				requestMessageHandler(configurationManager, transportManager, dpInternal, replyMessageHandler),
				replyMessageHandler(transportManager.getMessageAllocator()),
				configurationManager(configurationManager),
				transportManager(transportManager),
				dpInternal(dpInternal){};
	/*
	 * This callback is called from TM when a message of type DP is received.
	 * This message is either passed to DP internal (if it's a request) or
	 *  PendingRequestHandler (if it's a reply)
	 */
	bool resolveMessage(Message * msg, NodeId srcNodeId){
		if(msg == NULL){
			return false;
		}
		if(msg->isDPRequest()){ // must connect to DP Internal
			requestMessageHandler.resolveMessage(msg, srcNodeId);
		}else if(msg->isDPReply()){ // must connect to PendingRequestHandler
			replyMessageHandler.resolveMessage(msg, srcNodeId);
		}
		return true;
	}


    template<typename RequestType, typename ResponseType>
    bool send(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            NodeTargetShardInfo target,
            boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){
    	vector<NodeTargetShardInfo> nodeTargetShardInfoVector;
    	nodeTargetShardInfoVector.push_back(target);
    	broadcast(requestObj, waitForAll, withCallback, aggregator, timeoutValue, nodeTargetShardInfoVector,
    			clusterReadview);
    	return true;
    }

    template<typename RequestType, typename ResponseType>
    bool broadcast(RequestType * requestObj,
    		bool waitForAll,
    		bool withCallback,
    		boost::shared_ptr<ResponseAggregatorInterface<RequestType , ResponseType> > aggregator,
            time_t timeoutValue,
            vector<NodeTargetShardInfo> & targets,
            boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){

    	// prepare the pending request which takes care of the replies of this request
        PendingRequest<RequestType, ResponseType> * pendingRequest = NULL;
        if(withCallback){
            // register a pending request in the handler
            pendingRequest =
                    this->replyMessageHandler.registerPendingRequest(waitForAll, aggregator, targets.size());
        }

        // move on targets and prepare message for each target and send it
		unsigned serializedSize = 0;
		void * serializedRequestObj = NULL;
        for(vector<NodeTargetShardInfo>::iterator targetItr = targets.begin(); targetItr != targets.end(); ++targetItr){
        	// 1. check if it's a local target or external
        	if(targetItr->getNodeId() == clusterReadview->getCurrentNodeId()){// local target, no need to do serialization
        		//TODO : we must redirect the pair of (request, target) to Request message handler
        		unsigned messageIdForLocalShards = transportManager.getUniqueMessageIdValue();
        	    pthread_t internalMessageRouteThread;

        		//4. Register this message in pending messages
        		if(pendingRequest != NULL){
        	        // register a pending message in this pending request
        	        PendingMessage<RequestType, ResponseType> * pendingMessage =
        	                pendingRequest->registerPendingMessage(requestObj, clusterReadview->getCurrentNodeId(),
        	                		timeoutValue,
        	                        messageIdForLocalShards,
        	                        true);
        		}

				RequestType * newRequestObj = requestObj->clone();
        	    LocalRequestHandlerArgs * args = new LocalRequestHandlerArgs(
        	    		newRequestObj, clusterReadview->getCurrentNodeId(),
        	    		messageIdForLocalShards, *targetItr, RequestType::messageType(),
        	    		clusterReadview, &requestMessageHandler);

        	    // run for the same shard in a separate thread
        	    // the reason is that even the request message for some external threads are not sent
        	    // so if we use the same thread those external shards won't start the job until this shard
        	    // is finished which makes it sequential and wrong.
        	    if (pthread_create(&internalMessageRouteThread, NULL, resolveLocalRequest , args) != 0){
        	        //        Logger::console("Cannot create thread for handling local message");
        	        perror("Cannot create thread for handling local message");
        	        return false;
        	    }
        	    // since we don't join this thread it must be detached so that its resource gets deallocated.
        	    pthread_detach(internalMessageRouteThread);
        	}else{ // external target
        		//1. First serialize the request object into a byte array (this must be done only once)
        		if(serializedRequestObj == NULL){
					serializedSize = requestObj->getNumberOfBytes();
					serializedRequestObj = requestObj->serialize(transportManager.getMessageAllocator());
        		}
        		//2. Prepare a message that it's body size is targetData+requestData and copy the request byte array
        		//   into it (so that we save serialization time)
        		unsigned totalSizeOfBody = targetItr->getNumberOfBytes() + serializedSize;
        		void * messageBody = transportManager.getMessageAllocator()->allocateMessageReturnBody(totalSizeOfBody);
        		void * requestObjStart = targetItr->serialize(messageBody);
        		memcpy(requestObjStart, serializedRequestObj, serializedSize);
        		Message* requestMessage = Message::getMessagePointerFromBodyPointer(messageBody);
        		//3. send the prepared message to the node by using TM
        		requestMessage->setDPRequestMask();
        		requestMessage->setType(RequestType::messageType());
        		requestMessage->setMessageId(transportManager.getUniqueMessageIdValue());
        		//4. Register this message in pending messages
        		if(pendingRequest != NULL){
        	        // register a pending message in this pending request
        	        PendingMessage<RequestType, ResponseType> * pendingMessage =
        	                pendingRequest->registerPendingMessage(requestObj, targetItr->getNodeId(), timeoutValue,
        	                        requestMessage->getMessageId(),
        	                        false);
        		}
        	    // pass the ready message to TM to be sent to nodeId
        	    transportManager.sendMessage(targetItr->getNodeId(), requestMessage, timeoutValue);
        	    transportManager.getMessageAllocator()->deallocateByMessagePointer(requestMessage);
        	}
        }

		if(serializedRequestObj != NULL){
			transportManager.getMessageAllocator()->deallocateByreArray((char*)serializedRequestObj, serializedSize);
		}
        return true;
    }

private:
    ConfigManager& configurationManager;
    TransportManager& transportManager;
    DPInternalRequestHandler& dpInternal;
    RequestMessageHandler requestMessageHandler;
    ReplyMessageHandler replyMessageHandler;


    struct LocalRequestHandlerArgs{
    	LocalRequestHandlerArgs(void * requestObj,
        		NodeId node,
        		unsigned requestMessageId,
        		const NodeTargetShardInfo & target,
        		ShardingMessageType type,
        		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
        		RequestMessageHandler * requestMessageHandler):target(target){
    		this->requestObj = requestObj;
    		this->node = node;
    		this->requestMessageId = requestMessageId;
    		this->type = type;
    		this->clusterReadview = clusterReadview;
    		this->requestMessageHandler = requestMessageHandler;
    	}

    	void * requestObj;
    	NodeId node;
    	unsigned requestMessageId;
    	const NodeTargetShardInfo target;
    	ShardingMessageType type;
    	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
    	RequestMessageHandler * requestMessageHandler;
    };

    static void * resolveLocalRequest(void * args){
    	LocalRequestHandlerArgs * reqHandlerArgs = (LocalRequestHandlerArgs *) args;
    	switch (reqHandlerArgs->type) {
			case SearchCommandMessageType:
				reqHandlerArgs->requestMessageHandler->resolveMessage((SearchCommand*)reqHandlerArgs->requestObj,
						reqHandlerArgs->node, reqHandlerArgs->requestMessageId,
						reqHandlerArgs->target, reqHandlerArgs->type,
						reqHandlerArgs->clusterReadview);
				delete (SearchCommand*)reqHandlerArgs->requestObj;
				delete reqHandlerArgs;
				return NULL;
			case InsertUpdateCommandMessageType:
				reqHandlerArgs->requestMessageHandler->resolveMessage((InsertUpdateCommand*)reqHandlerArgs->requestObj,
						reqHandlerArgs->node, reqHandlerArgs->requestMessageId,
						reqHandlerArgs->target, reqHandlerArgs->type,
						reqHandlerArgs->clusterReadview);
				delete (InsertUpdateCommand*)reqHandlerArgs->requestObj;
				delete reqHandlerArgs;
				return NULL;
			case DeleteCommandMessageType:
				reqHandlerArgs->requestMessageHandler->resolveMessage((DeleteCommand*)reqHandlerArgs->requestObj,
						reqHandlerArgs->node, reqHandlerArgs->requestMessageId,
						reqHandlerArgs->target, reqHandlerArgs->type,
						reqHandlerArgs->clusterReadview);
				delete (DeleteCommand*)reqHandlerArgs->requestObj;
				delete reqHandlerArgs;
				return NULL;
			case GetInfoCommandMessageType:
				reqHandlerArgs->requestMessageHandler->resolveMessage((GetInfoCommand*)reqHandlerArgs->requestObj,
						reqHandlerArgs->node, reqHandlerArgs->requestMessageId,
						reqHandlerArgs->target, reqHandlerArgs->type,
						reqHandlerArgs->clusterReadview);
				delete (GetInfoCommand*)reqHandlerArgs->requestObj;
				delete reqHandlerArgs;
				return NULL;
			default:
				break;
		}
    	ASSERT(false);
    	return NULL;
    }

};

}
}

#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_MESSAGE_HANDLER_H_
