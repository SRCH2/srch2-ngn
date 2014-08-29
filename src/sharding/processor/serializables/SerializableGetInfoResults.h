#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_

#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


class GetInfoCommandResults{
public:

	struct ShardResults{
		ShardResults(const string shardIdentifier):shardIdentifier(shardIdentifier){};
		ShardResults(const string shardIdentifier,
							unsigned readCount,
							unsigned writeCount,
							unsigned numberOfDocumentsInIndex,
							string lastMergeTimeString,
							unsigned docCount,
							string versionInfo):shardIdentifier(shardIdentifier){
			this->readCount = readCount;
			this->writeCount = writeCount;
			this->numberOfDocumentsInIndex = numberOfDocumentsInIndex;
			this->lastMergeTimeString = lastMergeTimeString;
			this->docCount = docCount;
			this->versionInfo = versionInfo;
		};
		const string shardIdentifier;
		unsigned readCount;
		unsigned writeCount;
		unsigned numberOfDocumentsInIndex;
		string lastMergeTimeString;
		unsigned docCount;
		string versionInfo;
	    unsigned getNumberOfBytes() const{
	        unsigned numberOfBytes = 0;
	        numberOfBytes += sizeof(unsigned) + shardIdentifier.size();
	        numberOfBytes += sizeof(readCount);
	        numberOfBytes += sizeof(writeCount);
	        numberOfBytes += sizeof(numberOfDocumentsInIndex);
	        numberOfBytes += sizeof(docCount);
	        numberOfBytes += sizeof(unsigned) + lastMergeTimeString.size();
	        numberOfBytes += sizeof(unsigned) + versionInfo.size();
	        return numberOfBytes;
	    }
	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer){
	        // copy data
	        buffer = srch2::util::serializeString(shardIdentifier, buffer);
	        buffer = srch2::util::serializeFixedTypes(readCount, buffer);
	        buffer = srch2::util::serializeFixedTypes(writeCount, buffer);
	        buffer = srch2::util::serializeFixedTypes(numberOfDocumentsInIndex, buffer);
	        buffer = srch2::util::serializeFixedTypes(docCount, buffer);
	        buffer = srch2::util::serializeString(lastMergeTimeString, buffer);
	        buffer = srch2::util::serializeString(versionInfo, buffer);
	        return buffer;
	    }

	    //given a byte stream recreate the original object
	    static ShardResults * deserialize(void* buffer){
	        // read data
	    	string shardIdentifier;
	        buffer = srch2::util::deserializeString(buffer, shardIdentifier);
	        ShardResults * newShardResults = new ShardResults(shardIdentifier);
	        buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->readCount);
	        buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->writeCount);
	        buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->numberOfDocumentsInIndex);
	        buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->docCount);
	        buffer = srch2::util::deserializeString(buffer, newShardResults->lastMergeTimeString);
	        buffer = srch2::util::deserializeString(buffer, newShardResults->versionInfo);
	        // create object and return it
	        return newShardResults;
	    }

	};


    GetInfoCommandResults(){}
    ~GetInfoCommandResults(){
    	for(unsigned infoIdx = 0; infoIdx < shardResults.size() ; ++infoIdx){
    		delete shardResults.at(infoIdx);
    	}
    }

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
        // calculate the number of bytes needed
        unsigned numberOfBytes = getNumberOfBytes();

        // allocate space
        void * buffer = aloc->allocateByteArray(numberOfBytes);
        void * bufferWritePointer = buffer;
        // copy data
        bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)shardResults.size(), bufferWritePointer);
    	for(unsigned infoIdx = 0; infoIdx < shardResults.size() ; ++infoIdx){
    		bufferWritePointer = shardResults.at(infoIdx)->serialize(bufferWritePointer);
    	}
        return buffer;
    }


    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += sizeof(unsigned) ; // number of shard info objects
    	for(unsigned infoIdx = 0; infoIdx < shardResults.size() ; ++infoIdx){
    		numberOfBytes += shardResults.at(infoIdx)->getNumberOfBytes();
    	}
        return numberOfBytes;
    }

    //given a byte stream recreate the original object
    static GetInfoCommandResults * deserialize(void* buffer){
    	GetInfoCommandResults * getInfoCommandResult = new GetInfoCommandResults();
        // read data
    	unsigned numberOfShards = 0;
        buffer = srch2::util::deserializeFixedTypes(buffer, numberOfShards);
        for(unsigned infoIdx = 0; infoIdx < numberOfShards ; ++infoIdx){
        	ShardResults * shardResult = ShardResults::deserialize(buffer);
        	buffer = (void*)((char*)buffer +  shardResult->getNumberOfBytes());
        	getInfoCommandResult->shardResults.push_back(shardResult);
        }
        // create object and return it
        return getInfoCommandResult;
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType(){
        return GetInfoResultsMessageType;
    }


    vector<ShardResults *> getShardResults() const{
    	return shardResults;
    }
    void addShardResults(ShardResults * info){
    	shardResults.push_back(info);
    }

private:
    vector<ShardResults *> shardResults;
};

}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
