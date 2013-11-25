
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

#include "operation/QueryEvaluatorInternal.h"
#include <instantsearch/Query.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/TypedValue.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "operation/IndexerInternal.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/ActiveNode.h"
#include "operation/TermVirtualList.h"
#include "query/QueryResultsInternal.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "util/Assert.h"
#include "index/ForwardIndex.h"
#include "geo/QuadTree.h"
#include "HistogramManager.h"
#include "QueryOptimizer.h"
#include "physical_plan/PhysicalPlan.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

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
    this->cacheManager = dynamic_cast<Cache*>(indexer->getCache());
    this->indexer = indexer;
    setPhysicalOperatorFactory(new PhysicalOperatorFactory());
}

/*
 * Finds the suggestions for a keyword based on fuzzyMatchPenalty.
 * Returns the number of suggestions found.
 */
// TODO : FIXME: This function is not compatible with the new api
int QueryEvaluatorInternal::suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions ){

}

/*
 * Returns the estimated number of results
 */
unsigned QueryEvaluatorInternal::estimateNumberOfResults(const LogicalPlan * logicalPlan){

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

	/*
	 * 1. Use CatalogManager to collect statistics and meta data about the logical plan
	 * ---- 1.1. computes and attaches active node sets for each term
	 * ---- 1.2. estimates and saves the number of results of each internal logical operator
	 * ---- 1.3. ...
	 */
	HistogramManager histogramManager(this);
	histogramManager.annotate(logicalPlan);
	/*
	 * 2. Use QueryOptimizer to build PhysicalPlan and optimize it
	 * ---- 2.1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
	 *           and makes sure inputs and outputs of operators are consistent.
	 * ---- 2.2. Applies optimization rules on the physical plan
	 * ---- 2.3. ...
	 */
	QueryOptimizer queryOptimizer(this,logicalPlan);
	PhysicalPlan physicalPlan(this);
	queryOptimizer.buildAndOptimizePhysicalPlan(physicalPlan);
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
	 * 3. Passing the results to QueryResults : This module is also responsible of populating QueryResults from
	 * ---- the outputs of the root operator and make everything inside core transparent to outside layers.
	 * 4. Exact and fuzzy policy : If exactOnly is passed as true to getNext(...) of any operator, that operator
	 * ---- only returns exact results (if not, all the results are returned.) Therefore, the physical plan is executed first
	 * ---- by exactOnly=true, and then if we don't have enough results we re-execute the plan by passing exactOnly=false.
	 * ---- please note that since topK, or GetAll (the search type) is embedded in the structure of PhysicalPlan,
	 * ---- this policy is applied on all search types.
	 */


}

/**
 * Does Map Search
 */
int QueryEvaluatorInternal::search(const Query *query, QueryResults *queryResults){
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    int returnValue = this->searchMapQuery(query, queryResults);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
    return returnValue;
}

// for doing a geo range query with a circle
void QueryEvaluatorInternal::search(const Circle &queryCircle, QueryResults *queryResults){
    QueryResultsInternal *queryResultsInternal = queryResults->impl;
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    this->indexData->quadTree->rangeQueryWithoutKeywordInformation(queryCircle,queryResultsInternal);
    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
}

// for doing a geo range query with a rectangle
void QueryEvaluatorInternal::search(const Rectangle &queryRectangle, QueryResults *queryResults){
    QueryResultsInternal *queryResultsInternal = queryResults->impl;
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    this->indexData->quadTree->rangeQueryWithoutKeywordInformation(queryRectangle,queryResultsInternal);
    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
}

// for retrieving only one result by having the primary key
void QueryEvaluatorInternal::search(const std::string & primaryKey, QueryResults *queryResults){
	unsigned internalRecordId ; // ForwardListId is the same as InternalRecordId
	if ( this->indexData->forwardIndex->getInternalRecordIdFromExternalRecordId(primaryKey , internalRecordId) == false ){
		return;
	}
	// The query result to be returned.
	// First check to see if the record is valid.
	bool validForwardList;
	this->indexData->forwardIndex->getForwardList(internalRecordId, validForwardList);
	if (validForwardList == false) {
		return;
	}

	QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
	queryResult->externalRecordId = primaryKey;
	queryResult->internalRecordId = internalRecordId;
	queryResult->_score.setTypedValue((float)0.0);
	queryResults->impl->sortedFinalResults.push_back(queryResult);
	return;
}

// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
std::string QueryEvaluatorInternal::getInMemoryData(unsigned internalRecordId) const {

}

PrefixActiveNodeSet *QueryEvaluatorInternal::computeActiveNodeSet(Term *term) const{

}

