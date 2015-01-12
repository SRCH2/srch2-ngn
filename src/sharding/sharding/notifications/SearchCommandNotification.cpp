#include "SearchCommandNotification.h"

#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/LogicalPlan.h>
#include "core/util/SerializationHelper.h"
#include "sharding/sharding/notifications/Notification.h"
#include "sharding/sharding/ShardManager.h"
#include "../state_machine/StateMachine.h"
#include "SearchCommandResultsNotification.h"
#include "sharding/processor/DistributedProcessorInternal.h"

namespace srch2is = srch2::instantsearch;
using namespace std;
namespace srch2 {
namespace httpwrapper {

SearchCommand::SearchCommand(unsigned coreId, LogicalPlan * logicalPlan, NodeTargetShardInfo target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){
	this->coreId = coreId;
    this->logicalPlan = logicalPlan;
    this->target = target;
    this->clusterReadview = clusterReadview;
    this->schema = NULL;
}
SearchCommand::SearchCommand(){
	this->coreId = (unsigned)-1;
	this->logicalPlan = NULL;
	this->schema = NULL;
	ShardManager::getReadview(this->clusterReadview);
}

SearchCommand::~SearchCommand(){
	if(isCreatedByDeserialization()){
		if(logicalPlan != NULL){
			delete logicalPlan;
		}
	}
}

//serializes the object to a byte array and places array into the region
//allocated by given allocator
/*
 * Serialization scheme:
 * | isLogicalPlanNULL | LogicalPlan(only is isLogicalPlanNULL is true) |
 */
void* SearchCommand::serializeBody(void * buffer) const{
    buffer = srch2::util::serializeFixedTypes(coreId, buffer);

    buffer = srch2::util::serializeFixedTypes((bool)(logicalPlan == NULL), buffer);
    if(logicalPlan != NULL){
		buffer = logicalPlan->serializeForNetwork(buffer);
    }
    buffer = target.serialize(buffer);
    return buffer;
}


unsigned SearchCommand::getNumberOfBytesBody() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(coreId);
    bool boolVar;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(boolVar); // NULL or Not NULL
    if(logicalPlan != NULL){
		numberOfBytes += logicalPlan->getNumberOfBytesForSerializationForNetwork();
    }
    numberOfBytes += target.getNumberOfBytes();
    return numberOfBytes;
}

//NOTE : schema must be set before this method is called, otherwise the logical plan
//       wont be deserialized
void * SearchCommand::deserializeBody(void* buffer){

    buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
    bool isNull = false;
    buffer = srch2::util::deserializeFixedTypes(buffer, isNull);

    const CoreInfo_t * coreInfo = clusterReadview->getCore(coreId);
	if(coreInfo != NULL){
		this->setSchema(coreInfo->getSchema());
	}else{
		this->setSchema(NULL);
	}

    if(! isNull || schema == NULL){
        this->logicalPlan = new LogicalPlan();
        buffer = LogicalPlan::deserializeForNetwork(*(this->logicalPlan), buffer, schema);
    }else{
    	this->logicalPlan = NULL;
    }
    buffer = target.deserialize(buffer);
	return buffer;
}

//Returns the type of message which uses this kind of object as transport
ShardingMessageType SearchCommand::messageType() const {
    return ShardingSearchCommandMessageType;
}


bool SearchCommand::resolveNotification(SP(ShardingNotification) _notif){
	SP(SearchCommandResults) response =
			ShardManager::getShardManager()->getDPInternal()->
			internalSearchCommand(boost::dynamic_pointer_cast<SearchCommand>(_notif));
	if(! response){
		response = create<SearchCommandResults>();
	}
    response->setSrc(_notif->getDest());
    response->setDest(_notif->getSrc());
	send(response);
	return true;
}

boost::shared_ptr<const ClusterResourceMetadata_Readview> SearchCommand::getReadview() const{
    return clusterReadview;
}
const NodeTargetShardInfo & SearchCommand::getTarget() const{
    return target;
}
LogicalPlan * SearchCommand::getLogicalPlan() const{
    return logicalPlan;
}
void SearchCommand::setSchema(const Schema * schema){
	this->schema = schema;
}
bool SearchCommand::isCreatedByDeserialization(){
	return (ShardManager::getCurrentNodeId() != this->getSrc().nodeId);
}

}
}
