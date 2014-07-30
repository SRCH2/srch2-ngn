#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "sharding/processor/DistributedProcessorInternal.h"
#include "transport/Message.h"
#include "transport/MessageAllocator.h"
#include "sharding/transport/CallbackHandler.h"

namespace srch2is = srch2::instantsearch;
using namespace std;


namespace srch2 {
namespace httpwrapper {


class ShardManager;
class RoutingManager;

class RequestMessageHandler : public CallBackHandler {
public:

    RequestMessageHandler(ConfigManager & cm,TransportManager & tm,
    		DPInternalRequestHandler& internalDP,
    		ReplyMessageHandler & replyHandler)
    : configManager(cm), transportManager(tm), internalDP(internalDP), replyHandler(replyHandler){
    };


    template<typename Reply>
    bool sendReply(Reply * replyObject, NodeId waitingNode, unsigned requestMessageId, boost::shared_ptr<const Cluster> clusterReadview){
    	if(replyObject == NULL){
    		ASSERT(false);
    		return false;
    	}
    	// if waiting node is the current node, we just call resolveMessage(objects) from replyMessageHandler
    	if(clusterReadview->getCurrentNodeId() == waitingNode){
    		replyHandler.resolveReply(replyObject, clusterReadview->getCurrentNodeId(), requestMessageId);
    	}else{
    		// send the reply to the waiting node
    		Message * replyMessage =
    				Message::getMessagePointerFromBodyPointer(
    						replyObject->serialize(transportManager.getMessageAllocator()));
    		replyMessage->setDPReplyMask();
    		replyMessage->setType(Reply::messageType());
    		replyMessage->setMessageId(transportManager.getUniqueMessageIdValue());
    		replyMessage->setRequestMessageId(requestMessageId);
    		// TODO : timeout zero is left in the API but not used.
    		transportManager.sendMessage(waitingNode, replyMessage, 0);
    		// delete message
    	    transportManager.getMessageAllocator()->deallocateByMessagePointer(replyMessage);
    	    // delete object
    	    deleteResponseRequestObjectBasedOnType(Reply::messgeType(), replyObject);
    	}
    }

    bool resolveMessage(Message * msg, NodeId node);

    template<typename Request>
    bool resolveMessage(Request * requestObj,
    		NodeId node,
    		unsigned requestMessageId,
    		const NodeTargetShardInfo & target,
    		ShardingMessageType type,
    		boost::shared_ptr<const Cluster> clusterReadview){

    	bool resultFlag = true;
    	switch(type){
        case SearchCommandMessageType: // -> for LogicalPlan object
        	// give search query to DP internal and pass the result to sendReply
        	resultFlag = sendReply(internalDP.internalSearchCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
        	resultFlag = sendReply(internalDP.internalInsertUpdateCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
        	resultFlag = sendReply(internalDP.internalDeleteCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case SerializeCommandMessageType: // -> for SerializeCommandInput object
        	resultFlag = sendReply(internalDP.internalSerializeCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
        	resultFlag = sendReply(internalDP.internalGetInfoCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case CommitCommandMessageType: // -> for CommitCommandInput object
        	resultFlag = sendReply(internalDP.internalCommitCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
        	resultFlag = sendReply(internalDP.internalResetLogCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        case MergeCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
        	resultFlag = sendReply(internalDP.internalMergeCommand(target, clusterReadview, requestObj), node, requestMessageId, clusterReadview);
        	break;
        default:
            ASSERT(false);
            return false;
        }

    	if(clusterReadview->getCurrentNodeId() != node){ // request comes from external node
    		// delete request object we don't need it
    		deleteResponseRequestObjectBasedOnType(Request::messageType(), requestObj);
    	}

    }

private:

    DPInternalRequestHandler& internalDP;
    ConfigManager & configManager;
    TransportManager & transportManager;
    ReplyMessageHandler & replyHandler;
    void deleteResponseRequestObjectBasedOnType(ShardingMessageType type, void * responseObject);
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
