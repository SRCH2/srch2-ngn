
// $Id: IndexSearcherInternal.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

namespace srch2
{
namespace instantsearch
{


IndexSearcherInternal::IndexSearcherInternal(IndexReaderWriter *indexer , IndexSearcherRuntimeParametersContainer * parameters)
{
	// if parameters is NULL, the default constructor of the object is used which has the default values for everything.
	if(parameters != NULL){
		this->parameters = *parameters;
	}
    this->indexData = dynamic_cast<const IndexData*>(indexer->getReadView(this->indexReadToken));
    this->cacheManager = dynamic_cast<Cache*>(indexer->getCache());
    this->indexer = indexer;

}

// This is a helper function of nextRecord. It takes a record ID from the 0-th list,
// and looks for the remaining lists for the next record ID by doing an intersection operation ("AND" logic).
int IndexSearcherInternal::getNextMatchingRecordID(int recordID, vector<TermVirtualList* >* virtualListVector)
{
    // suppose we keep n(0 ~ n-1) virtual list, the first one(idx:0) called lead,
    // we loop the others lists to get a recordID (which exist in the lead for sure), exist in all the other lists(1 ~ n-1)
    for (int i = 1; i< virtualListVector->size(); i++) {
        if (virtualListVector->at(i)->currentRecordID < recordID) {
            // Move the cursor of the i-th list forward.
            virtualListVector->at(i)->currentRecordID = virtualListVector->at(i)->bitSetIter->advance(recordID);
            if (virtualListVector->at(i)->currentRecordID > recordID) {
                // The record ID doesn't exist on the list. So we need to change this ID, and restart the iteration from the 0-th list.
                recordID = virtualListVector->at(i)->currentRecordID;
                virtualListVector->at(0)->currentRecordID
                = virtualListVector->at(0)->bitSetIter->advance(recordID);
                recordID = virtualListVector->at(0)->currentRecordID;
                // After "i++" we will continue from the 1-st list.
                i = 0;
                continue;
            }
        }
    }
    return recordID;
}

// return the nextRecord which exists in all the virtual lists, if there are no more records, will return NO_MORE_RECORDS
int IndexSearcherInternal::getNextRecordID(vector<TermVirtualList* >* virtualListVector)
{
    // choose the first virtual list as the lead
    TermVirtualList *lead = virtualListVector->at(0);
    // get the next record of lead
    lead->currentRecordID = lead->bitSetIter->getNextRecordId();
    if (virtualListVector->size() > 1)// if there are still other lists, we need to call doNext function to get next recordID
        return getNextMatchingRecordID(lead->currentRecordID, virtualListVector);
    else // otherwise we can return lead's recordID as the next record
        return lead->currentRecordID;
}


// find the next k answer starting from "offset". Can be used for
// pagination. Returns the number of records found
int IndexSearcherInternal::searchGetAllResultsQuery(const Query *query, QueryResults* queryResults,
		unsigned estimatedNumberOfResultsThresholdGetAll , unsigned numberOfEstimatedResultsToFindGetAll)
{

	// iterate on terms and find the estimated number of results for each term
	// if a term is too popular no term virtual list should be made for it
	// if all terms are too popular find the least popular and only make TVL for that one

	// this vector is passed to computeTermVirtualList to be used
	vector<PrefixActiveNodeSet *> activeNodesVector;
	// this vector is passed to computeTermVirtualList to see if
	// term virtual lists should be constructed completely or partially.
	vector<float> isTermTooPopularVectorAndScoresOfTopRecords ;
	// this vector keeps the popularity values of terms to be used in case
	// we need to find the least popular one
	vector<unsigned> termPopularities;

	// iterate on terms and if a term is too popular, set the flag so that we don't traverse to leaf nodes for it
	for (vector<Term*>::const_iterator vectorIterator = query->getQueryTerms()->begin();
			vectorIterator != query->getQueryTerms()->end();
			++vectorIterator) {
		Term *term = *vectorIterator;
		// compute activenodes
		PrefixActiveNodeSet * activeNodes =  this->computeActiveNodeSet(term);
		activeNodesVector.push_back(activeNodes);
		// see how popular the term is
		unsigned popularity = 0;
		bool isPopular = this->isTermTooPopular(term , activeNodes , popularity);
		// By default the value of topRecordScore is -1. If this term is too popular, this value will be the score of the
		// the top record of the most popular suggestion inverted list
		float topRecordScoreIfTooPopular = -1;
		topRecordScoreIfTooPopular = findTopRunTimeScoreOfLeafNodes(term ,  query->getPrefixMatchPenalty() , activeNodes);
		termPopularities.push_back(popularity);
		isTermTooPopularVectorAndScoresOfTopRecords.push_back(topRecordScoreIfTooPopular);

	}

	// for example : q="to be or not to be"
	// if there is more than one keyword, we find the least popular term and compute term virtual list only
	// for that one
	unsigned minPopularityIndex = 0;
	for(int termIndex = 1 ; termIndex != query->getQueryTerms()->size() ; ++termIndex){
		if(termPopularities.at(termIndex) < termPopularities.at(minPopularityIndex)){
			minPopularityIndex = termIndex;
		}
	}
	// isTermTooPopularVector is a vector of true values, we should set the value of
	// least popular term top record score to -1
	isTermTooPopularVectorAndScoresOfTopRecords.at(minPopularityIndex) = -1;

	// before preparing termVirtualLists for actual search, we estimate the number of results.
	// If the number of results is going to be too big, we call topK to find just a number of records.
	unsigned estimatedNumberOfResults = this->estimateNumberOfResults(query , activeNodesVector);
	std::cout << "Estimated number of results : " << estimatedNumberOfResults << std::endl;
	if(estimatedNumberOfResults > estimatedNumberOfResultsThresholdGetAll){
		// we must call top k here
		unsigned numberOfResultsFound = searchTopKQuery(query , 0 , numberOfEstimatedResultsToFindGetAll , queryResults , &activeNodesVector);
		if(numberOfResultsFound >= numberOfEstimatedResultsToFindGetAll){
			std::vector<QueryResult *>::iterator startOfErase = queryResults->impl->sortedFinalResults.begin();
			std::advance(startOfErase ,numberOfEstimatedResultsToFindGetAll);
			queryResults->impl->sortedFinalResults.erase(startOfErase , queryResults->impl->sortedFinalResults.end());
			numberOfResultsFound = numberOfEstimatedResultsToFindGetAll;
		}
		return numberOfResultsFound;
	}

	this->computeTermVirtualList(queryResults, &activeNodesVector, &isTermTooPopularVectorAndScoresOfTopRecords);

    QueryResultsInternal *queryResultsInternal = queryResults->impl;
    // get the std::vector of virtual lists of each term
    std::vector<TermVirtualList* > *virtualListVector = queryResultsInternal->getVirtualListVector();
    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    unsigned queryTermsLength = 1; // To prevent division by 0 in normalised edit distance calculation
    for (vector<Term *>::const_iterator queryTermsIterator = queryTerms->begin();
            queryTermsIterator != queryTerms->end(); queryTermsIterator++ ) {
        Term *term = *queryTermsIterator;
        queryTermsLength += term->getKeyword()->size();
    }

    //Empty Query case
    if (queryTerms->size() == 0) {
        return 0;
    }

    std::vector<float> queryResultTermScores;
    std::vector<std::string> queryResultMatchingKeywords;
    std::vector<unsigned> queryResultBitmaps;
    std::vector<unsigned> queryResultEditDistances;
    HeapItemForIndexSearcher *heapItem = new HeapItemForIndexSearcher();

    //find the smallest virtualList
    unsigned smallestVirtualListVectorId = -1; // -1 for unsigned is a very big number
    unsigned smallestVirtualListVectorSize = -1;
    for (unsigned int iter = 0; iter < virtualListVector->size(); ++iter) {
    	if(virtualListVector->at(iter)->isTermVirtualListDisabled() == false){
    		smallestVirtualListVectorId = iter;
    		unsigned smallestVirtualListVectorSize = virtualListVector->at(iter)->getVirtualListTotalLength();
    	}
    }
    ASSERT(smallestVirtualListVectorId != (unsigned)-1);
    for (unsigned int iter = 0; iter < virtualListVector->size(); ++iter) {
    	if(virtualListVector->at(iter)->isTermVirtualListDisabled() == true){
    		continue;
    	}
        unsigned currentSize = virtualListVector->at(iter)->getVirtualListTotalLength();
        if (smallestVirtualListVectorSize > currentSize ) {
            smallestVirtualListVectorId = iter;
            smallestVirtualListVectorSize = currentSize;
        }
    }
    // if the smallest virtual list also need to merge, we will use nextRecord function to get all the records
    if (virtualListVector->at(smallestVirtualListVectorId)->usingBitset) {
        if (smallestVirtualListVectorId)// if smallestVirtualListVectorId is not the first one, we will swap it to be the first one, which will improve the performance of iterating through its records
            swap(virtualListVector->at(smallestVirtualListVectorId), virtualListVector->at(0));
        int recordID;
        // we loop the record and add them to the result.
        while ((recordID = getNextRecordID(virtualListVector)) != RecordIdSetIterator::NO_MORE_RECORDS) {
            QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
            queryResult->internalRecordId = recordID;                       // keep the internalRecordId
            queryResult->_score.setTypedValue(1.0);                              // since we can't make a difference of these records, we give them the same score
            queryResult->matchingKeywords = queryResultMatchingKeywords;    // The matching words will also be the same with search query
            queryResult->attributeBitmaps = queryResultBitmaps;             // We lose the Bitmaps the keywords mathched
            queryResult->editDistances = queryResultEditDistances;          // and also the edit distance
            queryResultsInternal->insertResult(queryResult);                // insert the result to queryResultsInternal
        }
    } else { // we don't need a bitse to merge the smallestVirtualListVectorId virtual list
        // fill the visited list with the current queryResults
        std::set<unsigned> visitedList;

        //unsigned idsFound = 0;
        while (virtualListVector->at(smallestVirtualListVectorId)->getNext(heapItem)) {

            unsigned internalRecordId = heapItem->recordId;

            // if the record has been seen before, do nothing
            if (visitedList.count(internalRecordId)) {
                continue;
            }
            // mark the record as seen
            visitedList.insert(internalRecordId);

            // assign the vectors with default values to clear them
            queryResultTermScores.assign(queryTerms->size(), 0);
            queryResultMatchingKeywords.assign(queryTerms->size(), "");
            queryResultBitmaps.assign(queryTerms->size(), 0);
            queryResultEditDistances.assign(queryTerms->size(), 0);

            /*//Check if the hit is from a stemmed keyword.
            if(this->indexData->analyzerInternal->getStemmerNormalizerType()
               != srch2::instantsearch::NO_STEMMER_NORMALIZER)
            {
                unsigned heapItemKeywordId;//Unused
                float heapItemScore;//Unused
                bool heapItemIsStemmed;
                unsigned minId = heapItem->trieNode->getMinId();
                unsigned maxId = heapItem->trieNode->getMaxId();
                unsigned termSearchableAttributeIdToFilterTermHits =
                    virtualListVector->at(smallestVirtualListVectorId)->getTermSearchableAttributeIdToFilterTermHits();

                //TODO Optimise this lookup, so that score and matchingKeywordId are not calculated
                // twice for a stemmed heapItem hit.
                if (this->indexData->forwardIndex->haveWordInRangeWithStemmer(internalRecordId, minId, maxId,
                                                  termSearchableAttributeIdToFilterTermHits,
                                                  heapItemKeywordId, heapItemScore,
                                                  heapItemIsStemmed)) {
                    if(heapItemIsStemmed) {
                        // "STEM" is the matching keyword that denotes the stemmed keyword
                        queryResultMatchingKeywords.at(smallestVirtualListVectorId) = "STEM";
                    }
                    else {
                        std::vector<CharType> temp;
                        this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                               heapItem->trieNode, temp);
                        string str;
                        charTypeVectorToUtf8String(temp, str);
                        queryResultMatchingKeywords.at(smallestVirtualListVectorId) = str;
                    }
                }
            }
            else {*/
            std::vector<CharType> temp;
            this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root, heapItem->trieNode, temp);
            string str;
            charTypeVectorToUtf8String(temp, str);
            queryResultMatchingKeywords.at(smallestVirtualListVectorId) = str;
            //}
            queryResultBitmaps.at(smallestVirtualListVectorId) = heapItem->attributeBitMap;
            queryResultEditDistances.at(smallestVirtualListVectorId) = heapItem->ed;
            queryResultTermScores.at(smallestVirtualListVectorId) = heapItem->termRecordRuntimeScore;

            // Do random access on the other TermVirtualLists
            if (randomAccess(virtualListVector, queryResultTermScores, queryResultMatchingKeywords, queryResultBitmaps,
                     queryResultEditDistances, query, internalRecordId, smallestVirtualListVectorId, 0)) {
                bool  validForwardList = false;
                const ForwardList *fl = this->indexData->forwardIndex->getForwardList(internalRecordId,
                                                  validForwardList);
                if (validForwardList) {
                    // add this record to topK results if its score is good enough
                    QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
                    queryResult->internalRecordId = internalRecordId;
                    //unsigned sumOfEditDistances = std::accumulate(queryResultEditDistances.begin(),
                    //                          queryResultEditDistances.end(), 0);
                    queryResult->_score.setTypedValue(query->getRanker()->computeOverallRecordScore(query, queryResultTermScores));
                    //We compute the score for this query result here. This score will be used later to sort the results.
                    //    query->getRanker()->computeResultScoreUsingAttributeScore(query, recordScore,
                    //                                  sumOfEditDistances,
                    //                                  queryTermsLength);
                    queryResult->matchingKeywords = queryResultMatchingKeywords;
                    queryResult->attributeBitmaps = queryResultBitmaps;
                    queryResult->editDistances = queryResultEditDistances;
                    queryResultsInternal->insertResult(queryResult);
                }
            }
        }
    }
    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    delete heapItem;
    return queryResultsInternal->sortedFinalResults.size();
}

// Given a trie node, a distance, and an upper bound, we want to insert its descendants to the mapSearcherTerm.exapnsions (as restricted by the distance and the bound)
void IndexSearcherInternal::addMoreNodesToExpansion(const TrieNode* trieNode, unsigned distance, unsigned bound, MapSearcherTerm &mapSearcherTerm)
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


int IndexSearcherInternal::searchMapQuery(const Query *query, QueryResults* queryResults)
{
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

// find the next k answer starting from "offset". Can be used for
// pagination. Returns the number of records found
/**
 * 1. For each term, compute ActiveNodeSet
 * 2. Take all ActiveNodeSet and construct the VirtualLists.
 * 3. Do Fagin's Algorithm.
 *    1. Do round robin pass over the VirtualLists,
 *        1. Get the Top from a VirtualList,i.e, a record.
 *        2. Do RandomAccess -> ForwardIndex::haveKeywordInRange(), and find if all
 *           keywords occur in the record.
 *        3. If above condition is satisfied, apply Ranking Function to compute the score.
 *        4. Pop it and add it to VisitedList.
 *        5. Add it to resultSet
 *        6. Check for the stopping condition of TA, upper bound is less than the already seen candidates.
 *
 */
int IndexSearcherInternal::searchTopKQuery(const Query *query, const int offset,
        const int nextK, QueryResults* queryResults, vector<PrefixActiveNodeSet *> * activeNodesVectorFromArgs)
{

    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);

    // Empty Query case
    if (query->getQueryTerms()->size() == 0) {
        return 0;
    }

    // there must be one activeNodeSet for each term.
    ASSERT(activeNodesVectorFromArgs == NULL || query->getQueryTerms()->size() == activeNodesVectorFromArgs->size());

    //TODO: Corner case: check that queryResults was created using the query.
    QueryResultsInternal *queryResultsInternal = queryResults->impl;

    // if queryResults has enough results to answer this query return those results
    if (queryResults->getNumberOfResults() >= (unsigned)offset+nextK) {
        return nextK;
    }

    // set nextK to compute
    queryResultsInternal->setNextK(offset + nextK - queryResults->getNumberOfResults());

    /********Step 1**********/
    // Cache lookup, assume a query with the first k terms found in the cache
    /*
    queryResults->addMessage("Conjunction Cache Get");
    ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry;
    int cacheResponse = this->cacheManager->getCachedConjunctionResult(query->getQueryTerms(),
                        conjunctionCacheResultsEntry);
    */
    // disabling conjunction cache for now. TODO
    ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry = NULL;
    int cacheResponse = 0;

    // Case 1: Cache hit and "k" results are in cache. No need to compute termVirtualList.
    if (conjunctionCacheResultsEntry != NULL
            && cacheResponse == 1
            && conjunctionCacheResultsEntry->queryTerms->size() == query->getQueryTerms()->size()
            && conjunctionCacheResultsEntry->candidateList->size() >= (unsigned)offset+nextK) {
        queryResults->addMessage("Conjunction Cache Hit - case 1");
        // Do random access to find out if the candidates are valid results
        for (unsigned int i = 0; i < conjunctionCacheResultsEntry->candidateList->size(); ++i) {
            unsigned internalRecordId = conjunctionCacheResultsEntry->candidateList->at(i).internalRecordId;

            // create the score vector with k cached term scores and leave the rest as 0 to be filled in random access
            std::vector<float> queryResultTermScores(conjunctionCacheResultsEntry->candidateList->at(i).termScores);
            queryResultTermScores.resize(query->getQueryTerms()->size(), 0);

            std::vector<std::string> queryResultMatchingKeywords(conjunctionCacheResultsEntry->candidateList->at(i).matchingKeywords);
            queryResultMatchingKeywords.resize(query->getQueryTerms()->size(), "");

            std::vector<unsigned> queryResultAttributeBitmaps(conjunctionCacheResultsEntry->candidateList->at(i).attributeBitmaps);
            queryResultAttributeBitmaps.resize(query->getQueryTerms()->size(), 0);

            std::vector<unsigned> queryResultEditDistances(conjunctionCacheResultsEntry->candidateList->at(i).editDistances);
            queryResultEditDistances.resize(query->getQueryTerms()->size(), 0);

            //TODO OPT struct copy in insertResult
            // add this record to topK results if its score is good enough
            QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
            queryResult->internalRecordId = internalRecordId;

            bool forwardListValid = false;
            this->indexData->forwardIndex->getForwardList(internalRecordId, forwardListValid);
            if (forwardListValid) {
                queryResult->_score.setTypedValue(query->getRanker()->computeOverallRecordScore(query, queryResultTermScores));
                queryResult->matchingKeywords = queryResultMatchingKeywords;
                queryResult->attributeBitmaps = queryResultAttributeBitmaps;
                queryResult->editDistances = queryResultEditDistances;
                queryResultsInternal->insertResult(queryResult);
            }
        }
        // Give back cache busy bit.
        conjunctionCacheResultsEntry->busyBit->setFree();
        if (conjunctionCacheResultsEntry->isCached == 0)
            delete conjunctionCacheResultsEntry;
        queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    } else {
        // fill the visited list with the current queryResults
        std::set<unsigned> visitedList;
        queryResultsInternal->fillVisitedList(visitedList);


    	// iterate on terms and find the estimated number of results for each term
    	// if query has a single term and the term is too popular no term virtual list should be made for it
    	// otherwise, we just continue doing normal topK.

    	// this vector is passed to computeTermVirtualList to be used
    	vector<PrefixActiveNodeSet *> activeNodesVector;
    	// iterate on terms and compute active nodes
    	unsigned termIndex = 0;
        for (vector<Term*>::const_iterator vectorIterator = query->getQueryTerms()->begin();
                vectorIterator != query->getQueryTerms()->end();
                ++vectorIterator, ++termIndex) {
            Term *term = *vectorIterator;
            // compute activenodes
            PrefixActiveNodeSet * activeNodes = NULL;
            // if active nodes are passed by arguments we should not compute them again.
            if(activeNodesVectorFromArgs == NULL){
            	activeNodes = this->computeActiveNodeSet(term);;
            }else{
            	activeNodes = activeNodesVectorFromArgs->at(termIndex);
            }
            activeNodesVector.push_back(activeNodes);
        }

        // case of single keyword: if term is too popular we "estimate" the results
        if(query->getQueryTerms()->size() == 1){ // example : q=a
			// see how popular the term is
		    unsigned popularity = 0;
		    bool isPopular = this->isTermTooPopular(query->getQueryTerms()->at(0) , activeNodesVector.at(0) , popularity);

		    if(isPopular){
                unsigned numberOfResults = searchTopKFindResultsForOnlyOnePopularKeyword(query, activeNodesVector.at(0) ,
                                            offset + nextK - queryResults->getNumberOfResults() , queryResults);
                // By setting this flag, we inform the user that results are approximated.
        		queryResultsInternal->resultsApproximated = true;

                if (activeNodesVector.at(0)->isResultsCached() == true){
                    activeNodesVector.at(0)->busyBit->setFree();
                }else{
        	        delete activeNodesVector.at(0);
        	    }
                queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
                return numberOfResults;
           }

        }


        // NULL is passed as the third argument to notify TermVirtualList constructor that TVLs must be created for everything.
    	this->computeTermVirtualList(queryResults, &activeNodesVector, NULL);

    	queryResultsInternal->estimatedNumberOfResults = this->estimateNumberOfResults(query, activeNodesVector );

        // get the std::vector of virtual lists of each term
        std::vector<TermVirtualList* > *virtualListVector = queryResultsInternal->getVirtualListVector();

        // container for the records that contains all the terms in this query (for caching purposes)
        vector<CandidateResult> *candidateList = new std::vector<CandidateResult>();

        // Cached results for the first k terms
        // TODO revive caching here
        if ( false
                && (conjunctionCacheResultsEntry != NULL)
                && (cacheResponse == 1)
                // && (conjunctionCacheResultsEntry->queryTerms != NULL)
                // && (virtualListVector != NULL)
                && (conjunctionCacheResultsEntry->queryTerms->size() <= virtualListVector->size()) ) {
            queryResults->addMessage("Conjunction Cache Hit - case 2");
            // Set the cursors of the first k TermVirtualLists
            for (unsigned int i = 0; i < conjunctionCacheResultsEntry->queryTerms->size(); ++i)    {
                virtualListVector->at(i)->setCursors(conjunctionCacheResultsEntry->virtualListCursorsVector->at(i));
            }

            // Do random access to find out if the candidates are valid results
            for (unsigned int i = 0; i < conjunctionCacheResultsEntry->candidateList->size(); ++i) {
                unsigned internalRecordId = conjunctionCacheResultsEntry->candidateList->at(i).internalRecordId;

                // if the record has been seen before, do nothing
                if (visitedList.count(internalRecordId)) {
                    continue;
                }
                // mark the record as seen
                visitedList.insert(internalRecordId);

                // create the score vector with k cached term scores and leave the rest as 0 to be filled in random access
                std::vector<float> queryResultTermScores(conjunctionCacheResultsEntry->candidateList->at(i).termScores);
                queryResultTermScores.resize(query->getQueryTerms()->size(), 0);

                std::vector<std::string> queryResultMatchingKeywords(conjunctionCacheResultsEntry->candidateList->at(i).matchingKeywords);
                std::vector<unsigned> queryResultAttributeBitmaps(conjunctionCacheResultsEntry->candidateList->at(i).attributeBitmaps);
                queryResultMatchingKeywords.resize(query->getQueryTerms()->size(), "");

                std::vector<unsigned> queryResultEditDistances(conjunctionCacheResultsEntry->candidateList->at(i).editDistances);
                queryResultEditDistances.resize(query->getQueryTerms()->size(), 0);

                if (randomAccess(virtualListVector, queryResultTermScores,
                                 queryResultMatchingKeywords, queryResultAttributeBitmaps, queryResultEditDistances,
                                 query, internalRecordId,
                                 conjunctionCacheResultsEntry->queryTerms->size()-1,
                                 conjunctionCacheResultsEntry->queryTerms->size())) {
                    //TODO OPT struct copy in insertResult
                    // add this record to topK results if its score is good enough
                    bool validForwardList;
                    this->indexData->forwardIndex->getForwardList(internalRecordId, validForwardList);
                    if (validForwardList) {
                        QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
                        queryResult->internalRecordId = internalRecordId;

                        queryResult->_score.setTypedValue(query->getRanker()->computeOverallRecordScore(query, queryResultTermScores));//TODO
                        queryResult->matchingKeywords = queryResultMatchingKeywords;
                        queryResult->attributeBitmaps = queryResultAttributeBitmaps;
                        queryResult->editDistances = queryResultEditDistances;
                        queryResultsInternal->insertResult(queryResult);

                        // add this record to the candidate list for caching purposes
                        CandidateResult candidate;
                        candidate.internalRecordId = internalRecordId;
                        candidate.termScores = queryResultTermScores;
                        candidate.matchingKeywords = queryResultMatchingKeywords;
                        candidate.attributeBitmaps = queryResultAttributeBitmaps;
                        candidate.editDistances = queryResultEditDistances;
                        candidateList->push_back(candidate);
                    }
                }
            }
            // Give back cache busy bit.
            conjunctionCacheResultsEntry->busyBit->setFree();
            if (conjunctionCacheResultsEntry->isCached == 0)
                delete conjunctionCacheResultsEntry;
        }

        struct timespec tend;
        clock_gettime(CLOCK_REALTIME, &tend);
        unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
                + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
        std::cout << "Time for pre-topK : " << ts1 << std::endl;
        clock_gettime(CLOCK_REALTIME, &tstart);

        // Fagin's Algorithm
        float maxScoreForUnvisitedRecords;
        bool stop = false;
        std::vector<float> queryResultTermScores;
        std::vector<std::string> queryResultMatchingKeywords;
        std::vector<unsigned> queryResultAttributeBitmaps;
        std::vector<unsigned> queryResultEditDistances;

        HeapItemForIndexSearcher *heapItem = new HeapItemForIndexSearcher();
        queryResults->addMessage("Fagin's Loop Start");
        while (!stop) {
            // maxScoreForUnvisitedRecords is the upper bound of the scores of these unvisited records
            maxScoreForUnvisitedRecords = 0;
            for (unsigned int i = 0; i < virtualListVector->size(); ++i) {
                float score;

                if(virtualListVector->at(i)->isTermVirtualListDisabled() == true){
                	// when the list is disabled, the highest score is always the score of the top record in the beginning
                	// to make sure early termination does not produce incorrect results.
                	score = virtualListVector->at(i)->getScoreOfTopRecordWhenListIsDisabled();
                }else{
					if (!virtualListVector->at(i)->getMaxScore(score)) {
						stop = true;
						break;
					}
                }

                maxScoreForUnvisitedRecords =
                    query->getRanker()->aggregateBoostedTermRuntimeScore(maxScoreForUnvisitedRecords,
                            query->getQueryTerms()->at(i)->getBoost(), score) ;
            }
            // round robin: go through all the virtual lists
            for (unsigned int i = 0; i < virtualListVector->size(); ++i) {

                if(virtualListVector->at(i)->isTermVirtualListDisabled() == true){
                	// This means term i is too popular and no term virtual list is actually created for it.
                	// so we ignore it because we cannot get next heap item from it
                	continue;
                }

                // Step 2
                // get one element from one virtual list
                // if the term virtual list has no more item, stop
                if (!virtualListVector->at(i)->getNext(heapItem)) {
                    stop = true;
                    break;
                }

                float newScore = heapItem->termRecordRuntimeScore ;
                virtualListVector->at(i)->getMaxScore(newScore);

                // old value = old_value - popped item score + next item score
                maxScoreForUnvisitedRecords = query->getRanker()->aggregateBoostedTermRuntimeScore(maxScoreForUnvisitedRecords,
                                              query->getQueryTerms()->at(i)->getBoost(),
                                              newScore - heapItem->termRecordRuntimeScore /*order reversed to make it 'subtract'*/) ;

                // get the recordId
                unsigned internalRecordId = heapItem->recordId;

                // if the record has been seen before, do nothing
                if (visitedList.count(internalRecordId))
                    continue;

                // mark the record as seen
                visitedList.insert(internalRecordId);

                // assign the vectors with default values to clear them
                queryResultTermScores.assign(query->getQueryTerms()->size(), 0);
                queryResultMatchingKeywords.assign(query->getQueryTerms()->size(), "");
                queryResultAttributeBitmaps.assign(query->getQueryTerms()->size(), 0);
                queryResultEditDistances.assign(query->getQueryTerms()->size(), 0);

                /*// Check if the hit is from a stemmed keyword.
                if (this->indexData->analyzerInternal->getStemmerNormalizerType()
                        != srch2::instantsearch::NO_STEMMER_NORMALIZER) {
                    unsigned heapItemKeywordId;//Unused
                    float heapItemScore;//Unused
                    bool heapItemIsStemmed;
                    unsigned minId = heapItem->trieNode->getMinId();
                    unsigned maxId = heapItem->trieNode->getMaxId();
                    unsigned termSearchableAttributeIdToFilterTermHits =
                    virtualListVector->at(i)->getTermSearchableAttributeIdToFilterTermHits();

                    // TODO Optimise this lookup, so that score , matchingKeywordId is not calculated twice for a stemmed heapItem hit.
                    if (this->indexData->forwardIndex->haveWordInRangeWithStemmer(internalRecordId,
                                              minId, maxId,
                                              termSearchableAttributeIdToFilterTermHits,
                                              heapItemKeywordId,
                                              heapItemScore,
                                              heapItemIsStemmed)) {
                        if(heapItemIsStemmed) {
                            queryResultMatchingKeywords.at(i)="STEM"; // "STEM" is the matching keyword that denotes the stemmed keyword
                        }
                        else {
                            std::vector<CharType> temp;
                            this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root, heapItem->trieNode, temp);
                            string str;
                            charTypeVectorToUtf8String(temp, str);
                            queryResultMatchingKeywords.at(i) = str;
                        }
                    }
                }
                else {*/
                if (virtualListVector->at(i)->usingBitset) {
                    queryResultMatchingKeywords.at(i) = "";     //if the list is merge to a bitset, the matching keywords is the same to the query term.
                } else {
                    std::vector<CharType> temp;
                    this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                                           heapItem->trieNode, temp);
                    string str;
                    charTypeVectorToUtf8String(temp, str);
                    queryResultMatchingKeywords.at(i) = str;
                }
                //}

                queryResultEditDistances.at(i) = heapItem->ed;
                queryResultTermScores.at(i) = heapItem->termRecordRuntimeScore;
                queryResultAttributeBitmaps.at(i) = heapItem->attributeBitMap;

                // Step 3
                // Do random access on the other TermVirtualLists
                if (randomAccess(virtualListVector, queryResultTermScores,
                                 queryResultMatchingKeywords, queryResultAttributeBitmaps, queryResultEditDistances,
                                 query, internalRecordId, i, 0))    {
                    bool validForwardList;
                    this->indexData->forwardIndex->getForwardList(internalRecordId, validForwardList);
                    if (validForwardList) {
                        // add this record to topK results if its score is good enough
                        QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
                        queryResult->internalRecordId = internalRecordId;
                        queryResult->_score.setTypedValue(query->getRanker()->computeOverallRecordScore(query,queryResultTermScores));//TODO
                        queryResult->matchingKeywords = queryResultMatchingKeywords;
                        queryResult->attributeBitmaps = queryResultAttributeBitmaps;
                        queryResult->editDistances = queryResultEditDistances;
                        queryResultsInternal->insertResult(queryResult);

                        //add this record to the candidate list for caching purposes
                        CandidateResult candidate;
                        candidate.internalRecordId = internalRecordId;
                        candidate.termScores = queryResultTermScores;
                        candidate.matchingKeywords = queryResultMatchingKeywords;
                        candidate.attributeBitmaps = queryResultAttributeBitmaps;
                        candidate.editDistances = queryResultEditDistances;
                        candidateList->push_back(candidate);
                    }
                }

            }

            // check for the stopping condition
            if (queryResultsInternal->hasTopK(maxScoreForUnvisitedRecords))
                break;
        }

        delete heapItem;

        queryResults->addMessage("Fagin's loop end");
        queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
        queryResults->addMessage("Finalised the results.");

        clock_gettime(CLOCK_REALTIME, &tend);
        ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
                + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
        std::cout << "Time for topK : " << ts1 << std::endl;
        /*
        // cache the candidateList and the cursors with the query terms
        // construct the cursors vector
        std::vector<std::vector<unsigned>* >* virtualListCursorsVector = new std::vector<std::vector<unsigned>* >();
        for (unsigned i=0; i<query->getQueryTerms()->size(); ++i) {
            std::vector<unsigned>* termCursorsVector = new std::vector<unsigned>();
            virtualListVector->at(i)->getCursors(termCursorsVector);
            virtualListCursorsVector->push_back(termCursorsVector);
        }

        vector<Term* > *cacheQueryTerms = new std::vector<Term *>();

        // Optimize query terms deep copy
        for (std::vector<Term*>::const_iterator iter = query->getQueryTerms()->begin();
                iter!=query->getQueryTerms()->end(); iter++) {
            std::string tmp((*iter)->getKeyword()->c_str());
            cacheQueryTerms->push_back(new Term( tmp, (*iter)->getTermType(),
                                                 (*iter)->getBoost(), (*iter)->getSimilarityBoost(),
                                                 (*iter)->getThreshold() ) );
        }

        queryResults->addMessage("Cache Set start");

        this->cacheManager->setCachedConjunctionResult(query->getQueryTerms(),
                new ConjunctionCacheResultsEntry(cacheQueryTerms,
                                                 candidateList,
                                                 virtualListCursorsVector));
        queryResults->addMessage("Cache Set end");
        */
    }

