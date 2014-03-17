
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

    unsigned getNumberOfBytes(){
    	return sizeof(UnionLowestLevelTermVirtualListOperatorHeapItem);
    }

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
    UnionLowestLevelTermVirtualListOperatorHeapItem(
    		UnionLowestLevelTermVirtualListOperatorHeapItem & oldObj) {
        this->invertedListId = oldObj.invertedListId;
        this->cursorVectorPosition = oldObj.cursorVectorPosition;
        this->recordId = oldObj.recordId;
        this->attributeBitMap = oldObj.attributeBitMap;
        this->termRecordRuntimeScore = oldObj.termRecordRuntimeScore;
        this->termRecordStaticScore = oldObj.termRecordStaticScore;
        this->positionIndexOffset = oldObj.positionIndexOffset;
        this->trieNode = oldObj.trieNode;
        this->ed = oldObj.ed;
        this->isPrefixMatch = oldObj.isPrefixMatch;
    }

    ~UnionLowestLevelTermVirtualListOperatorHeapItem() {
        trieNode = NULL;
    }
};


class UnionLowestLevelTermVirtualListCacheEntry : public PhysicalOperatorCacheObject {
public:
    vector<UnionLowestLevelTermVirtualListOperatorHeapItem* > itemsHeap;
    unsigned numberOfItemsInPartialHeap;
    unsigned currentMaxEditDistanceOnHeap;
    vector<unsigned> cursorVector;


    UnionLowestLevelTermVirtualListCacheEntry(
    		vector<UnionLowestLevelTermVirtualListOperatorHeapItem* > itemsHeap,
    		unsigned numberOfItemsInPartialHeap,
    		unsigned currentMaxEditDistanceOnHeap,
    		vector<unsigned> cursorVector){
    	this->cursorVector = cursorVector;
    	this->currentMaxEditDistanceOnHeap = currentMaxEditDistanceOnHeap;
    	this->numberOfItemsInPartialHeap = numberOfItemsInPartialHeap;
    	for(unsigned i = 0 ; i < itemsHeap.size() ; ++i){
    		this->itemsHeap.push_back(
    				new UnionLowestLevelTermVirtualListOperatorHeapItem(*(itemsHeap.at(i))));
    	}
    }

    unsigned getNumberOfBytes(){
    	unsigned numberOfBytes = sizeof(UnionLowestLevelTermVirtualListCacheEntry);

    	// items heap
    	numberOfBytes += itemsHeap.capacity() * sizeof(UnionLowestLevelTermVirtualListOperatorHeapItem* );
    	for(unsigned itemsOffset = 0 ; itemsOffset < itemsHeap.size(); ++itemsOffset){
    		numberOfBytes += itemsHeap.at(itemsOffset)->getNumberOfBytes();
    	}

    	// cursorVector
    	numberOfBytes += cursorVector.capacity() * sizeof(unsigned);

    	return numberOfBytes;
    }

    ~UnionLowestLevelTermVirtualListCacheEntry(){
    	for(unsigned i = 0 ; i < itemsHeap.size() ; ++i){
    		delete itemsHeap.at(i);
    	}
    }

};

/*
 * This operator is TermVirtualList implementation as a physical operator.
 * Most of this operator is copied from TermVirtualList old implementation.
 */
class UnionLowestLevelTermVirtualListOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:

    struct UnionLowestLevelTermVirtualListOperatorHeapItemCmp {

        UnionLowestLevelTermVirtualListOperatorHeapItemCmp() {};

        // this operator should be consistent with two others in InvertedIndex.h and QueryResultsInternal.h
        bool operator() (const UnionLowestLevelTermVirtualListOperatorHeapItem *lhs, const UnionLowestLevelTermVirtualListOperatorHeapItem *rhs) const {
            return DefaultTopKRanker::compareRecordsLessThan(lhs->termRecordRuntimeScore, lhs->recordId,
                    rhs->termRecordRuntimeScore, rhs->recordId);
        }
    };

	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();

	// this function checks to see if a record (that its id can be found in parameters) is among the
	// results if this subtree. Different operators have different implementations this function.
	// When verification is performed, some information like match prefix is calculated and saved in
	// members of parameters argument, so if this function returns true, we use parameters members to
	// get that information.
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionLowestLevelTermVirtualListOperator();

private:
	UnionLowestLevelTermVirtualListOperator() ;
    inline srch2::instantsearch::TermType getTermType() const {
        return term->getTermType();
    }
    void initialiseTermVirtualListElement(TrieNodePointer prefixNode,
            TrieNodePointer leafNode, unsigned distance);
    void depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned editDistance, unsigned panDistance, unsigned bound);
    //Called when this->numberOfItemsInPartialHeap = 0
    bool _addItemsToPartialHeap();

    //this flag is set to true when the parent is feeding this operator
    // a cache entry and expects a newer one in close()
    bool parentIsCacheEnabled;

    QueryEvaluatorInternal * queryEvaluator;
    // the current recordId, initial value is -1
    int currentRecordID;
    // current inverted list Readview
    shared_ptr<vectorview<unsigned> > invertedListReadView;
    //int numberOfLeafNodes;
    //int totalInveretListLength ;

    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet;
    const InvertedIndex *invertedIndex;
    shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    shared_ptr<vectorview<unsigned> > invertedIndexKeywordIdsReadView;
    shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;
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
