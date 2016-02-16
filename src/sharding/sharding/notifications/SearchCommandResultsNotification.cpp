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
#include "SearchCommandResultsNotification.h"

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "core/util/SerializationHelper.h"
#include "highlighter/Highlighter.h"
#include "sharding/sharding/notifications/Notification.h"
#include "../state_machine/StateMachine.h"
#include "sharding/sharding/ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;
namespace srch2 {
namespace httpwrapper {


void* SearchCommandResults::ShardResults::serialize(void * buffer){
	buffer = srch2::util::serializeString(shardIdentifier, buffer);
	buffer = queryResults.serializeForNetwork(buffer);
	buffer = srch2::util::serializeFixedTypes(searcherTime, buffer);
    return buffer;
}

unsigned SearchCommandResults::ShardResults::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += srch2::util::getNumberOfBytesString(shardIdentifier);
    numberOfBytes += queryResults.getNumberOfBytesForSerializationForNetwork();
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(searcherTime);
    return numberOfBytes;
}

//given a byte stream recreate the original object
SearchCommandResults::ShardResults *
	SearchCommandResults::ShardResults::deserialize(void* buffer){

	string shardIdentifier ;
	buffer = srch2::util::deserializeString(buffer, shardIdentifier);
	ShardResults * newShardResults = new ShardResults(shardIdentifier);
	buffer = QueryResults::deserializeForNetwork(newShardResults->queryResults, buffer, &(newShardResults->resultsFactory));
	buffer = srch2::util::deserializeFixedTypes(buffer, newShardResults->searcherTime);
    return newShardResults;
}


SearchCommandResults::~SearchCommandResults(){
    for(unsigned shardIdx = 0; shardIdx < shardResults.size(); ++shardIdx){
    	delete shardResults.at(shardIdx);
    }

}

void SearchCommandResults::addShardResults(SearchCommandResults::ShardResults * shardResults){
	if(shardResults == NULL){
		return;
	}
	this->shardResults.push_back(shardResults);
}

vector<SearchCommandResults::ShardResults *> & SearchCommandResults::getShardResults(){
	return shardResults;
}

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* SearchCommandResults::serializeBody(void * buffer) const{
    buffer = srch2::util::serializeFixedTypes((uint32_t)shardResults.size(), buffer);
    for(unsigned qrIdx = 0; qrIdx < shardResults.size(); ++qrIdx){
    	buffer = shardResults.at(qrIdx)->serialize(buffer);
    }
    return buffer;
}

unsigned SearchCommandResults::getNumberOfBytesBody() const{
    unsigned numberOfBytes = 0;
    uint32_t intVar;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(intVar);// number of shardresults
    for(unsigned qrIdx = 0; qrIdx < shardResults.size(); ++qrIdx){
    	numberOfBytes += shardResults.at(qrIdx)->getNumberOfBytes();
    }
    return numberOfBytes;
}

//given a byte stream recreate the original object
void * SearchCommandResults::deserializeBody(void * buffer){
	uint32_t sizeValue = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, sizeValue);
	for(unsigned qrIdx = 0 ;  qrIdx < sizeValue; ++qrIdx){
		ShardResults * newShardResults = ShardResults::deserialize(buffer);
		buffer = (void*)((char*)buffer + newShardResults->getNumberOfBytes());
		this->shardResults.push_back(newShardResults);
	}
	return buffer;
}

ShardingMessageType SearchCommandResults::messageType() const{
	return ShardingSearchResultsMessageType;
}

bool SearchCommandResults::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getStateMachine()->handle(_notif);
	return true;
}

}
}
