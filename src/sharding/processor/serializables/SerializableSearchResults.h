#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>

namespace srch2 {
namespace httpwrapper {


struct SerializableSearchResults{

	SerializableSearchResults(){
		queryResults = NULL;
		searcherTime = 0;
	}

	QueryResults * queryResults;
	unsigned searcherTime;
	// extra information to be added later


    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    const SerializableSearchResults& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messsageKind();
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
