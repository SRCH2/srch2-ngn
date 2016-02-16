/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_
#define __SHARDING_PROCESSOR_SERIALIZABLE_GETINFO_RESULTS_H_

#include "core/operation/IndexHealthInfo.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


class GetInfoCommandResults{
public:

	struct ShardResults{
		ShardResults(ShardId * shardId){
			this->shardId = shardId;
			this->isClusterShard = this->shardId->isClusterShard();
		};
		ShardResults(ShardId * shardId,const IndexHealthInfo & healthInfo){
			this->shardId = shardId;
			this->isClusterShard = this->shardId->isClusterShard();
			this->healthInfo = healthInfo;
		};
		~ShardResults(){
			if(shardId != NULL){
				delete shardId;
			}
		}


		/*
		* NOTE : This constructor is only added for Serialization_Test.cpp
		* Do not use this constructor.
		*/
		ShardResults(ShardId * shardId,
							unsigned readCount,
							unsigned writeCount,
							string lastMergeTimeString,
							unsigned docCount,
							string versionInfo,
							bool isMergeRequired,
							bool isBulkLoadDone){
			this->shardId = shardId;
			this->isClusterShard = this->shardId->isClusterShard();
			this->healthInfo.readCount = readCount;
			this->healthInfo.writeCount = writeCount;
			this->healthInfo.docCount = docCount;
			this->healthInfo.lastMergeTimeString = lastMergeTimeString;
			this->versionInfo = versionInfo;
			this->healthInfo.isMergeRequired = isMergeRequired;
			this->healthInfo.isBulkLoadDone = isBulkLoadDone;
		};
		bool isClusterShard;
		ShardId * shardId;
		// Data shard information
		IndexHealthInfo healthInfo;
		std::string versionInfo;

	    unsigned getNumberOfBytes() const{
	        unsigned numberOfBytes = 0;
	        numberOfBytes += sizeof(isClusterShard);
            if(this->isClusterShard){
                numberOfBytes += ((ClusterShardId *)shardId)->getNumberOfBytes();
            }else{
                numberOfBytes += ((NodeShardId *)shardId)->getNumberOfBytes();
            }
	        numberOfBytes += healthInfo.getNumberOfBytes();
	        numberOfBytes += sizeof(unsigned) + versionInfo.size();
	        return numberOfBytes;
	    }
	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer) const{
	        // copy data
	    	buffer = srch2::util::serializeFixedTypes(isClusterShard, buffer);
	    	if(this->isClusterShard){
				buffer = ((ClusterShardId *)shardId)->serialize(buffer);
	    	}else{
				buffer = ((NodeShardId *)shardId)->serialize(buffer);
	    	}
	        buffer = healthInfo.serialize(buffer);
	        buffer = srch2::util::serializeString(versionInfo, buffer);
	        return buffer;
	    }

	    //given a byte stream recreate the original object
	    static ShardResults * deserialize(void* buffer){
	        // read data
	    	bool isClusterShard;
	    	buffer = srch2::util::deserializeFixedTypes(buffer, isClusterShard);
	    	ShardId * shardId;
	    	if(isClusterShard){
	    		shardId = new ClusterShardId();
				buffer = ((ClusterShardId *)shardId)->deserialize(buffer);
	    	}else{
	    		shardId = new NodeShardId();
				buffer = ((NodeShardId *)shardId)->deserialize(buffer);
	    	}
	        ShardResults * newShardResults = new ShardResults(shardId);
	        newShardResults->isClusterShard = isClusterShard;
	        buffer = newShardResults->healthInfo.deserialize(buffer);
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
        void * buffer = aloc->allocateMessageReturnBody(numberOfBytes);
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
