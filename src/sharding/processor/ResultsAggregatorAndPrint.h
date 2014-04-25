#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class ResultAggregatorAndPrint {
public:
	struct Metadata{

	}
public:

	ResultAggregatorAndPrint(RoutingManager * routingManager,
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


	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultAggregatorAndPrint::Metadata metadata){

	}
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and the actual MessageType dependent
	 * call back function (e.g. aggregateSearchResults(...) )
	 */
	void timeoutProcessing(){

	}
	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultAggregatorAndPrint::Metadata metadata){

	}


	/*
	 * The main function responsible of aggregating search results
	 * this function uses aggregateRecords and aggregateFacets for
	 * aggregating result records and calculated records
	 */
	void aggregateSearchResults(RoutingManager::Message * messages);

	void printResults();

	/**
	 * Iterate over the recordIDs in queryResults and get the record.
	 * Add the record information to the request.out string.
	 */
	void ResultAggregatorAndPrint::printResults(evhttp_request *req,
	        const evkeyvalq &headers, const LogicalPlan &queryPlan,
	        const CoreInfo_t *indexDataConfig,
	        const vector<QueryResult *> queryResultsVector, const Query *query,
	        const unsigned start, const unsigned end,
	        const unsigned retrievedResults, const string & message,
	        const unsigned ts1, struct timespec &tstart, struct timespec &tend ,
	        const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) ;


	void aggregateCommandStatus();

	/*
	 * The main function responsible of aggregating info coming from
	 * different shards
	 */
	void aggregateCoreInfo(RoutingManager::Message * messages);


	/*
	 * The main function responsible of aggregating success/failure of
	 * serlializing indexes and records
	 */
	void aggregateSerlializingIndexesAndRecords(RoutingManager::Message * messages);

	/*
	 * The main function responsible of aggregating success/failure of
	 * resetting logs
	 */
	void aggregateResettingLogs(RoutingManager::Message * messages);


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
	ResultAggregatorAndPrint::AggregatedQueryResults results;

	RoutingManager * routingManager;
	ConfigManager * configurationManager;
	LogicalPlan * logicalPlan;
	evhttp_request *req;
	CoreShardInfo * coreShardInfo;
	ParsedParameterContainer paramContainer;
};



class Partitioner {
public:

	Partitioner(RoutingManager * transportManager, ConfigManager * configurationManager){
		this->routingManager = transportManager;
		this->configurationManager = configurationManager;
	}

	/*
	 * Hash implementation :
	 * 1. Uses getRecordValueToHash to find the value to be hashed for this record
	 * 2. Uses SynchronizationManager to get total number of shards to know the hash space
	 * 3. Uses hash(...) to choose which shard should be responsible for this record
	 * 4. Returns the information of corresponding Shard (which can be discovered from SM)
	 */
	unsigned getShardIDForRecord(Record * record);


private:
	/*
	 * Uses Configuration file and the given expression to
	 * calculate the record corresponding value to be hashed
	 * for example if this value is just ID for each record we just return
	 * the value of ID
	 */
	unsigned getRecordValueToHash(Record * record);

	/*
	 * Uses a hash function to hash valueToHash to a value in range [0,hashSpace)
	 * and returns this value
	 */
	unsigned hash(unsigned valueToHash, unsigned hashSpace);


private:
	RoutingManager * routingManager;
	ConfigManager * configurationManager;
};



}
}


#endif // __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
