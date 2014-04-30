#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_

#include <instantsearch/Record.h>
#include "wrapper/ParsedParameterContainer.h"
#include "wrapper/URLParser.h"

#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"
#include "util/CustomizableJsonWriter.h"

#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableSerializeCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableCommandStatus.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"
#include "serializables/SerializableResetLogCommandInput.h"
#include "serializables/SerializableCommitCommandInput.h"

#include "Partitioner.h"
#include "sharding/configuration/ConfigManager.h"
#include "core/highlighter/Highlighter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

/**
 * Create evbuffer. If failed, send 503 response.
 * @param req request
 * @return buffer
 */
evbuffer *create_buffer(evhttp_request *req) {
    evbuffer *buf = evbuffer_new();
    if (!buf) {
        fprintf(stderr, "Failed to create response buffer\n");
        evhttp_send_reply(req, HTTP_SERVUNAVAIL,
                "Failed to create response buffer", NULL);
        return NULL;
    }
    return buf;
}

void bmhelper_check_add_callback(evbuffer *buf, const evkeyvalq &headers,
        const string &out_payload) {
    const char *jsonpCallBack = evhttp_find_header(&headers,
            URLParser::jsonpCallBackName);
    if (jsonpCallBack) {
        size_t sz;
        char *jsonpCallBack_cstar = evhttp_uridecode(jsonpCallBack, 0, &sz);
        //std::cout << "[" << jsonpCallBack_cstar << "]" << std::endl;

        evbuffer_add_printf(buf, "%s(%s)", jsonpCallBack_cstar,
                out_payload.c_str());

        // libevent uses malloc for memory allocation. Hence, use free
        free(jsonpCallBack_cstar);
    } else {
        evbuffer_add_printf(buf, "%s", out_payload.c_str());
    }
}


void bmhelper_add_content_length(evhttp_request *req, evbuffer *buf) {
    size_t length = EVBUFFER_LENGTH(buf);
    std::stringstream length_str;
    length_str << length;
    evhttp_add_header(req->output_headers, "Content-Length",
            length_str.str().c_str());
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) {
    evbuffer *returnbuffer = create_buffer(req);
    bmhelper_check_add_callback(returnbuffer, headers, out_payload);
    bmhelper_add_content_length(req, returnbuffer);
    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload) {
    evbuffer *returnbuffer = create_buffer(req);

    evbuffer_add_printf(returnbuffer, "%s", out_payload.c_str());
    bmhelper_add_content_length(req, returnbuffer);

    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

struct ResultsAggregatorAndPrintMetadata{

};

template <class Request, class Response>
class ResultAggregatorAndPrint {
public:


	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	virtual void preProcessing(ResultsAggregatorAndPrintMetadata metadata){};
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	virtual void timeoutProcessing(CoreShardInfo * coreShardInfo,
			Request * sentRequest, ResultsAggregatorAndPrintMetadata metadata){};

	/*
	 * The callBack function used by routing manager
	 */
	virtual void callBack(Response * responseObject){};
	virtual void callBack(vector<Response *> responseObjects){};

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	virtual void finalize(ResultsAggregatorAndPrintMetadata metadata){};


	virtual ~ResultAggregatorAndPrint(){};

};

class SearchResultAggregatorAndPrint : public ResultAggregatorAndPrint<SerializableSearchCommandInput , SerializableSearchResults> {

public:

	SearchResultAggregatorAndPrint(ConfigManager * configurationManager,
			RoutingManager * routingManager,
			evhttp_request *req,
			CoreShardInfo * coreShardInfo){
		this->configurationManager = configurationManager;
		this->routingManager = routingManager;
		this->req = req;
		this->coreShardInfo = coreShardInfo;
	}
	LogicalPlan & getLogicalPlan(){
		return logicalPlan;
	}
	ParsedParameterContainer * getParamContainer(){
		return &paramContainer;
	}

	void setParsingValidatingRewritingTime(unsigned time){
		this->parsingValidatingRewritingTime = time;
	}

	unsigned getParsingValidatingRewritingTime(){
		return this->parsingValidatingRewritingTime;
	}

	struct timespec & getStartTimer(){
		return this->tstart;
	}
	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo,
			SerializableSearchCommandInput * sentRequest,
			ResultsAggregatorAndPrintMetadata metadata){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		messages << "{\"search\":\"failed\",\"reason\":\"Corresponging shard ("<<
						coreShardInfo->shardId<<") timedout.\"}";
	}


	/*
	 * The main function responsible of aggregating search results
	 * this function uses aggregateRecords and aggregateFacets for
	 * aggregating result records and calculated records
	 */
	void callBack(vector<SerializableSearchResults *> responseObjects);

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultsAggregatorAndPrintMetadata metadata){
		// print the results
		printResults();
	}


	void printResults();

	/**
	 * Iterate over the recordIDs in queryResults and get the record.
	 * Add the record information to the request.out string.
	 */
	void printResults(evhttp_request *req,
	        const evkeyvalq &headers, const LogicalPlan &queryPlan,
	        const CoreInfo_t *indexDataConfig,
	        const vector<QueryResult *> queryResultsVector, const Query *query,
	        const unsigned start, const unsigned end,
	        const unsigned retrievedResults, const string & message,
	        const unsigned ts1, const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) ;

	void printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
	        const LogicalPlan &queryPlan,
	        const CoreInfo_t *indexDataConfig,
	        const vector<QueryResult *> queryResultsVector,
	        const string & message,
	        const unsigned ts1);

	class QueryResultsComparatorOnlyScore{
	public:
		bool operator()(QueryResult * left, QueryResult * right){
			return (left->getResultScore() > right->getResultScore());
		}
	};


	class AggregatedQueryResults{
	public:
		vector<QueryResult *> allResults;
		vector<string> recordData;
		std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > > aggregatedFacetResults;
		unsigned aggregatedEstimatedNumberOfResults;
		bool isResultsApproximated;
		unsigned aggregatedSearcherTime;

		AggregatedQueryResults(){
			aggregatedEstimatedNumberOfResults = 0;
			isResultsApproximated = false;
			aggregatedSearcherTime = 0;
		}
	};

