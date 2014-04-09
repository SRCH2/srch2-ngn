
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
 * The following two operators are used for verifying a term/record match using forward index.
 * This operator can only appear at the lowest level of the physical plan.
 * This operator acts like an always empty list. for example getNext always returns NULL for this operator.
 * An example of the case in which this operator can be used is when we decide to merge by shortest list
 * and only one child returns records and the rest of keywords are only used for forward index verification.
 * In this case all other keywords will be mapped to this operator. Please note that for this example, if we
 * set other keywords to TVL, it would still work correctly because verifyByRandomAccess is also implemented for
 * TVL operator, however, TVL has the overhead of iterating on leaf nodes which is not needed for the case of
 * shortest list use.
 * Example :
 * q = A and B and C
 * [sort by score]
 *      |
 * [merge by shortest list]
 *      |_______[R.A.V. on A]
 *      |_______[R.A.V. on B]
 *      |_______[SCAN on C] // assuming C is the most selective keyword
 */
class RandomAccessVerificationTermOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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


/*
 * This operator has the same purpose as RandomAccessVerificationTermOperator
 * (please read the comments of that class as well) but the difference is that this
 * operator maps an AND logical operator. A record is verified by this node only if all children of
 * this node verify it (AND logic). Example :
 * q = A AND NOT (B AND C)
 * [sort by score]
 *       |
 * [merge by shortest list]
 *       |_____ [SCAN A]
 *       |
 *       |_____ [R.A.V.NOT]______ [R.A.V.AND]____[R.A.V. B]
 *                                     |_________[R.A.V. C]
 */
class RandomAccessVerificationAndOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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

/*
 * Please read the comments for RandomAccessVerificationTermOperator
 * and RandomAccessVerificationAndOperator.
 * This operator verifies a record if at least one of the children returns
 * true (OR logic).
 */
class RandomAccessVerificationOrOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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


/*
 * Please read the comments for RandomAccessVerificationTermOperator
 * and RandomAccessVerificationAndOperator.
 * The only physical operator option for NOT is RandomAccessVerificationNotOperator.
 * This node verifies a record if its child (it only has one child) return
 * false (NOT logic).
 */
class RandomAccessVerificationNotOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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
 * NOTE: one tricky optimization is implemented in sort:
 * instead of sorting the results after loading them (O(NlogN)), we only build a heap (O(N)).
 * then, in each getNext call the top of this heap is removed and its heapified again (O(logN)).
 * This approach saves us some computation in topK search type because we don't need all results.
 * sort approach complexity                      heap approach complexity
 *      O(NlogN) + k*O(1)                            O(N) + kO(logN) // second is better for large N and small K
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
	string toString();
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
 * This operator sorts the input based on runtime score. Calling open results in fetching all the results from
 * input and sorting them. Sorted results will be buffered and returned later by calling get next.
 * Please read the comments of SortByIdOperator to understand the optimization used here.
 * There is only a small change in this sort operator. Instead of making the complete heap in OPEN
 * we only compute and keep a sorted vector of top K results. If getNext is called more than K times, then the rest of
 * records are heapified and that trick is used.
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
	string toString();
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
 * And example of a physical plan can be :
 * q = A AND B OR C
 * [sort by score]
 *       |
 * [OR sorted by on ID]
 *       |
 *       |______ [sort by ID]___[SCAN C]
 *       |
 *       |______ [merge sorted by ID]
 *                       |______ [sort by ID]___[SCAN A]
 *                       |
 *                       |______ [sort by ID]___[SCAN B]
 */
class MergeSortedByIDOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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
 * Mostly, the following plan is chosen when there is a prefix in query which is too
 * popular and iterating on leaf nodes becomes too expensive.
 *
 * q = python AND p     // python is selective but p is the prefix of many many words in a big dataset
 * main core plan is :
 *
 * [sort by score]
 *       |
 * [merge by shortest list] _____ [R.A.V. p]
 *       |
 *       |______ [SCAN python]
 *
 */
class MergeByShortestListOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	bool verifyRecordWithChildren(PhysicalPlanRecordItem * recordItem ,
						std::vector<float> & runTimeTermRecordScores,
						std::vector<float> & staticTermRecordScores,
						std::vector<TrieNodePointer> & termRecordMatchingKeywords,
						std::vector<unsigned> & attributeBitmaps,
						std::vector<unsigned> & prefixEditDistances,
						std::vector<unsigned> & positionIndexOffsets,
						std::vector<TermType>& termTypes,
						const PhysicalPlanExecutionParameters & params, unsigned onlyThisChild = -1 );
	~MergeByShortestListOperator();
private:
	MergeByShortestListOperator() ;
	QueryEvaluatorInternal * queryEvaluator;
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	// this variable keeps the index of the shortest list child
	unsigned indexOfShortestListChild ;
	// if the shortest list is exhausted, this boolean is set to true
	// and from that point, getNext always returns false.
	bool isShortestListFinished;
	// If we get a cache entry, first we move on old candidate
	// results and verify them with the new last keyword, this variable
	// keeps the cursor on this list
	unsigned indexOfCandidateListFromCache;
	// candidate results that came from cache and should be verified with
	// the new last keyword
	vector<PhysicalPlanRecordItem *> candidateListFromCache;
	// all candidate lists which are found which will be sent to cache for a
	// later query.
	vector<PhysicalPlanRecordItem *> candidateListForCache;
};

class MergeByShortestListCacheEntry : public PhysicalOperatorCacheObject {
public:
	unsigned indexOfShortestListChild ;
	bool isShortestListFinished;
	vector<PhysicalPlanRecordItem *> candidatesList;
	MergeByShortestListCacheEntry(	QueryEvaluatorInternal * queryEvaluator,
												unsigned indexOfShortestListChild,
												bool isShortestListFinished,
												vector<PhysicalPlanRecordItem *> candidatesList){
		this->indexOfShortestListChild = indexOfShortestListChild;
		this->isShortestListFinished = isShortestListFinished;
		for(unsigned i = 0; i < candidatesList.size() ; ++i){
			this->candidatesList.push_back(queryEvaluator->getPhysicalPlanRecordItemPool()->
					cloneForCache(candidatesList.at(i)));
		}
	}

    unsigned getNumberOfBytes() {
    	unsigned numberOfBytes = sizeof(MergeByShortestListCacheEntry);

    	// candidateList
    	numberOfBytes += candidatesList.capacity() * sizeof(PhysicalPlanRecordItem *);
    	for(unsigned i = 0 ; i < candidatesList.size() ; ++i){
    		numberOfBytes += candidatesList.at(i)->getNumberOfBytes();
    	}

    	for(unsigned childOffset = 0 ; childOffset < children.size() ; ++childOffset){
    		if(children.at(childOffset) != NULL){
				numberOfBytes += children.at(childOffset)->getNumberOfBytes();
    		}
    	}

    	return numberOfBytes;
    }

	~MergeByShortestListCacheEntry(){
		for(unsigned i = 0; i < candidatesList.size() ; ++i){
			delete candidatesList.at(i);
		}
	}
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
 * It moves down on all inputs in parallel and unions them.
 */
class UnionSortedByIDOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
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


class PhraseSearchOperator;
class PhraseSearchOptimizationOperator;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Physical operators (except for post processing operators such as SortByRefiningAttribute and Facet)
 * use a factory model for instantiation to avoid memory leaks. This class is the factory which has one function per
 * class.
 */
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
	PhraseSearchOperator * createPhraseSearchOperator(PhraseInfo * phraseSearchInfo);
	PhraseSearchOptimizationOperator * createPhraseSearchOptimzationOperator();

private:
	vector<PhysicalPlanNode *> executionNodes;
	vector<PhysicalPlanOptimizationNode *> optimizationNodes;

};

}
}

#endif
