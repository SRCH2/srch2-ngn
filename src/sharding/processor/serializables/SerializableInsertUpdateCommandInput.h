#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_INSERT_UPDATE_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_INSERT_UPDATE_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/Record.h>
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {


class InsertUpdateCommand{
public:
    enum OperationCode{
        DP_INSERT,
        DP_UPDATE
    };


    InsertUpdateCommand(Record * record, OperationCode insertOrUpdate){
        this->record = record;
        this->insertOrUpdate = insertOrUpdate;
    }
    ~InsertUpdateCommand(){
        delete record;
    }

    OperationCode getInsertOrUpdate() const{
        return insertOrUpdate;
    }
    srch2is::Record * getRecord() const{
        return this->record;
    }
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
        ASSERT(record != NULL);
        // calculate the size
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(OperationCode);
        numberOfBytes += record->getNumberOfBytesSize();
        // allocate the space
        void * buffer = aloc->allocateMessageReturnBody(numberOfBytes);
        void * bufferWritePointer = buffer;
        // and serialize things in calculate
        bufferWritePointer = srch2::util::serializeFixedTypes(insertOrUpdate, bufferWritePointer);
        bufferWritePointer = record->serializeForNetwork(bufferWritePointer);

        return buffer;
    }

    //given a byte stream recreate the original object
    static InsertUpdateCommand* deserialize(void* buffer, const Schema * schema){
        Record * record = new Record(schema);
        OperationCode insertOrUpdate ;
        buffer = srch2::util::deserializeFixedTypes(buffer, insertOrUpdate);
        buffer = Record::deserializeForNetwork(buffer, *record);
        return new InsertUpdateCommand(record, insertOrUpdate);
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return InsertUpdateCommandMessageType;
    }

private:
    OperationCode insertOrUpdate;
    Record * record;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_INSERT_UPDATE_COMMAND_INPUT_H_
