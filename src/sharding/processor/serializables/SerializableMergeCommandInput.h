#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_MERGE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_MERGE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {

class MergeCommand{
public:

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
    return aloc->allocateMessageReturnBody(getNumberOfBytes());
    }

    unsigned getNumberOfBytes() const{
        return 0;
    }


    //given a byte stream recreate the original object
    static MergeCommand * deserialize(void*){
    return new MergeCommand();
    }


    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
    return MergeCommandMessageType;
    }
};

}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_Merge_COMMAND_INPUT_H_
