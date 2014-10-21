#ifndef __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_
#define __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/Record.h>
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {


class InsertUpdateNotification : public ShardingNotification{
public:
    enum OperationCode{
        DP_INSERT,
        DP_UPDATE
    };



    ShardingMessageType messageType() const{
    	return InsertUpdateCommandMessageType;
    }
    InsertUpdateNotification(Record * record, OperationCode insertOrUpdate){
        this->record = record;
        this->insertOrUpdate = insertOrUpdate;
    }
    ~InsertUpdateNotification(){
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
    void* serialize(void * buffer){
        // and serialize things in calculate
    	buffer = ShardingNotification::serialize(buffer);
    	buffer = srch2::util::serializeFixedTypes(insertOrUpdate, buffer);
        buffer = record->serializeForNetwork(buffer);
        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
    	numberOfBytes += ShardingNotification::getNumberOfBytes();
        numberOfBytes += sizeof(OperationCode);
        numberOfBytes += record->getNumberOfBytesSize();
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static InsertUpdateNotification* deserialize(void* buffer, const Schema * schema){
    	buffer = ShardingNotification::deserialize(buffer);
        Record * record = new Record(schema);
        OperationCode insertOrUpdate ;
        buffer = srch2::util::deserializeFixedTypes(buffer, insertOrUpdate);
        buffer = Record::deserializeForNetwork(buffer, *record);
        return new InsertUpdateNotification(record, insertOrUpdate);
    }


    InsertUpdateNotification * clone(){
    	return new InsertUpdateNotification(new Record(*this->record), this->insertOrUpdate);
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

#endif // __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_
