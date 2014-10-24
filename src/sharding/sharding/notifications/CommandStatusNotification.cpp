#include "CommandStatusNotification.h"



#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "../state_machine/StateMachine.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {

CommandStatusNotification::ShardStatus::ShardStatus(const ShardId * shardId){
	this->messages = Json::Value(Json::arrayValue);
	this->shardId = shardId;
}

bool CommandStatusNotification::ShardStatus::getStatusValue() const{
	ASSERT(statusValues.size() == 1);
	return statusValues.at(0);
}
void CommandStatusNotification::ShardStatus::setStatusValue(bool statusValue){
	this->statusValues.push_back(statusValue);
}

/*
 * Contains the messages/warnings/errors coming from the shard ...
 */
Json::Value messages;

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* CommandStatusNotification::ShardStatus::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes((bool)(shardId == NULL), buffer);
	if(shardId != NULL){
		buffer = srch2::util::serializeFixedTypes(shardId->isClusterShard(), buffer);
		buffer = shardId->serialize(buffer);
	}
	buffer = srch2::util::serializeVectorOfFixedTypes(statusValues, buffer);
    buffer = srch2::util::serializeString(global_customized_writer.write(messages), buffer);
    return buffer;
}

unsigned CommandStatusNotification::ShardStatus::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += sizeof(bool);
    if(shardId != NULL){
		numberOfBytes += sizeof(bool);
		numberOfBytes += shardId->getNumberOfBytes();
    }
    numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(statusValues);
    numberOfBytes += sizeof(unsigned) + global_customized_writer.write(messages).size();
    return numberOfBytes;
}

//given a byte stream recreate the original object
CommandStatusNotification::ShardStatus * CommandStatusNotification::ShardStatus::deserialize(void* buffer){

	if(buffer == NULL){
		ASSERT(false);
		return NULL;
	}
	bool isShardIdNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isShardIdNull);
	ShardId * shardId = NULL;
	if(! isShardIdNull){
		bool isClusterShardId = false;
		buffer = srch2::util::deserializeFixedTypes(buffer, isClusterShardId);
		if(isClusterShardId){
			shardId = new ClusterShardId();
		}else{
			shardId = new NodeShardId();
		}
		buffer = shardId->deserialize(buffer);
	}
    ShardStatus * newShardResult = new ShardStatus(shardId);
    buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, newShardResult->statusValues);
    string messagesStr ;
    buffer = srch2::util::deserializeString(buffer, messagesStr);
    Json::Reader reader;
    reader.parse(messagesStr, newShardResult->messages);
    // allocate and construct the object
    return newShardResult;
}

CommandStatusNotification::CommandStatusNotification(ShardCommandCode commandCode){
    this->commandCode = commandCode;
}
CommandStatusNotification::CommandStatusNotification(){}
//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* CommandStatusNotification::serializeBody(void * buffer) const{
    buffer = srch2::util::serializeFixedTypes(commandCode, buffer);
    buffer = srch2::util::serializeFixedTypes((unsigned)(shardResults.size()), buffer);
    for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
    	buffer = shardResults.at(shardIdx)->serialize(buffer);
    }
    return buffer;
}

unsigned CommandStatusNotification::getNumberOfBytesBody() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += sizeof(ShardCommandCode);
    numberOfBytes += sizeof(unsigned);
    for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
    	numberOfBytes += shardResults.at(shardIdx)->getNumberOfBytes();
    }
    return numberOfBytes;
}

//given a byte stream recreate the original object
void * CommandStatusNotification::deserializeBody(void* buffer){
	if(buffer == NULL){
		ASSERT(false);
		return NULL;
	}
    buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);
    unsigned vectorSize = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, vectorSize);
    for(unsigned shardIdx = 0; shardIdx < vectorSize ; ++shardIdx){
    	ShardStatus * newShardResult = ShardStatus::deserialize(buffer);
    	buffer = (void*)((char*)buffer +  newShardResult->getNumberOfBytes());
    	shardResults.push_back(newShardResult);
    }
    return buffer;
}

bool CommandStatusNotification::resolveNotification(SP(ShardingNotification) notif){
	ShardManager::getShardManager()->getStateMachine()->handle(notif);
	return true;
}

//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommandStatusNotification::messageType() const{
    return StatusMessageType;
}

ShardCommandCode CommandStatusNotification::getCommandCode() const {
    return commandCode;
}

vector<CommandStatusNotification::ShardStatus *> CommandStatusNotification::getShardsStatus() const {
    return shardResults;
}

void CommandStatusNotification::addShardResult(CommandStatusNotification::ShardStatus * shardResult){
	shardResults.push_back(shardResult);
}

}
}
