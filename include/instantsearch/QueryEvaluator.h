/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
