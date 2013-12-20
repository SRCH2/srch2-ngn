
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

#ifndef __WRAPPER_PHYSICALOPERATOR_H__
#define __WRAPPER_PHYSICALOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class UnionLowestLevelTermVirtualListOperator;
class UnionLowestLevelTermVirtualListOptimizationOperator;
class UnionLowestLevelSimpleScanOperator;
class UnionLowestLevelSimpleScanOptimizationOperator;
class UnionLowestLevelSuggestionOperator;
class UnionLowestLevelSuggestionOptimizationOperator;
class MergeTopKOperator;
class MergeTopKOptimizationOperator;
class FilterQueryOperator;
class FilterQueryOptimizationOperator;
class PhysicalOperatorFactory;

/*
 * This operator is can only appear at the lowest level of the physical plan.
 * This operator acts like an always empty list. for example getNext always returns NULL for in this operator.
 * An example of the case for which this operator can be used is when we decide to build term virtual list only for the shortest list
 * and use other keywords only for forward index verification. In this case all other keywords will be mapped to this operator.
 */
class RandomAccessVerificationTermOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~RandomAccessVerificationTermOperator();
private:
	RandomAccessVerificationTermOperator();
	QueryEvaluatorInternal * queryEvaluator;
};

class RandomAccessVerificationTermOptimizationOperator : public PhysicalPlanOptimizationNode {
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

class RandomAccessVerificationAndOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~RandomAccessVerificationAndOperator();
private:
	RandomAccessVerificationAndOperator();
};

class RandomAccessVerificationAndOptimizationOperator : public PhysicalPlanOptimizationNode {
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


class RandomAccessVerificationOrOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~RandomAccessVerificationOrOperator();
private:
	RandomAccessVerificationOrOperator();
};

class RandomAccessVerificationOrOptimizationOperator : public PhysicalPlanOptimizationNode {
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


class RandomAccessVerificationNotOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~RandomAccessVerificationNotOperator();
private:
	RandomAccessVerificationNotOperator();
};

class RandomAccessVerificationNotOptimizationOperator : public PhysicalPlanOptimizationNode {
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


/*
 * This operator sorts the input based on ID. Calling open results in fetching all the results from
 * input and sorting them. Sorted results will be buffered and returned later by calling get next.
 */
class SortByIdOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	struct SortByIdRecordMinHeapComparator{
		bool operator()(const PhysicalPlanRecordItem * left , const PhysicalPlanRecordItem * right) const{
			if(left->getRecordId() == right->getRecordId()){
				return (left->getRecordRuntimeScore() < right->getRecordRuntimeScore());
			}else{
				return (left->getRecordId() > right->getRecordId());
			}
		}
	};
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~SortByIdOperator();
private:
	SortByIdOperator() ;
	vector< PhysicalPlanRecordItem * > records;
};

class SortByIdOptimizationOperator : public PhysicalPlanOptimizationNode {
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


/*
 * This operator sorts the input based on ID. Calling open results in fetching all the results from
 * input and sorting them. Sorted results will be buffered and returned later by calling get next.
 */
class SortByScoreOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	struct SortByScoreRecordMaxHeapComparator{
		bool operator()(const PhysicalPlanRecordItem * left , const PhysicalPlanRecordItem * right) const{
			if(left->getRecordRuntimeScore() == right->getRecordRuntimeScore()){
				return (left->getRecordId() > right->getRecordId());
			}else{
				return (left->getRecordRuntimeScore() < right->getRecordRuntimeScore());
			}
		}
	};
	struct SortByScoreRecordMinHeapComparator{
		bool operator()(const PhysicalPlanRecordItem * left , const PhysicalPlanRecordItem * right) const{
			if(left->getRecordRuntimeScore() == right->getRecordRuntimeScore()){
				return (left->getRecordId() < right->getRecordId());
			}else{
				return (left->getRecordRuntimeScore() > right->getRecordRuntimeScore());
			}
		}
	};
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~SortByScoreOperator();
private:
	SortByScoreOperator() ;
	vector< PhysicalPlanRecordItem * > recordsAfterTopK;
	bool isRecordsAfterTopKVectorSorted ;
	vector< PhysicalPlanRecordItem * > topKBestRecords;
};

class SortByScoreOptimizationOperator : public PhysicalPlanOptimizationNode {
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

/*
 * This operator merges the inputs assuming they are sorted by ID.
 * It moves down on all inputs in parallel and merges the them.
 */
class MergeSortedByIDOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~MergeSortedByIDOperator();
private:
	MergeSortedByIDOperator() ;

	/*
	 * This variable is true unless one of the children lists become empty
	 */
	bool listsHaveMoreRecordsInThem;
	vector<unsigned> previousResultsFound;
};

