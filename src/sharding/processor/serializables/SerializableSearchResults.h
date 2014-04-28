#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <vector>

namespace srch2 {
namespace httpwrapper {


class QueryResultPtr {
  void *ptr;
  unsigned pos;

public:
  QueryResult operator*();

  //getterMethods;
  TypedValue getResultsScore() const;

  srch2::instantsearch::StoredRecordBuffer getInMemoryData() const;
};

class SerializableSearchResults {
  public:

	SerializableSearchResults(){
		this->queryResults = new QueryResults();
		this->resultsFactory = new QueryResultFactory();
	}

	~SerializableSearchResults(){
		delete this->queryResults;
		delete this->resultsFactory;
	}

	QueryResults * getQueryResults(){
		return &queryResults;
	}
	QueryResultFactory * getQueryResultsFactory(){
		return &resultsFactory;
	}
	void setSearcherTime(unsigned searcherTime){
		this->searcherTime = searcherTime;
	}
	unsigned getSearcherTime(){
		return searcherTime;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char>);

    //given a byte stream recreate the original object
    const SerializableSearchResults& deserialize(void*);

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind();

    std::vector<QueryResultPtr> getSortedFinalResults();

  private:
	QueryResults * queryResults;
    QueryResultFactory * resultsFactory;
   	// extra information to be added later
	unsigned searcherTime;

};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
