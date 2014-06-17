#include "InternalMessageBroker.h"
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
#include "transport/Message.h"
#include "util/Version.h"
#include "RoutingManager.h"
#include "RoutingUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

void InternalMessageBroker::resolveMessage(Message * msg, NodeId node){
	std::pair<Message * , void *> resultOfDPInternal = notifyReturnResponse(msg);

	Message* replyMessage = resultOfDPInternal.first;

	if(replyMessage == NULL){
		Logger::console("Reply is null");
		return;
	}

	if(msg->isLocal()){ // local message, resolve in pending messages
        replyMessage->setRequestMessageId(msg->getMessageId());
        replyMessage->setReply()->setInternal();
        if ( ! routingManager.getPendingRequestsHandler()->resolveResponseMessage(replyMessage, node, resultOfDPInternal.second)){
            // reply could not be resolved. This means the request of this response is already timedout and gone.
            // reply and response object must be deallocated here.
            routingManager.getTransportManager().getMessageAllocator()->deallocateByMessagePointer(replyMessage);
            this->deleteResponseObjectBasedOnType(replyMessage, resultOfDPInternal.second);
        }
	}else{ // request message is coming from another shard, send the response back
		ASSERT(resultOfDPInternal.second == NULL);
		replyMessage->setRequestMessageId(msg->getMessageId());
		replyMessage->setReply()->setInternal();
		routingManager.getTransportManager().sendMessage(node, replyMessage, 0);
		getMessageAllocator()->deallocateByMessagePointer(replyMessage);
	}
}


void InternalMessageBroker::deleteResponseObjectBasedOnType(Message * replyMsg, void * responseObject){
    switch (replyMsg->getType()) {
    case SearchCommandMessageType: // -> for LogicalPlan object
        delete (SearchCommandResults*)responseObject;
        return;
    case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
        delete (CommandStatus*)responseObject;
        return;
    case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
        delete (CommandStatus*)responseObject;
        return;
    case SerializeCommandMessageType: // -> for SerializeCommandInput object
        delete (CommandStatus*)responseObject;
        return;
    case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
        delete (GetInfoCommandResults*)responseObject;
        return;
    case CommitCommandMessageType: // -> for CommitCommandInput object
        delete (CommandStatus*)responseObject;
        return;
    case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
        delete (CommandStatus*)responseObject;
        return;
    case SearchResultsMessageType: // -> for SerializedQueryResults object
    case GetInfoResultsMessageType: // -> for GetInfoResults object
    case StatusMessageType: // -> for CommandStatus object (object returned from insert, delete, update)
    default:
        ASSERT(false);
        return;
    }
}


Srch2ServerHandle InternalMessageBroker::getSrch2ServerHandle(ShardId & shardId){
    return routingManager.getSrch2ServerIndex(shardId);
}

MessageAllocator * InternalMessageBroker::getMessageAllocator() {
    // return routingManager.transportManager.getMessageAllocator();
    return routingManager.getMessageAllocator();
}

template<typename RequestType, typename ResponseType>
std::pair<Message*,ResponseType*> InternalMessageBroker::processRequestMessage(Message *requestMessage, Srch2ServerHandle serverHandle,
        ResponseType * (DPInternalRequestHandler::*internalDPRequestHandlerFunction) (Srch2ServerHandle, RequestType*)) {

    RequestType *inputSerializedObject = NULL;
    if(requestMessage->isLocal()){ // message comes from current node
        // ASSERT(currentNodeId = msg->shardId->nodeId);
        // This object (inputSerializedObject) will be deleted in aggregator because it's a local message
        // and this object was originally created in DPExternal
        inputSerializedObject = decodeInternalMessage<RequestType>(requestMessage);
    }else{
        inputSerializedObject = decodeExternalMessage<RequestType>(requestMessage);
    }

    // if msg is local this response object will be deleted in results aggregator.
    // otherwise, TODO red line
    ResponseType * outputSerializedObject = (internalDP.*internalDPRequestHandlerFunction)(serverHandle, inputSerializedObject);

    // prepare the reply message
    Message *replyMessage = NULL;
    if(requestMessage->isLocal()){
        // the message will contain only the pointer to response object
        replyMessage = this->routingManager.prepareInternalMessage<ResponseType>(ShardId(), outputSerializedObject );
        // this message object will be deleted in pendinMessages in TM
        return std::make_pair<Message*,ResponseType*>(replyMessage, outputSerializedObject);
    }else{
        // the message will contain the serialized response object
        replyMessage = this->routingManager.prepareExternalMessage<ResponseType>(ShardId(), outputSerializedObject );
        // we delete this response object here because it's an external request and response
        // will be wrapped in a Message from now on
        delete inputSerializedObject;
        delete outputSerializedObject;
        return std::make_pair<Message*,ResponseType*>(replyMessage, NULL);
    }
}