class MergeSortedByIDOptimizationOperator : public PhysicalPlanOptimizationNode {
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

/*
 * This operator merges the input lists by moving on the shortest one and doing
 * forward index validation.
 */
class MergeByShortestListOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	bool verifyRecordWithChildren(PhysicalPlanRecordItem * recordItem ,
						std::vector<float> & runTimeTermRecordScores,
						std::vector<float> & staticTermRecordScores,
						std::vector<TrieNodePointer> & termRecordMatchingKeywords,
						std::vector<unsigned> & attributeBitmaps,
						std::vector<unsigned> & prefixEditDistances,
						std::vector<unsigned> & positionIndexOffsets,
						const PhysicalPlanExecutionParameters & params);
	~MergeByShortestListOperator();
private:
	MergeByShortestListOperator() ;
	unsigned indexOfShortestListChild ;
	bool isShortestListFinished;
	vector<unsigned> previousResultsFound;
};

class MergeByShortestListOptimizationOperator : public PhysicalPlanOptimizationNode {
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
	unsigned getShortestListOffsetInChildren();
};

/*
 * This operator unions the inputs assuming they are sorted by ID.
 * It moves down on all inputs in parallel and unions the them.
 */
class UnionSortedByIDOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionSortedByIDOperator();
private:
	UnionSortedByIDOperator() ;
	bool listsHaveMoreRecordsInThem;
	/* this vector always contains the next record coming out of children
	* this means each record first goes to this vector and then it can be used
	* by the operator.
	* getNextRecordOfChild(unsigned childOffset, parameters) returns the record
	* and makes sure to populate the vector again from the child operator.
	*/
	vector<PhysicalPlanRecordItem *> nextItemsFromChildren;
	vector<unsigned> visitedRecords;
};

class UnionSortedByIDOptimizationOperator : public PhysicalPlanOptimizationNode {
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




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PhysicalOperatorFactory{
public:

	~PhysicalOperatorFactory();


	RandomAccessVerificationTermOperator * createRandomAccessVerificationTermOperator();
	RandomAccessVerificationTermOptimizationOperator * createRandomAccessVerificationTermOptimizationOperator();
	RandomAccessVerificationAndOperator * createRandomAccessVerificationAndOperator();
	RandomAccessVerificationAndOptimizationOperator * createRandomAccessVerificationAndOptimizationOperator();
	RandomAccessVerificationOrOperator * createRandomAccessVerificationOrOperator();
	RandomAccessVerificationOrOptimizationOperator * createRandomAccessVerificationOrOptimizationOperator();
	RandomAccessVerificationNotOperator * createRandomAccessVerificationNotOperator();
	RandomAccessVerificationNotOptimizationOperator * createRandomAccessVerificationNotOptimizationOperator();
	SortByIdOperator * createSortByIdOperator();
	SortByIdOptimizationOperator * createSortByIdOptimizationOperator();
	SortByScoreOperator* createSortByScoreOperator();
	SortByScoreOptimizationOperator* createSortByScoreOptimizationOperator();
	MergeTopKOperator * createMergeTopKOperator();
	MergeTopKOptimizationOperator * createMergeTopKOptimizationOperator();
	MergeSortedByIDOperator * createMergeSortedByIDOperator();
	MergeSortedByIDOptimizationOperator * createMergeSortedByIDOptimizationOperator();
	MergeByShortestListOperator * createMergeByShortestListOperator();
	MergeByShortestListOptimizationOperator * createMergeByShortestListOptimizationOperator();
	UnionSortedByIDOperator * createUnionSortedByIDOperator();
	UnionSortedByIDOptimizationOperator * createUnionSortedByIDOptimizationOperator();
	UnionLowestLevelTermVirtualListOperator * createUnionLowestLevelTermVirtualListOperator();
	UnionLowestLevelTermVirtualListOptimizationOperator * createUnionLowestLevelTermVirtualListOptimizationOperator();
	UnionLowestLevelSimpleScanOperator * createUnionLowestLevelSimpleScanOperator();
	UnionLowestLevelSimpleScanOptimizationOperator * createUnionLowestLevelSimpleScanOptimizationOperator();
	UnionLowestLevelSuggestionOperator * createUnionLowestLevelSuggestionOperator();
	UnionLowestLevelSuggestionOptimizationOperator * createUnionLowestLevelSuggestionOptimizationOperator();
	FilterQueryOperator * createFilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator);
	FilterQueryOptimizationOperator * createFilterQueryOptimizationOperator();


private:
	vector<PhysicalPlanNode *> executionNodes;
	vector<PhysicalPlanOptimizationNode *> optimizationNodes;

};

}
}

#endif
