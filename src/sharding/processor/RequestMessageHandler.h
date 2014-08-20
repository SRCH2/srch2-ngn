#ifndef __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
#define __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_

#include "sharding/processor/DistributedProcessorInternal.h"
#include "transport/Message.h"
#include "transport/MessageAllocator.h"
#include "sharding/transport/CallbackHandler.h"
#include "sharding/transport/TransportManager.h"
#include "sharding/configuration/ConfigManager.h"
#include "ReplyMessageHandler.h"
#include "sharding/sharding/ShardManager.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"

#include "processor/serializables/SerializableSearchCommandInput.h"
#include "processor/serializables/SerializableSearchCommandInput.h"
#include "processor/serializables/SerializableSearchResults.h"
#include "processor/serializables/SerializableInsertUpdateCommandInput.h"
#include "processor/serializables/SerializableDeleteCommandInput.h"
#include "processor/serializables/SerializableCommandStatus.h"
#include "processor/serializables/SerializableSerializeCommandInput.h"
#include "processor/serializables/SerializableResetLogCommandInput.h"
#include "processor/serializables/SerializableCommitCommandInput.h"
#include "processor/serializables/SerializableGetInfoCommandInput.h"
#include "processor/serializables/SerializableGetInfoResults.h"
#include "processor/serializables/SerializableMergeCommandInput.h"

namespace srch2is = srch2::instantsearch;
using namespace std;


namespace srch2 {
namespace httpwrapper {


class RequestMessageHandler : public CallBackHandler {
public:

    RequestMessageHandler(ConfigManager & cm,TransportManager & tm,
    		DPInternalRequestHandler& internalDP,
    		ReplyMessageHandler & replyHandler)
    : configManager(cm), transportManager(tm), internalDP(internalDP), replyHandler(replyHandler){
    };


    template<typename Reply>
    bool sendReply(Reply * replyObject, NodeId waitingNode, unsigned requestMessageId, boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){
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
    	    deleteResponseRequestObjectBasedOnType(Reply::messageType(), replyObject);
    	}
    	return true;
    }

    bool resolveMessage(Message * msg, NodeId node);

    template<class Request>
    bool resolveMessage(Request * requestObj,
    		NodeId node,
    		unsigned requestMessageId,
    		const NodeTargetShardInfo & target,
    		ShardingMessageType type,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){

    	bool resultFlag = true;
    	switch(type){
        case SearchCommandMessageType: // -> for LogicalPlan object
        	// give search query to DP internal and pass the result to sendReply
        	resultFlag = sendReply<SearchCommandResults>(internalDP.internalSearchCommand(target, clusterReadview, (SearchCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
        	resultFlag = sendReply<CommandStatus>(internalDP.internalInsertUpdateCommand(target, clusterReadview, (InsertUpdateCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
        	resultFlag = sendReply<CommandStatus>(internalDP.internalDeleteCommand(target, clusterReadview, (DeleteCommand *)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case SerializeCommandMessageType: // -> for SerializeCommandInput object
        	resultFlag = sendReply<CommandStatus>(internalDP.internalSerializeCommand(target, clusterReadview, (SerializeCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
        	resultFlag = sendReply<GetInfoCommandResults>(internalDP.internalGetInfoCommand(target, clusterReadview, (GetInfoCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case CommitCommandMessageType: // -> for CommitCommandInput object
        	resultFlag = sendReply<CommandStatus>(internalDP.internalCommitCommand(target, clusterReadview, (CommitCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
        	resultFlag = sendReply<CommandStatus>(internalDP.internalResetLogCommand(target, clusterReadview, (ResetLogCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        case MergeCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
        	resultFlag = sendReply<CommandStatus>(internalDP.internalMergeCommand(target, clusterReadview, (MergeCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        default:
            ASSERT(false);
            return false;
        }

		deleteResponseRequestObjectBasedOnType(Request::messageType(), requestObj);

    	return false;
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
