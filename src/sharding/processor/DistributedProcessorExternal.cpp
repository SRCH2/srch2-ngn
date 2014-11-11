
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



#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableSearchResults.h"

#include "sharding/sharding/ShardManager.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/CoreInfo.h"
#include "sharding/processor/Partitioner.h"
#include "sharding/processor/aggregators/SearchResultsAggregatorAndPrint.h"
#include "sharding/processor/aggregators/GetInfoAggregatorAndPrint.h"
#include "server/HTTPJsonResponse.h"

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

    // NOTE: this is where we connect to the URI object coming
    // from web
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
    //1. first create query parser to parse the url
    QueryParser qp(headers, resultAggregator->getParamContainer());
    bool isSyntaxValid = qp.parse(indexDataContainerConf->getSchema());
    if (!isSyntaxValid) {
        // if the query is not valid print the error message to the response
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Bad Request",
                resultAggregator->getParamContainer()->getMessageString(), headers);
        evhttp_clear_headers(&headers);
        return;
    }
    //2. validate the query
    //TODO : Temporary fix until ACL is fixed.
    attributeAcl = new AttributeAccessControl((SchemaInternal *)indexDataContainerConf->getSchema());
    QueryValidator qv(*(indexDataContainerConf->getSchema()),
            *(indexDataContainerConf), resultAggregator->getParamContainer(),
            *attributeAcl);

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
            resultAggregator->getParamContainer(),
            *attributeAcl);

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
  * 1. Receives a getinfo request from a client (not from another shard)
  * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
  * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
  *       results. Results will be aggregator by another thread since it's not a blocking call.
  */
void DPExternalRequestHandler::externalGetInfoCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req, unsigned coreId, PortType_t portType){


	bool debugRequest = false;
	switch (portType) {
		case InfoPort_Nodes_NodeID:
		{
			JsonResponseHandler httpResponse(req);
			httpResponse.setResponseAttribute(c_cluster_name, Json::Value(clusterReadview->getClusterName()));
			Json::Value nodes(Json::arrayValue);
			ShardManager::getShardManager()->getNodeInfoJson(nodes);
			httpResponse.setResponseAttribute(c_nodes, nodes);
			httpResponse.finalizeOK();
			return;
		}
		case DebugStatsPort:
			debugRequest = true;
			break;
		case InfoPort:
		case InfoPort_Cluster_Stats:
			break;
		default:
			ASSERT(false);
			break;
	}

	boost::shared_ptr<GetInfoJsonResponse > brokerSideInformationJson =
			boost::shared_ptr<GetInfoJsonResponse > (new GetInfoJsonResponse(req));

    vector<unsigned> coreIds;
    if(coreId == (unsigned) -1){
    	vector<const CoreInfo_t *> cores;
    	clusterReadview->getAllCores(cores);
    	for(unsigned cid = 0 ; cid < cores.size(); cid++){
    		coreIds.push_back(cores.at(cid)->getCoreId());
    	}
    }else{
    	coreIds.push_back(coreId);
    }


	vector<NodeTargetShardInfo> targets;
    for(unsigned cid = 0; cid < coreIds.size(); ++cid){
    	unsigned coreId = coreIds.at(cid);
		const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);
		const string coreName = indexDataContainerConf->getName();
		CorePartitioner * corePartitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
		corePartitioner->getAllTargets(targets);
		delete corePartitioner;
    }

    boost::shared_ptr<GetInfoResponseAggregator> resultsAggregator(
    		new GetInfoResponseAggregator(configurationManager,brokerSideInformationJson, clusterReadview, coreId, debugRequest));
	if(targets.size() == 0){
		brokerSideInformationJson->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_All_Shards_Down_Error));
		brokerSideInformationJson->finalizeOK();
	}else{
		brokerSideInformationJson->finalizeOK();
		time_t timeValue;
		time(&timeValue);
		timeValue = timeValue + TIMEOUT_WAIT_TIME;
		GetInfoCommand * getInfoInput = new GetInfoCommand(GetInfoRequestType_);
		resultsAggregator->addRequestObj(getInfoInput);
		bool routingStatus = dpMessageHandler.broadcast<GetInfoCommand, GetInfoCommandResults>(getInfoInput,
						true,
						true,
						resultsAggregator,
						timeValue,
						targets,
						clusterReadview);

		if(! routingStatus){
			brokerSideInformationJson->finalizeError("Internal Server Error.");
		}
	}

}

}
}
