
#ifndef __TERMVIRTUALLIST_H__
#define __TERMVIRTUALLIST_H__

#include "ActiveNode.h"
#include "index/InvertedIndex.h"
#include <instantsearch/Term.h>
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
