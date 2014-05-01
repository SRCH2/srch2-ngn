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

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2::httpwrapper;

template<typename InputType, typename Deserializer, typename OutputType>
void InternalMessageBroker::broker(Message *msg, Srch2Server* server,
    OutputType (DPInternalRequestHandler::*fn) (Srch2Server*, InputType*)) {
  InputType *input = (msg->isLocal()) ? (InputType*) msg->buffer
               : (InputType*) &Deserializer::deserialize((void*) msg->buffer);
  OutputType output((internalDP.*fn)(server, input));
  void *reply = (msg->isLocal()) ? (void*) input 
                                 : input->serialize(getMessageAllocator());
  sendReply(msg, reply);
  if(!msg->isLocal()) 
    delete input, output;
}

void InternalMessageBroker::processInternalMessage(Message * message){
	if(message == NULL){
		return;
	}

	Srch2Server* server = getShardIndex(message->shard);
	if(server == NULL){
		//TODO : what if message shardID is not present in the map?
		// example : message is late and shard is not present anymore ....
		return;
	}

	//1. Deserialize message and get command input 	object
	//2. Call the appropriate internal DP function and get the response object
	//3. Serialize response object into a message
	//4. give the new message out
	switch (message->type) {
		case SearchCommandMessageType: // -> for LogicalPlan object
      broker<SerializableSearchCommandInput, SerializableSearchCommandInput,
        SerializableSearchResults>(message, server, 
            &DPInternalRequestHandler::internalSearchCommand);
		break;
    // -> for Record object (used for insert and update)
		case InsertUpdateCommandMessageType: 
 /*     broker<SerializableInsertUpdateCommandInput, 
        SerializableInsertUpdateCommandInput, 
        SerializableCommandStatus>(message, server, 
            &DPInternalRequestHandler::internalInsertUpdateCommand);*/
    break;
    // -> for DeleteCommandInput object (used for delete)
    case DeleteCommandMessageType: 
      broker<SerializableDeleteCommandInput, SerializableDeleteCommandInput,
        SerializableCommandStatus>(message, server, 
            &DPInternalRequestHandler::internalDeleteCommand);
    break;
    // -> for SerializeCommandInput object
    // (used for serializing index and records)
    case SerializeCommandMessageType: 
      broker<SerializableSerializeCommandInput, 
        SerializableSerializeCommandInput,
        SerializableCommandStatus>(message, server, 
            &DPInternalRequestHandler::internalSerializeCommand);
    break;
    // -> for GetInfoCommandInput object (used for getInfo)
		case GetInfoCommandMessageType:
    //TODO: needs versionInfo
    /*
      broker<SerializableGetInfoCommandInput, SerializableGetInfoCommandInput,
        SerializableGetInfoResults>(message, server, 
            &DPInternalRequestHandler::internalGetInfoCommand);*/
    break;
		case CommitCommandMessageType: // -> for CommitCommandInput object
      broker<SerializableCommitCommandInput, SerializableCommitCommandInput,
        SerializableCommandStatus>(message, server, 
            &DPInternalRequestHandler::internalCommitCommand);
    break;
    // -> for ResetLogCommandInput (used for resetting log)
		case ResetLogCommandMessageType:
      broker<SerializableResetLogCommandInput,SerializableResetLogCommandInput,
        SerializableCommandStatus>(message, server, 
            &DPInternalRequestHandler::internalResetLogCommand);
    break;
		default:
			//TODO : what should we do here ?
		break;
	}
}

Srch2Server * InternalMessageBroker::getShardIndex(ShardId & shardId){
	//get shardId from message to use map to get Indexer object
	std::map<ShardId, Srch2Server*>::iterator indexerItr = 
    routingManager.getShardToIndexMap().find(shardId);
	if(indexerItr == routingManager.getShardToIndexMap().end()){
		return NULL;
	}

	return indexerItr->second;
}
/*
std::allocator<char>* getMessageAllocator() {
  return routingManager.transportManager.getMessageAllocator();
}

void sendReply(Message* msg, void* replyObject) {
  routingManager.transportManager(msg, input);
}*/
