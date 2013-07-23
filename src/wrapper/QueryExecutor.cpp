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

namespace srch2{
namespace httpwrapper{


QueryExecutor::QueryExecutor(QueryPlan & queryPlan , QueryResultFactory * resultsFactory  ){
	this->queryPlan = queryPlan;
	this->queryResultFactory = resultsFactory;
}

void QueryExecutor::execute(QueryResults * finalResults){

    //urlParserHelper.print();
    //evhttp_clear_headers(&headers);

    int idsExactFound = 0;
    srch2is::IndexSearcher *indexSearcher = srch2is::IndexSearcher::create(server->indexer);
    srch2is::QueryResultFactory * resultsFactory = new srch2is::QueryResultFactory();

    //do the search
    switch(urlParserHelper.searchType)
    {
    case 0://TopK
    {
    	if (indexDataContainerConf->getIndexType() != 0)
    	{
    		evhttp_send_reply(req, HTTP_BADREQUEST, "Query type is wrong", NULL);
    	}
    	else
    	{

    		srch2is::QueryResults *exactQueryResults = new srch2is::QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->exactQuery);
    		idsExactFound = indexSearcher->search(urlToDoubleQuery->exactQuery, exactQueryResults, 0, urlParserHelper.offset + urlParserHelper.resultsToRetrieve);

    		//fill visitedList
    		std::set<std::string> exactVisitedList;
    		for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
    		{
    			exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
    		}

    		int idsFuzzyFound = 0;

    		if ( urlParserHelper.isFuzzy && (idsExactFound < (int)(urlParserHelper.resultsToRetrieve + urlParserHelper.offset)))
    		{
    			QueryResults *fuzzyQueryResults = new QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->fuzzyQuery);
    			idsFuzzyFound = indexSearcher->search(urlToDoubleQuery->fuzzyQuery, fuzzyQueryResults, 0, urlParserHelper.offset + urlParserHelper.resultsToRetrieve);
    			// create final queryResults to print.

    			QueryResultsInternal *exact_qs = exactQueryResults->impl;
    			QueryResultsInternal *fuzzy_qs = fuzzyQueryResults->impl;

    			unsigned fuzzyQueryResultsIter = 0;

    			while (exact_qs->sortedFinalResults.size() < (unsigned)(urlParserHelper.offset + urlParserHelper.resultsToRetrieve)
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





    		//                ResultsPostProcessor postProcessor(indexSearcher);


    		QueryResults * finalQueryResults = new QueryResults(resultsFactory,indexSearcher,urlToDoubleQuery->exactQuery);
    		//                postProcessor.runPlan(urlToDoubleQuery->exactQuery, exactQueryResults , finalQueryResults);



    		// compute elapsed time in ms
    		struct timespec tend;
    		clock_gettime(CLOCK_REALTIME, &tend);
    		unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    		//std::stringstream search_time;
    		//search_time << req->uri << "|" << idsExactFound <<"[executed in " << ts1 << " ms]";

    		//std::cout << search_time.str() << std::endl;
    		//TODO: Logging
    		//cry_wrapper(conn, search_time.str().c_str());

    		unsigned idsFound = finalQueryResults->getNumberOfResults();

    		finalQueryResults->printStats();

    		HTTPRequestHandler::printResults(
    				req,
    				headers,
    				urlParserHelper,
    				indexDataContainerConf,
    				finalQueryResults,
    				urlToDoubleQuery->exactQuery,
    				server->indexer,
    				urlParserHelper.offset,
    				idsFound,
    				idsFound,
    				ts1,
    				tstart,
    				tend);

    		delete exactQueryResults;
    		delete finalQueryResults;
    	}
    }
    break;

    case 1://GetAllResults
    {
    	if (indexDataContainerConf->getIndexType() != 0)
    	{
    		evhttp_send_reply(req, HTTP_BADREQUEST, "Query type is wrong", NULL);
    	}
    	else
    	{

    		srch2is::QueryResults *queryResults = NULL;
    		unsigned idsFound = 0;

    		if ( !urlParserHelper.isFuzzy )
    		{
    			queryResults = new srch2is::QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->exactQuery);
    			idsFound = indexSearcher->search(urlToDoubleQuery->exactQuery, queryResults, 0);
    		}
    		else
    		{
    			queryResults =  new QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->fuzzyQuery);
    			idsFound = indexSearcher->search(urlToDoubleQuery->fuzzyQuery, queryResults, 0);
    		}

    		// compute elapsed time in ms
    		struct timespec tend;
    		clock_gettime(CLOCK_REALTIME, &tend);
    		unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    		//std::stringstream search_time;
    		//search_time << req->uri << "|" << idsExactFound <<"[executed in " << ts1 << " ms]";

    		//std::cout << search_time.str() << std::endl;
    		//TODO: Logging
    		//cry_wrapper(conn, search_time.str().c_str());



    		//            	ResultsPostProcessor postProcessor(indexSearcher);


    		QueryResults * finalQueryResults = new QueryResults(resultsFactory,indexSearcher,
    				(urlParserHelper.isFuzzy)?urlToDoubleQuery->fuzzyQuery:urlToDoubleQuery->exactQuery);

    		//            	postProcessor.runPlan(
    		//            			(urlParserHelper.isFuzzy)?urlToDoubleQuery->fuzzyQuery:urlToDoubleQuery->exactQuery,
    		//            					queryResults , finalQueryResults);




    		queryResults->printStats();

    		if (urlParserHelper.offset + urlParserHelper.resultsToRetrieve  > idsFound) // Case where you have return 10,20, but we got only 0,15 results.
    		{
    			HTTPRequestHandler::printResults(req, headers, urlParserHelper, indexDataContainerConf, finalQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
    					urlParserHelper.offset, idsFound, idsFound, ts1, tstart, tend);
    		}
    		else // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
    		{
    			HTTPRequestHandler::printResults(req, headers, urlParserHelper, indexDataContainerConf, finalQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
    					urlParserHelper.offset, urlParserHelper.offset + urlParserHelper.resultsToRetrieve, idsFound, ts1, tstart, tend);
    		}

    		delete queryResults;
    		delete finalQueryResults;
    	}
    }
    break;

