
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

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */

#include "operation/QueryEvaluatorInternal.h"
#include <instantsearch/Query.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/TypedValue.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "operation/IndexerInternal.h"
#include "operation/ActiveNode.h"
#include "operation/TermVirtualList.h"
#include "query/QueryResultsInternal.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "util/Assert.h"
#include "index/ForwardIndex.h"
#include "HistogramManager.h"
#include "physical_plan/PhysicalPlan.h"
#include "physical_plan/PhysicalOperators.h"
#include "physical_plan/FacetOperator.h"
#include "physical_plan/SortByRefiningAttributeOperator.h"
#include "physical_plan/PhraseSearchOperator.h"
#include "physical_plan/KeywordSearchOperator.h"
#include "../highlighter/ServerHighLighter.h"
#include "physical_plan/FeedbackRankingOperator.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

/**
 * Creates an QueryEvaluatorInternal object.
 * @param indexer - An object holding the index structures and cache.
 */
QueryEvaluatorInternal::QueryEvaluatorInternal(IndexReaderWriter *indexer , QueryEvaluatorRuntimeParametersContainer * parameters){
	// if parameters is NULL, the default constructor of the object is used which has the default values for everything.
	if(parameters != NULL){
		this->parameters = *parameters;
	}
    this->indexData = dynamic_cast<const IndexData*>(indexer->getReadView(this->indexReadToken));
    this->cacheManager = dynamic_cast<CacheManager*>(indexer->getCache());
    this->indexer = indexer;
    setPhysicalOperatorFactory(new PhysicalOperatorFactory());
    this->physicalPlanRecordItemPool = new PhysicalPlanRecordItemPool();
}

/*
 * Finds the suggestions for a keyword based on fuzzyMatchPenalty.
 * Returns the number of suggestions found.
 */
// TODO : FIXME: This function is not compatible with the new api
int QueryEvaluatorInternal::suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions ){
	// non valid cases for input
	if(keyword.compare("") == 0 || numberOfSuggestionsToReturn == 0){
		return 0;
	}

	boost::shared_lock< boost::shared_mutex > lock(this->indexData->globalRwMutexForReadersWriters); // need to lock the mutex
    if (this->indexData->isBulkLoadDone() == false){
        return -1;
    }

	// make sure fuzzyMatchPenalty is in [0,1]
	if(fuzzyMatchPenalty < 0 || fuzzyMatchPenalty > 1){
		fuzzyMatchPenalty = 0.5;
	}
	// calculate editDistanceThreshold
	unsigned editDistanceThreshold = srch2::instantsearch::computeEditDistanceThreshold(keyword.length(), fuzzyMatchPenalty);

	// compute active nodes
	// 1. first we must create term object which is used to compute activenodes.
	//  TERM_TYPE_COMPLETE and 0 in the arguments will not be used.
	Term * term = new Term(keyword , TERM_TYPE_COMPLETE , 0, fuzzyMatchPenalty , editDistanceThreshold);
	// 2. compute active nodes.
	boost::shared_ptr<PrefixActiveNodeSet> termActiveNodeSet = this->computeActiveNodeSet(term);
	// 3. now iterate on active nodes and find suggestions for each on of them
    std::vector<SuggestionInfo > suggestionPairs;
    findKMostPopularSuggestionsSorted(term , termActiveNodeSet.get() , numberOfSuggestionsToReturn, suggestionPairs);
	// 4. we don't need the term anymore
	delete term;

    int suggestionCount = 0;
    for(std::vector<SuggestionInfo >::iterator suggestion = suggestionPairs.begin() ;
    		suggestion != suggestionPairs.end() && suggestionCount < numberOfSuggestionsToReturn ; ++suggestion , ++suggestionCount){
      string suggestionString ;
      boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;

      // We need to get the read view from this->indexReadToken
      // instead of calling this->getTrie()->getTrieRootNode_ReadView()
      // since the latter may give a read view that is different from
      // the one we got when the search started.
      trieRootNode_ReadView = this->indexReadToken.trieRootNodeSharedPtr;
      this->getTrie()->getPrefixString(trieRootNode_ReadView->root,
				       suggestion->suggestedCompleteTermNode, suggestionString);
      suggestions.push_back(suggestionString);
    }
    return 0;
}

bool suggestionComparator(const SuggestionInfo & left ,
		const SuggestionInfo & right ){
	return left.probabilityValue > right.probabilityValue;
}