private:

	/*
	 * Combines the results coming from all shards and
	 * resorts them based on their scores
	 */
	void aggregateRecords();

	/*
	 * Combines the facet results coming from all shards and
	 * re-calculates facet values
	 */
	void aggregateFacets();


	/*
	 * Merges destination with source and adds new items to source
	 */
	void mergeFacetVectors(std::vector<std::pair<std::string, float> > & source,
			const std::vector<std::pair<std::string, float> > & destination);


	/*
	 * Aggregates the estimatedNumberOfResults from all responses
	 * and also sets resultsApproximated if any of the responses has this flag set
	 */
	void aggregateEstimations();


	/*
	 * This vector will be populated from QueryResults coming from all shards
	 * If some shards fail or don't return any results we ignore them
	 * Search should not be blocking in failure case
	 */
	vector<QueryResults *> resultsOfAllShards;

	/*
	 * This variable contains the final aggregated results
	 */
	SearchResultAggregatorAndPrint::AggregatedQueryResults results;
	RoutingManager * routingManager;
	ConfigManager * configurationManager;
	evhttp_request *req;
	CoreShardInfo * coreShardInfo;

	LogicalPlan logicalPlan;
	unsigned parsingValidatingRewritingTime;
	struct timespec tstart;
	ParsedParameterContainer paramContainer;

	// these members can be accessed concurrently by multiple threads
	mutable boost::shared_mutex _access;
	std::stringstream messages;
};



template <class RequestWithStatusResponse>
class CommandStatusAggregatorAndPrint : public ResultAggregatorAndPrint<RequestWithStatusResponse,SerializableCommandStatus> {
public:

	CommandStatusAggregatorAndPrint(RoutingManager * routingManager, ConfigManager * configurationManager, evhttp_request *req){
		this->routingManager = routingManager;
		this->configurationManager = configurationManager;
		this->req = req;
	}

