// $Id: QueryResults.cpp 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */
#include "QueryExecutor.h"
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Indexer.h>
#include "ParsedParameterContainer.h" // only for ParameterName enum , FIXME : must be changed when we fix constants problem
#include "query/QueryResultsInternal.h"
#include "operation/QueryEvaluatorInternal.h"
#include "src/sharding/configuration/ConfigManager.h"
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
    		configuration->getGetAllResultsNumberOfResultsToFindInEstimationMode(), configuration);
    this->queryEvaluator = new srch2is::QueryEvaluator(server->getIndexer() , &runTimeParameters );

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

    int idsFound = queryEvaluator->search(&queryPlan,finalResults);

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

void QueryExecutor::executePostProcessingPlan(Query * query,
        QueryResults * inputQueryResults, QueryResults * outputQueryResults) {
    QueryEvaluatorInternal * queryEvaluatorInternal = this->queryEvaluator->impl;
    ForwardIndex * forwardIndex = queryEvaluatorInternal->getForwardIndex();
    Schema * schema = queryEvaluatorInternal->getSchema();

    // run a plan by iterating on filters and running
    ResultsPostProcessorPlan * postProcessingPlan =
            this->queryPlan.getPostProcessingPlan();

    // short circuit in case the plan doesn't have any filters in it.
    // if no plan is set in Query or there is no filter in it,
    // then there is no post processing so just mirror the results
    // TODO : in the future try to avoid this copy of pointers
    if (postProcessingPlan == NULL) {
        outputQueryResults->copyForPostProcessing(inputQueryResults);
        return;
    }

    postProcessingPlan->beginIteration();
    if (!postProcessingPlan->hasMoreFilters()) {
        outputQueryResults->copyForPostProcessing(inputQueryResults);
        postProcessingPlan->closeIteration();
        return;
    }

    // iterating on filters and applying them on list of results
    ResultsPostProcessorFilter * filter = postProcessingPlan->nextFilter();
    while(true){
        // clear the output to be ready to accept the results of the filter
        outputQueryResults->clear();
        // apply the filter on the input and put the results in output
        filter->doFilter(queryEvaluator, query, inputQueryResults,
                outputQueryResults);
        // if there is going to be other filters, chain the output to the input
        if (postProcessingPlan->hasMoreFilters()) {
            inputQueryResults->copyForPostProcessing(outputQueryResults);
        }
        //
        if(postProcessingPlan->hasMoreFilters()){
            filter = postProcessingPlan->nextFilter();
        }else{
            break;
        }
    }
    postProcessingPlan->closeIteration();
}

}
}
