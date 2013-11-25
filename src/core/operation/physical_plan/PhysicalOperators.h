
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
#include "UnionLowestLevelTermVirtualListOperator.h"
#include "UnionLowestLevelSimpleScanOperator.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

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
};

class RandomAccessVerificationTermOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~SortByIdOperator();
private:
	SortByIdOperator() ;
};

class SortByIdOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~SortByScoreOperator();
private:
	SortByScoreOperator() ;
};

class SortByScoreOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

/*
 * This operator output the best K results coming from input.
 * The assumption of this operator is that input is sorted based on score.
 * This function is the core of TopK and implements the threshold algorithm.
 */
class MergeTopKOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem *
	getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~MergeTopKOperator();
private:
	MergeTopKOperator() ;
};

class MergeTopKOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
};

class MergeSortedByIDOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
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
	~MergeByShortestListOperator();
private:
	MergeByShortestListOperator() ;
};

class MergeByShortestListOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
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
};

class UnionSortedByIDOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

/*
 * This operator unions the inputs assuming they are sorted by score.
 * It uses a trivial approach for doing the union. It only returns top K results.
 */
class UnionSortedByScoreOperatorTopK : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionSortedByScoreOperatorTopK();
private:
	UnionSortedByScoreOperatorTopK() ;
};

class UnionSortedByScoreOptimizationOperatorTopK : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

/*
 * This operator unions the inputs assuming they are sorted by score.
 * It uses a trivial approach for doing the union.
 */
class UnionSortedByScoreOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionSortedByScoreOperator();
private:
	UnionSortedByScoreOperator() ;
};

class UnionSortedByScoreOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

class PhysicalOperatorFactory{
public:

	~PhysicalOperatorFactory(){
		for( vector<PhysicalPlanNode *>::iterator node = executionNodes.begin(); node != executionNodes.end(); ++node ){
			delete (*node);
		}
		for( vector<PhysicalPlanOptimizationNode *>::iterator node = optimizationNodes.begin(); node != optimizationNodes.end(); ++node ){
			delete (*node);
		}
	}


