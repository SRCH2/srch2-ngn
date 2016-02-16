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
    bool boolVar;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(boolVar);
    if(shardId != NULL){
		numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(boolVar);
		numberOfBytes += shardId->getNumberOfBytes();
    }
    numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(statusValues);
    numberOfBytes += srch2::util::getNumberOfBytesString(global_customized_writer.write(messages));
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
CommandStatusNotification::CommandStatusNotification(){
    this->commandCode = ShardCommandCode_Merge;
}
//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* CommandStatusNotification::serializeBody(void * buffer) const{
    buffer = srch2::util::serializeFixedTypes((uint32_t)commandCode, buffer);
    buffer = srch2::util::serializeFixedTypes((uint32_t)(shardResults.size()), buffer);
    for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
    	buffer = shardResults.at(shardIdx)->serialize(buffer);
    }
    return buffer;
}

unsigned CommandStatusNotification::getNumberOfBytesBody() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)commandCode);
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)(shardResults.size()));
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
	uint32_t intVar;
    buffer = srch2::util::deserializeFixedTypes(buffer, intVar);
    commandCode = (ShardCommandCode)intVar;
    uint32_t vectorSize = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, vectorSize);
    for(uint32_t shardIdx = 0; shardIdx < vectorSize ; ++shardIdx){
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
