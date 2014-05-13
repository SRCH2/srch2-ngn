
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
#include "ParserUtility.h"
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

#include "sharding/configuration/ConfigManager.h"
#include "sharding/routing/RoutingManager.h"
#include "sharding/processor/Partitioner.h"
#include "sharding/processor/SearchResultsAggregatorAndPrint.h"
#include "sharding/processor/CommandStatusAggregatorAndPrint.h"
#include "sharding/processor/GetInfoAggregatorAndPrint.h"
#include "sharding/processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

//###########################################################################
//                       External Distributed Processor
//###########################################################################


DPExternalRequestHandler::DPExternalRequestHandler(ConfigManager * configurationManager, RoutingManager * routingManager){
	this->configurationManager = configurationManager;
	this->routingManager = routingManager;
	partitioner = new Partitioner(configurationManager);
}
/*
 * 1. Receives a search request from a client (not from another shard)
 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
 * 3. Gives ResultAggregator functions as callback function to TransportationManager
 * 4. ResultAggregator callback functions will aggregate the results and print them on
 *    http channel
 */
void DPExternalRequestHandler::externalSearchCommand(evhttp_request *req , CoreShardInfo * coreShardInfo){


    struct timespec tstart;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
    // CoreInfo_t is a view of configurationManager which contains all information for the
    // core that we want to search on, this object is accesses through configurationManager.
    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreShardInfo->coreName);

    SearchResultAggregatorAndPrint * resultAggregator = new SearchResultAggregatorAndPrint(configurationManager, req, coreShardInfo);

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

    // pass logical plan to broadcast through SerializableSearchCommandInput
    SerializableSearchCommandInput * searchInput =
    		new SerializableSearchCommandInput(&resultAggregator->getLogicalPlan());

    // add request object to results aggregator which is the callback object
    resultAggregator->addRequestObject(searchInput);
	// broadcasting search request to all shards , non-blocking, with timeout and callback to ResultAggregator
    timeval t;
    t.tv_sec = 2000;
    routingManager->broadcast_wait_for_all_w_cb_n_timeout(searchInput, resultAggregator , t , *coreShardInfo);
    // aggregateSearchResults in ResultAggregator will get the responses of all shards and aggregate them


    evhttp_clear_headers(&headers);
}

/*
 * 1. Receives an insert request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalInsertCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){


    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreShardInfo->coreName);
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
            	if(JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root,
            			configurationManager->getCoreInfo(coreShardInfo->coreName), log_str, recSerializer) == false){
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
        			configurationManager->getCoreInfo(coreShardInfo->coreName), log_str, recSerializer) == false){

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


    CommandStatusAggregatorAndPrint<SerializableInsertUpdateCommandInput> * resultsAggregator =
    		new CommandStatusAggregatorAndPrint<SerializableInsertUpdateCommandInput>(configurationManager,req);
    resultsAggregator->setMessages(log_str);
    for(vector<Record *>::iterator recordItr = recordsToInsert.begin(); recordItr != recordsToInsert.end() ; ++recordItr){

    	ShardId shardInfo = partitioner->getShardIDForRecord(*recordItr,coreShardInfo->coreName);

		SerializableInsertUpdateCommandInput  * insertUpdateInput=
				new SerializableInsertUpdateCommandInput(*recordItr,SerializableInsertUpdateCommandInput::INSERT);

		// add request object to results aggregator which is the callback object
		resultsAggregator->addRequestObject(insertUpdateInput);
		timeval t;
		t.tv_sec = 2000;
		routingManager->route_w_cb_n_timeout(insertUpdateInput, resultsAggregator , t , shardInfo);
    }
    // aggregated response will be prepared in CommandStatusAggregatorAndPrint::callBack and printed in
    // CommandStatusAggregatorAndPrint::finalize
}

/*
 * 1. Receives an update request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalUpdateCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){


    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreShardInfo->coreName);
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

	//std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

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
		    	bool parseJson = JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root,
		    			configurationManager->getCoreInfo(coreShardInfo->coreName), log_str, recSerializer);
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
	    			configurationManager->getCoreInfo(coreShardInfo->coreName), log_str, recSerializer);
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



    CommandStatusAggregatorAndPrint<SerializableInsertUpdateCommandInput> * resultsAggregator =
    		new CommandStatusAggregatorAndPrint<SerializableInsertUpdateCommandInput>(configurationManager,req);
    resultsAggregator->setMessages(log_str);
    for(vector<Record *>::iterator recordItr = recordsToUpdate.begin(); recordItr != recordsToUpdate.end() ; ++recordItr){

    	ShardId shardInfo = partitioner->getShardIDForRecord(*recordItr,coreShardInfo->coreName);

		SerializableInsertUpdateCommandInput * insertUpdateInput =
				new SerializableInsertUpdateCommandInput(*recordItr, SerializableInsertUpdateCommandInput::UPDATE);
		// add request object to results aggregator which is the callback object
		resultsAggregator->addRequestObject(insertUpdateInput);
		timeval t;
		t.tv_sec = 2000;
		routingManager->route_w_cb_n_timeout(insertUpdateInput, resultsAggregator , t, shardInfo);
    }
    // aggregated response will be prepared in CommandStatusAggregatorAndPrint::callBack and printed in
    // CommandStatusAggregatorAndPrint::finalize

}

/*
 * 1. Receives an delete request from a client (not from another shard)
 * 2. Uses Partitioner to know which shard should handle this request
 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
 *    Since it's a blocking call, the results are retrieved at the same point and
 *    printed on the HTTP channel.
 */
