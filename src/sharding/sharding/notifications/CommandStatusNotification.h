#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
namespace srch2 {
namespace httpwrapper {



class CommandStatusNotification : public ShardingNotification{
public:

	struct ShardStatus{
	public:
		ShardStatus(const ShardId * shardId = NULL){
			this->messages = Json::Value(Json::arrayValue);
			this->shardId = shardId;
		}

	    /*
	     * Contains the identifier of the shard ...
	     */
	    const ShardId * shardId;
	    /*
	     * Success => True
	     * Failure => False
	     */
//	    bool statusValue;
	    vector<bool> statusValues;

	    bool getStatusValue(){
	    	ASSERT(statusValues.size() == 1);
	    	return statusValues.at(0);
	    }
	    void setStatusValue(bool statusValue){
	    	this->statusValues.push_back(statusValue);
	    }

	    /*
	     * Contains the messages/warnings/errors coming from the shard ...
	     */
	    Json::Value messages;

	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer){
	    	buffer = srch2::util::serializeFixedTypes((bool)(shardId == NULL), buffer);
	    	if(shardId != NULL){
				buffer = srch2::util::serializeFixedTypes(shardId->isClusterShard(), buffer);
				buffer = shardId->serialize(buffer);
	    	}
	    	buffer = srch2::util::serializeVectorOfFixedTypes(statusValues, buffer);
	        buffer = srch2::util::serializeString(global_customized_writer.write(messages), buffer);
	        return buffer;
	    }

	    unsigned getNumberOfBytes() const{
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
	    static ShardStatus * deserialize(void* buffer){

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

		private:
	};

    CommandStatusNotification(ShardCommandCode commandCode){
        this->commandCode = commandCode;
    }
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer){
    	buffer = ShardingNotification::serialize(buffer);
        buffer = srch2::util::serializeFixedTypes(commandCode, buffer);
        buffer = srch2::util::serializeFixedTypes((unsigned)(shardResults.size()), buffer);
        for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
        	buffer = shardResults.at(shardIdx)->serialize(buffer);
        }
        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += ShardingNotification::getNumberOfBytes();
        numberOfBytes += sizeof(ShardCommandCode);
        numberOfBytes += sizeof(unsigned);
        for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
        	numberOfBytes += shardResults.at(shardIdx)->getNumberOfBytes();
        }
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static CommandStatusNotification * deserialize(void* buffer){

    	if(buffer == NULL){
    		ASSERT(false);
    		return NULL;
    	}
    	buffer = ShardingNotification::deserialize(buffer);
        ShardCommandCode commandCode;
        buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);
        CommandStatusNotification * commandStatus = new CommandStatusNotification(commandCode);
        unsigned vectorSize = 0;
        buffer = srch2::util::deserializeFixedTypes(buffer, vectorSize);
        for(unsigned shardIdx = 0; shardIdx < vectorSize ; ++shardIdx){
        	ShardStatus * newShardResult = ShardStatus::deserialize(buffer);
        	buffer = (void*)((char*)buffer +  newShardResult->getNumberOfBytes());
        	commandStatus->shardResults.push_back(newShardResult);
        }
        return commandStatus;
    }

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messageType() const{
        return StatusMessageType;
    }

    ShardCommandCode getCommandCode() const {
        return commandCode;
    }

    vector<ShardStatus *> getShardsStatus() const {
        return shardResults;
    }

    void addShardResult(CommandStatusNotification::ShardStatus * shardResult){
    	shardResults.push_back(shardResult);
    }

private:
    /*
     * commandCode is the code for the request command corresponding to this CommandStatus
     * like insert or update or ...
     */
    ShardCommandCode commandCode;

    vector<CommandStatusNotification::ShardStatus *> shardResults;
};



}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
