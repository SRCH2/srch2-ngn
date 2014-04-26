#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


struct SerializableGetInfoResults{

	unsigned readCount;
	unsigned writeCount;
	unsigned numberOfDocumentsInIndex;
	string lastMergeTimeString;
	unsigned docCount;
	string versionInfo;

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

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
