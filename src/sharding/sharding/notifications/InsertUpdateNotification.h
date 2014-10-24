#ifndef __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_
#define __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/Record.h>
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "./Notification.h"

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
    InsertUpdateNotification(){
        this->record = NULL;
        this->insertOrUpdate = DP_INSERT;
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
	bool resolveNotification(SP(ShardingNotification) _notif){
		return true;//TODO
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serializeBody(void * buffer) const{
        // and serialize things in calculate
    	buffer = srch2::util::serializeFixedTypes(insertOrUpdate, buffer);
        buffer = record->serializeForNetwork(buffer);
        return buffer;
    }

    unsigned getNumberOfBytesBody() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(OperationCode);
        numberOfBytes += record->getNumberOfBytesSize();
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static InsertUpdateNotification* deserializeBody(void* buffer, const Schema * schema){
        Record * record = new Record(schema);
        OperationCode insertOrUpdate ;
        buffer = srch2::util::deserializeFixedTypes(buffer, insertOrUpdate);
        buffer = Record::deserializeForNetwork(buffer, *record);
        return new InsertUpdateNotification(record, insertOrUpdate);
    }


    InsertUpdateNotification * clone(){
    	return new InsertUpdateNotification(new Record(*this->record), this->insertOrUpdate);
    }

private:
    OperationCode insertOrUpdate;
    Record * record;
};


}
}

#endif // __SHARDING_PROCESSOR_INSERT_UPDATE_NOTIF_H_
