#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"

namespace srch2 {
namespace httpwrapper {



struct SerializableCommandStatus{
	unsigned commandNumber; // 0->insert, 1->update, 2->delete, 3->getInfo, 4->serializeIndex, 5->serializeRecords, 6->resetting logs
	bool status;
    string message ;

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    const SerializableCommandStatus& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messsageKind();

};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_COMMAND_STATUS_H_
