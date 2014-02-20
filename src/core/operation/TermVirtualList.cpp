
// $Id: TermVirtualList.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include <instantsearch/Ranker.h>
#include "TermVirtualList.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"

using srch2::util::Logger;
namespace srch2
{
namespace instantsearch
{

void TermVirtualList::initialiseTermVirtualListElement(TrieNodePointer prefixNode,
        TrieNodePointer leafNode, unsigned distance)
{
    unsigned invertedListId = leafNode->getInvertedListOffset();
    unsigned invertedListCounter = 0;

    shared_ptr<vectorview<unsigned> > invertedListReadView;
    this->invertedIndex->getInvertedListReadView(invertedListId, invertedListReadView);
    unsigned recordId = invertedListReadView->getElement(invertedListCounter);
    // calculate record offset online
    unsigned recordOffset = this->invertedIndex->getKeywordOffset(recordId, invertedListId);
    ++ invertedListCounter;

    bool foundValidHit = 0;
    float termRecordStaticScore = 0;
    unsigned termAttributeBitmap = 0;
    while (1) {
        if (this->invertedIndex->isValidTermPositionHit(recordId, recordOffset,
                term->getAttributeToFilterTermHits(), termAttributeBitmap,
                termRecordStaticScore) ) {
            foundValidHit = 1;
            break;
        }

        if (invertedListCounter < invertedListReadView->size()) {
            recordId = invertedListReadView->getElement(invertedListCounter);
            // calculate record offset online
            recordOffset = this->invertedIndex->getKeywordOffset(recordId, invertedListId);
            ++invertedListCounter;
        } else {
            break;
        }
    }

    if (foundValidHit == 1) {
        this->numberOfItemsInPartialHeap ++; // increment partialHeap counter

        if (this->numberOfItemsInPartialHeap == 0)
            this->currentMaxEditDistanceOnHeap = distance;

        if (this->getTermType() == srch2::instantsearch::TERM_TYPE_PREFIX) { // prefix term
            bool isPrefixMatch = (prefixNode != leafNode);
            float termRecordRuntimeScore =
                DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                        distance,
                        term->getKeyword()->size(),
                        isPrefixMatch,
                        this->prefixMatchPenalty , term->getSimilarityBoost());
            this->itemsHeap.push_back(new HeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   recordOffset, prefixNode,
                                                   distance, isPrefixMatch));
        } else { // complete term
            float termRecordRuntimeScore =
                DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                        distance,
                        term->getKeyword()->size(),
                        false,
                        this->prefixMatchPenalty , term->getSimilarityBoost());// prefix match == false
            this->itemsHeap.push_back(new HeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   recordOffset, leafNode, distance, false));
        }

        // Cursor points to the next element on InvertedList
        this->cursorVector.push_back(invertedListCounter);
        // keep the inverted list readviews in invertedListVector such that we can safely access them
        this->invertedListReadViewVector.push_back(invertedListReadView);
    }
}

void TermVirtualList::depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned distance, unsigned bound)
{
    if (trieNode->isTerminalNode())
        initialiseTermVirtualListElement(NULL, trieNode, distance);
    if (distance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeTermVirtualListElement(child, distance+1, bound);
        }
    }
}

// this function will depth initialize bitset, which will be used for complete term
void TermVirtualList::depthInitializeBitSet(const TrieNode* trieNode, unsigned distance, unsigned bound)
{
    if (trieNode->isTerminalNode()) {
        unsigned invertedListId = trieNode->getInvertedListOffset();
        this->invertedIndex->getInvertedListReadView(invertedListId, invertedListReadView);
        for (unsigned invertedListCounter = 0; invertedListCounter < invertedListReadView->size(); invertedListCounter++) {
            // set the bit of the record id to be true
            if (!bitSet.getAndSet(invertedListReadView->at(invertedListCounter)))
                bitSetSize++;
        }
        //termCount++;
        //totalInveretListLength  += invertedListReadView->size();
    }
    if (distance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeTermVirtualListElement(child, distance+1, bound);
        }
    }
}