	void setMessages(std::stringstream log_str){
		this->messages = log_str;
	}
	std::stringstream & getMessages(){
		return this->messages;
	}

	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo, RequestWithStatusResponse * sentRequest, ResultsAggregatorAndPrintMetadata metadata){

		if(((string)"SerializableInsertUpdateCommandInput").compare(typeid(sentRequest).name()) == 0){// timeout in insert and update

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableInsertUpdateCommandInput * sentInsetUpdateRequest = dynamic_cast<SerializableInsertUpdateCommandInput *>(sentRequest);
			messages << "{\"rid\":\"" << sentInsetUpdateRequest->record->getPrimaryKey()
					<< "\",\"" << (sentInsetUpdateRequest->insertOrUpdate?"insert":"update") << "\":\"failed\",\"reason\":\"Corresponging shard ("<<
							coreShardInfo->shardId<<") timedout.\"}";

		}else if (((string)"SerializableDeleteCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableDeleteCommandInput * sentDeleteRequest = dynamic_cast<SerializableDeleteCommandInput *>(sentRequest);
			messages << "{\"rid\":\"" << sentDeleteRequest->primaryKey
					<< "\",\"delete\":\"failed\",\"reason\":\"Corresponging ("<<
							coreShardInfo->shardId << ") shard timedout.\"}";

		}else if(((string)"SerializableSerializeCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableSerializeCommandInput * serializeRequest = dynamic_cast<SerializableSerializeCommandInput *>(sentRequest);
			messages << "{\""<< (serializeRequest->indexOrRecord?"save":"export") << "\":\"failed\",\"reason\":\"Corresponging (" <<
							coreShardInfo->shardId << ") shard timedout.\"}";

		}else if(((string)"SerializableResetLogCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableResetLogCommandInput * resetRequest = dynamic_cast<SerializableResetLogCommandInput *>(sentRequest);
			messages << "{\"reset_log\":\"failed\",\"reason\":\"Corresponging (" << coreShardInfo->shardId<<") shard timedout.\"}";

		}else if(((string)"SerializableCommitCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableCommitCommandInput * resetRequest = dynamic_cast<SerializableCommitCommandInput *>(sentRequest);
			messages << "{\"commit\":\"failed\",\"reason\":\"Corresponging (" << coreShardInfo->shardId<<") shard timedout.\"}";

		}else{
			//TODO : what should we do here?
			ASSERT(false);
			return;
		}
	}


	/*
	 * The main function responsible of aggregating status (success or failure) results
	 */
	void callBack(SerializableCommandStatus * responseObject){

		boost::unique_lock< boost::shared_mutex > lock(_access);
		messages << responseObject->getMessage();

	}

	void callBack(vector<SerializableCommandStatus *> responseObjects){

		boost::unique_lock< boost::shared_mutex > lock(_access);
		for(vector<SerializableCommandStatus *>::iterator responseItr = responseObjects.begin(); responseItr != responseObjects.end(); ++responseItr){
			messages << (*responseItr)->getMessage();
		}
	}
	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultsAggregatorAndPrintMetadata metadata){
        Logger::info("%s", messages.str().c_str());

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + messages.str() + "]}\n");
	}


private:



	RoutingManager * routingManager;
	ConfigManager * configurationManager;
	evhttp_request *req;

	mutable boost::shared_mutex _access;
	std::stringstream messages;
};


class GetInfoAggregatorAndPrint : public ResultAggregatorAndPrint<SerializableGetInfoCommandInput,SerializableGetInfoResults> {
public:
	GetInfoAggregatorAndPrint(RoutingManager * routingManager, ConfigManager * configurationManager, evhttp_request *req){
		this->routingManager = routingManager;
		this->configurationManager = configurationManager;
		this->req = req;

		this->readCount = 0;
		this->writeCount = 0;
		this->numberOfDocumentsInIndex  = 0;
		this->docCount = 0;
	}

	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo, SerializableGetInfoCommandInput * sentRequest,
			ResultsAggregatorAndPrintMetadata metadata){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		messages << "{\"shard getInfo\":\"failed\",\"reason\":\"Corresponging shard ("<<
						coreShardInfo->shardId<<") timedout.\"}";
	}


	/*
	 * The main function responsible of aggregating status (success or failure) results
	 */
	void callBack(vector<SerializableGetInfoResults *> responseObjects){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		for(vector<SerializableGetInfoResults *>::iterator responseItr = responseObjects.begin();
				responseItr != responseObjects.end() ; ++responseItr){
			this->readCount += (*responseItr)->getReadCount();
			this->writeCount += (*responseItr)->getWriteCount();
			this->numberOfDocumentsInIndex = (*responseItr)->getNumberOfDocumentsInIndex();
			this->lastMergeTimeStrings.push_back((*responseItr)->getLastMergeTimeString());
			this->docCount += (*responseItr)->getDocCount();
			this->versionInfoStrings.push_back((*responseItr)->getVersionInfo());
		}
	}

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultsAggregatorAndPrintMetadata metadata){

		//TODO : this print should be checked to make sure it prints correct json format
		std::stringstream str;
        str << "\"engine_status\":{";
        str << "\"search_requests\":\"" << this->readCount << "\",";
        str << "\"write_requests\":\"" <<  this->writeCount << "\",";
        str << "\"docs_in_index\":\"" << this->numberOfDocumentsInIndex << "\",";
        for(unsigned i=0; i < lastMergeTimeStrings.size() ; ++i){
			str << "\"shard_status\":{";
        	str << "\"last_merge\":\"" << this->lastMergeTimeStrings.at(i) << "\",";
        	str << "\"version\":\"" << this->versionInfoStrings.at(i) << "\"";
			str << "},";
        }
        str << "\"doc_count\":\"" << this->docCount << "\",";
        str << "}\n";
        str << "\"messages\":[" << messages.str() << "]\n";
        Logger::info("%s", messages.str().c_str());

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + str.str() + "]}\n");
	}

private:
	RoutingManager * routingManager;
	ConfigManager * configurationManager;
	evhttp_request *req;

	mutable boost::shared_mutex _access;
	std::stringstream messages;
	unsigned readCount;
	unsigned writeCount;
	unsigned numberOfDocumentsInIndex;
	vector<string> lastMergeTimeStrings;
	vector<string> versionInfoStrings;
	unsigned docCount;
};

}
}


#endif // __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
