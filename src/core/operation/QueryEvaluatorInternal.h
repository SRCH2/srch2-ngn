
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
#include "geo/QuadNodeInternalStructures.h"
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

    /**
     * Does Map Search
     */
    int geoSearch(const Query *query, QueryResults *queryResults) ;

    // for doing a geo range query with a circle
    void geoSearch(const Circle &queryCircle, QueryResults *queryResults) ;

    // for doing a geo range query with a rectangle
    void geoSearch(const Rectangle &queryRectangle, QueryResults *queryResults) ;

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
    	readView = this->forwardIndexDirectoryReadView;
    }
    void setForwardIndex_ReadView(){
        this->indexData->forwardIndex->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);
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
private:
    const IndexData *indexData;
    IndexReaderWriter *indexer;
    QueryEvaluatorRuntimeParametersContainer parameters;
    CacheManager *cacheManager;
    PhysicalOperatorFactory * physicalOperatorFactory;
    PhysicalPlanRecordItemPool * physicalPlanRecordItemPool;

    shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;
    // search functions for map search
    int searchMapQuery(const Query *query, QueryResults* queryResults);
    void addMoreNodesToExpansion(const TrieNode* trieNode, unsigned distance, unsigned bound, MapSearcherTerm &mapSearcherTerm);
};

}
}

#endif // __QUERYEVALUATORINTERNAL_H__
