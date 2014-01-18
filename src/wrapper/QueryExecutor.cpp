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
	 * 2. MapQuery and retrievById will remain unchanged (their search function must change because the names will change)
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
    QueryEvaluatorRuntimeParametersContainer runTimeParameters(this->server->indexDataContainerConf->getKeywordPopularityThreshold(),
    		this->server->indexDataContainerConf->getGetAllResultsNumberOfResultsThreshold() ,
    		this->server->indexDataContainerConf->getGetAllResultsNumberOfResultsToFindInEstimationMode());
    this->queryEvaluator = new srch2is::QueryEvaluator(server->indexer , &runTimeParameters );

    //do the search
    switch (queryPlan.getQueryType()) {
    case srch2is::SearchTypeTopKQuery: //TopK
    case srch2is::SearchTypeGetAllResultsQuery: //GetAllResults
        executeKeywordSearch(finalResults);
        break;
    case srch2is::SearchTypeMapQuery: //MapQuery
        executeGeo(finalResults);
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


void QueryExecutor::executeGeo(QueryResults * finalResults) {

    // execute post processing
    // since this object is only allocated with an empty constructor, this init function needs to be called to
    // initialize the object.
    finalResults->init(this->queryResultFactory, queryEvaluator,
            this->queryPlan.getExactQuery());

    int idsExactFound = 0;
    // for the range query without keywords.
    srch2is::QueryResults *exactQueryResults = new QueryResults(
            this->queryResultFactory, queryEvaluator,
            this->queryPlan.getExactQuery());
    if (this->queryPlan.getExactQuery()->getQueryTerms()->empty()) //check if query type is a range query without keywords
    {
        vector<double> values;
        this->queryPlan.getExactQuery()->getRange(values); //get  query range: use the number of values to decide if it is rectangle range or circle range
        //range query with a circle
        if (values.size() == 3) {
            Point p;
            p.x = values[0];
            p.y = values[1];
            Circle *circleRange = new Circle(p, values[2]);
            queryEvaluator->geoSearch(*circleRange, exactQueryResults);
            delete circleRange;
        } else {
            pair<pair<double, double>, pair<double, double> > rect;
            rect.first.first = values[0];
            rect.first.second = values[1];
            rect.second.first = values[2];
            rect.second.second = values[3];
            Rectangle *rectangleRange = new Rectangle(rect);
            queryEvaluator->geoSearch(*rectangleRange, exactQueryResults);
            delete rectangleRange;
        }
    } else // keywords and geo search
    {
        queryEvaluator->geoSearch(this->queryPlan.getExactQuery(),
                exactQueryResults);
        idsExactFound = exactQueryResults->getNumberOfResults();

        //fill visitedList
        std::set<std::string> exactVisitedList;
        for (unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i) {
            exactVisitedList.insert(exactQueryResults->getRecordId(i)); // << queryResults->getRecordId(i);
        }

        int idsFuzzyFound = 0;

        if (this->queryPlan.isFuzzy()
                && idsExactFound
                        < (int) (this->queryPlan.getOffset()
                                + this->queryPlan.getNumberOfResultsToRetrieve())) {
            QueryResults *fuzzyQueryResults = new QueryResults(
                    this->queryResultFactory, queryEvaluator,
                    this->queryPlan.getFuzzyQuery());
            queryEvaluator->geoSearch(this->queryPlan.getFuzzyQuery(),
                    fuzzyQueryResults);
            idsFuzzyFound = fuzzyQueryResults->getNumberOfResults();

            // create final queryResults to print.
            QueryResultsInternal *exact_qs = exactQueryResults->impl;
            QueryResultsInternal *fuzzy_qs = fuzzyQueryResults->impl;

            unsigned fuzzyQueryResultsIter = 0;

            while (exact_qs->sortedFinalResults.size()
                    < (unsigned) (this->queryPlan.getOffset()
                            + this->queryPlan.getNumberOfResultsToRetrieve())
                    && fuzzyQueryResultsIter
                            < fuzzyQueryResults->getNumberOfResults()) {
                std::string recordId = fuzzyQueryResults->getRecordId(
                        fuzzyQueryResultsIter);
                if (!exactVisitedList.count(recordId)) // recordid not there
                        {
                    exact_qs->sortedFinalResults.push_back(
                            fuzzy_qs->sortedFinalResults[fuzzyQueryResultsIter]);
                }
                fuzzyQueryResultsIter++;
            }
            delete fuzzyQueryResults;
        }
    }

    // this post processing plan will be applied on exactQueryResults object and
    // the final results will be copied into finalResults
    executePostProcessingPlan(this->queryPlan.getExactQuery(),
            exactQueryResults, finalResults);

    delete exactQueryResults;
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