void DPExternalRequestHandler::externalDeleteCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){


	// it must be an update query
	ASSERT(req->type == EVHTTP_REQ_DELETE);
	if(req->type != EVHTTP_REQ_DELETE){
        Logger::error(
                "error: The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        return;
	}


	const CoreInfo_t * indexDataContainerConf = configurationManager->getCoreInfo(coreShardInfo->coreName);

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

	//set the primary key of the record we want to delete
	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

	const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());
	//TODO : we should parse more than primary key later
	if (pKeyParamName){
	    CommandStatusAggregatorAndPrint<SerializableDeleteCommandInput> * resultsAggregator =
	    		new CommandStatusAggregatorAndPrint<SerializableDeleteCommandInput>(configurationManager,req);

		size_t sz;
		char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);
		// TODO : should we free pKeyParamName_cstar?

		//std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
		const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
		free(pKeyParamName_cstar);

		ShardId shardInfo = partitioner->getShardIDForRecord(primaryKeyStringValue,coreShardInfo->coreName);

		SerializableDeleteCommandInput * deleteInput =
				new SerializableDeleteCommandInput(primaryKeyStringValue,coreShardInfo->coreId); // TODO : do we need coreId here ?
		// add request object to results aggregator which is the callback object
		resultsAggregator->addRequestObject(deleteInput);
		timeval t;
		t.tv_sec = 2000;
		routingManager->route_w_cb_n_timeout(deleteInput, resultsAggregator , t , shardInfo);
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
 * 1. Receives a GetInfo request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalGetInfoCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){

    GetInfoAggregatorAndPrint * resultsAggregator =	new GetInfoAggregatorAndPrint(configurationManager,req);
    SerializableGetInfoCommandInput * getInfoInput = new SerializableGetInfoCommandInput();
	// add request object to results aggregator which is the callback object
	resultsAggregator->addRequestObject(getInfoInput);
    timeval t;
    t.tv_sec = 2000;
    routingManager->broadcast_wait_for_all_w_cb_n_timeout(getInfoInput, resultsAggregator, t, *coreShardInfo);
}

/*
 * 1. Receives a SerializeIndex request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalSerializeIndexCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
    	CommandStatusAggregatorAndPrint<SerializableSerializeCommandInput> * resultsAggregator =
    			new CommandStatusAggregatorAndPrint<SerializableSerializeCommandInput>(configurationManager,req);

    	SerializableSerializeCommandInput * serializeInput =
    			new SerializableSerializeCommandInput(SerializableSerializeCommandInput::SERIALIZE_INDEX);
    	// add request object to results aggregator which is the callback object
    	resultsAggregator->addRequestObject(serializeInput);
    	timeval t;
    	t.tv_sec = 2000;
        routingManager->broadcast_wait_for_all_w_cb_n_timeout(serializeInput, resultsAggregator, t, *coreShardInfo);
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
 * 1. Receives a SerializeRecords request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalSerializeRecordsCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        // if search-response-format is 0 or 2
        if (configurationManager->getCoreInfo(coreShardInfo->coreName)->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            std::stringstream log_str;
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            const char *exportedDataFileName = evhttp_find_header(&headers, URLParser::nameParamName);
            // TODO : should we free exportedDataFileName?
            if(exportedDataFileName){
            	CommandStatusAggregatorAndPrint<SerializableSerializeCommandInput> * resultsAggregator =
            			new CommandStatusAggregatorAndPrint<SerializableSerializeCommandInput>(configurationManager,req);

            	SerializableSerializeCommandInput * serializeInput =
            			new SerializableSerializeCommandInput(SerializableSerializeCommandInput::SERIALIZE_RECORDS, string(exportedDataFileName));
            	// add request object to results aggregator which is the callback object
            	resultsAggregator->addRequestObject(serializeInput);
            	timeval t;
            	t.tv_sec = 2000;
                routingManager->broadcast_wait_for_all_w_cb_n_timeout(serializeInput, resultsAggregator, t, *coreShardInfo);
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
 * 1. Receives a ResetLog request from a client (not from another shard)
 * 2. Broadcasts this command to all shards and blocks to get their response
 * 3. prints Success or Failure on HTTP channel
 */
void DPExternalRequestHandler::externalResetLogCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){
    switch(req->type) {
    case EVHTTP_REQ_PUT: {
    	CommandStatusAggregatorAndPrint<SerializableResetLogCommandInput> * resultsAggregator =
    			new CommandStatusAggregatorAndPrint<SerializableResetLogCommandInput>(configurationManager,req);
    	SerializableResetLogCommandInput * resetInput = new SerializableResetLogCommandInput();
    	// add request object to results aggregator which is the callback object
    	resultsAggregator->addRequestObject(resetInput);
    	timeval t;
    	t.tv_sec = 2000;
        routingManager->broadcast_wait_for_all_w_cb_n_timeout(resetInput, resultsAggregator, t, *coreShardInfo);
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
 * Receives a commit request and boardcasts it to other shards
 */
void DPExternalRequestHandler::externalCommitCommand(evhttp_request *req, CoreShardInfo * coreShardInfo){
	CommandStatusAggregatorAndPrint<SerializableCommitCommandInput> * resultsAggregator =
			new CommandStatusAggregatorAndPrint<SerializableCommitCommandInput>(configurationManager, req);

	SerializableCommitCommandInput * commitInput = new SerializableCommitCommandInput();
	// add request object to results aggregator which is the callback object
	resultsAggregator->addRequestObject(commitInput);
	timeval t;
	t.tv_sec = 2000;
	routingManager->broadcast_wait_for_all_w_cb_n_timeout(commitInput, resultsAggregator, t, *coreShardInfo);
}



}
}