void QueryEvaluatorInternal::findKMostPopularSuggestionsSorted(Term *term ,
		PrefixActiveNodeSet * activeNodes,
		unsigned numberOfSuggestionsToReturn ,
		std::vector<SuggestionInfo > & suggestionPairs) const{
	// first make sure input is OK
	if(term == NULL || activeNodes == NULL) return;

	// now iterate on active nodes and find suggestions for each on of them
    ActiveNodeSetIterator iter(activeNodes, term->getThreshold());
    for (; !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        // this function is only called if query has only one keyword.
        // If this keyword is prefix, we should traverse down the trie and find possible completions;
        // otherwise, we should just check to see if active node is terminal or not.
        if(term->getTermType() == TERM_TYPE_PREFIX){
        	trieNode->findMostPopularSuggestionsInThisSubTrie(trieNode , distance, suggestionPairs , numberOfSuggestionsToReturn);
        }else{
        	if(trieNode->isTerminalNode() == true){
        		suggestionPairs.push_back(SuggestionInfo(distance , trieNode->getNodeProbabilityValue(), trieNode , trieNode));
        	}
        }
        if(suggestionPairs.size() >= numberOfSuggestionsToReturn){
        	break;
        }
    }

    // now sort the suggestions
    std::sort(suggestionPairs.begin() , suggestionPairs.end() , suggestionComparator);
}

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
int QueryEvaluatorInternal::search(LogicalPlan * logicalPlan , QueryResults *queryResults){

	// used for feedback ranking.
	this->queryStringWithTermsAndOps = logicalPlan->queryStringWithTermsAndOpsOnly;
	ASSERT(logicalPlan != NULL);
	// need to lock the mutex
	boost::shared_lock< boost::shared_mutex > lock(this->indexData->globalRwMutexForReadersWriters);

	string key = logicalPlan->getUniqueStringForCaching();

	// if the query is present in the user feedback index, then skip cache because
	// its score needs to be re-calculated and its entry in the cache is no longer valid
	// Possible optimization:(TODO) compare whether user feedback entry for this query is
	//  more recent than cache entry. If yes, skip cache , otherwise use cache.
	if (!indexer->getFeedbackIndexer()->hasFeedbackDataForQuery(this->queryStringWithTermsAndOps)) {
		//1. first check to see if we have this query in cache
		boost::shared_ptr<QueryResultsCacheEntry> cachedObject ;
		if(this->cacheManager->getQueryResultsCache()->getQueryResults(key , cachedObject) == true){
			// cache hit
			cachedObject->copyToQueryResultsInternal(queryResults->impl);
			return queryResults->impl->sortedFinalResults.size();
		}
	}
	 /*
	  * 3. Execute physical plan
	 */
	/*
	 * PhysicalPlanExecutor is responsible of executing the PhysicalPlan.
	 * Also it takes care of the following tasks :
	 * 1. Pagination: In the case of topK, it handles pagination by setting K to the right value
	 * ---- which is a function of K and offset.
	 * 2. Post processing: Post processing physical operators are added to the tree by QueryOptimizer.
	 * ---- So there is nothing much to be done here, maybe just passing up the recorded data by some filters
	 * ---- like facet.
	 * 3. Passing the results to QueryResults : This module is also responsible of populating QuebuildPhysicalPlanFirstVersionryResults from
	 * ---- the outputs of the root operator and make everything inside core transparent to outside layers.
	 * 4. Exact and fuzzy policy : If exactOnly is passed as true to getNext(...) of any operator, that operator
	 * ---- only returns exact results (if not, all the results are returned.) Therefore, the physical plan is executed first
	 * ---- by exactOnly=true, and then if we don't have enough results we re-execute the plan by passing exactOnly=false.
	 * ---- please note that since topK, or GetAll (the search type) is embedded in the structure of PhysicalPlan,
	 * ---- this policy is applied on all search types.
	 */

	PhysicalPlanNode * topOperator = NULL;
	PhysicalPlanNode * bottomOfChain = NULL;
	FacetOperator * facetOperatorPtr = NULL;
	SortByRefiningAttributeOperator * sortOperator = NULL;
	if(logicalPlan->getPostProcessingInfo() != NULL){
		if(logicalPlan->getPostProcessingInfo()->getfacetInfo() != NULL){
			facetOperatorPtr = new FacetOperator(this, logicalPlan->getPostProcessingInfo()->getfacetInfo()->types,
					logicalPlan->getPostProcessingInfo()->getfacetInfo()->fields,
					logicalPlan->getPostProcessingInfo()->getfacetInfo()->rangeStarts,
					logicalPlan->getPostProcessingInfo()->getfacetInfo()->rangeEnds,
					logicalPlan->getPostProcessingInfo()->getfacetInfo()->rangeGaps,
					logicalPlan->getPostProcessingInfo()->getfacetInfo()->numberOfTopGroupsToReturn);

			FacetOptimizationOperator * facetOptimizationOperatorPtr = new FacetOptimizationOperator();
			facetOperatorPtr->setPhysicalPlanOptimizationNode(facetOptimizationOperatorPtr);
			facetOptimizationOperatorPtr->setExecutableNode(facetOperatorPtr);

			topOperator = bottomOfChain =  facetOperatorPtr;
		}
		if(logicalPlan->getPostProcessingInfo()->getSortEvaluator() != NULL){
			sortOperator = new SortByRefiningAttributeOperator(logicalPlan->getPostProcessingInfo()->getSortEvaluator());
			SortByRefiningAttributeOptimizationOperator * sortOpOperator =
					new SortByRefiningAttributeOptimizationOperator();
			sortOperator->setPhysicalPlanOptimizationNode(sortOpOperator);
			sortOpOperator->setExecutableNode(sortOperator);

			if(bottomOfChain != NULL){
				bottomOfChain->getPhysicalPlanOptimizationNode()->addChild(sortOpOperator);
				bottomOfChain = bottomOfChain->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode();
			}else{
				topOperator = bottomOfChain = sortOperator;
			}
		}
	}


	KeywordSearchOperator keywordSearchOperator(logicalPlan);
	KeywordSearchOptimizationOperator keywordSearchOptimizationOperator;
	keywordSearchOperator.setPhysicalPlanOptimizationNode(&keywordSearchOptimizationOperator);
	keywordSearchOptimizationOperator.setExecutableNode(&keywordSearchOperator);

	if(bottomOfChain != NULL){
		bottomOfChain->getPhysicalPlanOptimizationNode()->addChild(&keywordSearchOptimizationOperator);
		bottomOfChain = bottomOfChain->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode();
	}else{
		topOperator = bottomOfChain = &keywordSearchOperator;
	}


	PhysicalPlanExecutionParameters dummy(0,true,1,SearchTypeTopKQuery); // this parameter will be created inside KeywordSearchOperator
	topOperator->open(this, dummy );


	boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;

	// We need to get the read view from this->indexReadToken
	// instead of calling this->getTrie()->getTrieRootNode_ReadView()
	// since the latter may give a read view that is different from
	// the one we got when the search started.
	trieRootNode_ReadView = this->indexReadToken.trieRootNodeSharedPtr;
	while(true){

		PhysicalPlanRecordItem * newRecord = topOperator->getNext(dummy);

		if(newRecord == NULL){
			break;
		}

		QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
		queryResults->impl->sortedFinalResults.push_back(queryResult);

		queryResult->internalRecordId = newRecord->getRecordId();
		newRecord->getRecordMatchEditDistances(queryResult->editDistances);
		//
		queryResult->_score.setTypedValue(newRecord->getRecordRuntimeScore(),ATTRIBUTE_TYPE_FLOAT);
		queryResult->exactResultFlag = newRecord->isExactResult();

		newRecord->getRecordMatchingPrefixes(queryResult->matchingKeywordTrieNodes);

		newRecord->getTermTypes(queryResult->termTypes);

		for(unsigned i=0; i < queryResult->matchingKeywordTrieNodes.size() ; i++){
			std::vector<CharType> temp;
			this->getTrie()->getPrefixString(trieRootNode_ReadView->root,
					queryResult->matchingKeywordTrieNodes.at(i), temp);
			string str;
			charTypeVectorToUtf8String(temp, str);
			queryResult->matchingKeywords.push_back(str);
		}
		newRecord->getRecordMatchAttributeBitmaps(queryResult->attributeIdsList);

		// We need to get the read view from this->indexReadToken
		// instead of calling this->getTrie()->getTrieRootNode_ReadView()
		// since the latter may give a read view that is different from
		// the one we got when the search started.
		this->getForwardIndex()->getExternalRecordIdFromInternalRecordId(this->indexReadToken.forwardIndexReadViewSharedPtr,
										 queryResult->internalRecordId,queryResult->externalRecordId );



		/////////////////////////////////////////////////
		StoredRecordBuffer buffer = this->getForwardIndex()->getInMemoryData(queryResult->internalRecordId);
		if (buffer.start.get() != NULL)
			queryResult->inMemoryRecordString = string(buffer.start.get(), buffer.length);
		else
			queryResult->inMemoryRecordString = "";

		queryResult->valuesOfParticipatingRefiningAttributes = newRecord->valuesOfParticipatingRefiningAttributes;
		/////////////////////////////////////////////////
	}

	if(facetOperatorPtr != NULL){
		facetOperatorPtr->getFacetResults(queryResults);
	}

	topOperator->close(dummy);

	// set estimated number of results
	queryResults->impl->estimatedNumberOfResults = logicalPlan->getTree()->stats->getEstimatedNumberOfResults();

    /*
     * 	QueryResults contains the list of records which are going to be sent to
	 *  the sharding broker node to be aggregated. Populate RecordSnippet in
	 *  QueryResult object.
	 *
     *  Do snippet generation only if
     *  1. There are attributes marked to be highlighted
     *  2. Query is not facet only
     *  3. Highlight is not turned off in the query ( default is on )
     */
	const srch2::httpwrapper::CoreInfo_t * coreInfo = this->getQueryEvaluatorRuntimeParametersContainer()->coreInfo;
	if (coreInfo->getHighlightAttributeIdsVector().size() > 0 && !logicalPlan->facetOnlyFlag
			&& logicalPlan->highLightingOnFlag) {
		srch2::httpwrapper::ServerHighLighter highlighter =
				srch2::httpwrapper::ServerHighLighter(queryResults, indexer, coreInfo,
						logicalPlan->accessibleSearchableAttributes);
		highlighter.generateSnippets();
	}

	// save in cache
	boost::shared_ptr<QueryResultsCacheEntry> cacheObject ;
	cacheObject.reset(new QueryResultsCacheEntry());
	cacheObject->copyFromQueryResultsInternal(queryResults->impl);
	this->cacheManager->getQueryResultsCache()->setQueryResults(key , cacheObject);


	if(facetOperatorPtr != NULL){
		delete facetOperatorPtr->getPhysicalPlanOptimizationNode();
		delete facetOperatorPtr;
	}

	if (sortOperator) {
		delete sortOperator->getPhysicalPlanOptimizationNode();
		delete sortOperator;
	}


	return queryResults->impl->sortedFinalResults.size();
}