    return queryResults->getNumberOfResults() - offset;
}


bool suggestionComparator(const pair<std::pair< float , unsigned > , const TrieNode *> & left ,
		const pair<std::pair< float , unsigned > , const TrieNode *> & right ){
	return left.first.first > right.first.first;
}

// This function estimates the results for a single very popular (probably short prefix)
int IndexSearcherInternal::searchTopKFindResultsForOnlyOnePopularKeyword(const Query *query,
		PrefixActiveNodeSet * activeNodes , unsigned k , QueryResults * queryResults){
	ASSERT(query->getQueryTerms()->size() == 1);

	Term * term = query->getQueryTerms()->at(0);

	// 1. first iterate on active nodes and find best estimated leaf nodes.
    std::vector<std::pair< std::pair< float , unsigned > , const TrieNode *> > suggestionPairs;
    findKMostPopularSuggestionsSorted(term , activeNodes , k , suggestionPairs);

    // 3. now iterate on leaf nodes and add records to the result set
    unsigned numberOfResults = 0;
    for(std::vector<std::pair<std::pair< float , unsigned > , const TrieNode * > >::iterator suggestion = suggestionPairs.begin() ;
    		suggestion != suggestionPairs.end() && numberOfResults < k ; ++suggestion){

		shared_ptr<vectorview<unsigned> > invertedListReadView;
		this->indexData->invertedIndex->getInvertedListReadView(suggestion->second->getInvertedListOffset(), invertedListReadView);
        unsigned termAttributeBitmap = 0;
        float termRecordStaticScore = 0;
        // move on inverted list and add the records which are valid
        unsigned invertedListCursor = 0;
        while(invertedListCursor < invertedListReadView->size()){
			unsigned recordId = invertedListReadView->getElement(invertedListCursor++);
			unsigned recordOffset = this->indexData->invertedIndex->getKeywordOffset(recordId, suggestion->second->getInvertedListOffset());
			if (this->indexData->invertedIndex->isValidTermPositionHit(recordId, recordOffset,
					0x7fffffff,  termAttributeBitmap, termRecordStaticScore)) { // 0x7fffffff means OR on all attributes
                QueryResult * queryResult = queryResults->impl->getReultsFactory()->impl->createQueryResult();
                // prepare query results matching keywords
                std::vector<std::string> queryResultMatchingKeywords;
            	string suggestionString ;
                this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                                       suggestion->second, suggestionString);
                queryResultMatchingKeywords.push_back(suggestionString);

                // prepare edit distances
                std::vector<unsigned> editDistances;
                editDistances.push_back(suggestion->first.second);
                // prepare attribute bitmaps
                std::vector<unsigned> attributeBitmaps;
                attributeBitmaps.push_back(0x7fffffff);
                // prepare score
                std::vector<float> queryTermResultScores;
                queryTermResultScores.push_back(
                		query->getRanker()->computeTermRecordRuntimeScore(termRecordStaticScore, suggestion->first.second,
                                                term->getKeyword()->size(),
                                                true,
                                                query->getPrefixMatchPenalty() , term->getSimilarityBoost() ) );
                queryResult->internalRecordId = recordId;
                queryResult->matchingKeywords = queryResultMatchingKeywords;
                queryResult->editDistances = editDistances;
                queryResult->attributeBitmaps = attributeBitmaps;
                queryResult->_score.setTypedValue(query->getRanker()->computeOverallRecordScore(query, queryTermResultScores));
                queryResults->impl->insertResult(queryResult);
				numberOfResults ++;
				if(numberOfResults >= k){
					break;
				}
			}
        }
    }
	return numberOfResults;


}

