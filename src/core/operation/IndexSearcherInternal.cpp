
// $Id: IndexSearcherInternal.cpp 3480 2013-06-19 08:00:34Z jiaying $

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
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>

#include "operation/IndexerInternal.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/ActiveNode.h"
#include "operation/TermVirtualList.h"
#include "query/QueryResultsInternal.h"
#include "index/Trie.h"
#include "util/Assert.h"
#include "index/ForwardIndex.h"
#include "geo/QuadTree.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <gperftools/profiler.h>

using namespace std;

namespace srch2
{
namespace instantsearch
{

IndexSearcherInternal::IndexSearcherInternal(IndexReaderWriter *indexer)
{
    this->indexData = dynamic_cast<const IndexData*>(indexer->getReadView(this->indexReadToken));
    this->cacheManager = dynamic_cast<Cache*>(indexer->getCache());
    this->indexer = indexer;
}

// find the next k answer starting from "offset". Can be used for
// pagination. Returns the number of records found
int IndexSearcherInternal::searchGetAllResultsQuery(const Query *query, QueryResults* queryResults)
{
    //TODO: check the queryResults was create using query.
    //TODO: Use edit distance and length in ranking
    QueryResultsInternal *queryResultsInternal = dynamic_cast<QueryResultsInternal *>(queryResults);
    this->computeTermVirtualList(queryResultsInternal);

    // get the std::vector of virtual lists of each term
    std::vector<TermVirtualList* > *virtualListVector = queryResultsInternal->getVirtualListVector();
    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    unsigned queryTermsLength = 1; // To prevent division by 0 in normalised edit distance calculation
    for (vector<Term *>::const_iterator queryTermsIterator = queryTerms->begin(); 
         queryTermsIterator != queryTerms->end(); queryTermsIterator++ )
    {
        Term *term = *queryTermsIterator;
        queryTermsLength += term->getKeyword()->size();
    }

    //Empty Query case
    if (queryTerms->size() == 0)
    {
        return 0;
    }

    std::vector<float> queryResultTermScores;
    std::vector<std::string> queryResultMatchingKeywords;
    std::vector<unsigned> queryResultBitmaps;
    std::vector<unsigned> queryResultEditDistances;
    HeapItemForIndexSearcher *heapItem = new HeapItemForIndexSearcher();

    //find the smallest virtualList
    unsigned smallestVirtualListVectorId = 0;
    unsigned smallestVirtualListVectorSize = virtualListVector->at(0)->getVirtualListTotalLength();
    for (unsigned int iter = 1; iter < virtualListVector->size(); ++iter) {
        unsigned currentSize = virtualListVector->at(iter)->getVirtualListTotalLength();
        if( smallestVirtualListVectorSize > currentSize ) {
            smallestVirtualListVectorId = iter;
            smallestVirtualListVectorSize = currentSize;
        }
    }
    
    // fill the visited list with the current queryResults
    std::set<unsigned> visitedList;

    //unsigned idsFound = 0;
    while(virtualListVector->at(smallestVirtualListVectorId)->getNext(heapItem))
    {

        unsigned internalRecordId = heapItem->recordId;

        // if the record has been seen before, do nothing
        if(visitedList.count(internalRecordId))
        {
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
        
        // Do random access on the other TermVirtualLists
        if (randomAccess(virtualListVector, queryResultTermScores, queryResultMatchingKeywords, queryResultBitmaps,
                 queryResultEditDistances, query, internalRecordId, smallestVirtualListVectorId, 0)) {
            bool  validForwardList = false;
            const ForwardList *fl = this->indexData->forwardIndex->getForwardList(internalRecordId, 
                                              validForwardList);
            if (validForwardList) {
                // add this record to topK results if its score is good enough
                QueryResult queryResult;
                queryResult.internalRecordId = internalRecordId;
                float recordScore = 
                    fl->getForwardListSortableAttributeScore(this->indexData->forwardIndex->getSchema(), query->getSortableAttributeId());
                //unsigned sumOfEditDistances = std::accumulate(queryResultEditDistances.begin(), 
                //                          queryResultEditDistances.end(), 0);
                queryResult.score = recordScore;
                //    query->getRanker()->computeResultScoreUsingAttributeScore(query, recordScore, 
                //                                  sumOfEditDistances, 
                //                                  queryTermsLength);
                queryResult.matchingKeywords = queryResultMatchingKeywords;
                queryResult.attributeBitmaps = queryResultBitmaps;
                queryResult.editDistances = queryResultEditDistances;
                queryResultsInternal->insertResult(queryResult);
            }
        }
    }

    queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    
    delete heapItem;
    
    return queryResultsInternal->getNumberOfResults();
}

int IndexSearcherInternal::searchMapQuery(const Query *query, QueryResults* queryResults)
{
    QueryResultsInternal *queryResultsInternal = dynamic_cast<QueryResultsInternal *>(queryResults);

    const std::vector<Term* > *queryTerms = query->getQueryTerms();

    //Empty Query case
    if (queryTerms->size() == 0)
    {
        return 0;
    }

    //build mario's SearcherTerm for each query term
    //timespec ts1;
    //timespec ts2;
    //clock_gettime(CLOCK_REALTIME, &ts1);
    vector<MapSearcherTerm> mapSearcherTermVector;
    for(unsigned i = 0; i < queryTerms->size(); i++)
    {
        MapSearcherTerm mapSearcherTerm;
        // TODO
        // after the bug in active node is fixed, see if we should use LeafNodeSetIterator/ActiveNodeSetIterator for PREFIX/COMPLETE terms.
        // see TermVirtualList::TermVirtualList() in src/operation/TermVirtualList.cpp
        PrefixActiveNodeSet *prefixActiveNodeSet = computeActiveNodeSet(queryTerms->at(i));
        for (ActiveNodeSetIterator iter(prefixActiveNodeSet, queryTerms->at(i)->getThreshold()); !iter.isDone(); iter.next())
        {
            TrieNodePointer trieNode;
            unsigned distance;
            iter.getItem(trieNode, distance);
            ExpansionStructure expansion(trieNode->getMinId(), trieNode->getMaxId(), (unsigned char)distance, trieNode);
            //expansion.termPtr = queryTerms->at(i);
            vector<CharType> str;
            ts_shared_ptr<TrieRootNodeAndFreeList > rv;
            this->indexData->trie->getTrieRootNode_ReadView(rv);
            this->indexData->trie->getPrefixString(rv->root, trieNode, str);
            mapSearcherTerm.expansionStructureVector.push_back(expansion);
        }

        // Similar to the part in TermVirtualList.cpp destructor, which is used in text only index.
        // If it's in the cache, set the busyBit off so that it can be freed in the future.
        // if it's not in the cache, we can delete it right away.
        if (prefixActiveNodeSet->isResultsCached() == true)
        {
            prefixActiveNodeSet->busyBit->setFree();
        }
        else
        { 
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
    if (values.size()==3)
    {
        Point p;
        p.x = values[0];
        p.y = values[1];
        searchRange = new Circle(p, values[2]);
    }
    else
    {
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

    return queryResultsInternal->getNumberOfResults();
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
        const int nextK, QueryResults* queryResults)
{
    // Empty Query case
    if (query->getQueryTerms()->size() == 0) {
        return 0;
    }

    //TODO: Corner case: check that queryResults was created using the query.
    QueryResultsInternal *queryResultsInternal = dynamic_cast<QueryResultsInternal *>(queryResults);

    // if queryResults has enough results to answer this query return those results
    if(queryResultsInternal->getNumberOfResults() >= (unsigned)offset+nextK) {
        return nextK;
    }

    // set nextK to compute
    queryResultsInternal->setNextK(offset + nextK - queryResultsInternal->getNumberOfResults());

    /********Step 1**********/
    // Cache lookup, assume a query with the first k terms found in the cache
    queryResults->addMessage("Conjunction Cache Get");
    ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry;
    int cacheResponse = this->cacheManager->getCachedConjunctionResult(query->getQueryTerms(), 
                                       conjunctionCacheResultsEntry);

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
            QueryResult queryResult;
            queryResult.internalRecordId = internalRecordId;
        
            bool forwardListValid = false;
            this->indexData->forwardIndex->getForwardList(internalRecordId, forwardListValid);
            if (forwardListValid) {
                queryResult.score = query->getRanker()->computeOverallRecordScore(query, queryResultTermScores);
                queryResult.matchingKeywords = queryResultMatchingKeywords;
                queryResult.attributeBitmaps = queryResultAttributeBitmaps;
                queryResult.editDistances = queryResultEditDistances;
                queryResultsInternal->insertResult(queryResult);
            }
        }
        // Give back cache busy bit.
        conjunctionCacheResultsEntry->busyBit->setFree();
        if (conjunctionCacheResultsEntry->isCached == 0)
            delete conjunctionCacheResultsEntry;
        queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
    }
    else {
        // fill the visited list with the current queryResults
        std::set<unsigned> visitedList;
        queryResultsInternal->fillVisitedList(visitedList);
    
        ProfilerStart("CPUProfile");
        this->computeTermVirtualList(queryResultsInternal);
        ProfilerStop();
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
                        QueryResult queryResult;
                        queryResult.internalRecordId = internalRecordId;
                
                        queryResult.score = query->getRanker()->computeOverallRecordScore(query, queryResultTermScores);
                        queryResult.matchingKeywords = queryResultMatchingKeywords;
                        queryResult.attributeBitmaps = queryResultAttributeBitmaps;
                        queryResult.editDistances = queryResultEditDistances;
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
            for (unsigned int i = 0; i < virtualListVector->size(); ++i)
			{
			   float score;

			   if(!virtualListVector->at(i)->getMaxScore(score))
			   {
				   stop = true;
				   break;
			   }
			   maxScoreForUnvisitedRecords =
					   query->getRanker()->aggregateBoostedTermRuntimeScore(maxScoreForUnvisitedRecords,
							   query->getQueryTerms()->at(i)->getBoost(), score) ;
			}
            // round robin: go through all the virtual lists
            for (unsigned int i = 0; i < virtualListVector->size(); ++i) {
                // Step 2
                // get one element from one virtual list
                // if the term virtual list has no more item, stop
                if(!virtualListVector->at(i)->getNext(heapItem)) {
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
                    std::vector<CharType> temp;
                    this->indexData->trie->getPrefixString(this->indexReadToken.trieRootNodeSharedPtr->root, 
                                   heapItem->trieNode, temp);
                    string str;
                    charTypeVectorToUtf8String(temp, str);
                    queryResultMatchingKeywords.at(i) = str;
                //}
            
                queryResultEditDistances.at(i) = heapItem->ed;
                queryResultTermScores.at(i) = heapItem->termRecordRuntimeScore;
                queryResultAttributeBitmaps.at(i) = heapItem->attributeBitMap;
                
                // Step 3
                // Do random access on the other TermVirtualLists
                if(randomAccess(virtualListVector, queryResultTermScores, 
                        queryResultMatchingKeywords, queryResultAttributeBitmaps, queryResultEditDistances,
                        query, internalRecordId, i, 0))    {
                    bool validForwardList;
                    this->indexData->forwardIndex->getForwardList(internalRecordId, validForwardList);
                    if (validForwardList) {
                        // add this record to topK results if its score is good enough
                        QueryResult queryResult;
                        queryResult.internalRecordId = internalRecordId;
                        queryResult.score = query->getRanker()->computeOverallRecordScore(query,
                                                         queryResultTermScores);
                        queryResult.matchingKeywords = queryResultMatchingKeywords;
                        queryResult.attributeBitmaps = queryResultAttributeBitmaps;
                        queryResult.editDistances = queryResultEditDistances;
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
    
        queryResults->addMessage("Fagin's loop end");
        queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
        queryResults->addMessage("Finalised the results.");
    
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
    
        delete heapItem;
    }
    
    return queryResultsInternal->getNumberOfResults() - offset;
}

int IndexSearcherInternal::search(const Query *query, QueryResults* queryResults, const int offset, const int nextK)
{
    int returnValue = -1;

    if (this->indexData->isCommited() == false)
        return returnValue;
    
    if (query->getQueryType() == srch2::instantsearch::TopKQuery) {
        this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
        returnValue = this->searchTopKQuery(query, offset, nextK, queryResults);
        this->indexData->rwMutexForIdReassign->unlockRead();
    }
    else if(query->getQueryType() == srch2::instantsearch::GetAllResultsQuery) {
        this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
        returnValue = this->searchGetAllResultsQuery(query, queryResults);
        this->indexData->rwMutexForIdReassign->unlockRead();
    }
    //queryResults->printResult();
    return returnValue;
}

// find top-k answer. returns the number of records found
int IndexSearcherInternal::search(const Query *query, QueryResults* queryResults, const int topK)
{
    return search(query, queryResults, 0, topK);
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
	QueryResultsInternal *queryResultsInternal = dynamic_cast<QueryResultsInternal *>(queryResults);
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
	  QueryResultsInternal *queryResultsInternal = dynamic_cast<QueryResultsInternal *>(queryResults);
	      this->indexer->rwMutexForWriter->lockRead(); // need to lock the mutex
	      this->indexData->rwMutexForIdReassign->lockRead(); // need to lock the mutex
	      this->indexData->quadTree->rangeQueryWithoutKeywordInformation(queryCircle,queryResultsInternal);
	      queryResultsInternal->finalizeResults(this->indexData->forwardIndex);
	      this->indexData->rwMutexForIdReassign->unlockRead();
	      this->indexer->rwMutexForWriter->unlockRead();
  }

void IndexSearcherInternal::computeTermVirtualList(QueryResultsInternal *queryResults) const
{
    const Query *query = queryResults->getQuery();
    const vector<Term* > *queryTerms = query->getQueryTerms();
    if (query->getQueryType() != MapQuery) {
        for (vector<Term*>::const_iterator vectorIterator = queryTerms->begin();
             vectorIterator != queryTerms->end();
             vectorIterator++ ) {
            // compute the active nodes for this term
            Term *term = *vectorIterator;
            std::stringstream str;

            str << "ActiveNodes compute:"<<*term->getKeyword();
            queryResults->addMessage(str.str().c_str());

            PrefixActiveNodeSet *termActiveNodeSet = this->computeActiveNodeSet(term);

            queryResults->addMessage("ActiveNode computed");

            // compute the virtual list for this term
            float prefixMatchPenalty = query->getPrefixMatchPenalty();
            TermVirtualList *termVirtualList = new TermVirtualList(this->indexData->invertedIndex, termActiveNodeSet, 
                                       term, prefixMatchPenalty);
            queryResults->addMessage("TermVList computed");

            ///check if termActiveNodeSet is cached, if not delete it to prevent memory leaks.
            //if(termActiveNodeSet->isResultsCached() == false)
            //    delete termActiveNodeSet;

            queryResults->virtualListVector->push_back(termVirtualList);
        }
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

    if ( (initialPrefixActiveNodeSet == NULL) || (cacheResponse == 0)) // NO CacheHit,  response = 0
    {
        //std::cout << "|NO Cache|" << std::endl;;
        // No prefix has a cached TermActiveNode Set. Create one for the empty std::string "".
        initialPrefixActiveNodeSet = new PrefixActiveNodeSet(this->indexReadToken.trieRootNodeSharedPtr, term->getThreshold());
        initialPrefixActiveNodeSet->busyBit->setBusy();
    }
    cachedPrefixLength = initialPrefixActiveNodeSet->getPrefixLength();

    /// 2. do the incremental computation. BusyBit of prefixActiveNodeSet is busy.
    PrefixActiveNodeSet *prefixActiveNodeSet = initialPrefixActiveNodeSet;

    for (unsigned iter = cachedPrefixLength; iter < keywordLength; iter++)
    {
        CharType additionalCharacter = charTypeKeyword[iter]; // get the appended character

        PrefixActiveNodeSet *newPrefixActiveNodeSet = prefixActiveNodeSet->computeActiveNodeSetIncrementally(additionalCharacter);
        newPrefixActiveNodeSet->busyBit->setBusy();

        // If the last result was not put into the cache successfully (e.g., due to
        // limited cache size), we can safely delete it now
        if (prefixActiveNodeSet->isResultsCached() == false)
        {
            //if (prefixActiveNodeSet->busyBit->isBusy())
                delete prefixActiveNodeSet;
        }
        prefixActiveNodeSet = newPrefixActiveNodeSet;

        //std::cout << "Cache Set:" << *(prefixActiveNodeSet->getPrefix()) << std::endl;

        if (iter >= 2 && (cacheResponse != -1)) // Cache not busy and keywordLength is at least 2.
        {
            cacheResponse = this->cacheManager->setPrefixActiveNodeSet(prefixActiveNodeSet);
        }
    }
    // Possible memory leak due to last prefixActiveNodeSet not being cached. This is checked for
    // and deleted by the caller "QueryResultsInternal()"
    return prefixActiveNodeSet;
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
    
        PrefixActiveNodeSet *prefixActiveNodeSet;
        virtualListVector->at(j)->getPrefixActiveNodeSet(prefixActiveNodeSet);
        unsigned termSearchableAttributeIdToFilterTermHits = 
            virtualListVector->at(j)->getTermSearchableAttributeIdToFilterTermHits();
        
        bool found = false;

        // assume the iterator returns the ActiveNodes in the increasing order based on edit distance
        for (ActiveNodeSetIterator iter(prefixActiveNodeSet, queryTerms->at(j)->getThreshold());
                !iter.isDone(); iter.next()) {
            const TrieNode *trieNode;
            unsigned distance;
            iter.getItem(trieNode, distance);
        
            unsigned minId = trieNode->getMinId();
            unsigned maxId = trieNode->getMaxId();
            if (virtualListVector->at(j)->getTermType() == srch2::instantsearch::COMPLETE) {
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
                                      query->getPrefixMatchPenalty()); 
                    found = true;
                    break;
                }
            }
        //}

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