void QueryEvaluatorInternal::cacheClear() {

}

int QueryEvaluatorInternal::searchMapQuery(const Query *query, QueryResults* queryResults){
    QueryResultsInternal *queryResultsInternal = queryResults->impl;

    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    //Empty Query case
    if (queryTerms->size() == 0) {
        return 0;
    }

    //build mario's SearcherTerm for each query term
    //timespec ts1;
    //timespec ts2;
    //clock_gettime(CLOCK_REALTIME, &ts1);
    vector<MapSearcherTerm> mapSearcherTermVector;
    for (unsigned i = 0; i < queryTerms->size(); i++) {
        MapSearcherTerm mapSearcherTerm;
        // TODO
        // after the bug in active node is fixed, see if we should use LeafNodeSetIterator/ActiveNodeSetIterator for PREFIX/COMPLETE terms.
        // see TermVirtualList::TermVirtualList() in src/operation/TermVirtualList.cpp
        PrefixActiveNodeSet *prefixActiveNodeSet = computeActiveNodeSet(queryTerms->at(i));
        for (ActiveNodeSetIterator iter(prefixActiveNodeSet, queryTerms->at(i)->getThreshold()); !iter.isDone(); iter.next()) {
            TrieNodePointer trieNode;
            unsigned distance;
            iter.getItem(trieNode, distance);
            ExpansionStructure expansion(trieNode->getMinId(), trieNode->getMaxId(), (unsigned char)distance, trieNode);
            //expansion.termPtr = queryTerms->at(i);

            if(queryTerms->at(i)->getTermType() == TERM_TYPE_COMPLETE){
                distance = prefixActiveNodeSet->getEditdistanceofPrefix(trieNode);
                // If the keyword is a fuzzy complete keyword, we also need to add additional keywords with a distance up to the threashold
                addMoreNodesToExpansion(trieNode, distance, queryTerms->at(i)->getThreshold(), mapSearcherTerm);
            }
            else{
                mapSearcherTerm.expansionStructureVector.push_back(expansion);
            }
        }

        // Similar to the part in TermVirtualList.cpp destructor, which is used in text only index.
        // If it's in the cache, set the busyBit off so that it can be freed in the future.
        // if it's not in the cache, we can delete it right away.
        if (prefixActiveNodeSet->isResultsCached() == true) {
            prefixActiveNodeSet->busyBit->setFree();
        } else {
            // see ticket https://trac.assembla.com/srch2-root/ticket/142
            // prefixActiveNodeSet->busyBit->setFree();
            delete prefixActiveNodeSet;
        }

        mapSearcherTerm.termPtr = queryTerms->at(i);
        mapSearcherTermVector.push_back(mapSearcherTerm);
    }
    //clock_gettime(CLOCK_REALTIME, &ts2);
    //cout << "Time to compute active nodes " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

    //clock_gettime(CLOCK_REALTIME, &ts1);
    // TODO bad design, should change Query::getRange()
    vector<double> values;
    query->getRange(values);
    Shape *searchRange = NULL;
    if (values.size()==3) {
        Point p;
        p.x = values[0];
        p.y = values[1];
        searchRange = new Circle(p, values[2]);
    } else {
        pair<pair<double, double>, pair<double, double> > rect;
        rect.first.first = values[0];
        rect.first.second = values[1];
        rect.second.first = values[2];
        rect.second.second = values[3];
        searchRange = new Rectangle(rect);
    }
    const SpatialRanker *ranker = dynamic_cast<const SpatialRanker*>(query->getRanker());
    indexData->quadTree->rangeQuery(queryResultsInternal, *searchRange, mapSearcherTermVector, ranker, query->getPrefixMatchPenalty());

    delete searchRange;

    //clock_gettime(CLOCK_REALTIME, &ts2);
    //cout << "Time to range query " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);

    return queryResultsInternal->sortedFinalResults.size();
}

// Given a trie node, a distance, and an upper bound, we want to insert its descendants to the mapSearcherTerm.exapnsions (as restricted by the distance and the bound)
void QueryEvaluatorInternal::addMoreNodesToExpansion(const TrieNode* trieNode, unsigned distance, unsigned bound, MapSearcherTerm &mapSearcherTerm)
{
    if (trieNode->isTerminalNode()) {
        ExpansionStructure expansion(trieNode->getMinId(), trieNode->getMaxId(), (unsigned char)distance, trieNode);
        mapSearcherTerm.expansionStructureVector.push_back(expansion);
    }
    if (distance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            addMoreNodesToExpansion(child, distance+1, bound, mapSearcherTerm);
        }
    }
}

}}
