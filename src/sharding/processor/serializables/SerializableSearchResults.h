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
#include "highlighter/Highlighter.h"

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

class SearchCommandResults {
public:

	struct ShardResults{
	public:
		ShardResults(const string & shardIdentifier):shardIdentifier(shardIdentifier){
		}
		const string shardIdentifier;
	    map<string, std::pair<string, RecordSnippet> > inMemoryRecordStrings;
	    QueryResults queryResults;
	    QueryResultFactory resultsFactory;
	    // extra information to be added later
	    unsigned searcherTime;


	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer){
	    	buffer = srch2::util::serializeString(shardIdentifier, buffer);
	    	buffer = serializeInMemoryRecordStrings(inMemoryRecordStrings, buffer);
	    	buffer = queryResults.serializeForNetwork(buffer);
	    	buffer = srch2::util::serializeFixedTypes(searcherTime, buffer);
	        return buffer;
	    }

	    unsigned getNumberOfBytes() const{
	        unsigned numberOfBytes = 0;
	        numberOfBytes += sizeof(unsigned) + shardIdentifier.size();
	        numberOfBytes += getNumberOfBytesOfInMemoryRecordStrings(inMemoryRecordStrings);
	        numberOfBytes += queryResults.getNumberOfBytesForSerializationForNetwork();
	        numberOfBytes += sizeof(unsigned);
	        return numberOfBytes;
	    }

	    //given a byte stream recreate the original object
	    static ShardResults * deserialize(void* buffer){

	    	string shardIdentifier ;
	    	buffer = srch2::util::deserializeString(buffer, shardIdentifier);
	    	ShardResults * newShardResults = new ShardResults(shardIdentifier);
	    	buffer = newShardResults->deserializeInMemoryRecordStrings(buffer, newShardResults->inMemoryRecordStrings);
	    	buffer = QueryResults::deserializeForNetwork(newShardResults->queryResults, buffer, &(newShardResults->resultsFactory));
	    	buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->searcherTime);
	        return newShardResults;
	    }


	private:
	    /*
	     * | size of map | id1 | string1 | id2 | string2 | ...
	     */
	    void * serializeInMemoryRecordStrings(map<string, std::pair<string, RecordSnippet> > & inMemoryStrings
	    		,void * buffer){
	        // serialize size of map
	        buffer = srch2::util::serializeFixedTypes(((unsigned)inMemoryStrings.size()), buffer);
	        // serialize map
	        for(map<string, std::pair<string, RecordSnippet> >::iterator recordDataItr = inMemoryStrings.begin();
	                recordDataItr != inMemoryStrings.end() ; ++recordDataItr){
	            // serialize key
	            buffer = srch2::util::serializeString(recordDataItr->first, buffer);
	            // serialize value
	            buffer = srch2::util::serializeString(recordDataItr->second.first, buffer);
	            buffer = recordDataItr->second.second.serializeForNetwork(buffer);
	        }
	        return buffer;
	    }

	    /*
	     * | size of map | id1 | string1 | id2 | string2 | ...
	     */
	    void * deserializeInMemoryRecordStrings(void * buffer,
	    		map<string, std::pair<string, RecordSnippet> > & inMemoryStrings){
	        // serialize size of map
	        unsigned sizeOfMap = 0;
	        buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap);
	        // serialize map
	        for(unsigned recordDataIndex = 0; recordDataIndex < sizeOfMap ; ++recordDataIndex){
	            // deserialize key
	            string key ;
	            buffer = srch2::util::deserializeString(buffer, key);
	            // deserialize value
	            string value;
	            RecordSnippet recSnippet;
	            buffer = srch2::util::deserializeString(buffer, value);
	            buffer =  RecordSnippet::deserializeForNetwork(buffer, recSnippet);
	            inMemoryStrings[key] = std::make_pair(value, recSnippet);

	        }
	        return buffer;
	    }

	    /*
	     * | size of map | id1 | string1 | id2 | string2 | ...
	     */
	    unsigned getNumberOfBytesOfInMemoryRecordStrings(
	    		const map<string, std::pair<string, RecordSnippet> > & inMemoryStrings) const{
	        unsigned numberOfBytes = 0;
	        // size of map
	        numberOfBytes += sizeof(unsigned);
	        // map
	        for(map<string, std::pair<string, RecordSnippet> >::const_iterator recordDataItr = inMemoryStrings.begin();
	                recordDataItr != inMemoryStrings.end() ; ++recordDataItr){
	            // key
	            numberOfBytes += sizeof(unsigned) + recordDataItr->first.size();
	            // value
	            numberOfBytes += sizeof(unsigned) + recordDataItr->second.first.size();
	            numberOfBytes += recordDataItr->second.second.getNumberOfBytesOfSnippets();
	        }
	        return numberOfBytes;
	    }
	};

    SearchCommandResults(){
    }

    ~SearchCommandResults(){
        for(unsigned shardIdx = 0; shardIdx < shardResults.size(); ++shardIdx){
        	delete shardResults.at(shardIdx);
        }

    }

    void addShardResults(SearchCommandResults::ShardResults * shardResults){
    	if(shardResults == NULL){
    		return;
    	}
    	this->shardResults.push_back(shardResults);
    }

    vector<ShardResults *> getShardResults() const{
    	return shardResults;
    }

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
        // first calculate the number of bytes needed
        unsigned numberOfBytes = getNumberOfBytes();
        // allocate space
        void * buffer = aloc->allocateMessageReturnBody(numberOfBytes);
        // serialize
        void * bufferWritePointer = buffer;
        bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)shardResults.size(), bufferWritePointer);
        for(unsigned qrIdx = 0; qrIdx < shardResults.size(); ++qrIdx){
        	bufferWritePointer = shardResults.at(qrIdx)->serialize(bufferWritePointer);
        }
        return buffer;
    }

    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(unsigned);// number of shardresults
        for(unsigned qrIdx = 0; qrIdx < shardResults.size(); ++qrIdx){
        	numberOfBytes += shardResults.at(qrIdx)->getNumberOfBytes();
        }
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static SearchCommandResults * deserialize(void* buffer){
		SearchCommandResults * searchResults = new SearchCommandResults();
		unsigned sizeValue = 0;
		buffer = srch2::util::deserializeFixedTypes(buffer, sizeValue);
		for(unsigned qrIdx = 0 ;  qrIdx < sizeValue; ++qrIdx){
			ShardResults * newShardResults = ShardResults::deserialize(buffer);
			buffer = (void*)((char*)buffer + newShardResults->getNumberOfBytes());
			searchResults->shardResults.push_back(newShardResults);
		}
		return searchResults;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return SearchResultsMessageType;
    }


private:
    vector<ShardResults *> shardResults;
};


}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_SEARCH_RESULTS_H_