// Iterate over active nodes, fill the vector, and call make_heap on it.
TermVirtualList::TermVirtualList(const InvertedIndex* invertedIndex, PrefixActiveNodeSet *prefixActiveNodeSet,
                                 Term *term, float prefixMatchPenalty, float shouldIterateToLeafNodesAndScoreOfTopRecord )
{
    this->invertedIndex = invertedIndex;
    this->prefixActiveNodeSet = prefixActiveNodeSet;
    this->term = term;
    this->prefixMatchPenalty = prefixMatchPenalty;
    this->numberOfItemsInPartialHeap = 0;
    this->currentMaxEditDistanceOnHeap = 0;
    this->currentRecordID = -1;
    this->usingBitset = false;
    this->bitSetSize = 0;
    this->maxScoreForBitSetCase = 0;
    // this flag indicates whether this TVL is for a tooPopular term or not.
    // If it is a TVL of a too popular term, this TVL is disabled, meaning it should not be used for iteration over
    // heapItems. In this case shouldIterateToLeafNodesAndScoreOfTopRecord is not equal to -1
    this->topRecordScoreWhenListIsDisabled = shouldIterateToLeafNodesAndScoreOfTopRecord;
    if(this->topRecordScoreWhenListIsDisabled != -1){
    	return;
    }
    // check the TermType
    if (this->getTermType() == TERM_TYPE_PREFIX) { //case 1: Term is prefix
        LeafNodeSetIterator iter(prefixActiveNodeSet, term->getThreshold());
        // This is our query-optimization logic. If the total number of leaf nodes for the term is
        // greater than a threshold, we use bitset to do the union/intersection operations.
        if (iter.size() >= TERM_COUNT_THRESHOLD){
        	// NOTE: BitSet is disabled for now since the fact that it must always return max score of leaf nodes
        	// disables early termination and even makes the engine slower.
//            this->usingBitset = true;
        	this->usingBitset = false;
        }
        if (this->usingBitset) {// This term needs bitset
            bitSet.resize(this->invertedIndex->getRecordNumber());
            TrieNodePointer leafNode;
            TrieNodePointer prefixNode;
            unsigned distance;
            //numberOfLeafNodes = 0;
            //totalInveretListLength  = 0;
            this->maxScoreForBitSetCase = 0;
            for (; !iter.isDone(); iter.next()) {
                iter.getItem(prefixNode, leafNode, distance);
                float runTimeScoreOfThisLeafNode = DefaultTopKRanker::computeTermRecordRuntimeScore(leafNode->getMaximumScoreOfLeafNodes(),
    					distance,
    					term->getKeyword()->size(),
    					true,
    					this->prefixMatchPenalty , term->getSimilarityBoost());
                if( runTimeScoreOfThisLeafNode > this->maxScoreForBitSetCase){
                	this->maxScoreForBitSetCase = runTimeScoreOfThisLeafNode;
                }
                unsigned invertedListId = leafNode->getInvertedListOffset();
                this->invertedIndex->getInvertedListReadView(invertedListId, invertedListReadView);
                // loop the inverted list to add it to the Bitset
                for (unsigned invertedListCounter = 0; invertedListCounter < invertedListReadView->size(); invertedListCounter++) {
                    // We compute the union of these bitsets. We increment the number of bits only if the previous bit was 0.
                    if (!bitSet.getAndSet(invertedListReadView->at(invertedListCounter)))
                        bitSetSize ++;
                }
                //termCount++;
                //totalInveretListLength  += invertedListReadView->size();
            }

            bitSetIter = bitSet.iterator();
        } else { // If we don't use a bitset, we use the TA algorithm
            cursorVector.reserve(iter.size());
            invertedListReadViewVector.reserve(iter.size());
            for (; !iter.isDone(); iter.next()) {
                TrieNodePointer leafNode;
                TrieNodePointer prefixNode;
                unsigned distance;
                iter.getItem(prefixNode, leafNode, distance);
                initialiseTermVirtualListElement(prefixNode, leafNode, distance);
            }
        }
    } else { // case 2: Term is complete
        ActiveNodeSetIterator iter(prefixActiveNodeSet, term->getThreshold());
        if (iter.size() >= TERM_COUNT_THRESHOLD){
        	// NOTE: BitSet is disabled for now since the fact that it must always return max score of leaf nodes
        	// disables early termination and even makes the engine slower.
//            this->usingBitset = true;
        	this->usingBitset = false;
        }
        if (this->usingBitset) { // if this term virtual list is merged to a Bitset
            bitSet.resize(this->invertedIndex->getRecordNumber());
            TrieNodePointer trieNode;
            unsigned distance;
            //numberOfLeafNodes = 0;
            //totalInveretListLength  = 0;
            this->maxScoreForBitSetCase = 0;
            for (; !iter.isDone(); iter.next()) {
                iter.getItem(trieNode, distance);
                unsigned prefixDistance = prefixActiveNodeSet->getEditdistanceofPrefix(trieNode);
                float runTimeScoreOfThisLeafNode = DefaultTopKRanker::computeTermRecordRuntimeScore(trieNode->getMaximumScoreOfLeafNodes(),
    					distance,
    					term->getKeyword()->size(),
    					false,
    					this->prefixMatchPenalty , term->getSimilarityBoost());
                if( runTimeScoreOfThisLeafNode > this->maxScoreForBitSetCase){
                	this->maxScoreForBitSetCase = runTimeScoreOfThisLeafNode;
                }
                // loop the distance depth of the trie to add the term invertedlist to Bitset
                if (trieNode->isTerminalNode()) {
                        unsigned invertedListId = trieNode->getInvertedListOffset();
                        this->invertedIndex->getInvertedListReadView(invertedListId, invertedListReadView);
                        for (unsigned invertedListCounter = 0; invertedListCounter < invertedListReadView->size(); invertedListCounter++) {
                            // set the bit of the record id to be true
                            if (!bitSet.getAndSet(invertedListReadView->at(invertedListCounter)))
                                bitSetSize++;
                        }
                        //termCount++;
                        //totalInveretListLength  += invertedListReadView->size();
                    }
                    if (prefixDistance < term->getThreshold()) {
                        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
                            const TrieNode *child = trieNode->getChild(childIterator);
                            depthInitializeTermVirtualListElement(child, prefixDistance+1, term->getThreshold());
                        }
                    }
            }

            //cout << "term count:" << numberOfLeafNodes << endl;
            //cout << "record count:" << totalInveretListLength  << endl;
            bitSetIter = bitSet.iterator();
        } else {
            cursorVector.reserve(iter.size());
            invertedListReadViewVector.reserve(iter.size());
            for (; !iter.isDone(); iter.next()) {
                TrieNodePointer trieNode;
                unsigned distance;
                iter.getItem(trieNode, distance);
                unsigned prefixDistance = prefixActiveNodeSet->getEditdistanceofPrefix(trieNode);

                if (trieNode->isTerminalNode())
                {
                        initialiseTermVirtualListElement(NULL, trieNode, distance);
                }
                if (prefixDistance < term->getThreshold()) {
                    for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
                        const TrieNode *child = trieNode->getChild(childIterator);
                        depthInitializeTermVirtualListElement(child, prefixDistance+1, term->getThreshold());
                    }
                }
            }
        }
    }

    // Make partial heap by calling make_heap from begin() to begin()+"number of items within edit distance threshold"
    make_heap(itemsHeap.begin(), itemsHeap.begin()+numberOfItemsInPartialHeap, TermVirtualList::HeapItemCmp());
}
bool TermVirtualList::isTermVirtualListDisabled(){
	return (this->topRecordScoreWhenListIsDisabled != -1);
}

