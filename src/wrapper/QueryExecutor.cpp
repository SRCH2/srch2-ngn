#include "QueryExecutor.h"
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Indexer.h>
#include "ParsedParameterContainer.h" // only for ParameterName enum , FIXME : must be changed when we fix constants problem
#include "query/QueryResultsInternal.h"
#include "operation/QueryEvaluatorInternal.h"
#include "ConfigManager.h"
#include "util/Assert.h"

namespace srch2 {
namespace httpwrapper {

// we need config manager to pass estimatedNumberOfResultsThresholdGetAll & numberOfEstimatedResultsToFindGetAll
// in the case of getAllResults.
QueryExecutor::QueryExecutor(LogicalPlan & queryPlan,
        QueryResultFactory * resultsFactory, Srch2Server *server, const CoreInfo_t * config) :
        queryPlan(queryPlan), configuration(config) {
    this->queryResultFactory = resultsFactory;
    this->server = server;
}

void QueryExecutor::execute(QueryResults * finalResults) {


	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	/*
	 * changes:
	 * 1. GetAll and topK must be merged. The difference must be pushed to the core.
	 * 2. retrievById will remain unchanged (their search function must change because the names will change)
	 * 3. LogicalPlan must be passed to QueryEvaluator (which is in core) to be evaluated.
	 * 4. No exact/fuzzy policy must be applied here.
	 * 5. Postprocessing framework must be prepared to be applied on the results (its code comes from QueryPlanGen)
	 * 6. Post processing filters are applied on results list.
	 */
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////

    //urlParserHelper.print();
    //evhttp_clear_headers(&headers);
    // "IndexSearcherRuntimeParametersContainer" is the class which contains the parameters that we want to send to the core.
    // Each time IndexSearcher is created, we container must be made and passed to it as an argument.
    QueryEvaluatorRuntimeParametersContainer runTimeParameters(configuration->getKeywordPopularityThreshold(),
    		configuration->getGetAllResultsNumberOfResultsThreshold() ,
    		configuration->getGetAllResultsNumberOfResultsToFindInEstimationMode());
    this->queryEvaluator = new srch2is::QueryEvaluator(server->indexer , &runTimeParameters );

    //do the search
    switch (queryPlan.getQueryType()) {
    case srch2is::SearchTypeTopKQuery: //TopK
    case srch2is::SearchTypeGetAllResultsQuery: //GetAllResults
        executeKeywordSearch(finalResults);
        break;
    case srch2is::SearchTypeRetrieveById:
    	executeRetrieveById(finalResults);
    	break;
    default:
        ASSERT(false);
        break;
    };

    // Free objects
    delete queryEvaluator; // Physical plan and physical operators and physicalRecordItems are freed here
}

void QueryExecutor::executeKeywordSearch(QueryResults * finalResults) {


    // execute post processing
    // since this object is only allocated with an empty constructor, this init function needs to be called to
    // initialize the object.
    finalResults->init(this->queryResultFactory, queryEvaluator,
            this->queryPlan.getExactQuery());

    int idsFound = 0;

    idsFound = queryEvaluator->search(&queryPlan,finalResults);

    // this post processing plan will be applied on exactQueryResults object and
    // the final results will be copied into finalResults
//    executePostProcessingPlan(this->queryPlan.getExactQuery(),
//            exactQueryResults, finalResults);

}

/*
 * Retrieves the result by the primary key given by the user.
 */
void QueryExecutor::executeRetrieveById(QueryResults * finalResults){

	// since this object is only allocated with an empty constructor, this init function needs to be called to
    // initialize the object.
	// There is no Query object for this type of search, so we pass NULL.
    finalResults->init(this->queryResultFactory, queryEvaluator,NULL);
    this->queryEvaluator->search(this->queryPlan.getDocIdForRetrieveByIdSearchType() , finalResults);

}

}
}
