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
