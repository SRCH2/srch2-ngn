#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SERIALIZE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {


class SerializeCommand{
public:
    enum OperationCode{
        SERIALIZE_INDEX,
        SERIALIZE_RECORDS
    };
    SerializeCommand(OperationCode code){
        this->indexOrRecord = code;
        this->dataFileName = "";
    }
    SerializeCommand(OperationCode code, const string &dataFileName){
        this->indexOrRecord = code;
        this->dataFileName = dataFileName;
    }

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
        // calculate number of needed bytes
        unsigned numberOfBytes = getNumberOfBytes();
        // allocate space
        void * buffer = aloc->allocateByteArray(numberOfBytes);
        // serialize data now
        void * bufferWritePointer = buffer;
        bufferWritePointer = srch2::util::serializeFixedTypes(indexOrRecord, bufferWritePointer);
        if(indexOrRecord == SERIALIZE_RECORDS){
            bufferWritePointer = srch2::util::serializeString(dataFileName, bufferWritePointer);
        }

        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(indexOrRecord);
        if(indexOrRecord == SERIALIZE_RECORDS){
            numberOfBytes += sizeof(unsigned) + dataFileName.size();
        }
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static SerializeCommand* deserialize(void* buffer){
        OperationCode indexOrRecord;
        string dataFileName;
        buffer = srch2::util::deserializeFixedTypes(buffer, indexOrRecord);
        if(indexOrRecord == SERIALIZE_RECORDS){
            buffer = srch2::util::deserializeString(buffer, dataFileName);
            return new SerializeCommand(indexOrRecord, dataFileName);
        }
        return new SerializeCommand(indexOrRecord);
    }

    SerializeCommand * clone(){
    	return new SerializeCommand(this->indexOrRecord, this->dataFileName);
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return SerializeCommandMessageType;
    }

    string getDataFileName() const {
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
