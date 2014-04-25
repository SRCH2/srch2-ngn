

#include "DistributedProcessorExternal.h"



namespace srch2is = srch2::instantsearch;
using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

//###########################################################################
//                       External Distributed Processor
//###########################################################################
/*
 * 1. Receives a search request from a client (not from another shard)
 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
 * 3. Gives ResultAggregator functions as callback function to TransportationManager
 * 4. ResultAggregator callback functions will aggregate the results and print them on
 *    http channel
 */
void DPExternalRequestHandler::externalSearchCommand(evhttp_request *req , CoreShardInfo * coreShardInfo){

    // configuration object is retrieved from ConfigManager by passing coreShardInfo
    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;

    ResultAggregatorAndPrint * resultAggregator = new ResultAggregatorAndPrint(routingManager , configurationManager,req, coreShardInfo);

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
    //1. first create query parser to parse the url
    QueryParser qp(headers, resultAggregator->getParamContainer());
    bool isSyntaxValid = qp.parse();
    if (!isSyntaxValid) {
        // if the query is not valid print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
        		resultAggregator->getParamContainer()->getMessageString(), headers);
        evhttp_clear_headers(&headers);
        return;
    }

    //2. validate the query
    QueryValidator qv(*(server->indexer->getSchema()),
            *(server->indexDataConfig), resultAggregator->getParamContainer());

    bool valid = qv.validate();

    if (!valid) {
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
        		resultAggregator->getParamContainer()->getMessageString(), headers);
        evhttp_clear_headers(&headers);
        return;
    }
    //3. rewrite the query and apply analyzer and other stuff ...
    QueryRewriter qr(server->indexDataConfig,
            *(server->indexer->getSchema()),
            *(AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf)),
            resultAggregator->getParamContainer());

    LogicalPlan * logicalPlan = new LogicalPlan();
    resultAggregator->setLogicalPlan(logicalPlan);
    if(qr.rewrite(&logicalPlan) == false){
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
        		resultAggregator->getParamContainer(), headers);
        evhttp_clear_headers(&headers);
        return;
    }


    // prepare the message for Logical Plan and give it to Routing Manager
    RoutingManager::Message * logicalPlanMsg = RoutingManager::LogicalPlanMessage::buildLogicalPlanMessage(*logicalPlan);

	// broadcasting search request to all shards , non-blocking, with timeout and callback to ResultAggregator
    routingManager->broadcast_wait_for_all_w_cb_n_timeout(logicalPlanMsg,
    		resultAggregator , 2000 , coreShardInfo);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them

}

/*
 * 1. Receives an insert request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalInsertCommand(evhttp_request *req){

	// first, extract the record from request
	Record * record = NULL; // code exists in HTTPRequestHandler currently

	// now, use Partitioner to choose a shard for this record
	Partitioner partitioner(routingManager, configurationManager);
	unsigned shardIndex = partitioner.getShardIDForRecord(record);

	// and prepare the message
	RoutingManager::Message * insertRequestMsg = RoutingManager::HTTPInsertRequestMessage::buildHTTPRequestMessage(req);

	// and send it to the chosen shard
	RoutingManager::Message & response = routingManager->connect_w_response_n_timeout(insertRequestMsg, shardIndex , 2000);

	// print the message on HTTP channel
}

/*
 * 1. Receives an delete request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalDeleteCommand(evhttp_request *req){


	// first, extract all needed information for determining shardIndex
	Record * record = NULL; // this record is partially constructed and only contains information needed for hashing

	// now, use Partitioner to choose a shard for this record
	Partitioner partitioner(routingManager, configurationManager);
	unsigned shardIndex = partitioner.getShardIDForRecord(record);

	// and prepare the message
	RoutingManager::Message * deleteRequestMsg = RoutingManager::HTTPDeleteRequestMessage::buildHTTPRequestMessage(req);

	// and send it to the chosen shard
	RoutingManager::Message & response = routingManager->connect_w_response_n_timeout(deleteRequestMsg, shardIndex , 2000);

	// print the message on HTTP channel


}

/*
 * 1. Receives an update request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalUpdateCommand(evhttp_request *req){

	// first, extract all needed information for determining shardIndex
	Record * record = NULL; // this record is partially constructed and only contains information needed for hashing

	// now, use Partitioner to choose a shard for this record
	Partitioner partitioner(routingManager, configurationManager);
	unsigned shardIndex = partitioner.getShardIDForRecord(record);

	// and prepare the message
	RoutingManager::Message * updateRequestMsg = RoutingManager::HTTPUpdateRequestMessage::buildHTTPRequestMessage(req);

	// and send it to the chosen shard
	RoutingManager::Message & response = routingManager->connect_w_response_n_timeout(updateRequestMsg, shardIndex , 2000);

	// print the message on HTTP channel

}


/*
 * 1. Receives a GetInfo request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalGetInfoCommand(evhttp_request *req){
	// Prepare the message containing the getinfo request to send to all shards
	RoutingManager::Message * getInfoRequestMsg = RoutingManager::GetInfoMessage::buildGetInfoMessage(req);
	// broadcasting getInfo request to all shards , non-blocking, with timeout and callback to ResultAggregator
    routingManager->broadcast_wait_for_all_w_cb_n_timeout(getInfoRequestMsg,
    		ResultAggregatorAndPrint(configurationManager,routingManager, req) , 2000);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them
}

/*
 * 1. Receives a SerializeIndex request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalSerializeIndexCommand(evhttp_request *req){
	// Prepare the message containing the serializeIndex request to send to all shards
	RoutingManager::Message * serializeIndexRequestMsg = RoutingManager::SerializeIndexMessage::buildSerializeIndexMessage(req);
	// broadcasting getInfo request to all shards , non-blocking, with timeout and callback to ResultAggregator
    routingManager->broadcast_w_cb_n_timeout(serializeIndexRequestMsg,
    		ResultAggregatorAndPrint(configurationManager,routingManager, req) , 2000);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them
}

/*
 * 1. Receives a SerializeRecords request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalSerializeRecordsCommand(evhttp_request *req){
	// Prepare the message containing the serializeRecords request to send to all shards
	RoutingManager::Message * serializeRecordsRequestMsg = RoutingManager::SerializeRecordsMessage::buildSerializeRecordsMessage(req);
	// broadcasting getInfo request to all shards , non-blocking, with timeout and callback to ResultAggregator
    routingManager->broadcast_w_cb_n_timeout(serializeRecordsRequestMsg,
    		ResultAggregatorAndPrint(configurationManager,routingManager, req) , 2000);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them
}

/*
 * 1. Receives a ResetLog request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalResetLogCommand(evhttp_request *req){
	// Prepare the message containing the serializeRecords request to send to all shards
	RoutingManager::Message * resetLogRequestMsg = RoutingManager::ResetLogMessage::buildResetLogMessage(req);
	// broadcasting getInfo request to all shards , non-blocking, with timeout and callback to ResultAggregator
    routingManager->broadcast_w_cb_n_timeout(resetLogRequestMsg,
    		ResultAggregatorAndPrint(configurationManager,routingManager, req) , 2000);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them
}



}
}
