#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {


class SerializableSerializeCommandInput{
public:
	enum OperationCode{
		SERIALIZE_INDEX,
		SERIALIZE_RECORDS
	};
	SerializableSerializeCommandInput(OperationCode code){
		ASSERT(code == SERIALIZE_INDEX);
		this->indexOrRecord = code;
		this->dataFileName = "";
	}
	SerializableSerializeCommandInput(OperationCode code, const string &dataFileName){
		ASSERT(code == SERIALIZE_RECORDS);
		this->indexOrRecord = code;
		this->dataFileName = dataFileName;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char> * aloc){
    	// calculate number of needed bytes
    	unsigned numberOfBytes = 0;
    	numberOfBytes += sizeof(indexOrRecord);
    	if(indexOrRecord == SERIALIZE_RECORDS){
    		numberOfBytes += sizeof(unsigned) + dataFileName.size();
    	}
    	// allocate space
    	void * buffer = aloc->allocate(numberOfBytes);
    	// serialize data now
		void * bufferWritePointer = buffer;
		bufferWritePointer = srch2::util::serializeFixedTypes(indexOrRecord, bufferWritePointer);
    	if(indexOrRecord == SERIALIZE_RECORDS){
			bufferWritePointer = srch2::util::serializeString(dataFileName, bufferWritePointer);
    	}

		return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableSerializeCommandInput& deserialize(void* buffer){
    	OperationCode indexOrRecord;
    	string dataFileName;
    	buffer = srch2::util::deserializeFixedTypes(buffer, indexOrRecord);
    	if(indexOrRecord == SERIALIZE_RECORDS){
			buffer = srch2::util::deserializeString(buffer, dataFileName);
			return *(new SerializableSerializeCommandInput(indexOrRecord, dataFileName));
    	}
    	return *(new SerializableSerializeCommandInput(indexOrRecord));
    }

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
	OperationCode indexOrRecord;
	string dataFileName;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_
