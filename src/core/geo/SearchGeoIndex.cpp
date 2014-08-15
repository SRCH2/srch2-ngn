//$Id: SearchGeoIndex.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <math.h>

#include <iomanip>
#include "QuadTree.h"
#include "index/ForwardIndex.h"
#include "query/QueryResultsInternal.h"
//#include <instantsearch/Stat.h>

using namespace std;

namespace srch2
{
namespace instantsearch
{



//Do a geo query with a range but without keywords
void QuadTree::rangeQueryWithoutKeywordInformation(const Shape &shape,QueryResultsInternal *queryResultsInternal ) const
{

    rangeQueryWithoutKeywordInformation(queryResultsInternal, shape, root);
}

void QuadTree::rangeQueryWithoutKeywordInformation(QueryResultsInternal *queryResultsInternal, const Shape &shape, QuadNode *node) const
{
    unsigned i;
	if(node->isLeaf) //if node is leaf, its entries are geoElements
    {
		for(i = 0; i < node->numElements; i++)// check its entries one by one
		{
			intptr_t offset = (intptr_t)node->entries[i]; //  get the offset of the element on the global element index
			if(shape.contains(this->geoElementIndex[offset]->point)) //check whether the location of the geoElement is in the shape
			{
			QueryResult * queryResult = queryResultsInternal->getReultsFactory()->impl->createQueryResult();
			queryResult->internalRecordId = this->geoElementIndex[offset]->forwardListID;
			//take the distance to the center point of shape as the floatScore ,get the negative value of distance for sorting
			queryResult->_score.setTypedValue(
					((float)(0-shape.getMinDist2FromLatLong(this->geoElementIndex[offset]->point.x,this->geoElementIndex[offset]->point.y))),ATTRIBUTE_TYPE_FLOAT);//TODO
			queryResultsInternal->insertResult(queryResult);
			}
		}
		return;
	}
	else// else, node is  not a leaf
	{
	    for(i = 0; i < CHILD_NUM; i++)
	    {

	        if(node->entries[i] != NULL)
	        {
	            QuadNode *childNode = node->entries[i];
	            if(shape.intersects(childNode->mbr))
	            {
	                    rangeQueryWithoutKeywordInformation(queryResultsInternal, shape, childNode);
	            }
	        }
	    }
	}
}

bool QuadTree::verify(const ForwardList* forwardList, const SpatialRanker *ranker, unsigned forwardListID, const float prefixMatchPenalty, const vector<MapSearcherTerm> &mapSearcherTermVector, float &overallScore, unsigned termToSkip, vector<unsigned> &selectedExpansions, set<unsigned> &setOfTrueAnswersForPickedTerm, double &timer, ExpansionStructure *skippedExpansion) const
{
    // if there is a skipped term
    // first check if the expansion we find on o-filter indeed presents on the forward list
    if(termToSkip != mapSearcherTermVector.size())
    {
        ASSERT(skippedExpansion!=NULL);
        float score;
        unsigned minId = skippedExpansion->prefix.minId;
        unsigned maxId = skippedExpansion->prefix.maxId;
        unsigned keywordId;
        vector<unsigned> attributeIdsList;
        //stat.startMessage();
        // do the forward list check
        bool fullMatch = forwardList->haveWordInRange(this->forwardIndex->getSchema(),
        		minId, maxId,
        		mapSearcherTermVector[termToSkip].termPtr->getAttributesToFilter(),
        		mapSearcherTermVector[termToSkip].termPtr->getFilterAttrOperation(),
        		keywordId, attributeIdsList, score);
        //stat.endMessage();
        // e.g. we search for "cancer", we find "can" on o-filter, it gives us a result with "candy"
        if(!fullMatch)
            return false;

        bool isPrefixMatch = ( (!skippedExpansion->expansionNodePtr->isTerminalNode()) || (minId != keywordId) );

        bool isPrefixTerm = ( mapSearcherTermVector[termToSkip].termPtr->getTermType() == TERM_TYPE_PREFIX );

        if (isPrefixMatch && !isPrefixTerm)
            return false;

        score = ranker->computeTermRecordRuntimeScore(score, skippedExpansion->editDistance,
                                                      mapSearcherTermVector[termToSkip].termPtr->getKeyword()->size(),
                                                      isPrefixMatch,
                                                      prefixMatchPenalty , mapSearcherTermVector[termToSkip].termPtr->getSimilarityBoost());
        
        overallScore += score;
    }

    setOfTrueAnswersForPickedTerm.insert(forwardListID);

    for(unsigned i = 0; i < mapSearcherTermVector.size(); i++)
    {
        if(i == termToSkip) // don't need to verify the "best" term we picked earlier
            continue;
        bool termResult = false;
        for(unsigned j = 0; j < mapSearcherTermVector[i].expansionStructureVector.size(); j++)
        {

            unsigned minId = mapSearcherTermVector[i].expansionStructureVector[j].prefix.minId;
            unsigned maxId = mapSearcherTermVector[i].expansionStructureVector[j].prefix.maxId;
            float score;
            unsigned keywordId;
            vector<unsigned> attributeIdsList;
            // do the forward list check
            termResult = forwardList->haveWordInRange(this->forwardIndex->getSchema(), minId, maxId,
            		mapSearcherTermVector[i].termPtr->getAttributesToFilter(),
            		mapSearcherTermVector[i].termPtr->getFilterAttrOperation(),
            		keywordId, attributeIdsList, score);

            bool isPrefixMatch = ( (!mapSearcherTermVector[i].expansionStructureVector[j].expansionNodePtr->isTerminalNode()) || (minId != keywordId) );
            bool isPrefixTerm = ( mapSearcherTermVector[i].termPtr->getTermType() == TERM_TYPE_PREFIX );
            
            if (isPrefixMatch && !isPrefixTerm)
                termResult = false;

            if(termResult) // for each term, only one verified expansion is needed
            {//TODO if this expansion gives best score?
                score = ranker->computeTermRecordRuntimeScore(score, mapSearcherTermVector[i].expansionStructureVector[j].editDistance,
                                                              mapSearcherTermVector[i].termPtr->getKeyword()->size(),
                                                              isPrefixMatch,
                                                              prefixMatchPenalty , mapSearcherTermVector[i].termPtr->getSimilarityBoost());
        
                overallScore += score;
                selectedExpansions.push_back(j);
                break;
            }
        }
        if(!termResult) // if none of the expansions of this term passes the verification, then this record is not an answer
            return false;
    }

    return true;

}

// combine all terms' expansion' bitVectors on c-filter to one bitVector to decide which child node we should go to
// we combine expansions' bitVectors under a same term to this term's bitVector, it's OR operation
// then we combine each term's bitVector, it's AND operation
void QuadTree::combineBitVectorsOnCFilter(const vector<MapSearcherTerm> &mapSearcherTermVector, const CFilterMapPtr &cMapPtr, ChildBitSet &childrenBitVector) const
{
    ChildBitSet termBV;
    for(unsigned i = 0; i < mapSearcherTermVector.size(); i++)
    {
        termBV.reset();
        for(unsigned j = 0; j < mapSearcherTermVector[i].expansionStructureVector.size(); j++)
        {
            ChildBitSet* termExpansionBV = cMapPtr->getChildrenBitVector(mapSearcherTermVector[i].expansionStructureVector[j].prefix);
            if(termExpansionBV == NULL)
            {
                termBV.set(); // not present means all 1 vector, and we don't need to consider other expansions
                break;
            }
            termBV |= (*termExpansionBV); // OR operation between expansions of a single term
        }
        childrenBitVector &= termBV; //AND operation between terms
    }
}
/*
 * Jamshid : This function is similar to the merge phase of a merge-sort.
 * It merges o-filter and skip list to create the new skip list.
 */
void QuadTree::createSkipListToChild(QuadNode *node, vector<Prefix> *prefixVector, vector<Prefix> *skippedPrefixesToChild) const
{
    OFilterMapPtr oMapPtr = this->oFiltersOfQuadTree->getOFilterMap(node->oFilterOffset);

    vector<Prefix>::const_iterator vIter = prefixVector->begin();
    map< Prefix, GeoElementList >::const_iterator mIter = oMapPtr->omp->begin();

    while (vIter != prefixVector->end() && mIter != oMapPtr->omp->end())
    {
        if (mIter->first < *vIter || mIter->first == *vIter)
        {
            skippedPrefixesToChild->push_back(mIter->first);
            // Jamshid : If the one on the ofilter is lessthan or equal to the one on the skiplist, we continue skipping items
            // from the skiplist until we see some broader interval on the skiplist.
            while (vIter != prefixVector->end() && mIter->first.isAncestor(*vIter))
                vIter++;
            mIter++;
        }
        else
        {
            skippedPrefixesToChild->push_back(*vIter);
            vIter++;
        }
    }

    while (mIter != oMapPtr->omp->end())
    {
        skippedPrefixesToChild->push_back(mIter->first);
        mIter++;
    }

    while (vIter != prefixVector->end())
    {
        skippedPrefixesToChild->push_back(*vIter);
        vIter++;
    }

}

void QuadTree::rangeQuery(QueryResultsInternal *queryResultsInternal, const Shape &shape, vector<MapSearcherTerm> mapSearcherTermVector, const SpatialRanker *ranker, const float prefixMatchPenalty) const
{
    set<unsigned> setOfTrueAnswersForPickedTerm;
    double timer = 0.0;
    //Stat stat;
    rangeQueryInternal(queryResultsInternal, shape, mapSearcherTermVector, ranker, root, setOfTrueAnswersForPickedTerm, prefixMatchPenalty, timer);
    //cout << "timer : " << timer << endl;
    //stat.printForXiang();
}

void QuadTree::rangeQueryInternal(QueryResultsInternal *queryResultsInternal, const Shape &shape, vector<MapSearcherTerm> mapSearcherTermVector, const SpatialRanker *ranker, QuadNode *node, set<unsigned> &setOfTrueAnswersForPickedTerm, const float prefixMatchPenalty, double &timer) const
{
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->forwardIndex->getForwardListDirectory_ReadView(readView);
    if(node->isLeaf) // if the current quadtree node is a leaf node
    {
        //cout << node->numElements << endl;
        for(unsigned elementCounter = 0; elementCounter < node->numElements; elementCounter++) // we go through every element one by one
        {
            intptr_t offset = (intptr_t)node->entries[elementCounter]; // get the offset of the element on the global element index
            if(shape.contains(this->geoElementIndex[offset]->point)) // do location check
            {
                bool isValidForwardList = false;
                unsigned forwardListID = this->geoElementIndex[offset]->forwardListID;
                const ForwardList *fl = this->forwardIndex->getForwardList(readView, forwardListID, isValidForwardList);
                if (!isValidForwardList)
                    continue;

                unsigned termToSkip = mapSearcherTermVector.size(); // setting to the size of the term vector means don't skip any term here
                                                                    // we plainly verify each term one by one, indicating it's a verification on leaf node
                float keywordScore = 0;
                vector<unsigned> selectedExpansions;
                if( !setOfTrueAnswersForPickedTerm.count(forwardListID) // first check if it's already a result, then do the verification
                    && verify(fl, ranker, forwardListID, prefixMatchPenalty, mapSearcherTermVector, keywordScore, termToSkip, selectedExpansions, setOfTrueAnswersForPickedTerm, timer))
                {
                    // TODO push all ranking related stuff to Ranker.cpp
                    //double physicalDistance = 0.0;
                    float distanceScore = this->getDistanceScore(ranker, shape, this->geoElementIndex[offset]->point.x, this->geoElementIndex[offset]->point.y);
                    float combinedScore = ranker->combineKeywordScoreWithDistanceScore(keywordScore, distanceScore);

                    QueryResult * queryResult = queryResultsInternal->getReultsFactory()->impl->createQueryResult();
                    queryResult->internalRecordId = this->geoElementIndex[offset]->forwardListID;
                    queryResult->_score.setTypedValue(combinedScore,ATTRIBUTE_TYPE_FLOAT);//TODO
                    //queryResult.physicalDistance = Ranker::calculateHaversineDistanceBetweenTwoCoordinates();

                    // set up the matching keywords and editDistances for queryResults
                    for(unsigned i=0; i<mapSearcherTermVector.size(); i++)
                    {
                        ExpansionStructure expan = mapSearcherTermVector[i].expansionStructureVector[selectedExpansions[i]];
                        vector<CharType> temp;
                        string str;
                        // Don't need thread safe for now, since we acquire a lock outside
                        this->trie->getPrefixString_NotThreadSafe(expan.expansionNodePtr, temp);
                        charTypeVectorToUtf8String(temp, str);
                        queryResult->matchingKeywords.push_back(str);
                        queryResult->editDistances.push_back((unsigned)expan.editDistance);
                    }

                    queryResultsInternal->insertResult(queryResult);
                }
            }
        }
        return;
    }
    else // if the current quadtree node is NOT a leaf node
    {
        // first try o-filter

        // TODO Because the number of verifications can be huge,
        //      we need to find the "best" term to search on o-filter and use other terms to do verification,
        //      so that the total number of the verifications can be minimized.
        //      Some failed approaches can be found before v3017.
        shared_ptr<vectorview<ForwardListPtr> > readView;
        this->forwardIndex->getForwardListDirectory_ReadView(readView);
        // pick up the terms one by one to search their expansions on o-filter
        for(unsigned termIndex = 0; termIndex < mapSearcherTermVector.size(); termIndex++)
        {

            vector<ExpansionStructure>::iterator expansionIterator = mapSearcherTermVector[termIndex].expansionStructureVector.begin();
            while(expansionIterator != mapSearcherTermVector[termIndex].expansionStructureVector.end())
            { // go through each expansion of the picked term one by one

                OFilterMapPtr oMapPtr = this->oFiltersOfQuadTree->getOFilterMap(node->oFilterOffset);
                GeoElementList* geoElementListPtr = oMapPtr->getGeoElementList(expansionIterator->prefix);

                if(geoElementListPtr == NULL)
                // if couldn't find the expansion on the o-filter, go to the next one
                    expansionIterator++;
                else
                {// if on o-filter
                    // get the geoElement list pointed by the prefix on o-filter, go through each geoElement on it
                    unsigned geoElementListSize = (*geoElementListPtr).geoElementOffsets.size();
                    for(unsigned j = 0; j < geoElementListSize; j++)
                    {
                        GeoElement* geoElement = this->geoElementIndex[geoElementListPtr->geoElementOffsets[j]];
                        float keywordScore = 0;
                        vector<unsigned> selectedExpansions;

                        bool isValidForwardList= false;
                        unsigned forwardListID = geoElement->forwardListID;
                        const ForwardList *fl = this->forwardIndex->getForwardList(readView, forwardListID, isValidForwardList);
                        if(!isValidForwardList)
                            continue;

                        if(  shape.contains(geoElement->point) // check geo-info and do verification for other terms
                                && !setOfTrueAnswersForPickedTerm.count(forwardListID) // first check if it's already a result, then do the verification
                                && verify(fl, ranker, forwardListID, prefixMatchPenalty, mapSearcherTermVector, keywordScore, termIndex, selectedExpansions, setOfTrueAnswersForPickedTerm, timer, &*expansionIterator)
                            )
                        {
                            // TODO push all ranking related stuff to Ranker.cpp
                            //double physicalDistance = 0.0;
                            float distanceScore = this->getDistanceScore(ranker, shape, geoElement->point.x, geoElement->point.y);
                            float combinedScore = ranker->combineKeywordScoreWithDistanceScore(keywordScore, distanceScore);

                            QueryResult * queryResult = queryResultsInternal->getReultsFactory()->impl->createQueryResult();
                            queryResult->internalRecordId = geoElement->forwardListID;
                            queryResult->_score.setTypedValue(combinedScore,ATTRIBUTE_TYPE_FLOAT);//TODO
                            //queryResult.physicalDistance = Ranker::calculateHaversineDistanceBetweenTwoCoordinates();

                            // set up the matching keyword and editDistance of the picked term for queryResults
                            vector<CharType> temp;
                            // Don't need thread safe for now, since we acquire a lock outside
                            this->trie->getPrefixString_NotThreadSafe(expansionIterator->expansionNodePtr, temp);
                            string str;
                            charTypeVectorToUtf8String(temp, str);
                            queryResult->matchingKeywords.push_back(str);
                            queryResult->editDistances.push_back((unsigned)expansionIterator->editDistance);

                            // set up the matching keywords and editDistances of other terms for queryResults
                            unsigned selectedExpansionsCounter = 0;
                            for(unsigned i=0; i<mapSearcherTermVector.size(); i++)
                            {
                                if(i == termIndex)
                                    continue;
                                ExpansionStructure expan = mapSearcherTermVector[i].expansionStructureVector[selectedExpansions[selectedExpansionsCounter]];
                                // Don't need thread safe for now, since we acquire a lock outside
                                this->trie->getPrefixString_NotThreadSafe(expan.expansionNodePtr, temp);
                                charTypeVectorToUtf8String(temp, str);
                                queryResult->matchingKeywords.push_back(str);
                                queryResult->editDistances.push_back((unsigned)expan.editDistance);
                                selectedExpansionsCounter++;
                            }

                            queryResultsInternal->insertResult(queryResult);
                        }
                    }
                    // remove the expansions that are on o-filter, no matter if they lead to any results or not
                    expansionIterator = mapSearcherTermVector[termIndex].expansionStructureVector.erase(expansionIterator);
                }
            }
            if(mapSearcherTermVector[termIndex].expansionStructureVector.size() == 0) // if there is no leftover expansion for this term, then we have computed all results
                return;
        }

        // then try c-filter
        CFilterMapPtr cMapPtr = this->cFiltersOfQuadTree->getCFilterMap(node->cFilterOffset);

        // combine bitVectors on c-filter to decide which child node we should go to
        ChildBitSet childrenBitVector;
        childrenBitVector.set();
        // TODO suggested by Jamshid, maybe we can do pruning for expansions here
        combineBitVectorsOnCFilter(mapSearcherTermVector, cMapPtr, childrenBitVector);
        if(!childrenBitVector.none()) // some bits are 1
        {
            for(unsigned counter = 0; counter < CHILD_NUM; counter++)
            {
                // Search only those nodes, who had their bit turned on
                if(node->entries[counter] != NULL && childrenBitVector.test(counter))
                {
                    QuadNode *childNode = node->entries[counter];
                    if (shape.intersects(childNode->mbr))
                    {
                        rangeQueryInternal(queryResultsInternal, shape, mapSearcherTermVector, ranker, childNode, setOfTrueAnswersForPickedTerm, prefixMatchPenalty, timer);
                    }
                }
            }
        }
        //else, none bits are 1
        // we do nothing, cause no child has all prefixes present

    }
}

double QuadTree::getMinDist2UpperBound(const Shape &shape) const
{
    double searchRadius2 = shape.getSearchRadius2();
    return max( searchRadius2 , MIN_SEARCH_RANGE_SQUARE);
}

double QuadTree::getDistanceScore( const SpatialRanker *ranker, const Shape &shape, const double resultLat, const double resultLng) const
{
    // calculate the score
    double minDist2UpperBound = this->getMinDist2UpperBound(shape);
    double resultMinDist2 = shape.getMinDist2FromLatLong(resultLat, resultLng);
    double distanceRatio = ranker->getDistanceRatio(minDist2UpperBound, resultMinDist2);
    return max( distanceRatio * distanceRatio, MIN_DISTANCE_SCORE );
}

}}