int IndexSearcherInternal::suggest(const string & keyword,
		float fuzzyMatchPenalty ,
		const unsigned numberOfSuggestionsToReturn ,
		vector<string> & suggestions){

	// non valid cases for input
	if(keyword.compare("") == 0 || numberOfSuggestionsToReturn == 0){
		return 0;
	}
    if (this->indexData->isBulkLoadDone() == false){
        return -1;
    }

	// make sure fuzzyMatchPenalty is in [0,1]
	if(fuzzyMatchPenalty < 0 || fuzzyMatchPenalty > 1){
		fuzzyMatchPenalty = 0.5;
	}
	// calculate editDistanceThreshold
	// TODO use Ranker to implement this logic
	unsigned editDistanceThreshold = keyword.length() * (1 - fuzzyMatchPenalty);

	// compute active nodes
	// 1. first we must create term object which is used to compute activenodes.
	//  TERM_TYPE_COMPLETE and 0 in the arguments will not be used.
	Term * term = new Term(keyword , TERM_TYPE_COMPLETE , 0, fuzzyMatchPenalty , editDistanceThreshold);
	// 2. compute active nodes.
	PrefixActiveNodeSet *termActiveNodeSet = this->computeActiveNodeSet(term);
	// 3. we don't need the term anymore
	delete term;

	// 4. now iterate on active nodes and find suggestions for each on of them
    std::vector<std::pair<std::pair< float , unsigned > , const TrieNode *> > suggestionPairs;
    findKMostPopularSuggestionsSorted(term , termActiveNodeSet , numberOfSuggestionsToReturn , suggestionPairs);

    int suggestionCount = 0;
    for(std::vector<std::pair<std::pair< float , unsigned > , const TrieNode * > >::iterator suggestion = suggestionPairs.begin() ;
    		suggestion != suggestionPairs.end() && suggestionCount < numberOfSuggestionsToReturn ; ++suggestion , ++suggestionCount){
    	string suggestionString ;
        this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                               suggestion->second, suggestionString);
    	suggestions.push_back(suggestionString);
    }
	// 5. now delete activenode set
    if (termActiveNodeSet->isResultsCached() == true)
    	termActiveNodeSet->busyBit->setFree();
    else
        delete termActiveNodeSet;
	return 0;
}


