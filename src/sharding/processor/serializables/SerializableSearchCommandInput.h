#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_COMMAND_INPUT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/LogicalPlan.h>

namespace srch2 {
namespace httpwrapper {


class SerializableSearchCommandInput{
public:

	SerializableSearchCommandInput(LogicalPlan * logicalPlan){
		this->logicalPlan = logicalPlan;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    static const SerializableSearchCommandInput& deserialize(void*);

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