    case 2://MapQuery
    {
    	if (indexDataContainerConf->getIndexType() != 1)
    	{
    		bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", "{\"error\":\"query type is wrong\"}", headers);
    	}
    	else
    	{
    		// for the range query without keywords.
    		srch2is::QueryResults *exactQueryResults = new srch2is::QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->exactQuery);
    		if(urlToDoubleQuery->exactQuery->getQueryTerms()->empty())//check if query type is a range query without keywords
    		{
    			vector<double> values;
    			urlToDoubleQuery->exactQuery->getRange(values);//get  query range: use the number of values to decide if it is rectangle range or circle range
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
    			indexSearcher->search(urlToDoubleQuery->exactQuery, exactQueryResults);
    			idsExactFound = exactQueryResults->getNumberOfResults();

    			//fill visitedList
    			std::set<std::string> exactVisitedList;
    			for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
    			{
    				exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
    			}

    			int idsFuzzyFound = 0;

    			if ( urlParserHelper.isFuzzy && idsExactFound < (int)(urlParserHelper.resultsToRetrieve+urlParserHelper.offset))
    			{
    				QueryResults *fuzzyQueryResults = new QueryResults(resultsFactory,indexSearcher, urlToDoubleQuery->fuzzyQuery);
    				indexSearcher->search( urlToDoubleQuery->fuzzyQuery, fuzzyQueryResults);
    				idsFuzzyFound = fuzzyQueryResults->getNumberOfResults();

    				// create final queryResults to print.
    				QueryResultsInternal *exact_qs = exactQueryResults->impl;
    				QueryResultsInternal *fuzzy_qs = fuzzyQueryResults->impl;

    				unsigned fuzzyQueryResultsIter = 0;

    				while (exact_qs->sortedFinalResults.size() < (unsigned)(urlParserHelper.offset + urlParserHelper.resultsToRetrieve)
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
    		// compute elapsed time in ms
    		struct timespec tend;
    		clock_gettime(CLOCK_REALTIME, &tend);
    		unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    		//std::stringstream search_time;
    		//search_time << req->uri << "[executed in " << ts1 << " ms]";

    		//TODO: Logging
    		//cry_wrapper(conn, search_time.str().c_str());

    		int idsFound = exactQueryResults->getNumberOfResults();

    		exactQueryResults->printStats();

    		//cout << "reached map query executed" << endl;

    		if (urlParserHelper.offset + urlParserHelper.resultsToRetrieve  > idsFound) // Case where you have return 10,20, but we got only 0,15 results.
    		{
    			HTTPRequestHandler::printResults(req, headers, urlParserHelper, indexDataContainerConf, exactQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
    					urlParserHelper.offset, idsFound, idsFound, ts1, tstart, tend);
    		}
    		else // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
    		{
    			HTTPRequestHandler::printResults(req, headers, urlParserHelper, indexDataContainerConf, exactQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
    					urlParserHelper.offset, urlParserHelper.offset + urlParserHelper.resultsToRetrieve, idsFound, ts1, tstart, tend);
    		}

    		delete exactQueryResults;
    	}
    }
    };
    delete indexSearcher;
    delete resultsFactory;

    // Free the objects
    evhttp_clear_headers(&headers);
    delete urlToDoubleQuery;
}


}
}
