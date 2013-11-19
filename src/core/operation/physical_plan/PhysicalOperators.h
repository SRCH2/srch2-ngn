
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
#include "operation/CatalogManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


/*
 * This operator is can only appear at the lowest level of the physical plan.
 * This operator acts like an always empty list. for example getNext always returns NULL for in this operator.
 * An example of the case for which this operator can be used is when we decide to build term virtual list only for the shortest list
 * and use other keywords only for forward index verification. In this case all other keywords will be mapped to this operator.
 */
class LowestLevelNullOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~LowestLevelNullOperator();
private:
	LowestLevelNullOperator();
};

/*
 * This operator sorts the input based on ID. Calling open results in fetching all the results from
 * input and sorting them. Sorted results will be buffered and returned later by calling get next.
 */
class SortByIdOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~SortByIdOperator();
private:
	SortByIdOperator() ;
};

/*
 * This operator sorts the input based on ID. Calling open results in fetching all the results from
 * input and sorting them. Sorted results will be buffered and returned later by calling get next.
 */
class SortByScoreOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~SortByScoreOperator();
private:
	SortByScoreOperator() ;
};


/*
 * This operator output the best K results coming from input.
 * The assumption of this operator is that input is sorted based on score.
 * This function is the core of TopK and implements the threshold algorithm.
 */
class MergeTopKOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~MergeTopKOperator();
private:
	MergeTopKOperator() ;
};

/*
 * This operator merges the inputs assuming they are sorted by ID.
 * It moves down on all inputs in parallel and merges the them.
 */
class MergeSortedByIDOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~MergeSortedByIDOperator();
private:
	MergeSortedByIDOperator() ;
};


/*
 * This operator merges the input lists by moving on the shortest one and doing
 * forward index validation.
 */
class MergeByShortestListOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~MergeByShortestListOperator();
private:
	MergeByShortestListOperator() ;
};

/*
 * This operator unions the inputs assuming they are sorted by ID.
 * It moves down on all inputs in parallel and unions the them.
 */
class UnionSortedByIDOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~UnionSortedByIDOperator();
private:
	UnionSortedByIDOperator() ;
};

/*
 * This operator unions the inputs assuming they are sorted by score.
 * It uses a trivial approach for doing the union. It only returns top K results.
 */
class UnionSortedByScoreOperatorTopK : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~UnionSortedByScoreOperatorTopK();
private:
	UnionSortedByScoreOperatorTopK() ;
};


/*
 * This operator unions the inputs assuming they are sorted by score.
 * It uses a trivial approach for doing the union.
 */
class UnionSortedByScoreOperator : public PhysicalPlanNode {
public:
	bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close();
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	unsigned getCostOfOpen() ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	unsigned getCostOfGetNext() ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	unsigned getCostOfClose() ;
	void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	~UnionSortedByScoreOperator();
private:
	UnionSortedByScoreOperator() ;
};

}
}

#endif
