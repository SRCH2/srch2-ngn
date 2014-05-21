#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {

class SerializableCommitCommandInput{
public:

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
    	return aloc->allocateMessageReturnBody(0);
    }

    //given a byte stream recreate the original object
    static SerializableCommitCommandInput * deserialize(void*){
    	return new SerializableCommitCommandInput();
    }


    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageKind(){
    	return CommitCommandMessageType;
    }
};

}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_