// for retrieving only one result by having the primary key
void QueryEvaluatorInternal::search(const std::string & primaryKey, QueryResults *queryResults){
	unsigned internalRecordId ; // ForwardListId is the same as InternalRecordId
    // need to lock the mutex
	boost::shared_lock< boost::shared_mutex > lock(this->indexData->globalRwMutexForReadersWriters);
	if ( this->indexData->forwardIndex->getInternalRecordIdFromExternalRecordId(primaryKey , internalRecordId) == false ){
		return;
	}
	// The query result to be returned.
	// First check to see if the record is valid.
	bool validForwardList;
	shared_ptr<vectorview<ForwardListPtr> > readView;

	// We need to get the read view from this->indexReadToken
	// instead of calling this->getTrie()->getTrieRootNode_ReadView()
	// since the latter may give a read view that is different from
	// the one we got when the search started.
	readView = this->indexReadToken.forwardIndexReadViewSharedPtr;
	this->indexData->forwardIndex->getForwardList(readView, internalRecordId, validForwardList);
	if (validForwardList == false) {
		return;
	}

	QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
	queryResult->externalRecordId = primaryKey;
	queryResult->internalRecordId = internalRecordId;
	queryResult->_score.setTypedValue((float)0.0,ATTRIBUTE_TYPE_FLOAT);

	/////////////////////////////////////////////////
	StoredRecordBuffer buffer = this->getForwardIndex()->getInMemoryData(queryResult->internalRecordId);
	if (buffer.start.get() != NULL)
		queryResult->inMemoryRecordString = string(buffer.start.get(), buffer.length);
	else
		queryResult->inMemoryRecordString = "";

//	queryResult->valuesOfParticipatingRefiningAttributes = newRecord->valuesOfParticipatingRefiningAttributes;
	/////////////////////////////////////////////////

	queryResults->impl->sortedFinalResults.push_back(queryResult);
	return;
}

// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
StoredRecordBuffer QueryEvaluatorInternal::getInMemoryData(unsigned internalRecordId) const {
	return this->indexData->forwardIndex->getInMemoryData(internalRecordId);
}

// TODO : this function might need to be deleted from here ...
boost::shared_ptr<PrefixActiveNodeSet> QueryEvaluatorInternal::computeActiveNodeSet(Term *term) const{
    // it should not be an empty std::string
    string *keyword = term->getKeyword();
    vector<CharType> charTypeKeyword;
    utf8StringToCharTypeVector(*keyword, charTypeKeyword);
    unsigned keywordLength = charTypeKeyword.size();
    ASSERT(keywordLength > 0);

    //std::cout << "Keyword:" << *keyword;

    // We group the active trie nodes based on their edit distance to the term prefix

    // 1. Get the longest prefix that has active nodes
    unsigned cachedPrefixLength = 0;
    boost::shared_ptr<PrefixActiveNodeSet> initialPrefixActiveNodeSet ;
    //TODO: Active node cache is disabled for geo search for now. There is a bug related to Cache/Trie and Geo.
    // We should fix this bug when we will be actively working on Geo.
    int cacheResponse = 0 ; // this->cacheManager->getActiveNodesCache()->findLongestPrefixActiveNodes(term, initialPrefixActiveNodeSet); //initialPrefixActiveNodeSet is Busy

    if ( cacheResponse == 0) { // NO CacheHit,  response = 0
        //std::cout << "|NO Cache|" << std::endl;;
        // No prefix has a cached TermActiveNode Set. Create one for the empty std::string "".
    	initialPrefixActiveNodeSet.reset(new PrefixActiveNodeSet(this->indexReadToken.trieRootNodeSharedPtr, term->getThreshold(), this->indexData->getSchema()->getSupportSwapInEditDistance()));
    }
    cachedPrefixLength = initialPrefixActiveNodeSet->getPrefixLength();

    /// 2. do the incremental computation. BusyBit of prefixActiveNodeSet is busy.
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet = initialPrefixActiveNodeSet;

    for (unsigned iter = cachedPrefixLength; iter < keywordLength; iter++) {
        CharType additionalCharacter = charTypeKeyword[iter]; // get the appended character

        boost::shared_ptr<PrefixActiveNodeSet> newPrefixActiveNodeSet = prefixActiveNodeSet->computeActiveNodeSetIncrementally(additionalCharacter);

        prefixActiveNodeSet = newPrefixActiveNodeSet;

        //std::cout << "Cache Set:" << *(prefixActiveNodeSet->getPrefix()) << std::endl;

        if (iter >= 2 && (cacheResponse != -1)) { // Cache not busy and keywordLength is at least 2.
        	prefixActiveNodeSet->prepareForIteration(); // this is the last write operation on prefixActiveNodeSet
            cacheResponse = this->cacheManager->getActiveNodesCache()->setPrefixActiveNodeSet(prefixActiveNodeSet);
        }
    }
    // Possible memory leak due to last prefixActiveNodeSet not being cached. This is checked for
    // and deleted by the caller "QueryResultsInternal()"
    return prefixActiveNodeSet;
}