float TermVirtualList::getScoreOfTopRecordWhenListIsDisabled(){
	ASSERT(this->topRecordScoreWhenListIsDisabled != -1);
	return this->topRecordScoreWhenListIsDisabled;
}

TermVirtualList::~TermVirtualList()
{
    for (vector<HeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        HeapItem *currentItem = *iter;
        if (currentItem != NULL)
            delete currentItem;
    }
    this->itemsHeap.clear();
    this->cursorVector.clear();
    this->invertedListReadViewVector.clear();
    this->term = NULL;
    this->invertedIndex = NULL;

}
//Called when this->numberOfItemsInPartialHeap = 0
bool TermVirtualList::_addItemsToPartialHeap()
{
    bool returnValue = false;
    // If partial heap is empty, increase editDistanceThreshold and add more elements
    for ( vector<HeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        HeapItem *currentItem = *iter;
        if (this->numberOfItemsInPartialHeap == 0) {
            // partialHeap is empty, assign new maxEditDistance and add items to partialHeap
            if (currentItem->ed > this->currentMaxEditDistanceOnHeap) {
                this->currentMaxEditDistanceOnHeap = currentItem->ed;
                this->numberOfItemsInPartialHeap++;
                returnValue =true;
            }
        } else {
            // Edit distance on itemHeap is less than maxEditDistance so far seen
            if (currentItem->ed <= this->currentMaxEditDistanceOnHeap) {
                this->numberOfItemsInPartialHeap++;
                returnValue = true;
            }        //stopping condition: partialheap is not empty and edit distance is greater than maxEditDistance
            else
                break;
        }
    }
    // PartialHeap changed;
    if (returnValue) {
        make_heap(this->itemsHeap.begin(),this->itemsHeap.begin()+numberOfItemsInPartialHeap,
                  TermVirtualList::HeapItemCmp());
    }
    return returnValue;
}

