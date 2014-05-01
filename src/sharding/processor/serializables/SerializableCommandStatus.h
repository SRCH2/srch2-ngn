#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {



class SerializableCommandStatus{
public:
	enum CommandCode{
		INSERT_UPDATE,
		INSERT,
		UPDATE,
		DELETE,
		GET_INFO,
		SERIALIZE,
		SERIALIZE_INDEX,
		SERIALIZE_RECORDS,
		RESET_LOG,
		COMMIT
	};

	SerializableCommandStatus(	CommandCode commandCode, bool status, string message){
		this->commandCode = commandCode;
		this->status = status;
		this->message = message;
	}
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
	void* serialize(std::allocator<char> * allocatorObj){
		// calculate the size of object
		unsigned numberOfBytes = 0;
		numberOfBytes += sizeof(CommandCode);
		numberOfBytes += sizeof(status);
		numberOfBytes += (sizeof(unsigned) + message.size());
		// allocate the space
		void * buffer = allocatorObj->allocate(numberOfBytes);
		void * bufferWritePointer = buffer;

		bufferWritePointer = srch2::util::serializeFixedTypes(commandCode, bufferWritePointer);

		bufferWritePointer = srch2::util::serializeFixedTypes(status, bufferWritePointer);

		bufferWritePointer = srch2::util::serializeString(message, bufferWritePointer);

		return buffer;
	}

    //given a byte stream recreate the original object
    static const SerializableCommandStatus& deserialize(void* buffer){
    	CommandCode commandCode;
    	bool status;
    	string message;

    	buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);

    	buffer = srch2::util::deserializeFixedTypes(buffer, status);

    	buffer = srch2::util::deserializeString(buffer, message);
    	// allocate and construct the object
    	SerializableCommandStatus * commandStatus = new SerializableCommandStatus(commandCode,status, message);
    	return *commandStatus;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
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
	CommandCode commandCode;
	bool status;
	string message ;
};



}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