void QueryEvaluatorInternal::cacheClear() {
    this->cacheManager->clear();
}


QueryEvaluatorInternal::~QueryEvaluatorInternal() {
	delete physicalOperatorFactory;
    delete physicalPlanRecordItemPool;
}

PhysicalOperatorFactory * QueryEvaluatorInternal::getPhysicalOperatorFactory(){
    return this->physicalOperatorFactory;
}

void QueryEvaluatorInternal::setPhysicalOperatorFactory(PhysicalOperatorFactory * physicalOperatorFactory){
    this->physicalOperatorFactory = physicalOperatorFactory;
}

PhysicalPlanRecordItemPool * QueryEvaluatorInternal::getPhysicalPlanRecordItemPool(){
    return this->physicalPlanRecordItemPool;
}

//DEBUG function. Used in CacheIntegration_Test
bool QueryEvaluatorInternal::cacheHit(const Query *query)
{
//    const std::vector<Term* > *queryTerms = query->getQueryTerms();
//
//    //Empty Query case
//    if (queryTerms->size() == 0)
//        return false;
//
//    // Cache lookup, assume a query with the first k terms found in the cache
//    ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry;
//    this->cacheManager->getCachedConjunctionResult(queryTerms, conjunctionCacheResultsEntry);
//
//    // Cached results for the first k terms
//    if (conjunctionCacheResultsEntry != NULL)
//        return true;

	// TODO we must write this function again
    return false;
}

void QueryEvaluatorInternal::setQueryEvaluatorRuntimeParametersContainer(QueryEvaluatorRuntimeParametersContainer * parameters){
	this->parameters = *parameters;
}
QueryEvaluatorRuntimeParametersContainer * QueryEvaluatorInternal::getQueryEvaluatorRuntimeParametersContainer(){
	return &(this->parameters);
}

FeedbackIndex * QueryEvaluatorInternal::getFeedbackIndex() {
	return indexer->getFeedbackIndexer();
}

}}
