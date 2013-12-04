
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

#ifndef __WRAPPER_UNIONLOWESTLEVELTERMVIRTUALLISTOPERATOR_H__
#define __WRAPPER_UNIONLOWESTLEVELTERMVIRTUALLISTOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

struct UnionLowestLevelTermVirtualListOperatorHeapItem {
    //TODO (OPT) Use string and ed over each TermVirtualList rather than each HeapItem
    unsigned invertedListId;
    unsigned attributeBitMap;           //only used for attribute based query
    unsigned cursorVectorPosition;
    unsigned recordId; //invertedListTop
    float termRecordRuntimeScore;
    float termRecordStaticScore;
    unsigned positionIndexOffset;
    TrieNodePointer trieNode;
    unsigned ed;
    bool isPrefixMatch;

    UnionLowestLevelTermVirtualListOperatorHeapItem() {
        this->invertedListId = 0;
        this->cursorVectorPosition = 0;
        this->recordId = 0;
        this->attributeBitMap = 0;
        this->termRecordRuntimeScore = 0;
        this->termRecordStaticScore = 0;
        this->positionIndexOffset = 0;
        this->trieNode = NULL;
        this->ed = 0;
        this->isPrefixMatch = false;
    }
    UnionLowestLevelTermVirtualListOperatorHeapItem(unsigned invertedListId,
             unsigned cursorVectorPosition,
             unsigned recordId,
             unsigned attributeBitMap,
             float termRecordRuntimeScore,
             float termRecordStaticScore,
             unsigned positionIndexOffset,
             TrieNodePointer trieNode,
             unsigned ed,
             bool isPrefixMatch) {
        this->invertedListId = invertedListId;
        this->cursorVectorPosition = cursorVectorPosition;
        this->recordId = recordId;
        this->attributeBitMap = attributeBitMap;
        this->termRecordRuntimeScore = termRecordRuntimeScore;
        this->termRecordStaticScore = termRecordStaticScore;
        this->positionIndexOffset = positionIndexOffset;
        this->trieNode = trieNode;
        this->ed = ed;
        this->isPrefixMatch = isPrefixMatch;
    }
    ~UnionLowestLevelTermVirtualListOperatorHeapItem() {
        trieNode = NULL;
    }
};


/*
 * This operator is TermVirtualList implementation as a physical operator.
 */
class UnionLowestLevelTermVirtualListOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:

    struct UnionLowestLevelTermVirtualListOperatorHeapItemCmp {
        unsigned termLength; // length of the query term

        UnionLowestLevelTermVirtualListOperatorHeapItemCmp() {
        }

        // this operator should be consistent with two others in InvertedIndex.h and QueryResultsInternal.h
        bool operator() (const UnionLowestLevelTermVirtualListOperatorHeapItem *lhs, const UnionLowestLevelTermVirtualListOperatorHeapItem *rhs) const {
            return DefaultTopKRanker::compareRecordsLessThan(lhs->termRecordRuntimeScore, lhs->recordId,
                    rhs->termRecordRuntimeScore, rhs->recordId);

        }
    };

	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionLowestLevelTermVirtualListOperator();

private:
	UnionLowestLevelTermVirtualListOperator() ;
    inline srch2::instantsearch::TermType getTermType() const {
        return term->getTermType();
    }
    void initialiseTermVirtualListElement(TrieNodePointer prefixNode,
            TrieNodePointer leafNode, unsigned distance);
    void depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned distance, unsigned bound);
    //Called when this->numberOfItemsInPartialHeap = 0
    bool _addItemsToPartialHeap();

    QueryEvaluatorInternal * queryEvaluator;
    // the current recordId, initial value is -1
    int currentRecordID;
    // current inverted list Readview
    shared_ptr<vectorview<unsigned> > invertedListReadView;
    //int numberOfLeafNodes;
    //int totalInveretListLength ;

    PrefixActiveNodeSet *prefixActiveNodeSet;
    const InvertedIndex *invertedIndex;
    vector<UnionLowestLevelTermVirtualListOperatorHeapItem* > itemsHeap;
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
};

class UnionLowestLevelTermVirtualListOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

}
}

#endif // __WRAPPER_UNIONLOWESTLEVELTERMVIRTUALLISTOPERATOR_H__
