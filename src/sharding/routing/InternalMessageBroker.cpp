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
Message* InternalMessageBroker::processRequestMessage(Message *msg, Srch2Server* server,
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
	}else{
		// the message will contain the serialized response object
		replyMessage = this->routingManager.prepareExternalMessage<ResponseType>(ShardId(), outputSerializedObject );
		// we delete this response object here because it's an external request and response
		// will be wrapped in a Message from now on
//		delete outputSerializedObject,inputSerializedObject;
		// TODO : deleting objects is commented out...
	}

	return replyMessage;
}

Message* InternalMessageBroker::processRequestInsertUpdateMessage(Message *msg,
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
	}else{
		// the message will contain the serialized response object
		replyMessage = this->routingManager.
				prepareExternalMessage<SerializableCommandStatus>(ShardId(), outputSerializedObject );
		// we delete this response object here because it's an external request and response
		// will be wrapped in a Message from now on
		//delete outputSerializedObject,inputSerializedObject;
		// TODO : deleting objects is commented out...
	}

	return replyMessage;
}

Message* InternalMessageBroker::notifyWithReply(Message * message){
	if(message == NULL){
		return NULL;
	}

	ShardId shardId = message->getDestinationShardId();
	Srch2Server* server = getShardIndex(shardId);
	if(server == NULL){
		//TODO : what if message shardID is not present in the map?
		// example : message is late and shard is not present anymore ....
		return NULL;
	}

	//1. Deserialize message and get command input 	object
	//2. Call the appropriate internal DP function and get the response object
	//3. Serialize response object into a message
	//4. give the new message out
	switch (message->getType()) {
	case SearchCommandMessageType: // -> for LogicalPlan object
		return processRequestMessage<SerializableSearchCommandInput,SerializableSearchResults>
		(message, server, &DPInternalRequestHandler::internalSearchCommand);
	case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
		return processRequestInsertUpdateMessage(message, server, server->indexDataConfig->getSchema());
	case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
		return processRequestMessage<SerializableDeleteCommandInput, SerializableCommandStatus>
		(message, server,&DPInternalRequestHandler::internalDeleteCommand);
	case SerializeCommandMessageType: // -> for SerializeCommandInput object
		// (used for serializing index and records)
		return processRequestMessage<SerializableSerializeCommandInput, SerializableCommandStatus>
		(message, server,&DPInternalRequestHandler::internalSerializeCommand);
	case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
		return processRequestMessage<SerializableGetInfoCommandInput, SerializableGetInfoResults>
		(message, server,&DPInternalRequestHandler::internalGetInfoCommand);
	case CommitCommandMessageType: // -> for CommitCommandInput object
		return processRequestMessage<SerializableCommitCommandInput, SerializableCommandStatus>
		(message, server, &DPInternalRequestHandler::internalCommitCommand);
	case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
		return processRequestMessage<SerializableResetLogCommandInput,SerializableCommandStatus>
		(message, server, &DPInternalRequestHandler::internalResetLogCommand);
	case SearchResultsMessageType: // -> for SerializedQueryResults object
	case GetInfoResultsMessageType: // -> for GetInfoResults object
	case StatusMessageType: // -> for CommandStatus object (object returned from insert, delete, update)
	default:
		// These message types are only used for responses to other requests and code should
		// never reach to this point
		return NULL;
	}
	return NULL;
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
