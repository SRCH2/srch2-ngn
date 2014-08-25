
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

#ifndef __QUERYEVALUATOR_H__
#define __QUERYEVALUATOR_H__

#include <instantsearch/platform.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/Indexer.h>
#include <string>
#include <record/LocationRecordUtil.h>
#include <operation/CacheManager.h>
#include <instantsearch/LogicalPlan.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Query.h>

namespace srch2
{
namespace instantsearch
{



class QueryEvaluatorRuntimeParametersContainer{
public:
	unsigned keywordPopularityThreshold;
	unsigned getAllMaximumNumberOfResults;
	unsigned getAllTopKReplacementK;

	QueryEvaluatorRuntimeParametersContainer(){
		keywordPopularityThreshold = 50000;
		getAllMaximumNumberOfResults = 500;
	}

	QueryEvaluatorRuntimeParametersContainer(unsigned keywordPopularityThreshold){
		this->keywordPopularityThreshold = keywordPopularityThreshold;
		this->getAllMaximumNumberOfResults = 500;
		this->getAllTopKReplacementK = 500;
	}

	QueryEvaluatorRuntimeParametersContainer(unsigned keywordPopularityThreshold, unsigned getAllMaximumNumberOfResults, unsigned getAllTopKReplacementK){
		this->keywordPopularityThreshold = keywordPopularityThreshold;
		this->getAllMaximumNumberOfResults = getAllMaximumNumberOfResults;
		this->getAllTopKReplacementK = getAllTopKReplacementK;
	}

	QueryEvaluatorRuntimeParametersContainer(const QueryEvaluatorRuntimeParametersContainer & copy){
		this->keywordPopularityThreshold = copy.keywordPopularityThreshold;
		this->getAllMaximumNumberOfResults = copy.getAllMaximumNumberOfResults;
		this->getAllTopKReplacementK = copy.getAllTopKReplacementK;
	}
};

class QueryEvaluatorInternal;

/**
 * QueryEvaluator provides an interface to do search using the
 * index. The QueryEvaluator internally is a wrapper around the
 * IndexReader and supports search. It also does caching and ranking.
 * QueryEvaluator receives LogicalPlan, builds the internal evaluation plan and
 * evaluates the physical plan using the internal structures.
 */
class MYLIB_EXPORT QueryEvaluator
{
public:
    /**
     * Creates an QueryEvaluator object.
     * @param indexer - An object holding the index structures and cache.
     */
	QueryEvaluator(Indexer *indexer , QueryEvaluatorRuntimeParametersContainer * parameters = NULL);

    /*
     * Finds the suggestions for a keyword based on fuzzyMatchPenalty.
     * Returns the number of suggestions found.
     */
	// TODO : FIXME: This function is not compatible with the new api
    int suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions );

    /**
     * If the search type is set to TopK in LogicalPlan, this function
     * finds the next topK answers starting from
     * offset. This function can be used to support pagination of
     * search results. queryResults is a QueryResults
     * object, which must be created using the same query object
     * as the first argument in this function.
     *
     * returns the number of records found (at most topK).
     *
     * if search type is getAllResults, this function finds all the results.
     */
    int search(LogicalPlan * logicalPlan , QueryResults *queryResults);

    // for retrieving only one result by having the primary key
    void search(const std::string & primaryKey, QueryResults *queryResults) ;

    /// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
    StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const ;

    void cacheClear() ;
    /**
     * Destructor to free persistent resources used by the QueryEvaluator.
     */
    ~QueryEvaluator();

    QueryEvaluatorInternal * impl;

};

}
}

#endif // __QUERYEVALUATOR_H__
