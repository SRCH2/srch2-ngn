#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "server/HTTPJsonResponse.h"
namespace srch2 {
namespace httpwrapper {



class CommandStatus{
public:

	struct ShardResults{
	public:
		ShardResults(const string & shardIdentifiers):shardIdentifier(shardIdentifiers){
			this->messages = Json::Value(Json::arrayValue);
		}

	    /*
	     * Contains the identifier of the shard ...
	     */
	    const string shardIdentifier ;
	    /*
	     * Success => True
	     * Failure => False
	     */
	    bool statusValue;

	    /*
	     * Contains the messages/warnings/errors coming from the shard ...
	     */
	    Json::Value messages;

	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * bufferWritePointer){
	        bufferWritePointer = srch2::util::serializeString(shardIdentifier, bufferWritePointer);
	        bufferWritePointer = srch2::util::serializeFixedTypes(statusValue, bufferWritePointer);
	        bufferWritePointer = srch2::util::serializeString(global_customized_writer.write(messages), bufferWritePointer);
	        return bufferWritePointer;
	    }

	    unsigned getNumberOfBytes() const{
	        unsigned numberOfBytes = 0;
	        numberOfBytes += sizeof(unsigned) + shardIdentifier.size();
	        numberOfBytes += sizeof(unsigned) + global_customized_writer.write(messages).size();
	        numberOfBytes += sizeof(bool);
	        return numberOfBytes;
	    }

	    //given a byte stream recreate the original object
	    static ShardResults * deserialize(void* buffer){

	    	if(buffer == NULL){
	    		ASSERT(false);
	    		return NULL;
	    	}
	        string shardIdentifier;
	        buffer = srch2::util::deserializeString(buffer, shardIdentifier);
	        ShardResults * newShardResult = new ShardResults(shardIdentifier);
	        buffer = srch2::util::deserializeFixedTypes(buffer, newShardResult->statusValue);
	        string messagesStr = NULL;
	        buffer = srch2::util::deserializeString(buffer, messagesStr);
	        Json::Reader reader;
	        reader.parse(messagesStr, newShardResult->messages);
	        // allocate and construct the object
	        return newShardResult;
	    }

		private:
	};

    enum CommandCode{
        DP_INSERT_UPDATE,
        DP_INSERT,
        DP_UPDATE,
        DP_DELETE,
        DP_GET_INFO,
        DP_SERIALIZE,
        DP_SERIALIZE_INDEX,
        DP_SERIALIZE_RECORDS,
        DP_RESET_LOG,
        DP_COMMIT,
        DP_MERGE
    };

    CommandStatus(CommandCode commandCode){
        this->commandCode = commandCode;
    }
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * allocatorObj){
        // calculate the size of object
        unsigned numberOfBytes = getNumberOfBytes();
        // allocate the space
        void * buffer = allocatorObj->allocateMessageReturnBody(numberOfBytes);
        void * bufferWritePointer = buffer;

        bufferWritePointer = srch2::util::serializeFixedTypes(commandCode, bufferWritePointer);
        bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)(shardResults.size()), bufferWritePointer);
        for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
        	bufferWritePointer = shardResults.at(shardIdx)->serialize(bufferWritePointer);
        }
        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(CommandCode);
        numberOfBytes += sizeof(unsigned);
        for(unsigned shardIdx = 0; shardIdx < shardResults.size() ; ++shardIdx){
        	numberOfBytes += shardResults.at(shardIdx)->getNumberOfBytes();
        }
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static CommandStatus * deserialize(void* buffer){

    	if(buffer == NULL){
    		ASSERT(false);
    		return NULL;
    	}

        CommandCode commandCode;
        buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);
        CommandStatus * commandStatus = new CommandStatus(commandCode);
        unsigned vectorSize = 0;
        buffer = srch2::util::deserializeFixedTypes(buffer, vectorSize);
        for(unsigned shardIdx = 0; shardIdx < vectorSize ; ++shardIdx){
        	ShardResults * newShardResult = ShardResults::deserialize(buffer);
        	buffer = (void*)((char*)buffer +  newShardResult->getNumberOfBytes());
        	commandStatus->shardResults.push_back(newShardResult);
        }
        return commandStatus;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return StatusMessageType;
    }

    CommandCode getCommandCode() const {
        return commandCode;
    }

    vector<ShardResults *> getShardResults() const {
        return shardResults;
    }

    void addShardResult(CommandStatus::ShardResults * shardResult){
    	shardResults.push_back(shardResult);
    }

private:
    /*
     * commandCode is the code for the request command corresponding to this CommandStatus
     * like insert or update or ...
     */
    CommandCode commandCode;

    vector<CommandStatus::ShardResults *> shardResults;
};



}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
