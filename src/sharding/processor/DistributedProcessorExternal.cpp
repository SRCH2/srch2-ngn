
#include "DistributedProcessorExternal.h"

/*
 * System and thirdparty libraries
 */
#include <sys/time.h>
#include <sys/queue.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include "thirdparty/snappy-1.0.4/snappy.h"


/*
 * Utility libraries
 */
#include "util/Logger.h"
#include "util/CustomizableJsonWriter.h"
#include "util/RecordSerializer.h"
#include "util/RecordSerializerUtil.h"
#include "util/FileOps.h"

/*
 * Srch2 libraries
 */
#include "instantsearch/TypedValue.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "ParsedParameterContainer.h"
#include "QueryParser.h"
#include "QueryValidator.h"
#include "QueryRewriter.h"
#include "QueryPlan.h"
#include "util/ParserUtility.h"
#include "HTTPRequestHandler.h"
#include "IndexWriteUtil.h"
#include "ServerHighLighter.h"



#include "serializables/SerializableCommandStatus.h"
#include "serializables/SerializableCommitCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"
#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableResetLogCommandInput.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableSerializeCommandInput.h"

#include "sharding/sharding/ShardManager.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/CoreInfo.h"
#include "sharding/processor/Partitioner.h"
#include "sharding/processor/aggregators/SearchResultsAggregatorAndPrint.h"
#include "sharding/processor/aggregators/CommandStatusAggregatorAndPrint.h"
#include "sharding/processor/aggregators/GetInfoAggregatorAndPrint.h"
#include "sharding/processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace srch2::util;

#define TIMEOUT_WAIT_TIME 2

namespace srch2 {
namespace httpwrapper {

//###########################################################################
//                       External Distributed Processor
//###########################################################################


DPExternalRequestHandler::DPExternalRequestHandler(ConfigManager & configurationManager,
		TransportManager& transportManager, DPInternalRequestHandler& dpInternal):
			dpMessageHandler(configurationManager, transportManager, dpInternal){
	this->configurationManager = &configurationManager;
	transportManager.registerCallbackForDPMessageHandler(&dpMessageHandler);
};


/*
 * 1. Receives a search request from a client (not from another shard)
 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
 * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
 *       results. Results will be aggregator by another thread since it's not a blocking call.
 */
void DPExternalRequestHandler::externalSearchCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req , unsigned coreId){


    struct timespec tstart;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
    // CoreInfo_t is a view of configurationManager which contains all information for the
    // core that we want to search on, this object is accesses through configurationManager.
//    Logger::console("Cluster readview used for search: ");
//    Logger::console("====================================");
//    clusterReadview->print();
//    Logger::console("====================================");

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    boost::shared_ptr<SearchResultsAggregator> resultAggregator(new SearchResultsAggregator(configurationManager, req, clusterReadview, coreId));


    clock_gettime(CLOCK_REALTIME, &(resultAggregator->getStartTimer()));

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
    //1. first create query parser to parse the url
    QueryParser qp(headers, resultAggregator->getParamContainer());
    bool isSyntaxValid = qp.parse();
    if (!isSyntaxValid) {
        // if the query is not valid print the error message to the response
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Bad Request",
                resultAggregator->getParamContainer()->getMessageString(), headers);
        evhttp_clear_headers(&headers);
        return;
    }
    //2. validate the query
    QueryValidator qv(*(indexDataContainerConf->getSchema()),
            *(indexDataContainerConf), resultAggregator->getParamContainer());

