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
#include "RequestMessageHandler.h"

#include "transport/Message.h"
#include "util/Version.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


// DP requests that should go to DP Internal reach here ...
/*
 * We deserialize the msg in this functions and give the target object and the request object to
 * the overload of resolveMessage to pass to DP Internal.
 */
bool RequestMessageHandler::resolveMessage(Message * msg, NodeId node){

	if(msg == NULL){
		ASSERT(false);
		return false;
	}

	ASSERT(msg->isDPRequest());
	void * buffer = (void*) Message::getBodyPointerFromMessagePointer(msg);
	// 1. First read the target object
	NodeTargetShardInfo target;
	buffer = target.deserialize(buffer);
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
   	ShardManager::getShardManager()->getMetadataManager()->getClusterReadView(clusterReadview);
	// 2. Second, based on the type of this message, deserialize Request objects
   	bool resultFlag = true;
	switch(msg->getType()){
    case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
    {
    	GetInfoCommand* getInfoCommand = GetInfoCommand::deserialize(buffer);
        resultFlag = resolveMessage(getInfoCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete getInfoCommand;
    	break;
    }
    default:
    {
        ASSERT(false);
        return false;
    }
    }
	return resultFlag;
}


void RequestMessageHandler::deleteResponseRequestObjectBasedOnType(ShardingMessageType type, void * responseObject){
    switch (type) {
    case GetInfoResultsMessageType: // -> for DeleteCommandInput object (used for delete)
    	delete (GetInfoCommandResults*)responseObject;
    	return;
    case GetInfoCommandMessageType:
    	delete (GetInfoCommand *)responseObject;
    	return;
    default:
        ASSERT(false);
        return;
    }
}

}
}

