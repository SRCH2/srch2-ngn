#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/LogicalPlan.h>
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {


class SearchCommand{
public:

    SearchCommand(LogicalPlan * logicalPlan){
        this->logicalPlan = logicalPlan;
    }

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    /*
     * Serialization scheme:
     * | isLogicalPlanNULL | LogicalPlan(only is isLogicalPlanNULL is true) |
     */
    void* serialize(MessageAllocator * aloc){

        if(logicalPlan == NULL){
        	ASSERT(false);
            void * buffer = aloc->allocateMessageReturnBody(sizeof(bool));
            void * bufferWritePointer = buffer;
            bufferWritePointer = srch2::util::serializeFixedTypes(false, bufferWritePointer); // NULL
            return buffer;
        }
        //first calculate the number of bytes needed for serializing logical plan
        unsigned numberOfBytes = getNumberOfBytes();
        // allocate the space
        void * buffer = aloc->allocateMessageReturnBody(numberOfBytes);
        // serialize logical plan into buffer
        void * bufferWritePointer = buffer;
        bufferWritePointer = srch2::util::serializeFixedTypes(true, bufferWritePointer); // not NULL
        bufferWritePointer = logicalPlan->serializeForNetwork(bufferWritePointer);
        return buffer;
    }


    unsigned getNumberOfBytes() const{
    	if(logicalPlan == NULL){
    		ASSERT(false);
    		return 0;
    	}
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(bool); // Not NULL
        numberOfBytes += logicalPlan->getNumberOfBytesForSerializationForNetwork();
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static SearchCommand * deserialize(void* buffer){
        SearchCommand * searchInput = new SearchCommand(NULL);

        bool isNotNull = false;
        buffer = srch2::util::deserializeFixedTypes(buffer, isNotNull);
        if(isNotNull){
            searchInput->logicalPlan = new LogicalPlan();
            buffer = LogicalPlan::deserializeForNetwork(*searchInput->logicalPlan, buffer);
            return searchInput;
        }else{
            return searchInput;
        }
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return SearchCommandMessageType;
    }

    LogicalPlan * getLogicalPlan() const{
        return logicalPlan;
    }
private:
    LogicalPlan * logicalPlan;
    // extra information if needed
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_