    bool valid = qv.validate();
    if (!valid) {
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Bad Request",
                resultAggregator->getParamContainer()->getMessageString(), headers);
        evhttp_clear_headers(&headers);
        return;
    }
    //3. rewrite the query and apply analyzer and other stuff ...
    QueryRewriter qr(indexDataContainerConf,
            *(indexDataContainerConf->getSchema()),
            *(AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf)),
            resultAggregator->getParamContainer());

    if(qr.rewrite(resultAggregator->getLogicalPlan()) == false){
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Bad Request",
                resultAggregator->getParamContainer()->getMessageString().c_str(), headers);
        evhttp_clear_headers(&headers);
        return;
    }
    // compute elapsed time in ms , end the timer
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    resultAggregator->setParsingValidatingRewritingTime(ts1);

    // broadcasting search request to all shards , non-blocking, with timeout and callback to ResultAggregator
    // 1. first find all destination shards.
    CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
    vector<NodeTargetShardInfo> targets;
    partitioner->getAllReadTargets(targets);
    if(targets.size() == 0){
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                "No data shard is available for search for this core", headers);
    }else{
    	// 2. use destinations and do the broadcast by using RM
		time_t timeValue;
		time(&timeValue);
		timeValue = timeValue + TIMEOUT_WAIT_TIME;
		// pass logical plan to broadcast through SerializableSearchCommandInput
		SearchCommand * searchInput =
				new SearchCommand(&(resultAggregator->getLogicalPlan()));
		// request object must be saved in aggregator to be able to delete it safely
		resultAggregator->addRequestObj(searchInput);
		bool routingStatus = dpMessageHandler.broadcast<SearchCommand, SearchCommandResults>(searchInput,
						true,
						true,
						resultAggregator,
						timeValue,
						targets,
						clusterReadview);

		if(! routingStatus){
	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure",
	                "", headers);
		}
    }
    delete partitioner;


    evhttp_clear_headers(&headers);
}


void DPExternalRequestHandler::externalSearchAllCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request * req){
	//TODO
}

/*
 * 1. Receives an insert request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    in a non-blocking manner. The status response is taken care of by aggregator in
 *    another thread when these responses come.
 */
void DPExternalRequestHandler::externalInsertCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