unsigned IndexSearcherInternal::estimateNumberOfResults(const Query *query){
    // Empty Query case
    if (query->getQueryTerms()->size() == 0) {
        return 0;
    }
	// this vector is passed to computeTermVirtualList to be used
	vector<PrefixActiveNodeSet *> activeNodesVector;
	// iterate on terms and compute active nodes
    for (vector<Term*>::const_iterator vectorIterator = query->getQueryTerms()->begin();
            vectorIterator != query->getQueryTerms()->end();
            ++vectorIterator) {
        Term *term = *vectorIterator;
        // compute activenodes
        PrefixActiveNodeSet * activeNodes =  this->computeActiveNodeSet(term);
        activeNodesVector.push_back(activeNodes);
    }
    unsigned estimatedNumberOfresults = this->estimateNumberOfResults(query, activeNodesVector);

    // deallocate all active nodes
    for(vector<PrefixActiveNodeSet *>::iterator activeNodeIter = activeNodesVector.begin() ; activeNodeIter != activeNodesVector.end() ; ++activeNodeIter){
    	PrefixActiveNodeSet * activeNode = *activeNodeIter;
		if (activeNode->isResultsCached() == true)
			activeNode->busyBit->setFree();
		else
			delete activeNode;
    }

    return estimatedNumberOfresults;

}


