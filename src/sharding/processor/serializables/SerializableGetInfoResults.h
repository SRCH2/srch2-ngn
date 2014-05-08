#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_

#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


class SerializableGetInfoResults{
public:

	SerializableGetInfoResults(	unsigned readCount, unsigned writeCount,	unsigned numberOfDocumentsInIndex,
			string lastMergeTimeString, unsigned docCount, string versionInfo){
		this->readCount = readCount;
		this->writeCount = writeCount;
		this->numberOfDocumentsInIndex = numberOfDocumentsInIndex;
		this->lastMergeTimeString = lastMergeTimeString;
		this->docCount = docCount;
		this->versionInfo = versionInfo;
	}

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(MessageAllocator * aloc){
    	// calculate the number of bytes needed
    	unsigned numberOfBytes = 0;
    	numberOfBytes += sizeof(readCount);
    	numberOfBytes += sizeof(writeCount);
    	numberOfBytes += sizeof(numberOfDocumentsInIndex);
    	numberOfBytes += sizeof(docCount);
    	numberOfBytes += sizeof(unsigned) + lastMergeTimeString.size();
    	numberOfBytes += sizeof(unsigned) + versionInfo.size();

    	// allocate space
    	void * buffer = aloc->allocate(numberOfBytes);
		void * bufferWritePointer = buffer;
    	// copy data
    	bufferWritePointer = srch2::util::serializeFixedTypes(readCount, bufferWritePointer);
    	bufferWritePointer = srch2::util::serializeFixedTypes(writeCount, bufferWritePointer);
    	bufferWritePointer = srch2::util::serializeFixedTypes(numberOfDocumentsInIndex, bufferWritePointer);
    	bufferWritePointer = srch2::util::serializeFixedTypes(docCount, bufferWritePointer);
    	bufferWritePointer = srch2::util::serializeString(lastMergeTimeString, bufferWritePointer);
    	bufferWritePointer = srch2::util::serializeString(versionInfo, bufferWritePointer);

    	return buffer;
    }

    //given a byte stream recreate the original object
    static const SerializableGetInfoResults& deserialize(void* buffer){
    	unsigned readCount;
    	unsigned writeCount;
    	unsigned numberOfDocumentsInIndex;
    	unsigned docCount;
    	string lastMergeTimeString;
    	string versionInfo;
    	// read data
    	buffer = srch2::util::deserializeFixedTypes(buffer, readCount);
    	buffer = srch2::util::deserializeFixedTypes(buffer, writeCount);
    	buffer = srch2::util::deserializeFixedTypes(buffer, numberOfDocumentsInIndex);
    	buffer = srch2::util::deserializeFixedTypes(buffer, docCount);
    	buffer = srch2::util::deserializeString(buffer, lastMergeTimeString);
    	buffer = srch2::util::deserializeString(buffer, versionInfo);
    	// create object and return it
    	return *(new SerializableGetInfoResults(readCount, writeCount, numberOfDocumentsInIndex, lastMergeTimeString, docCount, versionInfo));
    }

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageKind(){
    	return GetInfoResultsMessageType;
    }

	unsigned getDocCount() const {
		return docCount;
	}

	string getLastMergeTimeString() const {
		return lastMergeTimeString;
	}

	unsigned getNumberOfDocumentsInIndex() const {
		return numberOfDocumentsInIndex;
	}

	unsigned getReadCount() const {
		return readCount;
	}

	string getVersionInfo() const {
		return versionInfo;
	}

	unsigned getWriteCount() const {
		return writeCount;
	}

private:
	unsigned readCount;
	unsigned writeCount;
	unsigned numberOfDocumentsInIndex;
	string lastMergeTimeString;
	unsigned docCount;
	string versionInfo;
};

}
}

#endif // __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
