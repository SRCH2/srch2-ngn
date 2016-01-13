

#ifndef __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__
#define __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

/*
 * This operator is a simple scan operator. It starts with a set of activenodes,
 * finds all the leaf nodes based on the term type (prefix or complete),
 * saves all the connected inverted lists, and then in each getNext call returns
 * one record from a list until all lists are iterated completely.
 * The difference between this operator and TVL is that this operator doesn't give
 * any guarantee of order for its output (the output is NOT sorted by score) and
 * it doesn't have the overhead of HEAP that TVL has.
 */
class UnionLowestLevelSimpleScanOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionLowestLevelSimpleScanOperator();
private:
	UnionLowestLevelSimpleScanOperator() ;

	void depthInitializeSimpleScanOperator(
			const TrieNode* trieNode,const TrieNode* prefixNode, unsigned editDistance, unsigned panDistance, unsigned bound);

    //this flag is set to true when the parent is feeding this operator
    // a cache entry and expects a newer one in close()
    bool parentIsCacheEnabled;

	QueryEvaluatorInternal * queryEvaluator;
	shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    shared_ptr<vectorview<unsigned> > invertedIndexKeywordIdsReadView;
	shared_ptr<vectorview<ForwardListPtr> >  forwardIndexDirectoryReadView;
	vector< vectorview<unsigned> * > invertedLists;
	vector< shared_ptr<vectorview<unsigned> > > invertedListsSharedPointers;
	vector< unsigned > invertedListDistances;
	vector< TrieNodePointer > invertedListPrefixes;
	vector< TrieNodePointer > invertedListLeafNodes;
	vector<unsigned> invertedListIDs;
	unsigned invertedListOffset;
	unsigned cursorOnInvertedList;
};

class UnionLowestLevelSimpleScanCacheEntry : public PhysicalOperatorCacheObject {
public:
	unsigned invertedListOffset;
	unsigned cursorOnInvertedList;
	UnionLowestLevelSimpleScanCacheEntry(	unsigned invertedListOffset,
										unsigned cursorOnInvertedList){
		this->invertedListOffset = invertedListOffset;
		this->cursorOnInvertedList = cursorOnInvertedList;
	}

    unsigned getNumberOfBytes() {
    	return sizeof(UnionLowestLevelSimpleScanCacheEntry);
    }

	~UnionLowestLevelSimpleScanCacheEntry(){
	}
};

class UnionLowestLevelSimpleScanOptimizationOperator : public PhysicalPlanOptimizationNode {
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

#endif // __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__
