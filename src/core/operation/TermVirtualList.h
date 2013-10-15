
// $Id: TermVirtualList.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#ifndef __TERMVIRTUALLIST_H__
#define __TERMVIRTUALLIST_H__

#include "ActiveNode.h"
//#include "index/InvertedIndex.h"
#include <instantsearch/Term.h>
//#include <instantsearch/Query.h>
#include <instantsearch/Ranker.h>

#include <string>
#include <vector>
#include <queue>
#include <set>
#include "util/cowvector/cowvector.h"
#include "util/BitSet.h"
#include "util/RecordIdSetIterator.h"

using std::vector;
using std::queue;
using std::pair;
using std::set;
using std::string;
#define TERM_COUNT_THRESHOLD 350

namespace srch2
{
namespace instantsearch
{
class InvertedIndex;
typedef const TrieNode* TrieNodePointer;

//TODO check the difference with HeapItem
struct HeapItemForIndexSearcher {
    unsigned recordId;
    float termRecordRuntimeScore;
    unsigned attributeBitMap;
    TrieNodePointer trieNode;
    unsigned ed;
    unsigned positionIndexOffset;
};

struct HeapItem {
    //TODO (OPT) Use string and ed over each TermVirtualList rather than each HeapItem
    unsigned invertedListId;
    unsigned attributeBitMap;           //only used for attribute based query
    unsigned cursorVectorPosition;
    unsigned recordId; //invertedListTop
    float termRecordRuntimeScore;
    unsigned positionIndexOffset;
    TrieNodePointer trieNode;
    unsigned ed;
    bool isPrefixMatch;

    HeapItem() {
        this->invertedListId = 0;
        this->cursorVectorPosition = 0;
        this->recordId = 0;
        this->attributeBitMap = 0;
        this->termRecordRuntimeScore = 0;
        this->positionIndexOffset = 0;
        this->trieNode = NULL;
        this->ed = 0;
        this->isPrefixMatch = false;
    }
    HeapItem(unsigned invertedListId,
             unsigned cursorVectorPosition,
             unsigned recordId,
             unsigned attributeBitMap,
             float termRecordRuntimeScore,
             unsigned positionIndexOffset,
             TrieNodePointer trieNode,
             unsigned ed,
             bool isPrefixMatch) {
        this->invertedListId = invertedListId;
        this->cursorVectorPosition = cursorVectorPosition;
        this->recordId = recordId;
        this->attributeBitMap = attributeBitMap;
        this->termRecordRuntimeScore = termRecordRuntimeScore;
        this->positionIndexOffset = positionIndexOffset;
        this->trieNode = trieNode;
        this->ed = ed;
        this->isPrefixMatch = isPrefixMatch;
    }
    ~HeapItem() {
        trieNode = NULL;
    }
};

class TermVirtualList
{
public:
    struct HeapItemCmp {
        unsigned termLength; // length of the query term

        HeapItemCmp() {
        }

        // this operator should be consistent with two others in InvertedIndex.h and QueryResultsInternal.h
        bool operator() (const HeapItem *lhs, const HeapItem *rhs) const {
            return DefaultTopKRanker::compareRecordsLessThan(lhs->termRecordRuntimeScore, lhs->recordId,
                    rhs->termRecordRuntimeScore, rhs->recordId);

        }
    };

    // TODO: the default value of prefixMatchPenality is 0.95.  This constant is used
    // in 3 places: this function, BimaleServeConf.cpp, and Query.cpp.
    // Unify them.
    TermVirtualList(const InvertedIndex* invertedIndex, PrefixActiveNodeSet *prefixActiveNodeSet,
                    Term *term, float prefixMatchPenalty = 0.95 , bool shouldIterateToLeafNodes = true);
    void initialiseTermVirtualListElement(TrieNodePointer prefixNode, TrieNodePointer leafNode, unsigned distance);
    // check bound-distance depth from trieNode and initialize TermVirtualListElement when it's a leaf
    void depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned distance, unsigned bound);
    void depthInitializeBitSet(const TrieNode* trieNode, unsigned distance, unsigned bound);
    bool getNext(HeapItemForIndexSearcher *heapItem);
    void getPrefixActiveNodeSet(PrefixActiveNodeSet* &prefixActiveNodeSet);
    void setCursors(vector<unsigned> *invertedListCursors);
    void getCursors(vector<unsigned>* &invertedListCursors);
    virtual ~TermVirtualList();

    inline srch2::instantsearch::TermType getTermType() const {
        return term->getTermType();
    }
    bool getMaxScore(float & score);
    unsigned getVirtualListTotalLength();
    /*unsigned getVirtualListTotalLength() {
        unsigned totalLen = 0;
        for (unsigned i=0; i<itemsHeap.size(); i++)
        {
            unsigned invId = itemsHeap[i]->invertedListId;
            unsigned itemLen = invertedIndex->getInvertedListSize_ReadView(invId);
            totalLen += itemLen;
        }
        return totalLen;
    }*/

    inline unsigned getTermSearchableAttributeIdToFilterTermHits() const {
        return this->term->getAttributeToFilterTermHits();
    }

    void print_test() const;

    // if the similar terms are too many, we will merge them in this bitset
    BitSet bitSet;
    // the current recordId, initial value is -1
    int currentRecordID;
    // The Iterator of bitset
    RecordIdSetIterator* bitSetIter;
    // current inverted list Readview
    shared_ptr<vectorview<unsigned> > invertedListReadView;
    //int numberOfLeafNodes;
    //int totalInveretListLength ;
    // a flag indicating whether we need to use a bitset
    bool usingBitset;
    // the number of records in the bitset, which is the total number of records in the data set
    int bitSetSize;

    //
    bool isTermVirtualListDisabled;
private:

    PrefixActiveNodeSet *prefixActiveNodeSet;
    const InvertedIndex *invertedIndex;
    vector<HeapItem* > itemsHeap;
    Term *term;
    float prefixMatchPenalty;
    unsigned numberOfItemsInPartialHeap;
    unsigned currentMaxEditDistanceOnHeap;

    /**
     * Vector of cursors.
     * Cursor always points to next element in invertedList.
     *
     * Enables the functions getCursors and setCursors for Caching purpose
     */
    // a vector to keep all the inverted list readviews in current term virtual list
    vector<shared_ptr<vectorview<unsigned> > > invertedListReadViewVector;
    vector<unsigned> cursorVector;
    //int addInvertedList(const InvertedList& invertedList);

    bool _addItemsToPartialHeap();
};

}
}

#endif //__TERMVIRTUALLIST_H__
