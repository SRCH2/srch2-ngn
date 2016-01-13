
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
    /**
     * Destructor to free persistent resources used by the QueryEvaluator.
     */
    ~QueryEvaluatorInternal();

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
    StoredRecordBuffer getInMemoryData_Safe(unsigned internalRecordId) const ;


    unsigned getTotalNumberOfRecords();

    const bool isBulkLoadDone() const;
    /*
     * This function is only used in TEST.
     * Do not use this API in any place higher than this layer.Nobody should access any of the indices
     * directly unless it's from within the query optimzer.
     */
    const InvertedIndex *testOnly_getInvertedIndex();

    /*
     * This function is only used in TEST.
     * Do not use this API in any place higher than this layer.Nobody should access any of the indices
     * directly unless it's from within the query optimzer.
     */
    ForwardIndex * testOnly_getForwardIndex() ;

    const Schema * getSchema() const ;
    const Trie* testOnly_getTrie() const;

    boost::shared_ptr<PrefixActiveNodeSet> computeActiveNodeSet(Term *term) const;

    void cacheClear() ;

    PhysicalOperatorFactory * getPhysicalOperatorFactory();
    void setPhysicalOperatorFactory(PhysicalOperatorFactory * physicalOperatorFactory);
    PhysicalPlanRecordItemPool * getPhysicalPlanRecordItemPool();

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


public:
    /*
     * The following two methods are exposed as public API
     * ONLY FOR TESTING PURPOSES. Do not use then anywhere outside
     * QueryEvaluatorInternal.
     */
    // Every reader goes through this function before starting the execution of
    // suggest or search
    void readerPreEnter();
    // Every reader goes through this function before exiting from suggest or search
    void readerPreExit();

};

}
}

#endif // __QUERYEVALUATORINTERNAL_H__
