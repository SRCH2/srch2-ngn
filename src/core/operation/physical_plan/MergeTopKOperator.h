
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

#include <boost/unordered_set.hpp>

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



class MergeTopKCacheEntry : public PhysicalOperatorCacheObject {
public:
	vector<PhysicalPlanRecordItem *> candidatesList;
	vector<PhysicalPlanRecordItem *> nextItemsFromChildren;
	boost::unordered_set<unsigned> visitedRecords;
	bool listsHaveMoreRecordsInThem;
	unsigned childRoundRobinOffset;

	MergeTopKCacheEntry(	QueryEvaluatorInternal * queryEvaluator,
			                vector<PhysicalPlanRecordItem *> candidatesList,
							vector<PhysicalPlanRecordItem *> nextItemsFromChildren,
							boost::unordered_set<unsigned> visitedRecords,
							bool listsHaveMoreRecordsInThem,
							unsigned childRoundRobinOffset){
		for(unsigned i = 0; i < candidatesList.size() ; ++i){
			this->candidatesList.push_back(queryEvaluator->getPhysicalPlanRecordItemFactory()->
					cloneForCache(candidatesList.at(i)));
		}

		for(unsigned i = 0; i < nextItemsFromChildren.size() ; ++i){
			if(nextItemsFromChildren.at(i) == NULL){
				this->nextItemsFromChildren.push_back(NULL);
			}else{
				this->nextItemsFromChildren.push_back(queryEvaluator->getPhysicalPlanRecordItemFactory()->
						cloneForCache(nextItemsFromChildren.at(i)));
			}
		}

		this->visitedRecords = visitedRecords;
		this->listsHaveMoreRecordsInThem = listsHaveMoreRecordsInThem;
		this->childRoundRobinOffset = childRoundRobinOffset;
	}

    unsigned getNumberOfBytes() {
    	unsigned numberOfButes = 0;
    	for(unsigned i = 0 ; i < candidatesList.size() ; ++i){
    		numberOfButes += candidatesList.at(i)->getNumberOfBytes();
    	}
    	for(unsigned i = 0 ; i < nextItemsFromChildren.size() ; i++){
    		if(nextItemsFromChildren.at(i) != NULL){
				numberOfButes += nextItemsFromChildren.at(i)->getNumberOfBytes();
    		}
    	}
    	numberOfButes += sizeof(unsigned) * visitedRecords.size() +
    			sizeof(listsHaveMoreRecordsInThem) + sizeof(childRoundRobinOffset);
    	for(unsigned childOffset = 0 ; childOffset < children.size() ; ++childOffset){
    		numberOfButes += children.at(childOffset)->getNumberOfBytes();
    	}
    	return numberOfButes;
    }

	~MergeTopKCacheEntry(){
		for(unsigned i = 0; i < candidatesList.size() ; ++i){
			delete candidatesList.at(i);
		}
		for(unsigned i=0; i < nextItemsFromChildren.size() ; ++i){
			delete nextItemsFromChildren.at(i);
		}
	}
};



/*
 * This operator uses the Threshold Algorithm (Fagin's Algorithm) to find the best record of
 * its subtree. Every time getNext is called threshold algorithm is used and it retrieves
 * records by calling getNext of children to find and return the best remaining record in the subtree.
 * The assumption of this operator is that input is sorted based on score (monotonicity property of Fagin's algorithm)
 * This operator is used in the most popular physical plan for instant search :
 * Example :
 * q = A AND B AND C
 *
 * [MergeTopKOperator]_____ [TVL A]
 *        |
 *        |_____ [TVL B]
 *        |
 *        |_____ [TVL C]
 */
class MergeTopKOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem *
	getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~MergeTopKOperator();
private:

	QueryEvaluatorInternal * queryEvaluator;

	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;

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
	vector<PhysicalPlanRecordItem *> fullCandidatesListForCache;
	/*
	 * This vector keeps all the records that have been visited so far (including
	 * the returned ones and the candidates and even those records which are not verified)
	 */
//	vector<unsigned> visitedRecords; // TODO :  THIS MIGHT BE A BOTTLENECK, maybe we should change it to hash table ?

	boost::unordered_set<unsigned> visitedRecords;

	/*
	 * This variable is true unless one of the children lists become empty
	 */
	bool listsHaveMoreRecordsInThem;

	PhysicalPlanRecordItem * getNextRecordOfChild(unsigned childOffset , const PhysicalPlanExecutionParameters & params);

	/*
	 * This function does the first call to getNext of all the children and puts the results in
	 * the nextItemsFromChildren vector. This vector is used later by getNextRecordOfChild(...)
	 */
	void initializeNextItemsFromChildren(PhysicalPlanExecutionParameters & params , unsigned fromIndex = 0);


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

	MergeTopKOperator() ;
	void insertResult(PhysicalPlanRecordItem * recordItem);
	bool hasTopK(const float maxScoreForUnvisitedRecords);
};

class MergeTopKOptimizationOperator : public PhysicalPlanOptimizationNode {
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

#endif //__WRAPPER_MERGETOPKOPERATOR_H__
