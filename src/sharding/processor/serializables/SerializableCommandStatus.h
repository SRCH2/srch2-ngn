#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
namespace srch2 {
namespace httpwrapper {



class CommandStatus{
public:
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
        DP_COMMIT
    };

    CommandStatus(CommandCode commandCode, bool status, string message){
        this->commandCode = commandCode;
        this->status = status;
        this->message = message;
    }
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * allocatorObj){
        // calculate the size of object
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(CommandCode);
        numberOfBytes += sizeof(status);
        numberOfBytes += (sizeof(unsigned) + message.size());
        // allocate the space
        void * buffer = allocatorObj->allocateMessageReturnBody(numberOfBytes);
        void * bufferWritePointer = buffer;

        bufferWritePointer = srch2::util::serializeFixedTypes(commandCode, bufferWritePointer);

        bufferWritePointer = srch2::util::serializeFixedTypes(status, bufferWritePointer);

        bufferWritePointer = srch2::util::serializeString(message, bufferWritePointer);

        return buffer;
    }

    //given a byte stream recreate the original object
    static CommandStatus * deserialize(void* buffer){

    	if(buffer == NULL){
    		ASSERT(false);
    		return NULL;
    	}

        CommandCode commandCode;
        bool status;
        string message;

        buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);

        buffer = srch2::util::deserializeFixedTypes(buffer, status);

        buffer = srch2::util::deserializeString(buffer, message);
        // allocate and construct the object
        CommandStatus * commandStatus = new CommandStatus(commandCode,status, message);
        return commandStatus;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageKind(){
        return StatusMessageType;
    }

    CommandCode getCommandCode() const {
        return commandCode;
    }

    string getMessage() const {
        return message;
    }

    bool getStatus() const {
        return status;
    }

private:
    /*
     * commandCode is the code for the request command corresponding to this CommandStatus
     * like insert or update or ...
     */
    CommandCode commandCode;

    /*
     * Success => True
     * Failure => False
     */
    bool status;

    /*
     * Contains the message coming from the shard ...
     */
    string message ;
};



}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