//    Logger::console("Cluster readview used for insert: ");
//    Logger::console("====================================");
//    clusterReadview->print();
//    Logger::console("====================================");

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);
    // it must be an insert query
    ASSERT(req->type == EVHTTP_REQ_PUT);
    if(req->type != EVHTTP_REQ_PUT){
        Logger::error(
                "error: The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        return;
    }
    size_t length = EVBUFFER_LENGTH(req->input_buffer);

    if (length == 0) {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "BAD REQUEST",
                "{\"message\":\"http body is empty\"}");
        Logger::warn("http body is empty");
        return;
    }

    const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

    std::stringstream log_str;


    vector<Record *> recordsToInsert;
    // Parse example data
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(post_data, root, false);

    if (parseSuccess == false) {
        log_str << "JSON object parse error";
    } else {
        Schema * storedSchema = Schema::create();
        RecordSerializerUtil::populateStoredSchema(storedSchema, indexDataContainerConf->getSchema());
        RecordSerializer recSerializer = RecordSerializer(*storedSchema);


        if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
            // Iterates over the sequence elements.
            for ( int index = 0; index < root.size(); ++index ) {

                /*
                 * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
                 */
                Record *record = new Record(indexDataContainerConf->getSchema());

                Json::Value defaultValueToReturn = Json::Value("");
                const Json::Value doc = root.get(index,
                        defaultValueToReturn);

                Json::FastWriter writer;
                if(JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(doc), doc,
                        indexDataContainerConf, log_str, recSerializer) == false){
                    log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\"}";
                    if (index < root.size() - 1){
                        log_str << ",";
                    }
                    delete record;
                }else{
                    // record is ready to insert
                    recordsToInsert.push_back(record);
                }

            }
        } else {  // only one json object needs to be inserted

            /*
             * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
             */
            Record *record = new Record(indexDataContainerConf->getSchema());

            const Json::Value doc = root;
            Json::FastWriter writer;
            if(JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root,
                    indexDataContainerConf, log_str, recSerializer) == false){

                log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\"}";

                Logger::info("%s", log_str.str().c_str());

                bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                        "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");
                record->clear();
                delete storedSchema;
                delete record;
                return;
            }
            // record is ready to insert
            recordsToInsert.push_back(record);
        }
        delete storedSchema;
    }

    if(recordsToInsert.size() == 0){
        Logger::info("%s", log_str.str().c_str());

        bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                + log_str.str() + "]}\n");
        return;
    }


    /*
     * Result aggregator is responsible of aggregating all response messages from all shards.
     * The reason that aggregator is wrapped in shared pointer is that we use a separate pending request for each
     * insert in a batch insert, so we want the aggregator to be deleted after all of them are resolved.
     */
    boost::shared_ptr<StatusAggregator<InsertUpdateCommand> >
    resultsAggregator(new StatusAggregator<InsertUpdateCommand>(configurationManager,req, clusterReadview, coreId,recordsToInsert.size()));
    resultsAggregator->setMessages(log_str);


	// 1. first find all destination shards.
	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
    for(vector<Record *>::iterator recordItr = recordsToInsert.begin(); recordItr != recordsToInsert.end() ; ++recordItr){

        time_t timeValue;
        time(&timeValue);
        timeValue = timeValue + TIMEOUT_WAIT_TIME;
        InsertUpdateCommand  * insertUpdateInput = new InsertUpdateCommand(*recordItr,InsertUpdateCommand::DP_INSERT);
        resultsAggregator->addRequestObj(insertUpdateInput);
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllWriteTargets(partitioner->hashDJB2(insertUpdateInput->getRecord()->getPrimaryKey().c_str()),
        		clusterReadview->getCurrentNodeId(), targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{

//        	// TODO remove
//        	NodeTargetShardInfo::printTargets(targets);

    		bool routingStatus = dpMessageHandler.broadcast<InsertUpdateCommand, CommandStatus>(insertUpdateInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }

    }
    delete partitioner;
    // aggregated response will be prepared in CommandStatusAggregatorAndPrint::callBack and printed in
    // CommandStatusAggregatorAndPrint::finalize
}



/*
 * 1. Receives an update request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    in a non-blocking manner. The status response is taken care of by aggregator in
 *    another thread when these responses come.
 */
void DPExternalRequestHandler::externalUpdateCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);
    // it must be an update query
    ASSERT(req->type == EVHTTP_REQ_PUT);
    if(req->type != EVHTTP_REQ_PUT){
        Logger::error(
                "error: The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        return;
    }


    size_t length = EVBUFFER_LENGTH(req->input_buffer);

    if (length == 0) {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "BAD REQUEST",
                "{\"message\":\"http body is empty\"}");
        Logger::warn("http body is empty");
        return;
    }

    const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

    std::stringstream log_str;

    vector<Record *> recordsToUpdate;
    // Parse example data
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(post_data, root, false);

    if (parseSuccess == false) {
        log_str << "JSON object parse error";
    } else {
        evkeyvalq headers;
        evhttp_parse_query(req->uri, &headers);

        Schema * storedSchema = Schema::create();
        RecordSerializerUtil::populateStoredSchema(storedSchema, indexDataContainerConf->getSchema());
        RecordSerializer recSerializer = RecordSerializer(*storedSchema);
        if (root.type() == Json::arrayValue) {
            //the record parameter is an array of json objects
            for(Json::UInt index = 0; index < root.size(); index++) {
                /*
                 * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
                 */
                Record *record = new Record(indexDataContainerConf->getSchema());

                Json::Value defaultValueToReturn = Json::Value("");
                const Json::Value doc = root.get(index,
                        defaultValueToReturn);

                Json::FastWriter writer;
                bool parseJson = JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(doc), doc,
                        indexDataContainerConf, log_str, recSerializer);
                if(parseJson == false) {
                    log_str << "failed\",\"reason\":\"parse: The record is not in a correct json format\",";
                    if (index < root.size() - 1){
                        log_str << ",";
                    }
                    delete record;
                }else{
                    recordsToUpdate.push_back(record);
                }

            }
        } else {
            /*
             * the record parameter is a single json object
             * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
             */
            Record *record = new Record(indexDataContainerConf->getSchema());
            const Json::Value doc = root;

            Json::FastWriter writer;
            bool parseJson = JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root,
                    indexDataContainerConf, log_str, recSerializer);
            if(parseJson == false) {
                log_str << "failed\",\"reason\":\"parse: The record is not in a correct json format\",";
                Logger::info("%s", log_str.str().c_str());

                bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                        "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");
                record->clear();
                delete storedSchema;
                delete record;
                return;
            }
            recordsToUpdate.push_back(record);

        }

        delete storedSchema;
        evhttp_clear_headers(&headers);
    }



    if(recordsToUpdate.size() == 0){
        Logger::info("%s", log_str.str().c_str());

        bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                + log_str.str() + "]}\n");
        return;
    }



    boost::shared_ptr<StatusAggregator<InsertUpdateCommand> >
    resultsAggregator(new StatusAggregator<InsertUpdateCommand>(configurationManager,req, clusterReadview, coreId, recordsToUpdate.size()));
    resultsAggregator->setMessages(log_str);
	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
    for(vector<Record *>::iterator recordItr = recordsToUpdate.begin(); recordItr != recordsToUpdate.end() ; ++recordItr){
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllWriteTargets(partitioner->hashDJB2((*recordItr)->getPrimaryKey().c_str()),
        		clusterReadview->getCurrentNodeId(), targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
			InsertUpdateCommand  * insertUpdateInput = new InsertUpdateCommand(*recordItr,InsertUpdateCommand::DP_UPDATE);
			resultsAggregator->addRequestObj(insertUpdateInput);
    		bool routingStatus = dpMessageHandler.broadcast<InsertUpdateCommand, CommandStatus>(insertUpdateInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
    }
    delete partitioner;
    // aggregated response will be prepared in CommandStatusAggregatorAndPrint::callBack and printed in
    // CommandStatusAggregatorAndPrint::finalize

}



/*
 * 1. Receives an delete request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    in a non-blocking manner. The status response is taken care of by aggregator in
 *    another thread when these responses come.
 */
void DPExternalRequestHandler::externalDeleteCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){


    // it must be an update query
    ASSERT(req->type == EVHTTP_REQ_DELETE);
    if(req->type != EVHTTP_REQ_DELETE){
        Logger::error(
                "error: The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        return;
    }

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    //set the primary key of the record we want to delete
    std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());
    //TODO : we should parse more than primary key later
    if (pKeyParamName){
        boost::shared_ptr<StatusAggregator<DeleteCommand> >
        resultsAggregator(new StatusAggregator<DeleteCommand>(configurationManager,req, clusterReadview, coreId));

        size_t sz;
        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);
        // TODO : should we free pKeyParamName_cstar?

        //std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
        free(pKeyParamName_cstar);
		CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllWriteTargets(partitioner->hashDJB2(primaryKeyStringValue.c_str()),
        		clusterReadview->getCurrentNodeId(), targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
			DeleteCommand * deleteInput = new DeleteCommand(primaryKeyStringValue,coreId);
			resultsAggregator->addRequestObj(deleteInput);
    		bool routingStatus = dpMessageHandler.broadcast<DeleteCommand, CommandStatus>(deleteInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
        delete partitioner;


    }else{
        std::stringstream log_str;
        log_str << "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\"wrong primary key\"}";
        Logger::info("%s", log_str.str().c_str());
        bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                + log_str.str() + "]}\n");
        // Free the objects
        evhttp_clear_headers(&headers);
        return;
    }


}