int IndexSearcherInternal::search(const Query *query, QueryResults* queryResults, const int offset, const int nextK ,
		unsigned estimatedNumberOfResultsThresholdGetAll , unsigned numberOfEstimatedResultsToFindGetAll)
{
    int returnValue = -1;

    if (this->indexData->isBulkLoadDone() == false)
        return returnValue;

    if (query->getQueryType() == srch2::instantsearch::SearchTypeTopKQuery) {
        this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
        returnValue = this->searchTopKQuery(query, offset, nextK, queryResults);
        this->indexData->rwMutexForIdReassign->unlockRead();
    } else if (query->getQueryType() == srch2::instantsearch::SearchTypeGetAllResultsQuery) {
        this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
        returnValue = this->searchGetAllResultsQuery(query, queryResults , estimatedNumberOfResultsThresholdGetAll , numberOfEstimatedResultsToFindGetAll);
        this->indexData->rwMutexForIdReassign->unlockRead();
    }
    //queryResults->printResult();

    return returnValue;
}

// find top-k answer. returns the number of records found
int IndexSearcherInternal::search(const Query *query, QueryResults* queryResults, const int topK,
		unsigned estimatedNumberOfResultsThresholdGetAll , unsigned numberOfEstimatedResultsToFindGetAll)
{
    return search(query, queryResults, 0, topK , estimatedNumberOfResultsThresholdGetAll , numberOfEstimatedResultsToFindGetAll);
}

