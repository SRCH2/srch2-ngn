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
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Indexer.h>
#include "ParsedParameterContainer.h" // only for ParameterName enum , FIXME : must be changed when we fix constants problem

namespace srch2{
namespace httpwrapper{


QueryExecutor::QueryExecutor(QueryPlan & queryPlan , QueryResultFactory * resultsFactory  ,Srch2Server *server){
	this->queryPlan = queryPlan;
	this->queryResultFactory = resultsFactory;
	this->server = server;
}

void QueryExecutor::execute(QueryResults * finalResults){

    //urlParserHelper.print();
    //evhttp_clear_headers(&headers);
    this->indexSearcher = srch2is::IndexSearcher::create(server->indexer);
    //do the search
    switch(queryPlan.searchType)
    {
    case TopKSearchType://TopK
    {

    	executeTopK(finalResults);
    }
    break;

    case GetAllResultsSearchType://GetAllResults
    {
    	executeGetAllResults(finalResults);
    }
    break;

    case GeoSearchType://MapQuery
    {
    	executeGeo(finalResults);
    }
    break;
    };

    // Free objects

    delete indexSearcher;


}


void QueryExecutor::executeTopK(QueryResults * finalResults){

    int idsExactFound = 0;



	srch2is::QueryResults *exactQueryResults =  new QueryResults(this->queryResultFactory,indexSearcher,this->queryPlan.getExactQuery());
	idsExactFound = indexSearcher->search(this->queryPlan.getExactQuery(), exactQueryResults, 0, this->queryPlan.getOffset() + this->queryPlan.getResultsToRetrieve());

	//fill visitedList
	std::set<std::string> exactVisitedList;
	for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
	{
		exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
	}

	int idsFuzzyFound = 0;

	if ( this->queryPlan.isIsFuzzy() && (idsExactFound < (int)(this->queryPlan.getResultsToRetrieve() + this->queryPlan.getOffset())))
	{
		QueryResults *fuzzyQueryResults = new QueryResults(this->queryResultFactory,indexSearcher, this->queryPlan.getFuzzyQuery());
		idsFuzzyFound = indexSearcher->search(this->queryPlan.getFuzzyQuery(), fuzzyQueryResults, 0, this->queryPlan.getOffset() + this->queryPlan.getResultsToRetrieve());
		// create final queryResults to print.

		QueryResultsInternal *exact_qs = exactQueryResults->impl;
		QueryResultsInternal *fuzzy_qs = fuzzyQueryResults->impl;

		unsigned fuzzyQueryResultsIter = 0;

		while (exact_qs->sortedFinalResults.size() < (unsigned)(this->queryPlan.getOffset() + this->queryPlan.getResultsToRetrieve())
				&& fuzzyQueryResultsIter < fuzzyQueryResults->getNumberOfResults())
		{
			std::string recordId = fuzzyQueryResults->getRecordId(fuzzyQueryResultsIter);
			if ( ! exactVisitedList.count(recordId) )// recordid not there
					{
				exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[fuzzyQueryResultsIter]);
					}
			fuzzyQueryResultsIter++;
		}
		delete fuzzyQueryResults;
	}


	// execute post processing

    finalResults->init(this->queryResultFactory,indexSearcher, this->queryPlan.getExactQuery());
    executePostProcessingPlan(this->queryPlan.getExactQuery(), exactQueryResults, finalResults);


