#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_

#include <instantsearch/Record.h>
#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableCommandStatus.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"
#include "Partitioner.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class Request, class Response>
class ResultAggregatorAndPrint {
public:
	struct Metadata{

	};

	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	virtual void preProcessing(ResultAggregatorAndPrint::Metadata metadata){};
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	virtual void timeoutProcessing(CoreShardInfo * coreShardInfo,
			Request * sentRequest, ResultAggregatorAndPrint::Metadata metadata){};

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
	virtual void finalize(ResultAggregatorAndPrint::Metadata metadata){};


	virtual ~ResultAggregatorAndPrint(){};

};

class SearchResultAggregatorAndPrint : public ResultAggregatorAndPrint<SerializableSearchCommandInput , SerializableSearchResults> {

public:

	SearchResultAggregatorAndPrint(RoutingManager * routingManager,
			ConfigManager * configurationManager,
			evhttp_request *req,
			CoreShardInfo * coreShardInfo){
		this->routingManager = routingManager;
		this->configurationManager = configurationManager;
		this->req = req;
		this->coreShardInfo = coreShardInfo;
	}

	void setLogicalPlan(LogicalPlan * logicalPlan){
		this->logicalPlan = logicalPlan;
	}
	LogicalPlan * getLogicalPlan(){
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
	void preProcessing(ResultAggregatorAndPrint<SerializableSearchCommandInput ,SerializableSearchResults>::Metadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo,
			SerializableSearchCommandInput * sentRequest,
			ResultAggregatorAndPrint<SerializableSearchCommandInput ,
			SerializableSearchResults>::Metadata metadata){
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
	void finalize(ResultAggregatorAndPrint<SerializableSearchCommandInput ,SerializableSearchResults>::Metadata metadata){
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
	        const QueryResults *queryResults,
	        const srch2is::Indexer *indexer,
	        const string & message,
	        const unsigned ts1);

	class QueryResultsComparator{
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
	LogicalPlan * logicalPlan;
	unsigned parsingValidatingRewritingTime;
	struct timespec tstart;
	evhttp_request *req;
	CoreShardInfo * coreShardInfo;
	ParsedParameterContainer paramContainer;

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
	void preProcessing(ResultAggregatorAndPrint<RequestWithStatusResponse,SerializableCommandStatus>::Metadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo, RequestWithStatusResponse * sentRequest,
			ResultAggregatorAndPrint<RequestWithStatusResponse,SerializableCommandStatus>::Metadata metadata){
		if(((string)"SerializableInsertUpdateCommandInput").compare(typeid(sentRequest).name()) == 0){// timeout in insert and update
			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableInsertUpdateCommandInput * sentInsetUpdateRequest = dynamic_cast<SerializableInsertUpdateCommandInput>(sentRequest);
			messages << "{\"rid\":\"" << rsentInsetUpdateRequest->record->getPrimaryKey()
					<< "\",\""+sentInsetUpdateRequest->insertOrUpdate?"insert":"update"+"\":\"failed\",\"reason\":\"Corresponging shard ("<<
							coreShardInfo->shardId<<") timedout.\"}";
		}else if (((string)"SerializableDeleteCommandInput").compare(typeid(sentRequest).name()) == 0){
			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableDeleteCommandInput * sentDeleteRequest = dynamic_cast<SerializableDeleteCommandInput>(sentRequest);
			messages << "{\"rid\":\"" << sentDeleteRequest->primaryKey
					<< "\",\"delete\":\"failed\",\"reason\":\"Corresponging ("<<
							coreShardInfo->shardId<<") shard timedout.\"}";
		}else if(((string)"SerializableSerializeCommandInput").compare(typeid(sentRequest).name()) == 0){
			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableSerializeCommandInput * serializeRequest = dynamic_cast<SerializableSerializeCommandInput>(sentRequest);
			messages << "{\""<< serializeRequest->indexOrRecord?"save":"export" <<"\":\"failed\",\"reason\":\"Corresponging ("<<
							coreShardInfo->shardId<<") shard timedout.\"}";
		}else if(((string)"SerializableResetLogCommandInput").compare(typeid(sentRequest).name()) == 0){
			SerializableResetLogCommandInput * resetRequest = dynamic_cast<SerializableResetLogCommandInput>(sentRequest);
			messages << "{\"reset_log\":\"failed\",\"reason\":\"Corresponging (" << coreShardInfo->shardId<<") shard timedout.\"}";
		}else if(((string)"SerializableCommitCommandInput").compare(typeid(sentRequest).name()) == 0){
			SerializableCommitCommandInput * resetRequest = dynamic_cast<SerializableCommitCommandInput>(sentRequest);
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
		messages << responseObject->message;

	}

	void callBack(vector<SerializableCommandStatus *> responseObjects){

		boost::unique_lock< boost::shared_mutex > lock(_access);
		for(vector<SerializableCommandStatus *>::iterator responseItr = responseObjects.begin(); responseItr != responseObjects.end(); ++responseItr){
			messages << (*responseItr)->message;
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
	void finalize(ResultAggregatorAndPrint<RequestWithStatusResponse,SerializableCommandStatus>::Metadata metadata){
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
	void preProcessing(ResultAggregatorAndPrint<SerializableGetInfoCommandInput,SerializableGetInfoResults>::Metadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(CoreShardInfo * coreShardInfo, SerializableGetInfoCommandInput * sentRequest,
			ResultAggregatorAndPrint<SerializableGetInfoCommandInput,SerializableGetInfoResults>::Metadata metadata){
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
			this->readCount += (*responseItr)->readCount;
			this->writeCount += (*responseItr)->writeCount;
			this->numberOfDocumentsInIndex = (*responseItr)->numberOfDocumentsInIndex;
			this->lastMergeTimeStrings.push_back((*responseItr)->lastMergeTimeString);
			this->docCount += (*responseItr)->docCount;
			this->versionInfoStrings.push_back((*responseItr)->versionInfo);
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
	void finalize(ResultAggregatorAndPrint<SerializableGetInfoCommandInput,SerializableGetInfoResults>::Metadata metadata){

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
