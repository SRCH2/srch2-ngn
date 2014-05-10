#ifndef __SHARDING_PROCESSOR_SEARCH_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_SEARCH_RESULTS_AGGREGATOR_AND_PRINT_H_

#include "ResultsAggregatorAndPrint.h"
#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableSearchCommandInput.h"
#include <event2/http.h>
#include "wrapper/ParsedParameterContainer.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

class SearchResultAggregatorAndPrint : public ResultAggregatorAndPrint<SerializableSearchCommandInput , SerializableSearchResults> {

public:

	SearchResultAggregatorAndPrint(ConfigManager * configurationManager, evhttp_request *req, CoreShardInfo * coreShardInfo);
	LogicalPlan & getLogicalPlan();
	ParsedParameterContainer * getParamContainer();

	void setParsingValidatingRewritingTime(unsigned time);

	unsigned getParsingValidatingRewritingTime();

	struct timespec & getStartTimer();
	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata){};
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(ShardId * shardInfo,
			SerializableSearchCommandInput * sentRequest,
			ResultsAggregatorAndPrintMetadata metadata){

	}


	/*
	 * The main function responsible of aggregating search results
	 * this function uses aggregateRecords and aggregateFacets for
	 * aggregating result records and calculated records
	 */
	void callBack(vector<const SerializableSearchResults *> responseObjects);

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
	vector<map<unsigned, string> > inMemoryRecordStrings;

	/*
	 * This variable contains the final aggregated results
	 */
	SearchResultAggregatorAndPrint::AggregatedQueryResults results;
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


}
}


#endif // __SHARDING_PROCESSOR_SEARCH_RESULTS_AGGREGATOR_AND_PRINT_H_
