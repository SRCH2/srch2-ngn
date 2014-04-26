#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"

namespace srch2 {
namespace httpwrapper {

struct SerializableCommitCommandInput{


    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    const SerializableDeleteCommandInput& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messsageKind();
};

}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMIT_COMMAND_INPUT_H_
