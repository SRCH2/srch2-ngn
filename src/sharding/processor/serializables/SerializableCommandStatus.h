#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"

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
	void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    static const SerializableCommandStatus& deserialize(void*);

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