bool TermVirtualList::getMaxScore(float & score)
{
    if (this->usingBitset) {
        score = this->maxScoreForBitSetCase;
        return true;
    } else {
        //If heapVector is empty
        if (this->itemsHeap.size() == 0) {
            return false;
        }
        //If partialHeap is empty
        if (this->numberOfItemsInPartialHeap == 0) {
            if ( this->_addItemsToPartialHeap() == false) {
                return false;
            }
        }
        if (this->numberOfItemsInPartialHeap != 0) {
            HeapItem *currentHeapMax = *(itemsHeap.begin());
            score = currentHeapMax->termRecordRuntimeScore;
            return true;
        } else {
            return false;
        }
    }
}

bool TermVirtualList::getNext(HeapItemForIndexSearcher *returnHeapItem)
{
    if (this->usingBitset) {
        returnHeapItem->recordId = this->bitSetIter->getNextRecordId();
        // When we use the bitset to get the next record for this virtual list, we don't have all the information for this record.
        returnHeapItem->termRecordRuntimeScore = 1.0;
        returnHeapItem->trieNode = 0;
        returnHeapItem->attributeBitMap = 0;
        returnHeapItem->ed = 0;
        returnHeapItem->positionIndexOffset = 0;
        if (returnHeapItem->recordId != RecordIdSetIterator::NO_MORE_RECORDS)
            return true;
        else
            return false;
    } else {
        // If partialHeap is empty
        if (this->numberOfItemsInPartialHeap == 0) {
            if (this->_addItemsToPartialHeap() == false)
                return false;
        }

        if (this->numberOfItemsInPartialHeap != 0) {
            // Elements are there in PartialHeap and pop them out to calling function
            HeapItem *currentHeapMax = *(itemsHeap.begin());
            pop_heap(itemsHeap.begin(), itemsHeap.begin() + this->numberOfItemsInPartialHeap,
                     TermVirtualList::HeapItemCmp());

            returnHeapItem->recordId = currentHeapMax->recordId;
            returnHeapItem->termRecordRuntimeScore = currentHeapMax->termRecordRuntimeScore;
            returnHeapItem->trieNode = currentHeapMax->trieNode;
            returnHeapItem->attributeBitMap = currentHeapMax->attributeBitMap;
            returnHeapItem->ed = currentHeapMax->ed;
            returnHeapItem->positionIndexOffset = currentHeapMax->positionIndexOffset;

            unsigned currentHeapMaxCursor = this->cursorVector[currentHeapMax->cursorVectorPosition];
            unsigned currentHeapMaxInvertetedListId = currentHeapMax->invertedListId;
            const shared_ptr<vectorview<unsigned> > &currentHeapMaxInvertedList = this->invertedListReadViewVector[currentHeapMax->cursorVectorPosition];
            unsigned currentHeapMaxInvertedListSize = currentHeapMaxInvertedList->size();

            bool foundValidHit = 0;

            // Check cursor is less than invertedList Size.
            while (currentHeapMaxCursor < currentHeapMaxInvertedListSize) {
                // InvertedList has more elements. Push invertedListElement at cursor into virtualList.

                unsigned recordId = currentHeapMaxInvertedList->getElement(currentHeapMaxCursor);
                // calculate record offset online
                unsigned recordOffset = this->invertedIndex->getKeywordOffset(recordId, currentHeapMaxInvertetedListId);
                unsigned termAttributeBitmap = 0;
                currentHeapMaxCursor++;

                // check isValidTermPositionHit
                float termRecordStaticScore = 0;
                if (this->invertedIndex->isValidTermPositionHit(recordId, recordOffset,
                        term->getAttributeToFilterTermHits(), termAttributeBitmap,
                        termRecordStaticScore)) {
                    foundValidHit = 1;
                    this->cursorVector[currentHeapMax->cursorVectorPosition] = currentHeapMaxCursor;
                    // Update cursor of popped virtualList in invertedListCursorVector.
                    // Cursor always points to next element in invertedList
                    currentHeapMax->recordId = recordId;
                    currentHeapMax->termRecordRuntimeScore =
                        DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                                currentHeapMax->ed,
                                term->getKeyword()->size(),
                                currentHeapMax->isPrefixMatch,
                                this->prefixMatchPenalty , term->getSimilarityBoost());
                    currentHeapMax->attributeBitMap = termAttributeBitmap;
                    currentHeapMax->positionIndexOffset = recordOffset;
                    push_heap(itemsHeap.begin(), itemsHeap.begin()+this->numberOfItemsInPartialHeap,
                              TermVirtualList::HeapItemCmp());
                    break;
                }
            }

            if (!foundValidHit) {
                //InvertedList cursor end reached and so decrement number of elements in partialHeap
                this->numberOfItemsInPartialHeap--;
                //Delete the head of heap that represents empty converted list
                delete currentHeapMax;
                //TODO OPT Don't erase, accumulate and delete at the end.
                this->itemsHeap.erase(itemsHeap.begin()+this->numberOfItemsInPartialHeap);
            }

            return true;
        } else {
            return false;
        }
    }
}

