#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"

namespace srch2 {
namespace httpwrapper {


class SerializableSerializeCommandInput{
public:
	enum OperationCode{
		SERIALIZE_INDEX,
		SERIALIZE_RECORDS
	};
	SerializableSerializeCommandInput(OperationCode code){
		this->indexOrRecord = code;
		this->dataFileName = "";
	}
	SerializableSerializeCommandInput(OperationCode code, const string &dataFileName){
		this->indexOrRecord = code;
		this->dataFileName = dataFileName;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    static const SerializableSerializeCommandInput& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
    	return SerializeCommandMessageType;
    }

	string getDataFileName() const {
		ASSERT(indexOrRecord == SERIALIZE_RECORDS);
		return dataFileName;
	}

	OperationCode getIndexOrRecord() const {
		return indexOrRecord;
	}

private:
	OperationCode indexOrRecord; // true => index, false=> records
	string dataFileName;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_
