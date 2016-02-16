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

#ifndef __QUERYEVALUATORINTERNAL_H__
#define __QUERYEVALUATORINTERNAL_H__

#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/platform.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/Indexer.h>
#include <record/LocationRecordUtil.h>
#include <operation/CacheManager.h>
#include <instantsearch/Query.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Term.h>
#include <instantsearch/Ranker.h>
#include <query/QueryResultsInternal.h>
#include "index/ForwardIndex.h"
#include "operation/IndexData.h"
#include <instantsearch/LogicalPlan.h>
#include "physical_plan/PhysicalPlan.h"
#include <vector>
#include <string>


namespace srch2
{
namespace instantsearch
{


class QueryResultsInternal;
class QueryResults;
class TermVirtualList;
class IndexReaderWriter;
class InvertedIndex;
class PhysicalOperatorFactory;
class PhysicalPlanRecordItemFactory;
class FeedbackIndex;
/**
 * QueryEvaluatorInternal is the implementation of QueryEvaluator.
 */
class MYLIB_EXPORT QueryEvaluatorInternal
{
	friend class HistogramManager;
public:
    /**
     * Creates an QueryEvaluator object.
     * @param indexer - An object holding the index structures and cache.
     */
	QueryEvaluatorInternal(IndexReaderWriter *indexer , QueryEvaluatorRuntimeParametersContainer * parameters = NULL);

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

    /// Added for stemmer
    //virtual int searchWithStemmer(const Query *query, QueryResults *queryResults, const int nextK = 0, bool &isStemmed) = 0;

    /// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
    StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const ;

    const InvertedIndex *getInvertedIndex() {
        return this->indexData->invertedIndex;
    }

    ForwardIndex * getForwardIndex() {
        return this->indexData->forwardIndex;
    }

    void getForwardIndex_ReadView(shared_ptr<vectorview<ForwardListPtr> > & readView){
	// We need to get the read view from this->indexReadToken
	// instead of calling this->getTrie()->getTrieRootNode_ReadView()
	// since the latter may give a read view that is different from
	// the one we got when the search started.
    	readView = this->indexReadToken.forwardIndexReadViewSharedPtr;
    }

    Schema * getSchema() {
        return this->indexData->schemaInternal;
    }

    const Trie* getTrie() const {
        return this->indexData->trie;
    }

    boost::shared_ptr<PrefixActiveNodeSet> computeActiveNodeSet(Term *term) const;

    void cacheClear() ;
    /**
     * Destructor to free persistent resources used by the QueryEvaluator.
     */
    ~QueryEvaluatorInternal();

    PhysicalOperatorFactory * getPhysicalOperatorFactory();
    void setPhysicalOperatorFactory(PhysicalOperatorFactory * physicalOperatorFactory);
    PhysicalPlanRecordItemPool * getPhysicalPlanRecordItemPool();

    //DEBUG function. Used in CacheIntegration_Test
    bool cacheHit(const Query *query);

    void setQueryEvaluatorRuntimeParametersContainer(QueryEvaluatorRuntimeParametersContainer * parameters);
    QueryEvaluatorRuntimeParametersContainer * getQueryEvaluatorRuntimeParametersContainer();


    CacheManager * getCacheManager(){
    	return this->cacheManager;
    }

public:
    IndexReadStateSharedPtr_Token indexReadToken;
    void findKMostPopularSuggestionsSorted(Term *term ,
    		PrefixActiveNodeSet * activeNodes,
    		unsigned numberOfSuggestionsToReturn ,
    		std::vector<SuggestionInfo > & suggestionPairs) const;
    FeedbackIndex * getFeedbackIndex();
    string queryStringWithTermsAndOps;
private:
    const IndexData *indexData;
    IndexReaderWriter *indexer;
    QueryEvaluatorRuntimeParametersContainer parameters;
    CacheManager *cacheManager;
    PhysicalOperatorFactory * physicalOperatorFactory;
    PhysicalPlanRecordItemPool * physicalPlanRecordItemPool;

};

}
}

#endif // __QUERYEVALUATORINTERNAL_H__