/*
  * 1. Receives a getinfo request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalGetInfoCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    boost::shared_ptr<GetInfoResponseAggregator> resultsAggregator(new GetInfoResponseAggregator(configurationManager,req, clusterReadview, coreId));
	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
    vector<NodeTargetShardInfo> targets;
    partitioner->getAllTargets(targets);
    if(targets.size() == 0){
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                "All nodes are down.");
    }else{
        time_t timeValue;
        time(&timeValue);
        timeValue = timeValue + TIMEOUT_WAIT_TIME;
		GetInfoCommand * getInfoInput = new GetInfoCommand();
		resultsAggregator->addRequestObj(getInfoInput);
		bool routingStatus = dpMessageHandler.broadcast<GetInfoCommand, GetInfoCommandResults>(getInfoInput,
						true,
						true,
						resultsAggregator,
						timeValue,
						targets,
						clusterReadview);

		if(! routingStatus){
	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
		}
    }
    delete partitioner;

}



/*
  * 1. Receives a save request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalSerializeIndexCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){
    /* Yes, we are expecting a post request */

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        boost::shared_ptr<StatusAggregator<SerializeCommand> >
        resultsAggregator(new StatusAggregator<SerializeCommand>(configurationManager,req, clusterReadview, coreId));


    	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllTargets(targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
			SerializeCommand * serializeInput =
					new SerializeCommand(SerializeCommand::SERIALIZE_INDEX);
			resultsAggregator->addRequestObj(serializeInput);
    		bool routingStatus = dpMessageHandler.broadcast<SerializeCommand, CommandStatus>(serializeInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
        delete partitioner;

        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };
}