void TermVirtualList::getPrefixActiveNodeSet(PrefixActiveNodeSet* &prefixActiveNodeSet)
{
    prefixActiveNodeSet = this->prefixActiveNodeSet;
}

void TermVirtualList::setCursors(std::vector<unsigned> *invertedListCursors)
{
    // TODO mar 1
    if (invertedListCursors != NULL) {
        unsigned a = invertedListCursors->size();
        unsigned b = this->cursorVector.size();
        (void)( a == b);

        //ASSERT(invertedListCursors->size() == this->cursorVector.size());
        copy(invertedListCursors->begin(), invertedListCursors->end(), this->cursorVector.begin());
    }
}

void TermVirtualList::getCursors(std::vector<unsigned>* &invertedListCursors)
{
    // TODO mar 1
    //invertedListCursors = new vector<unsigned>();
    if (invertedListCursors != NULL) {
        invertedListCursors->clear();
        invertedListCursors->resize(this->cursorVector.size());
        copy(this->cursorVector.begin(), this->cursorVector.end(), invertedListCursors->begin());
    }
}

void TermVirtualList::print_test() const
{
    Logger::debug("ItemsHeap Size %d", this->itemsHeap.size());
    for (vector<HeapItem* >::const_iterator heapIterator = this->itemsHeap.begin();
            heapIterator != this->itemsHeap.end(); heapIterator++) {
        Logger::debug("InvListPosition:\tRecord: %d\t Score:%.5f", (*heapIterator)->recordId, (*heapIterator)->termRecordRuntimeScore);
    }
}
unsigned TermVirtualList::getVirtualListTotalLength()
{
    if (this->usingBitset) {
        return bitSetSize;
    } else {
        unsigned totalLen = 0;
        for (unsigned i=0; i<itemsHeap.size(); i++) {
            totalLen += this->invertedListReadViewVector[itemsHeap[i]->cursorVectorPosition]->size();
        }
        return totalLen;
    }
}

}
}
