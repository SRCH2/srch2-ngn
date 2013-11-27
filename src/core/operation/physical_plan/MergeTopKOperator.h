
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

#ifndef __WRAPPER_MERGETOPKOPERATOR_H__
#define __WRAPPER_MERGETOPKOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


class PhysicalPlanRecordItemComparator
{
public:

  // descending order
  bool operator() (const PhysicalPlanRecordItem*  lhs, const PhysicalPlanRecordItem*  rhs) const
  {
      float leftRecordScore, rightRecordScore;
      unsigned leftRecordId  = lhs->getRecordId();
      unsigned rightRecordId = rhs->getRecordId();
		float _leftRecordScore = lhs->getRecordRuntimeScore();
		float _rightRecordScore = rhs->getRecordRuntimeScore();
		return DefaultTopKRanker::compareRecordsGreaterThan(_leftRecordScore,  leftRecordId,
		                                    _rightRecordScore, rightRecordId);
  }
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

	QueryEvaluatorInternal * queryEvaluator;

	/* this vector always contains the next record coming out of children
	* this means each record first goes to this vector and then it can be used
	* by the operator.
	* getNextRecordOfChild(unsigned childOffset, parameters) returns the record
	* and makes sure to populate the vector again from the child operator.
	*/
	vector<PhysicalPlanRecordItem *> nextItemsFromChildren;

	/*
	 * This vector keeps the candidates that we have visited so far, and might become
	 * top results in the next calls of getNext
	 */
	vector<PhysicalPlanRecordItem *> candidatesList;

	/*
	 * This vector keeps all the records that have been visited so far (including
	 * the returned ones and the candidates and even those records which are not verified)
	 */
	vector<unsigned> visitedRecords; // TODO :  THIS MIGHT BE A BOTTLENECK, maybe we should change it to hash table ?


	/*
	 * This variable is true unless one of the children lists become empty
	 */
	bool listsHaveMoreRecordsInThem;

	PhysicalPlanRecordItem * getNextRecordOfChild(unsigned childOffset , const PhysicalPlanExecutionParameters & params);

	/*
	 * This function does the first call to getNext of all the children and puts the results in
	 * the nextItemsFromChildren vector. This vector is used later by getNextRecordOfChild(...)
	 */
	void initializeNextItemsFromChildren(PhysicalPlanExecutionParameters & params);


	unsigned childRoundRobinOffset;
	unsigned getNextChildForSequentialAccess();

	bool verifyRecordWithChildren(PhysicalPlanRecordItem * recordItem, unsigned childOffset ,
						std::vector<float> & runTimeTermRecordScores,
						std::vector<float> & staticTermRecordScores,
						std::vector<TrieNodePointer> & termRecordMatchingKeywords,
						std::vector<unsigned> & attributeBitmaps,
						std::vector<unsigned> & prefixEditDistances,
						std::vector<unsigned> & positionIndexOffsets,
						const PhysicalPlanExecutionParameters & params);

	bool getMaximumScoreOfUnvisitedRecords(float & score);

	float computeAggregatedRuntimeScoreForAnd(std::vector<float> runTimeTermRecordScores);

	MergeTopKOperator() ;
	void insertResult(PhysicalPlanRecordItem * recordItem);
	bool hasTopK(const float maxScoreForUnvisitedRecords);
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

}
}

#endif //__WRAPPER_MERGETOPKOPERATOR_H__
