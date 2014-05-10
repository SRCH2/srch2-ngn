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
#include "sharding/transport/MessageAllocator.h"

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
	const map<unsigned, string> & getInMemoryRecordStrings() const {
		return inMemoryRecordStrings;
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
    void* serialize(MessageAllocator * aloc){
    	if(queryResults == NULL){
    		void * buffer = aloc->allocate(sizeof(bool));
    		void * bufferWritePointer = buffer;
    		bufferWritePointer = srch2::util::serializeFixedTypes(false, bufferWritePointer);
    		return buffer;
    	}
    	fillInMemoryRecordStrings();
    	// first calculate the number of bytes needed
    	unsigned numberOfBytes = 0;
    	numberOfBytes += sizeof(bool);
    	numberOfBytes += queryResults->getNumberOfBytesForSerializationForNetwork();
    	numberOfBytes += getNumberOfBytesOfInMemoryRecordStrings();
    	// allocate space
    	void * buffer = aloc->allocate(numberOfBytes);
    	// serialize
    	void * bufferWritePointer = buffer;
    	bufferWritePointer = srch2::util::serializeFixedTypes(true, bufferWritePointer);
    	bufferWritePointer = queryResults->serializeForNetwork(bufferWritePointer);
    	bufferWritePointer = serializeInMemoryRecordStrings(buffer);

    	return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableSearchResults& deserialize(void* buffer){

    	bool isNotNull = false;
		buffer = srch2::util::deserializeFixedTypes(buffer, isNotNull);
		if(isNotNull){
			SerializableSearchResults * searchResults = new SerializableSearchResults();
			buffer = QueryResults::deserializeForNetwork(*(searchResults->queryResults),buffer);
			buffer = deserializeInMemoryRecordStrings(buffer,searchResults);
			return *searchResults;
		}else{
			SerializableSearchResults * searchResults = new SerializableSearchResults();
			return *searchResults;
		}
    }

    void fillInMemoryRecordStrings(){
    	if(queryResults == NULL){
    		return;
    	}
    	// iterate on query results and save the inMemoryStrings in the map
    	for(unsigned resultIndex = 0 ; resultIndex < queryResults->getNumberOfResults(); ++resultIndex){
    		inMemoryRecordStrings[queryResults->getInternalRecordId(resultIndex)] =
    				queryResults->getInMemoryRecordString(resultIndex);
    	}
    }

    /*
     * | size of map | id1 | string1 | id2 | string2 | ...
     */
    void * serializeInMemoryRecordStrings(void * buffer){
    	// serialize size of map
    	buffer = srch2::util::serializeFixedTypes(((unsigned)inMemoryRecordStrings.size()), buffer);
    	// serialize map
    	for(map<unsigned,string>::iterator recordDataItr = inMemoryRecordStrings.begin();
    			recordDataItr != inMemoryRecordStrings.end() ; ++recordDataItr){
    		// serialize key
        	buffer = srch2::util::serializeFixedTypes(recordDataItr->first, buffer);
        	// serialize value
        	buffer = srch2::util::serializeString(recordDataItr->second, buffer);
    	}
    	return buffer;
    }

    /*
     * | size of map | id1 | string1 | id2 | string2 | ...
     */
    static void * deserializeInMemoryRecordStrings(void * buffer,SerializableSearchResults * searchResults){
    	// serialize size of map
    	unsigned sizeOfMap = 0;
    	buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap);
    	// serialize map
    	for(unsigned recordDataIndex = 0; recordDataIndex < sizeOfMap ; ++sizeOfMap){
    		// deserialize key
    		unsigned key = 0;
        	buffer = srch2::util::deserializeFixedTypes(buffer, key);
        	// serialize value
        	string value;
        	buffer = srch2::util::deserializeString(buffer, value);
        	searchResults->inMemoryRecordStrings[key] = value;
    	}
    	return buffer;
    }

    /*
     * | size of map | id1 | string1 | id2 | string2 | ...
     */
    unsigned getNumberOfBytesOfInMemoryRecordStrings(){
    	unsigned numberOfBytes = 0;
    	// size of map
    	numberOfBytes += sizeof(unsigned);
    	// map
    	for(map<unsigned,string>::iterator recordDataItr = inMemoryRecordStrings.begin();
    			recordDataItr != inMemoryRecordStrings.end() ; ++recordDataItr){
    		// key
    		numberOfBytes += sizeof(unsigned);
    		// value
    		numberOfBytes += sizeof(unsigned) + recordDataItr->second.size();
    	}
    	return numberOfBytes;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageKind(){
    	return SearchResultsMessageType;
    }

    std::vector<QueryResultPtr> getSortedFinalResults();

  private:
    QueryResults * queryResults;
    map<unsigned,string> inMemoryRecordStrings;
    QueryResultFactory * resultsFactory;
   	// extra information to be added later
	unsigned searcherTime;

};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
