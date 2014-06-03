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

using namespace srch2::httpwrapper;


template<typename RequestType, typename ResponseType>
std::pair<Message*,ResponseType*> InternalMessageBroker::processRequestMessage(Message *msg, Srch2Server* server,
		ResponseType * (DPInternalRequestHandler::*internalDPRequestHandlerFunction) (Srch2Server*, RequestType*)) {

	RequestType *inputSerializedObject = NULL;
	if(msg->isLocal()){ // message comes from current node
		// ASSERT(currentNodeId = msg->shardId->nodeId);
		// This object (inputSerializedObject) will be deleted in aggregator because it's a local message
		// and this object was originally created in DPExternal
		inputSerializedObject = decodeInternalMessage<RequestType>(msg);
	}else{
		inputSerializedObject = decodeExternalMessage<RequestType>(msg);
	}

	// if msg is local this response object will be deleted in results aggregator.
	// otherwise, TODO red line
	ResponseType * outputSerializedObject = (internalDP.*internalDPRequestHandlerFunction)(server, inputSerializedObject);

	// prepare the reply message
	Message *replyMessage = NULL;
	if(msg->isLocal()){
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

std::pair<Message*,SerializableCommandStatus*> InternalMessageBroker::processRequestInsertUpdateMessage(Message *msg,
		Srch2Server* server, const Schema * schema){
	SerializableInsertUpdateCommandInput *inputSerializedObject = NULL;
	if(msg->isLocal()){ // message comes from current node
		// ASSERT(currentNodeId = msg->shardId->nodeId);
		// This object (inputSerializedObject) will be deleted in aggregator because it's a local message
		// and this object was originally created in DPExternal
		inputSerializedObject = decodeInternalMessage<SerializableInsertUpdateCommandInput>(msg);
	}else{
		inputSerializedObject = decodeExternalInsertUpdateMessage(msg,schema);
	}

	// if msg is local this response object will be deleted in results aggregator.
	// otherwise, TODO red line
	SerializableCommandStatus * outputSerializedObject  = internalDP.internalInsertUpdateCommand(server,inputSerializedObject);

	// prepare the reply message
	Message *replyMessage = NULL;
	if(msg->isLocal()){
		// the message will contain only the pointer to response object
		replyMessage = this->routingManager.
				prepareInternalMessage<SerializableCommandStatus>(ShardId(), outputSerializedObject );
		// this message object will be deleted in pendinMessages in TM
		return std::make_pair<Message*,SerializableCommandStatus*>(replyMessage,outputSerializedObject);
	}else{
		// the message will contain the serialized response object
		replyMessage = this->routingManager.
				prepareExternalMessage<SerializableCommandStatus>(ShardId(), outputSerializedObject );
		// we delete this response object here because it's an external request and response
		// will be wrapped in a Message from now on
		delete inputSerializedObject;
		delete outputSerializedObject;
		return std::make_pair<Message*,SerializableCommandStatus*>(replyMessage,NULL);
	}
}

std::pair<Message*,void*> InternalMessageBroker::notifyWithReply(Message * message){
	if(message == NULL){
		return std::make_pair<Message*,void*>(NULL,NULL);
	}

	ShardId shardId = message->getDestinationShardId();
	Srch2Server* server = getShardIndex(shardId);
	if(server == NULL){
		//TODO : what if message shardID is not present in the map?
		// example : message is late and shard is not present anymore ....
		return std::make_pair<Message*,void*>(NULL,NULL);
	}

	//1. Deserialize message and get command input 	object
	//2. Call the appropriate internal DP function and get the response object
	//3. Serialize response object into a message
	//4. give the new message out
	switch (message->getType()) {
	case SearchCommandMessageType: // -> for LogicalPlan object
	{
		std::pair<Message*,SerializableSearchResults*> result = processRequestMessage<SerializableSearchCommandInput,SerializableSearchResults>
		(message, server, &DPInternalRequestHandler::internalSearchCommand);
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
	{
		std::pair<Message*,SerializableCommandStatus*> result = processRequestInsertUpdateMessage(message, server, server->indexDataConfig->getSchema());
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
	{
		std::pair<Message*,SerializableCommandStatus*> result = processRequestMessage<SerializableDeleteCommandInput, SerializableCommandStatus>
		(message, server,&DPInternalRequestHandler::internalDeleteCommand);
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case SerializeCommandMessageType: // -> for SerializeCommandInput object
	{
		// (used for serializing index and records)
		std::pair<Message*,SerializableCommandStatus*> result = processRequestMessage<SerializableSerializeCommandInput, SerializableCommandStatus>
		(message, server,&DPInternalRequestHandler::internalSerializeCommand);
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
	{
		std::pair<Message*,SerializableGetInfoResults*> result = processRequestMessage<SerializableGetInfoCommandInput, SerializableGetInfoResults>
		(message, server,&DPInternalRequestHandler::internalGetInfoCommand);
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case CommitCommandMessageType: // -> for CommitCommandInput object
	{
		std::pair<Message*,SerializableCommandStatus*> result = processRequestMessage<SerializableCommitCommandInput, SerializableCommandStatus>
		(message, server, &DPInternalRequestHandler::internalCommitCommand);
		return std::make_pair<Message * , void*>(result.first,result.second);
	}
	case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
	{
		std::pair<Message*,SerializableCommandStatus*> result = processRequestMessage<SerializableResetLogCommandInput,SerializableCommandStatus>
		(message, server, &DPInternalRequestHandler::internalResetLogCommand);
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


template<typename RequestType> inline
void InternalMessageBroker::processRequestMessageNoReply(Message *msg){
	RequestType *inputSerializedObject = NULL;
	if(msg->isLocal()){ // message comes from current node
		// ASSERT(currentNodeId = msg->shardId->nodeId);
		inputSerializedObject = decodeInternalMessage<RequestType>(msg);
		//
		// TODO do something with the message
		//
		// this msg contains a pointer to an object which must be deleted here
		delete inputSerializedObject;
	}else{
		// TODO do something with the message
	}
}

void InternalMessageBroker::notifyNoReply(Message * msg){
	// use DP internal or some other module to process this message without reply
	//TODO
	switch (msg->getType()) {
	case SearchCommandMessageType: // -> for LogicalPlan object
		processRequestMessageNoReply<SerializableSearchCommandInput>(msg);
		return;

	case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
		processRequestMessageNoReply<SerializableInsertUpdateCommandInput>(msg);
		return;

	case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
		processRequestMessageNoReply<SerializableDeleteCommandInput>(msg);
		return;

	case SerializeCommandMessageType: // -> for SerializeCommandInput object
		// (used for serializing index and records)
		processRequestMessageNoReply<SerializableSerializeCommandInput>(msg);
		return;

	case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
		processRequestMessageNoReply<SerializableGetInfoCommandInput>(msg);
		return;

	case CommitCommandMessageType: // -> for CommitCommandInput object
		processRequestMessageNoReply<SerializableCommitCommandInput>(msg);
		return;

	case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
		processRequestMessageNoReply<SerializableResetLogCommandInput>(msg);
		return;

	case SearchResultsMessageType: // -> for SerializedQueryResults object
	case GetInfoResultsMessageType: // -> for GetInfoResults object
	case StatusMessageType: // -> for CommandStatus object (object returned from insert, delete, update)
	default:
		// These message types are only used for reponses to other requests and code should
		// never reach to this point
		return ;
	}
	return ;
}

Srch2Server * InternalMessageBroker::getShardIndex(ShardId & shardId){
	return routingManager.getShardIndex(shardId);
}

MessageAllocator * InternalMessageBroker::getMessageAllocator() {
	// return routingManager.transportManager.getMessageAllocator();
	return routingManager.getMessageAllocator();
}
