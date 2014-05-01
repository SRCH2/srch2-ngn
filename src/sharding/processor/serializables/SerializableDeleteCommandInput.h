#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"

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

	string getPrimaryKey() const{
		return this->primaryKey;
	}
	unsigned getShardingKey() const{
		return this->shardingKey;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char> * aloc){
    	// calculate the needed size
    	unsigned numberOfBytes = 0 ;
    	numberOfBytes += sizeof(shardingKey);
    	numberOfBytes += sizeof(unsigned) + primaryKey.size();
    	// allocate memory
    	void * buffer = aloc->allocate(numberOfBytes);
    	// copy data
		void * bufferWritePointer = buffer;

		bufferWritePointer = srch2::util::serializeFixedTypes(shardingKey, bufferWritePointer);
		bufferWritePointer = srch2::util::serializeString(primaryKey, bufferWritePointer);

		return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableDeleteCommandInput& deserialize(void* buffer){

    	unsigned shardingKey;
    	string primaryKey;

    	buffer = srch2::util::deserializeFixedTypes(buffer, shardingKey);
    	buffer = srch2::util::deserializeString(buffer, primaryKey);

    	return *(new SerializableDeleteCommandInput(primaryKey, shardingKey));
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
    	return DeleteCommandMessageType;
    }
private:

	string primaryKey;
	unsigned shardingKey;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_DELETE_COMMAND_INPUT_H_
