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
#include "index/InvertedIndex.h"
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
    vector<unsigned> attributeIdsList;
    TrieNodePointer trieNode;
    unsigned ed;
    unsigned positionIndexOffset;
};

struct HeapItem {
    //TODO (OPT) Use string and ed over each TermVirtualList rather than each HeapItem
    unsigned invertedListId;
    vector<unsigned> attributeIdsList;           //only used for attribute based query
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
        this->termRecordRuntimeScore = 0;
        this->positionIndexOffset = 0;
        this->trieNode = NULL;
        this->ed = 0;
        this->isPrefixMatch = false;
    }
    HeapItem(unsigned invertedListId,
             unsigned cursorVectorPosition,
             unsigned recordId,
             const vector<unsigned>& attributeIdsList,
             float termRecordRuntimeScore,
             unsigned positionIndexOffset,
             TrieNodePointer trieNode,
             unsigned ed,
             bool isPrefixMatch) {
        this->invertedListId = invertedListId;
        this->cursorVectorPosition = cursorVectorPosition;
        this->recordId = recordId;
        this->attributeIdsList = attributeIdsList;
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
    TermVirtualList(const InvertedIndex* invertedIndex, const ForwardIndex * forwardIndex, PrefixActiveNodeSet *prefixActiveNodeSet,
                    Term *term, float prefixMatchPenalty = 0.95 , float shouldIterateToLeafNodesAndScoreOfTopRecord = -1);

    void initialiseTermVirtualListElement(TrieNodePointer prefixNode, TrieNodePointer leafNode, unsigned distance);
    // check bound-distance depth from trieNode and initialize TermVirtualListElement when it's a leaf
    void depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned editDistance, unsigned panDistance, unsigned bound);
    void depthInitializeBitSet(const TrieNode* trieNode, unsigned editDistance, unsigned distance, unsigned bound);
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

    bool isTermVirtualListDisabled();
    float getScoreOfTopRecordWhenListIsDisabled();

    inline vector<unsigned>& getTermSearchableAttributeIdToFilterTermHits() const {
        return this->term->getAttributesToFilter();
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
    // the maximum score of leaf nodes to be used in topK
    float maxScoreForBitSetCase;

    //
    float topRecordScoreWhenListIsDisabled;
private:

    PrefixActiveNodeSet *prefixActiveNodeSet;
    const InvertedIndex *invertedIndex;
    // the shared pointer to inverted index read view, we don't want to get this variable
    // from inverted index too many times because it has locking and is expensive
    shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    shared_ptr<vectorview<unsigned> > invertedIndexKeywordIdsReadView;
    shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;
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
