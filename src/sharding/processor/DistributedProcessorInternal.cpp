#include "DistributedProcessorInternal.h"




namespace srch2is = srch2::instantsearch;
using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

/*
 * 1. Receives a search request from a shard
 * 2. Uses core to evaluate this search query
 * 3. Sends the results to the shard which initiated this search query
 */
void DPInternalRequestHandler::internalSearchCommand(const RoutingManager::LogicalPlanMessage * msg,
											Srch2Server * server, RoutingManager::Message * responseMsg){

	// first find the search URL
	LogicalPlan logicalPlan;
	RoutingManager::LogicalPlanMessage::parseLogicalPlanMessage(logicalPlan);


	// search in core
    srch2is::QueryResultFactory * resultsFactory =
            new srch2is::QueryResultFactory();
    // TODO : is it possible to make executor and planGen singleton ?
    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;
    QueryExecutor qe(logicalPlan, resultsFactory, server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    QueryResults * finalResults = new QueryResults();
    qe.execute(finalResults);

	// prepare the QueryResultsMessage
	responseMsg = RoutingManager::QueryResultsMessage::buildQueryResultsMessage(finalResults);

}

/*
 * This call back is always called for insert and update, it will use
 * internalInsertCommand and internalUpdateCommand
 */
void DPInternalRequestHandler::internalInsertUpdateCommand(){

}

/*
 * 1. Receives an insert request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this insert query
 * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
 */
void DPInternalRequestHandler::internalInsertCommand(RoutingManager::HTTPInsertRequestMessage * msg, unsigned sourceShardID){

	// first find the insert URL
	evhttp_request * req = RoutingManager::HTTPInsertRequestMessage::parseHTTPRequestMessage(msg);

	// insert in core

	// ....

	// get results from core
	bool success = true; // ...


	// prepare the QueryResultsMessage
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(success);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}

/*
 * 1. Receives a delete request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this delete query
 * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
 */
void DPInternalRequestHandler::internalDeleteCommand(RoutingManager::HTTPDeleteRequestMessage * msg, unsigned sourceShardID){
	// first find the delete URL
	evhttp_request * req = RoutingManager::HTTPDeleteRequestMessage::parseHTTPRequestMessage(msg);

	// delete in core

	// ....

	// get results from core
	bool success = true; // ...


	// prepare the QueryResultsMessage
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(success);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}

/*
 * 1. Receives a update request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this update query
 * 3. Sends the results to the shard which initiated this update request (Failure or Success)
 */
void DPInternalRequestHandler::internalUpdateCommand(RoutingManager::HTTPUpdateRequestMessage * msg, unsigned sourceShardID){
	// first find the update URL
	evhttp_request * req = RoutingManager::HTTPUpdateRequestMessage::parseHTTPRequestMessage(msg);

	// update in core

	// ....

	// get results from core
	bool success = true; // ...


	// prepare the QueryResultsMessage
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(success);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}


/*
 * 1. Receives a GetInfo request from a shard
 * 2. Uses core to get info
 * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
 */
void DPInternalRequestHandler::internalGetInfoCommand(RoutingManager::GetInfoMessage * msg, unsigned sourceShardID){
	// first get the URL for request
	evhttp_request * req = RoutingManager::GetInfoMessage::parseGetInfoMessage(msg);

	// get info from core
	// ...

	unsigned info = 0; // example

	// prepare the response message
	RoutingManager::Message * infoMsg = RoutingManager::CoreInfoMessage::buildCoreInfoMessage(info);

	// send this message back to the source
	routingManager->route(infoMsg, sourceShardID);

}


/*
 * This call back function is called for serialization. It uses internalSerializeIndexCommand
 * and internalSerializeRecordsCommand for our two types of serialization.
 */
void DPInternalRequestHandler::internalSerializeCommand(){

}


/*
 * 1. Receives a SerializeIndex request from a shard
 * 2. Uses core to do the serialization
 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
 */
void DPInternalRequestHandler::internalSerializeIndexCommand(RoutingManager::SerializeIndexMessage * msg, unsigned sourceShardID){
	// first get the URL for request
	evhttp_request * req = RoutingManager::SerializeIndexMessage::parseSerializeIndexMessage(msg);

	// serialize indexes in core
	// ...

	bool status = true; // example

	// prepare the response message
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(status);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}

/*
 * 1. Receives a SerializeRecords request from a shard
 * 2. Uses core to do the serialization
 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
 */
void DPInternalRequestHandler::internalSerializeRecordsCommand(RoutingManager::SerializeRecordsMessage * msg, unsigned sourceShardID){
	// first get the URL for request
	evhttp_request * req = RoutingManager::SerializeRecordsMessage::parseSerializeRecordsMessage(msg);

	// serialize records in core
	// ...

	bool status = true; // example

	// prepare the response message
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(status);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}

/*
 * 1. Receives a ResetLog request from a shard
 * 2. Uses core to reset log
 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
 */
void DPInternalRequestHandler::internalResetLogCommand(RoutingManager::ResetLogMessage* msg, unsigned sourceShardID){
	// first get the URL for request
	evhttp_request * req = RoutingManager::ResetLogMessage::parseResetLogMessage(msg);

	// serialize records in core
	// ...

	bool status = true; // example

	// prepare the response message
	RoutingManager::Message * statusMsg = RoutingManager::StatusMessage::buildStatusMessage(status);

	// send this message back to the source
	routingManager->route(statusMsg, sourceShardID);
}


}
}