/*
  * 1. Receives a export request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalSerializeRecordsCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){
    /* Yes, we are expecting a post request */

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        // if search-response-format is 0 or 2
        if (indexDataContainerConf->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            std::stringstream log_str;
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            const char *exportedDataFileName = evhttp_find_header(&headers, URLParser::nameParamName);
            // TODO : should we free exportedDataFileName?
            if(exportedDataFileName){
                boost::shared_ptr<StatusAggregator<SerializeCommand> >
                resultsAggregator(new StatusAggregator<SerializeCommand>(configurationManager,req, clusterReadview, coreId));

            	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
                vector<NodeTargetShardInfo> targets;
                partitioner->getAllTargets(targets);
                if(targets.size() == 0){
                    bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                            "All nodes are down.");
                }else{
                    time_t timeValue;
                    time(&timeValue);
                    timeValue = timeValue + TIMEOUT_WAIT_TIME;
					SerializeCommand * serializeInput =
							new SerializeCommand(SerializeCommand::SERIALIZE_RECORDS, string(exportedDataFileName));
					resultsAggregator->addRequestObj(serializeInput);
            		bool routingStatus = dpMessageHandler.broadcast<SerializeCommand, CommandStatus>(serializeInput,
            						true,
            						true,
            						resultsAggregator,
            						timeValue,
            						targets,
            						clusterReadview);

            		if(! routingStatus){
            	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
            		}
                }
                delete partitioner;
            }else {
                bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                        "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
                Logger::error(
                        "The request has an invalid or missing argument. See Srch2 API documentation for details");
            }
        } else{
            bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
                    "{\"message\":\"The indexed data failed to export to disk, The request need to set search-response-format to be 0 or 2\"}\n");
        }
        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };


}



/*
  * 1. Receives a reset log request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalResetLogCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    switch(req->type) {
    case EVHTTP_REQ_PUT: {
        boost::shared_ptr<StatusAggregator<ResetLogCommand> >
        resultsAggregator(new StatusAggregator<ResetLogCommand>(configurationManager,req, clusterReadview, coreId));
    	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllTargets(targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
			ResetLogCommand * resetInput = new ResetLogCommand();
			resultsAggregator->addRequestObj(resetInput);
    		bool routingStatus = dpMessageHandler.broadcast<ResetLogCommand, CommandStatus>(resetInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
        delete partitioner;

        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };
}

/*
  * 1. Receives a commit request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalCommitCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    switch(req->type) {
    case EVHTTP_REQ_PUT: {
        boost::shared_ptr<StatusAggregator<CommitCommand> >
        resultsAggregator(new StatusAggregator<CommitCommand>(configurationManager,req, clusterReadview, coreId));
    	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllTargets(targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
			CommitCommand * commitInput = new CommitCommand();
			resultsAggregator->addRequestObj(commitInput);
    		bool routingStatus = dpMessageHandler.broadcast<CommitCommand, CommandStatus>(commitInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
        delete partitioner;

        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };
}

/*
  * 1. Receives a merge request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalMergeCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);

    switch(req->type) {
    case EVHTTP_REQ_PUT: {
        boost::shared_ptr<StatusAggregator<MergeCommand> >
        resultsAggregator(new StatusAggregator<MergeCommand>(configurationManager,req, clusterReadview, coreId));
    	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
        vector<NodeTargetShardInfo> targets;
        partitioner->getAllTargets(targets);
        if(targets.size() == 0){
            bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Node Failure",
                    "All nodes are down.");
        }else{
            time_t timeValue;
            time(&timeValue);
            timeValue = timeValue + TIMEOUT_WAIT_TIME;
            MergeCommand * mergeInput = new MergeCommand();
			resultsAggregator->addRequestObj(mergeInput);
    		bool routingStatus = dpMessageHandler.broadcast<MergeCommand, CommandStatus>(mergeInput,
    						true,
    						true,
    						resultsAggregator,
    						timeValue,
    						targets,
    						clusterReadview);

    		if(! routingStatus){
    	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Request Failure","");
    		}
        }
        delete partitioner;

        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };
}

}
}
