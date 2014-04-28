#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"

namespace srch2 {
namespace httpwrapper {


class SerializableDeleteCommandInput{
public:

	//TODO : primary key might not be enough in case we want to shard based on the value of
	// an expression given in config manager
	SerializableDeleteCommandInput(string primaryKey, unsigned shardingKey){
		this->primaryKey = primaryKey;
		this->shardingKey = shardingKey;
	}

	string getPrimaryKey(){
		return this->primaryKey;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    const SerializableDeleteCommandInput& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messsageKind();
private:

	string primaryKey;
	unsigned shardingKey;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_
