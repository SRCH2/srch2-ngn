#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/LogicalPlan.h>
#include "core/util/SerializationHelper.h"

namespace srch2 {
namespace httpwrapper {


class SerializableSearchCommandInput{
public:

	SerializableSearchCommandInput(LogicalPlan * logicalPlan){
		this->logicalPlan = logicalPlan;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
	/*
	 * Serialization scheme:
	 * | isLogicalPlanNULL | LogicalPlan(only is isLogicalPlanNULL is true) |
	 */
    void* serialize(std::allocator<char> * aloc){

    	if(logicalPlan == NULL){
    		void * buffer = aloc->allocate(sizeof(bool));
    		void * bufferWritePointer = buffer;
    		bufferWritePointer = srch2::util::serializeFixedTypes(false, bufferWritePointer); // NULL
    		return buffer;
    	}
    	//first calculate the number of bytes needed for serializing logical plan
    	unsigned numberOfBytes = 0;
    	numberOfBytes += sizeof(bool); // Not NULL
    	numberOfBytes += logicalPlan->getNumberOfBytesForSerializationForNetwork();
    	// allocate the space
    	void * buffer = aloc->allocate(numberOfBytes);
    	// serialize logical plan into buffer
		void * bufferWritePointer = buffer;
		bufferWritePointer = srch2::util::serializeFixedTypes(true, bufferWritePointer); // not NULL
		bufferWritePointer = logicalPlan->serializeForNetwork(bufferWritePointer);
		return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableSearchCommandInput& deserialize(void* buffer){
    	SerializableSearchCommandInput * searchInput = new SerializableSearchCommandInput(NULL);

    	bool isNotNull = false;
    	buffer = srch2::util::deserializeFixedTypes(buffer, isNotNull);
    	if(isNotNull){
    		searchInput->logicalPlan = new LogicalPlan();
    		buffer = LogicalPlan::deserializeForNetwork(*searchInput->logicalPlan, buffer);
    		return *searchInput;
    	}else{
    		return *searchInput;
    	}
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
    	return SearchCommandMessageType;
    }

	LogicalPlan * getLogicalPlan(){
		return logicalPlan;
	}
private:
	LogicalPlan * logicalPlan;
	// extra information if needed
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_
