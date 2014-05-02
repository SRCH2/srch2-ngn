#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "core/util/SerializationHelper.h"

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

	QueryResults * getQueryResults() const{
		return queryResults;
	}
	QueryResultFactory * getQueryResultsFactory() const{
		return &(*resultsFactory);
	}
	void setSearcherTime(unsigned searcherTime){
		this->searcherTime = searcherTime;
	}
	void setQueryResults(QueryResults * qr){
		this->queryResults = qr ;
	}

	unsigned getSearcherTime() const{
		return searcherTime;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(std::allocator<char> * aloc){
    	if(queryResults == NULL){
    		void * buffer = aloc->allocate(sizeof(bool));
    		void * bufferWritePointer = buffer;
    		bufferWritePointer = srch2::util::serializeFixedTypes(false, bufferWritePointer);
    		return buffer;
    	}
    	// first calculate the number of bytes needed
    	unsigned numberOfBytes = 0;
    	numberOfBytes += sizeof(bool);
    	numberOfBytes += queryResults->getNumberOfBytesForSerializationForNetwork();
    	// allocate space
    	void * buffer = aloc->allocate(numberOfBytes);
    	// serialize
    	void * bufferWritePointer = buffer;
    	bufferWritePointer = srch2::util::serializeFixedTypes(true, bufferWritePointer);
    	bufferWritePointer = queryResults->serializeForNetwork(bufferWritePointer);

    	return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableSearchResults& deserialize(void* buffer){

    	bool isNotNull = false;
		buffer = srch2::util::deserializeFixedTypes(buffer, isNotNull);
		if(isNotNull){
			SerializableSearchResults * searchResults = new SerializableSearchResults();
			buffer = QueryResults::deserializeForNetwork(*(searchResults->queryResults),buffer);
			return *searchResults;
		}else{
			SerializableSearchResults * searchResults = new SerializableSearchResults();
			return *searchResults;
		}
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messsageKind(){
    	return SearchResultsMessageType;
    }

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
