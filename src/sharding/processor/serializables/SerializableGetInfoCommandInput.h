#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {



class SerializableGetInfoCommandInput{
public:
	// we don't need anything in this class for now

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char> * aloc){
    	return aloc->allocate(0);
    }

    //given a byte stream recreate the original object
    static const SerializableGetInfoCommandInput& deserialize(void* buffer){
    	return *(new SerializableGetInfoCommandInput());
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
    	return GetInfoCommandMessageType;
    }
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_COMMAND_INPUT_H_
