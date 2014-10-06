#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {



class GetInfoCommand{
public:
	GetInfoCommand(GetInfoRequestType type = GetInfoRequestType_){
		this->type = type;
	}
    // we don't need anything in this class for now

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
        // calculate the number of bytes needed
        unsigned numberOfBytes = getNumberOfBytes();

        // allocate space
        void * buffer = aloc->allocateByteArray(numberOfBytes);
        void * bufferWritePointer = buffer;
        // copy data
        bufferWritePointer = srch2::util::serializeFixedTypes(this->type, buffer);
        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0 ;
        numberOfBytes += sizeof(this->type);
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static GetInfoCommand * deserialize(void* buffer){
    	GetInfoRequestType type;
    	buffer = srch2::util::deserializeFixedTypes(buffer, type);
        return new GetInfoCommand(type);
    }

    GetInfoCommand * clone(){
    	return new GetInfoCommand(this->type);
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return GetInfoCommandMessageType;
    }

    GetInfoRequestType getType() const{
    	return this->type;
    }

private:
    GetInfoRequestType type;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_