std::pair<Message*,CommandStatus*> InternalMessageBroker::processRequestInsertUpdateMessage(Message *reqInsertUpdateMsg,
        Srch2ServerHandle serverHandle, const Schema * schema){
    InsertUpdateCommand *inputSerializedObject = NULL;
    if(reqInsertUpdateMsg->isLocal()){ // message comes from current node
        // ASSERT(currentNodeId = msg->shardId->nodeId);
        // This object (inputSerializedObject) will be deleted in aggregator because it's a local message
        // and this object was originally created in DPExternal
        inputSerializedObject = decodeInternalMessage<InsertUpdateCommand>(reqInsertUpdateMsg);
    }else{
        inputSerializedObject = decodeExternalInsertUpdateMessage(reqInsertUpdateMsg,schema);
    }

    // if msg is local this response object will be deleted in results aggregator.
    // otherwise, TODO red line
    CommandStatus * outputSerializedObject  = internalDP.internalInsertUpdateCommand(serverHandle,inputSerializedObject);

    // prepare the reply message
    Message *replyMessage = NULL;
    if(reqInsertUpdateMsg->isLocal()){
        // the message will contain only the pointer to response object
        replyMessage = this->routingManager.
                prepareInternalMessage<CommandStatus>(ShardId(), outputSerializedObject );
        // this message object will be deleted in pendinMessages in TM
        return std::make_pair<Message*,CommandStatus*>(replyMessage,outputSerializedObject);
    }else{
        // the message will contain the serialized response object
        replyMessage = this->routingManager.
                prepareExternalMessage<CommandStatus>(ShardId(), outputSerializedObject );
        // we delete this response object here because it's an external request and response
        // will be wrapped in a Message from now on
        delete inputSerializedObject;
        delete outputSerializedObject;
        return std::make_pair<Message*,CommandStatus*>(replyMessage,NULL);
    }
}

std::pair<Message*,void*> InternalMessageBroker::notifyReturnResponse(Message * requestMessage){
    if(requestMessage == NULL){
        return std::make_pair<Message*,void*>(NULL,NULL);
    }

    ShardId shardId = requestMessage->getDestinationShardId();
    Srch2ServerHandle serverHandle = getSrch2ServerHandle(shardId);

    // get the schema from config file by using the coreId of shardId
    // TODO : S lock to read from cluster
    CoreInfo_t * coreInfo = NULL ;// TODO : API must be added to cluster to get id and return coreInfo_t

    if(serverHandle <= 0 || coreInfo == NULL){
    	// different error values :
    	// 0 : not found
    	// -1 : not intialized
    	// -2 : not ready
        //TODO : what if message shardID is not present in the map?
        // example : message is late and shard is not present anymore ....
        return std::make_pair<Message*,void*>(NULL,NULL);
    }

    //1. Deserialize message and get command input     object
    //2. Call the appropriate internal DP function and get the response object
    //3. Serialize response object into a message
    //4. give the new message out
    switch (requestMessage->getType()) {
    case SearchCommandMessageType: // -> for LogicalPlan object
    {
        std::pair<Message*,SearchCommandResults*> result = processRequestMessage<SearchCommand,SearchCommandResults>
        (requestMessage, serverHandle, &DPInternalRequestHandler::internalSearchCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
    {
        std::pair<Message*,CommandStatus*> result = processRequestInsertUpdateMessage(requestMessage, serverHandle, coreInfo->getSchema());
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
    {
        std::pair<Message*,CommandStatus*> result = processRequestMessage<DeleteCommand, CommandStatus>
        (requestMessage, serverHandle,&DPInternalRequestHandler::internalDeleteCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case SerializeCommandMessageType: // -> for SerializeCommandInput object
    {
        // (used for serializing index and records)
        std::pair<Message*,CommandStatus*> result = processRequestMessage<SerializeCommand, CommandStatus>
        (requestMessage, serverHandle,&DPInternalRequestHandler::internalSerializeCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
    {
        std::pair<Message*,GetInfoCommandResults*> result = processRequestMessage<GetInfoCommand, GetInfoCommandResults>
        (requestMessage, serverHandle,&DPInternalRequestHandler::internalGetInfoCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case CommitCommandMessageType: // -> for CommitCommandInput object
    {
        std::pair<Message*,CommandStatus*> result = processRequestMessage<CommitCommand, CommandStatus>
        (requestMessage, serverHandle, &DPInternalRequestHandler::internalCommitCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
    {
        std::pair<Message*,CommandStatus*> result = processRequestMessage<ResetLogCommand,CommandStatus>
        (requestMessage, serverHandle, &DPInternalRequestHandler::internalResetLogCommand);
        return std::make_pair<Message * , void*>(result.first,result.second);
    }
    case SearchResultsMessageType: // -> for SerializedQueryResults object
    case GetInfoResultsMessageType: // -> for GetInfoResults object
    case StatusMessageType: // -> for CommandStatus object (object returned from insert, delete, update)
    default:
        // These message types are only used for responses to other requests and code should
        // never reach to this point
        return std::make_pair<Message*,void*>(NULL,NULL);
    }
}

}
}