    delete exactQueryResults;
}

void QueryExecutor::executeGetAllResults(QueryResults * finalResults){

    int idsExactFound = 0;

	srch2is::QueryResults *queryResults = NULL;
	unsigned idsFound = 0;

	if ( ! this->queryPlan.isIsFuzzy() )
	{
		queryResults = new srch2is::QueryResults(this->queryResultFactory,indexSearcher, this->queryPlan.getExactQuery());
		idsFound = indexSearcher->search(this->queryPlan.getExactQuery(), queryResults, 0);
	}
	else
	{
		queryResults = new srch2is::QueryResults(this->queryResultFactory,indexSearcher, this->queryPlan.getFuzzyQuery());
		idsFound = indexSearcher->search(this->queryPlan.getFuzzyQuery(), queryResults, 0);
	}

	// execute post processing

	finalResults->init(this->queryResultFactory,indexSearcher,
    					(this->queryPlan.isIsFuzzy())?this->queryPlan.getFuzzyQuery():this->queryPlan.getExactQuery());
	executePostProcessingPlan((this->queryPlan.isIsFuzzy())?this->queryPlan.getFuzzyQuery():this->queryPlan.getExactQuery(),
			queryResults,finalResults);


	delete queryResults;


}

void QueryExecutor::executeGeo(QueryResults * finalResults){

    int idsExactFound = 0;
	// for the range query without keywords.
	srch2is::QueryResults *exactQueryResults = new QueryResults(this->queryResultFactory,indexSearcher, this->queryPlan.getExactQuery());
	if(this->queryPlan.getExactQuery()->getQueryTerms()->empty())//check if query type is a range query without keywords
	{
		vector<double> values;
		this->queryPlan.getExactQuery()->getRange(values);//get  query range: use the number of values to decide if it is rectangle range or circle range
		//range query with a circle
		if (values.size()==3)
		{
			Point p;
			p.x = values[0];
			p.y = values[1];
			Circle *circleRange = new Circle(p, values[2]);
			indexSearcher->search(*circleRange, exactQueryResults);
			delete circleRange;
		}
		else
		{
			pair<pair<double, double>, pair<double, double> > rect;
			rect.first.first = values[0];
			rect.first.second = values[1];
			rect.second.first = values[2];
			rect.second.second = values[3];
			Rectangle *rectangleRange = new Rectangle(rect);
			indexSearcher->search(*rectangleRange, exactQueryResults);
			delete rectangleRange;
		}
	}
	else// keywords and geo search
	{
		//cout << "reached map query" << endl;
		//srch2is::QueryResults *exactQueryResults = srch2is::QueryResults::create(indexSearcher, urlToDoubleQuery->exactQuery);
		indexSearcher->search(this->queryPlan.getExactQuery(), exactQueryResults);
		idsExactFound = exactQueryResults->getNumberOfResults();

		//fill visitedList
		std::set<std::string> exactVisitedList;
		for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
		{
			exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
		}

		int idsFuzzyFound = 0;

		if ( this->queryPlan.isIsFuzzy() && idsExactFound < (int)( this->queryPlan.getOffset() + this->queryPlan.getResultsToRetrieve()))
		{
			QueryResults *fuzzyQueryResults = new QueryResults(this->queryResultFactory,indexSearcher, this->queryPlan.getFuzzyQuery());
			indexSearcher->search(this->queryPlan.getFuzzyQuery(), fuzzyQueryResults);
			idsFuzzyFound = fuzzyQueryResults->getNumberOfResults();

			// create final queryResults to print.
			QueryResultsInternal *exact_qs = exactQueryResults->impl;
			QueryResultsInternal *fuzzy_qs = fuzzyQueryResults->impl;

			unsigned fuzzyQueryResultsIter = 0;

			while (exact_qs->sortedFinalResults.size() < (unsigned)(this->queryPlan.getOffset() + this->queryPlan.getResultsToRetrieve())
					&& fuzzyQueryResultsIter < fuzzyQueryResults->getNumberOfResults())
			{
				std::string recordId = fuzzyQueryResults->getRecordId(fuzzyQueryResultsIter);
				if ( ! exactVisitedList.count(recordId) )// recordid not there
				{
					exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[fuzzyQueryResultsIter]);
				}
				fuzzyQueryResultsIter++;
			}
			delete fuzzyQueryResults;
		}
	}


	// execute post processing
    finalResults->init(this->queryResultFactory,indexSearcher, this->queryPlan.getExactQuery());
    executePostProcessingPlan(this->queryPlan.getExactQuery(), exactQueryResults, finalResults);


    delete exactQueryResults;

}

void QueryExecutor::executePostProcessingPlan(Query * query,QueryResults * input, QueryResults *  output){
		IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal * >(indexSearcher);
		ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();
		Schema * schema = indexSearcherInternal->getSchema();


		// run a plan by iterating on filters and running

		// short circuit in case the plan doesn't have any filters in it.
		ResultsPostProcessorPlan * plan = this->queryPlan.getPostProcessingPlan();




		// if no plan is set in Query, there is no post processing, just mirror the results
		if(plan == NULL){
			input->impl->copyForPostProcessing(output->impl);
			return;
		}

		// iterating on filters and applying them on list of results
		QueryResults * inputOperand = input;
		plan->beginIteration();
		while(plan->hasMoreFilters()){
			ResultsPostProcessorFilter * filter = plan->nextFilter();

			QueryResults outputOperand(this->queryResultFactory,indexSearcherInternal,query);
			filter->doFilter(indexSearcher, query, inputOperand, &outputOperand);

			if(!plan->hasMoreFilters()){ // TODO : after adding pointer logic reimplement this logic
				// copy new results to the output
				outputOperand.impl->copyForPostProcessing(output->impl);
			}else{
				// pass the input to the next filter
				inputOperand = &outputOperand;
			}
		}
		plan->closeIteration();

}

}
}
