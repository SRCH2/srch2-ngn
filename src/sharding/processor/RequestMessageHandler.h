/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#include "processor/serializables/SerializableGetInfoCommandInput.h"
#include "processor/serializables/SerializableGetInfoResults.h"

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
        case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
        	resultFlag = sendReply<GetInfoCommandResults>(internalDP.internalGetInfoCommand(target, clusterReadview, (GetInfoCommand*)requestObj), node, requestMessageId, clusterReadview);
        	break;
        default:
            ASSERT(false);
            return false;
        }

//		deleteResponseRequestObjectBasedOnType(Request::messageType(), requestObj);

    	return false;
    }

private:

    ConfigManager & configManager;
    TransportManager & transportManager;
    DPInternalRequestHandler& internalDP;
    ReplyMessageHandler & replyHandler;
    void deleteResponseRequestObjectBasedOnType(ShardingMessageType type, void * responseObject);
};

}
}

#endif // __SHARDING_ROUTING_INTERNAL_MESSAGE_BROKER_H_