int IndexSearcherInternal::search(const Query *query, QueryResults* queryResults)
{
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    int returnValue = this->searchMapQuery(query, queryResults);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
    return returnValue;
}


// for doing a range query with a rectangle
void IndexSearcherInternal::search(const Rectangle &queryRectangle, QueryResults *queryResults)
{
    QueryResultsInternal *queryResultsInternal = queryResults->impl;
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    this->indexData->quadTree->rangeQueryWithoutKeywordInformation(queryRectangle,queryResultsInternal);
    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
}




// for doing a range query with a circle,
void IndexSearcherInternal::search(const Circle &queryCircle, QueryResults *queryResults)
{
    QueryResultsInternal *queryResultsInternal = queryResults->impl;
    this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
    this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
    this->indexData->quadTree->rangeQueryWithoutKeywordInformation(queryCircle,queryResultsInternal);
    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    this->indexData->rwMutexForIdReassign->unlockRead();
    this->indexer->rwMutexForWriter->unlockRead();
}

// for retrieving only one result by having the primary key
void IndexSearcherInternal::search(const std::string & primaryKey, QueryResults *queryResults){

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

void IndexSearcherInternal::computeTermVirtualList(QueryResults *queryResults ,
		vector<PrefixActiveNodeSet *> * activeNodes,
		const vector<float> * scoreOfTopRecords) const
{
    const Query *query = queryResults->impl->getQuery();
    const vector<Term* > *queryTerms = query->getQueryTerms();
    if (query->getQueryType() != SearchTypeMapQuery) {
    	unsigned termIndex = 0;
        for (vector<Term*>::const_iterator vectorIterator = queryTerms->begin();
                vectorIterator != queryTerms->end();
                vectorIterator++ ) {
            // compute the active nodes for this term
            Term *term = *vectorIterator;
            std::stringstream str;

            str << "ActiveNodes compute:"<<*term->getKeyword();
            queryResults->addMessage(str.str().c_str());

            /*timespec t1;
            timespec t2;
            clock_gettime(CLOCK_REALTIME, &t1);*/

            // By default this function computes the active nodes itself. But if a vector of pointers to
            // activeNodeSets is available, it uses that vector instead of computing them from scratch.
            PrefixActiveNodeSet *termActiveNodeSet;
            if(activeNodes == NULL){
            	termActiveNodeSet = this->computeActiveNodeSet(term);
            }else{
            	termActiveNodeSet = activeNodes->at(termIndex);
            }

            // By default no term is too popular. If the vector of tooPopular flags is available, we use this vector
            // to get the value for this term
            float scoreOfTopRecord = -1;
            if(scoreOfTopRecords != NULL){
            	scoreOfTopRecord = scoreOfTopRecords->at(termIndex);
            }

            /*clock_gettime(CLOCK_REALTIME, &t2);
            double time_span = (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;
            cout << "compute active node set cost: " << time_span << " milliseconds." << endl;*/

            queryResults->addMessage("ActiveNode computed");

            // compute the virtual list for this term
            float prefixMatchPenalty = query->getPrefixMatchPenalty();
            TermVirtualList *termVirtualList = new TermVirtualList(this->indexData->invertedIndex, termActiveNodeSet,
                    term, prefixMatchPenalty , scoreOfTopRecord);
            queryResults->addMessage("TermVList computed");

            ///check if termActiveNodeSet is cached, if not delete it to prevent memory leaks.
            //if(termActiveNodeSet->isResultsCached() == false)
            //    delete termActiveNodeSet;

            queryResults->impl->virtualListVector->push_back(termVirtualList);
            //
            termIndex ++;
        }
        std::cout << std::endl;
    }
}

PrefixActiveNodeSet *IndexSearcherInternal::computeActiveNodeSet(Term *term) const
{
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
    PrefixActiveNodeSet *initialPrefixActiveNodeSet = NULL;
    int cacheResponse = this->cacheManager->findLongestPrefixActiveNodes(term, initialPrefixActiveNodeSet); //initialPrefixActiveNodeSet is Busy

    if ( (initialPrefixActiveNodeSet == NULL) || (cacheResponse == 0)) { // NO CacheHit,  response = 0
        //std::cout << "|NO Cache|" << std::endl;;
        // No prefix has a cached TermActiveNode Set. Create one for the empty std::string "".
        initialPrefixActiveNodeSet = new PrefixActiveNodeSet(this->indexReadToken.trieRootNodeSharedPtr, term->getThreshold(), this->indexData->getSchema()->getSupportSwapInEditDistance());
        initialPrefixActiveNodeSet->busyBit->setBusy();
    }
    cachedPrefixLength = initialPrefixActiveNodeSet->getPrefixLength();

    /// 2. do the incremental computation. BusyBit of prefixActiveNodeSet is busy.
    PrefixActiveNodeSet *prefixActiveNodeSet = initialPrefixActiveNodeSet;

    for (unsigned iter = cachedPrefixLength; iter < keywordLength; iter++) {
        CharType additionalCharacter = charTypeKeyword[iter]; // get the appended character

        PrefixActiveNodeSet *newPrefixActiveNodeSet = prefixActiveNodeSet->computeActiveNodeSetIncrementally(additionalCharacter);
        newPrefixActiveNodeSet->busyBit->setBusy();

        // If the last result was not put into the cache successfully (e.g., due to
        // limited cache size), we can safely delete it now
        if (prefixActiveNodeSet->isResultsCached() == false) {
            //if (prefixActiveNodeSet->busyBit->isBusy())
            delete prefixActiveNodeSet;
        }
        prefixActiveNodeSet = newPrefixActiveNodeSet;

        //std::cout << "Cache Set:" << *(prefixActiveNodeSet->getPrefix()) << std::endl;

        if (iter >= 2 && (cacheResponse != -1)) { // Cache not busy and keywordLength is at least 2.
            cacheResponse = this->cacheManager->setPrefixActiveNodeSet(prefixActiveNodeSet);
        }
    }
    // Possible memory leak due to last prefixActiveNodeSet not being cached. This is checked for
    // and deleted by the caller "QueryResultsInternal()"
    return prefixActiveNodeSet;
}


// This function uses the histogram information of the trie to estimate the number of records which have this term
// it uses the most popular active node to estimate this value
// NOTE : we cannot add all active node popularity values together because some of them are descendants of another ones ...
unsigned IndexSearcherInternal::getEstimatedNumberOfRecordsWithThisTerm(Term *term , PrefixActiveNodeSet * activeNodes) const{

	if(activeNodes == NULL){
		ASSERT(false);
		return 0;
	}
	TrieNodePointer mostPopularTrieNode = NULL;
	// Iterate on active nodes and find the one with the largest histogramValue
    for (ActiveNodeSetIterator iter(activeNodes, term->getThreshold()); !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        if(mostPopularTrieNode == NULL){
        	mostPopularTrieNode = trieNode;
        	continue;
        }
        // logic of finding the maximum popularity/frequency value
        if(trieNode->getNodeProbabilityValue() > mostPopularTrieNode->getNodeProbabilityValue()){
        	mostPopularTrieNode = trieNode;
        }
    }
    if(mostPopularTrieNode == NULL){
    	return 0;
    }
    return this->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView() * mostPopularTrieNode->getNodeProbabilityValue();
}

bool IndexSearcherInternal::isTermTooPopular(Term *term , PrefixActiveNodeSet * activeNodes , unsigned & popularity) const{
	if(term == NULL){
		popularity = 0;
		return false;
	}
	popularity = getEstimatedNumberOfRecordsWithThisTerm(term , activeNodes);

	return (popularity > this->getKeywordPopularityThreshold());

}

float IndexSearcherInternal::findTopRunTimeScoreOfLeafNodes(Term *term , float prefixMatchPenalty , PrefixActiveNodeSet * activeNodes) const{

	if(activeNodes == NULL){
		return 0;
	}

	float topScore = 0;
	// now iterate on active nodes and find suggestions for each on of them
    ActiveNodeSetIterator iter(activeNodes, term->getThreshold());
    for (; !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        float runTimeScoreOfThisTrieNode = DefaultTopKRanker::computeTermRecordRuntimeScore(trieNode->getMaximumScoreOfLeafNodes() ,
                distance,
                term->getKeyword()->size(),
                true,
                prefixMatchPenalty , term->getSimilarityBoost());
        if(runTimeScoreOfThisTrieNode > topScore){
        	topScore = runTimeScoreOfThisTrieNode;
        }
    }

	return topScore;

}

void IndexSearcherInternal::findKMostPopularSuggestionsSorted(Term *term ,
		PrefixActiveNodeSet * activeNodes,
		unsigned numberOfSuggestionsToReturn ,
		std::vector<std::pair<std::pair< float , unsigned > , const TrieNode *> > & suggestionPairs) const{
	// first make sure input is OK
	if(term == NULL || activeNodes == NULL) return;

	// now iterate on active nodes and find suggestions for each on of them
    ActiveNodeSetIterator iter(activeNodes, term->getThreshold());
    for (; !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        trieNode->findMostPopularSuggestionsInThisSubTrie(distance, suggestionPairs , numberOfSuggestionsToReturn );
        if(suggestionPairs.size() >= numberOfSuggestionsToReturn){
        	break;
        }
    }

    // now sort the suggestions
    std::sort(suggestionPairs.begin() , suggestionPairs.end() , suggestionComparator);
}
unsigned IndexSearcherInternal::estimateNumberOfResults(const Query *query, std::vector<PrefixActiveNodeSet *>& activeNodes) const{

	float aggregatedProbability = 1;
	for(int i=0; i< query->getQueryTerms()->size() ; i++){
		aggregatedProbability *=
				getPrefixPopularityProbability(activeNodes.at(i) , query->getQueryTerms()->at(i)->getThreshold());
	}
	// now we should multiply the total probability and the total number of records.
	return (unsigned)(aggregatedProbability * this->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView());
}

float IndexSearcherInternal::getPrefixPopularityProbability(PrefixActiveNodeSet * activeNodes , unsigned threshold) const{

	/*
	 * Example :
	 *                          --t(112,112,112)$              ---- t ----- ***e ----- e ----- n(96,96,96)$
	 *    root                  |                              |  (96,96)   (96,96)  (96,96)
	 *      |                   |                              |
	 *      |       (32,112) (32,112)        (32,32,96)$       |---- s(80,80,80)$
	 *      ----------- c ----- a------------ ***n ------------|
	 *      |                                                  |---- ***c ----- e ----- r(64,64,64)$
	 *      |                                                  |     (64,64)  (64,64)
	 *      ***a(16,16)                                        |
	 *      |                                                  ---- a ----- d ----- a(48,48,48)$
	 *      |                                                     (48,48) (48,48)
	 *      n(16,16)
	 *      |
	 *      |
	 *      ***d(16,16,16)$
	 *
	 * and suppose those trie nodes that have *** next to them are activeNodes. (n,a,c,d, and e)
	 *
	 * After sorting in pre-order, the order of these active nodes will be :
	 * a , d , n , e , c
	 *
	 * and if we always compare the currentNode with the last top node:
	 * 1. 'a' is added to topNodes
	 * 2. 'd' is ignored because it's a descendant of 'a'
	 * 3. 'n' has no relationship with 'a' so it's added to topNodes.
	 * 4. 'e' is ignored because it's a descendant of 'n'
	 * 5. 'c' is ignored because it's a descendant of 'n'
	 * so we will have <a,n>
	 *
	 * and then we use joint probability to aggregate 'a' and 'n' values.
	 *
	 */
	std::vector<TrieNodePointer> topTrieNodes;
	std::vector<TrieNodePointer> preOrderSortedTrieNodes;
	// iterate on active nodes and keep them in preOrderSortedTrieNodes to be sorted in next step
    for (ActiveNodeSetIterator iter(activeNodes, threshold); !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        preOrderSortedTrieNodes.push_back(trieNode);
    }
    // now sort them in preOrder
    std::sort(preOrderSortedTrieNodes.begin() , preOrderSortedTrieNodes.end() , TrieNodePreOrderComparator());

    // now move from left to right and always compare the current node with the last node in topTrieNodes
    for(unsigned trieNodeIter = 0 ; trieNodeIter < preOrderSortedTrieNodes.size() ; ++trieNodeIter){
    	TrieNodePointer currentTrieNode = preOrderSortedTrieNodes.at(trieNodeIter);
    	if(topTrieNodes.size() == 0){ // We always push the first node into topTrieNodes.
    		topTrieNodes.push_back(currentTrieNode);
    		continue;
    	}
    	/// since trie nodes are coming in preOrder, currentTrieNode is either a descendant of the last node in topTrieNodes,
    	// or it has no relationship with that node. In the former case, we ignore currentTrieNode. In the latter case, we append it to topTrieNodes.
    	if(currentTrieNode->isDescendantOf(topTrieNodes.at(topTrieNodes.size()-1)) == false){ // if it's not descendant
    		topTrieNodes.push_back(currentTrieNode);
    	}// else : if it's a descendant we don't have to do anything

    }

    // now we have the top level trieNodes
    // we move on all top trie nodes and aggregate their probability by using Joint Probability formula
    float aggregatedProbability = 0;
    for(std::vector<TrieNodePointer>::iterator trieNodeIter = topTrieNodes.begin() ; trieNodeIter != topTrieNodes.end() ; ++trieNodeIter){
    	TrieNodePointer topTrieNode = *trieNodeIter;
    	aggregatedProbability = topTrieNode->aggregateValueByJointProbability(aggregatedProbability , topTrieNode->getNodeProbabilityValue());
    }

    return aggregatedProbability;
}

/**
 * skip is the index for TermVirtualList iterator. If one of the TermVirtualLists is popped up during Fagin's algorithm, it should be skipped during Random Access.
 * Start is the start index of the random Access. Say for query a1 a2 a3 a4, "a1 a2" is in cache, we have to do random access on "a3 a4", so skip is start-1, start is 2
 * skip = start-1 will guarantee not to skip anything not computed yet; because the term at index "start-1" is already cached.
 */
bool IndexSearcherInternal::randomAccess(std::vector<TermVirtualList* > *virtualListVector,
        std::vector<float> &queryResultTermScores,
        std::vector<std::string> &queryResultMatchingKeywords,
        std::vector<unsigned> &queryResultBitmaps,
        std::vector<unsigned> &queryResultEditDistances,
        const Query *query, unsigned recordId,
        unsigned skip, unsigned start)
{
    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    for (unsigned int j = start; j < virtualListVector->size(); ++j) {
        if (skip == j) // skip the virtual list popped up in round robin
            continue;
        bool found = false;
        // if the j th virtual list need merge to be a Bitset
        if (virtualListVector->at(j)->usingBitset) {
            if (virtualListVector->at(j)->bitSet.get(recordId)) { // if the bit of recordId is set, we find it
                found = true;                                   // we find it in this list
                queryResultMatchingKeywords.at(j) = "";         // the matching term is the same to the query term, we just ignore it
                queryResultBitmaps.at(j) = 0;                   // we lose this bitmap
                queryResultEditDistances.at(j) = 0;             // we lose the edit distance
                virtualListVector->at(j)->getMaxScore(queryResultTermScores.at(j));              // we assign the same score for it
            }
        } else {  //do the verification
            PrefixActiveNodeSet *prefixActiveNodeSet;
            virtualListVector->at(j)->getPrefixActiveNodeSet(prefixActiveNodeSet);
            unsigned termSearchableAttributeIdToFilterTermHits =
                virtualListVector->at(j)->getTermSearchableAttributeIdToFilterTermHits();
            // assume the iterator returns the ActiveNodes in the increasing order based on edit distance
            for (ActiveNodeSetIterator iter(prefixActiveNodeSet, queryTerms->at(j)->getThreshold());
                    !iter.isDone(); iter.next()) {
                const TrieNode *trieNode;
                unsigned distance;
                iter.getItem(trieNode, distance);

                unsigned minId = trieNode->getMinId();
                unsigned maxId = trieNode->getMaxId();
                if (virtualListVector->at(j)->getTermType() == srch2::instantsearch::TERM_TYPE_COMPLETE) {
                    if (trieNode->isTerminalNode())
                        maxId = minId;
                    else
                        continue;  // ignore non-terminal nodes
                }

                unsigned matchingKeywordId;
                float termRecordStaticScore;
                unsigned termAttributeBitmap;
                /* bool isStemmed;
                 // the similarity between a record and a prefix is the largest
                 // similarity between this prefix and keywords in the record
                 if (this->indexData->analyzerInternal->getStemmerNormalizerType()
                         != srch2::instantsearch::NO_STEMMER_NORMALIZER) {
                     if (this->indexData->forwardIndex->haveWordInRangeWithStemmer(recordId, minId, maxId,
                                               termSearchableAttributeIdToFilterTermHits,
                                               matchingKeywordId, termRecordStaticScore, isStemmed)) {
                         if(isStemmed) {
                             // "STEM" is the matching keyword that denotes the stemmed keyword
                             queryResultMatchingKeywords.at(j)="STEM";
                         }
                         else {
                             std::vector<CharType> temp;
                             this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                                trieNode, temp);
                             string str;
                             charTypeVectorToUtf8String(temp, str);
                             queryResultMatchingKeywords.at(j) = str;
                         }
                         queryResultEditDistances.at(j) = distance;

                         // the following flag shows whether the matching keyword is a prefix (not a complete) match
                         // of the query term
                         bool isPrefixMatch = ( (!trieNode->isTerminalNode()) || (minId != matchingKeywordId) );
                         queryResultTermScores.at(j) =
                             query->getRanker()->computeTermRecordRuntimeScore(termRecordStaticScore, distance,
                                                                           queryTerms->at(j)->getKeyword()->size(),
                                                                           isPrefixMatch,
                                                                           query->getPrefixMatchPenalty());
                         found = true;
                         break;
                     }
                 }
                 else {*/
                if (this->indexData->forwardIndex->haveWordInRange(recordId, minId, maxId,
                        termSearchableAttributeIdToFilterTermHits,
                        matchingKeywordId, termAttributeBitmap, termRecordStaticScore)) {
                    std::vector<CharType> temp;
                    this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root,
                                                           trieNode, temp);
                    string str;
                    charTypeVectorToUtf8String(temp, str);
                    queryResultMatchingKeywords.at(j) = str;
                    queryResultBitmaps.at(j) = termAttributeBitmap;
                    queryResultEditDistances.at(j) = distance;

                    bool isPrefixMatch = ( (!trieNode->isTerminalNode()) || (minId != matchingKeywordId) );
                    queryResultTermScores.at(j) =
                        query->getRanker()->computeTermRecordRuntimeScore(termRecordStaticScore, distance,
                                queryTerms->at(j)->getKeyword()->size(),
                                isPrefixMatch,
                                query->getPrefixMatchPenalty() , queryTerms->at(j)->getSimilarityBoost() );
                    found = true;
                    break;
                }
            }
            //}
        }
        if (!found)
            return false;
    }
    return true;
}

void IndexSearcherInternal::cacheClear()
{
    this->cacheManager->clear();
}

//DEBUG function. Used in CacheIntegration_Test
bool IndexSearcherInternal::cacheHit(const Query *query)
{
    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    //Empty Query case
    if (queryTerms->size() == 0)
        return false;

    // Cache lookup, assume a query with the first k terms found in the cache
    ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry;
    this->cacheManager->getCachedConjunctionResult(queryTerms, conjunctionCacheResultsEntry);

    // Cached results for the first k terms
    if (conjunctionCacheResultsEntry != NULL)
        return true;

    return false;
}

}
}