	RandomAccessVerificationTermOperator * createRandomAccessVerificationTermOperator(){
		RandomAccessVerificationTermOperator * op = new RandomAccessVerificationTermOperator();
		executionNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationTermOptimizationOperator * createRandomAccessVerificationTermOptimizationOperator(){
		RandomAccessVerificationTermOptimizationOperator * op = new RandomAccessVerificationTermOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationAndOperator * createRandomAccessVerificationAndOperator(){
		RandomAccessVerificationAndOperator * op = new RandomAccessVerificationAndOperator();
		executionNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationAndOptimizationOperator * createRandomAccessVerificationAndOptimizationOperator(){
		RandomAccessVerificationAndOptimizationOperator * op = new RandomAccessVerificationAndOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationOrOperator * createRandomAccessVerificationOrOperator(){
		RandomAccessVerificationOrOperator * op = new RandomAccessVerificationOrOperator();
		executionNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationOrOptimizationOperator * createRandomAccessVerificationOrOptimizationOperator(){
		RandomAccessVerificationOrOptimizationOperator *  op = new RandomAccessVerificationOrOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationNotOperator * createRandomAccessVerificationNotOperator(){
		RandomAccessVerificationNotOperator *  op = new RandomAccessVerificationNotOperator();
		executionNodes.push_back(op);
		return op;
	}
	RandomAccessVerificationNotOptimizationOperator * createRandomAccessVerificationNotOptimizationOperator(){
		RandomAccessVerificationNotOptimizationOperator *  op = new RandomAccessVerificationNotOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	SortByIdOperator * createSortByIdOperator(){
		SortByIdOperator *  op = new SortByIdOperator();
		executionNodes.push_back(op);
		return op;
	}
	SortByIdOptimizationOperator * createSortByIdOptimizationOperator(){
		SortByIdOptimizationOperator *  op = new SortByIdOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	SortByScoreOperator* createSortByScoreOperator(){
		SortByScoreOperator*  op = new SortByScoreOperator();
		executionNodes.push_back(op);
		return op;
	}
	SortByScoreOptimizationOperator* createSortByScoreOptimizationOperator(){
		SortByScoreOptimizationOperator*  op = new SortByScoreOptimizationOperator;
		optimizationNodes.push_back(op);
		return op;
	}
	MergeTopKOperator * createMergeTopKOperator(){
		MergeTopKOperator *  op =  new MergeTopKOperator();
		executionNodes.push_back(op);
		return op;
	}
	MergeTopKOptimizationOperator * createMergeTopKOptimizationOperator(){
		MergeTopKOptimizationOperator *  op = new MergeTopKOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	MergeSortedByIDOperator * createMergeSortedByIDOperator(){
		MergeSortedByIDOperator *  op = new MergeSortedByIDOperator();
		executionNodes.push_back(op);
		return op;
	}
	MergeSortedByIDOptimizationOperator * createMergeSortedByIDOptimizationOperator(){
		MergeSortedByIDOptimizationOperator *  op = new MergeSortedByIDOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	MergeByShortestListOperator * createMergeByShortestListOperator(){
		MergeByShortestListOperator * op = new MergeByShortestListOperator();
		executionNodes.push_back(op);
		return op;
	}
	MergeByShortestListOptimizationOperator * createMergeByShortestListOptimizationOperator(){
		MergeByShortestListOptimizationOperator *  op = new MergeByShortestListOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	UnionSortedByIDOperator * createUnionSortedByIDOperator(){
		UnionSortedByIDOperator *  op = new UnionSortedByIDOperator();
		executionNodes.push_back(op);
		return op;
	}
	UnionSortedByIDOptimizationOperator * createUnionSortedByIDOptimizationOperator(){
		UnionSortedByIDOptimizationOperator *  op = new UnionSortedByIDOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	UnionSortedByScoreOperatorTopK * createUnionSortedByScoreOperatorTopK(){
		UnionSortedByScoreOperatorTopK *  op = new UnionSortedByScoreOperatorTopK();
		executionNodes.push_back(op);
		return op;
	}
	UnionSortedByScoreOptimizationOperatorTopK * createUnionSortedByScoreOptimizationOperatorTopK(){
		UnionSortedByScoreOptimizationOperatorTopK *  op = new UnionSortedByScoreOptimizationOperatorTopK();
		optimizationNodes.push_back(op);
		return op;
	}
	UnionSortedByScoreOperator * createUnionSortedByScoreOperator(){
		UnionSortedByScoreOperator *  op = new UnionSortedByScoreOperator();
		executionNodes.push_back(op);
		return op;
	}
	UnionSortedByScoreOptimizationOperator * createUnionSortedByScoreOptimizationOperator(){
		UnionSortedByScoreOptimizationOperator *  op =  new UnionSortedByScoreOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	UnionLowestLevelTermVirtualListOperator * createUnionLowestLevelTermVirtualListOperator(){
		UnionLowestLevelTermVirtualListOperator * op = new UnionLowestLevelTermVirtualListOperator();
		executionNodes.push_back(op);
		return op;
	}
	UnionLowestLevelTermVirtualListOptimizationOperator * createUnionLowestLevelTermVirtualListOptimizationOperator(){
		UnionLowestLevelTermVirtualListOptimizationOperator *  op = new UnionLowestLevelTermVirtualListOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}
	UnionLowestLevelSimpleScanOperator * createUnionLowestLevelSimpleScanOperator(){
		UnionLowestLevelSimpleScanOperator * op = new UnionLowestLevelSimpleScanOperator();
		executionNodes.push_back(op);
		return op;
	}
	UnionLowestLevelSimpleScanOptimizationOperator * createUnionLowestLevelSimpleScanOptimizationOperator(){
		UnionLowestLevelSimpleScanOptimizationOperator *  op = new UnionLowestLevelSimpleScanOptimizationOperator();
		optimizationNodes.push_back(op);
		return op;
	}


private:
	vector<PhysicalPlanNode *> executionNodes;
	vector<PhysicalPlanOptimizationNode *> optimizationNodes;

};

}
}

#endif
