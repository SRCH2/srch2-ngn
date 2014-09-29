
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

#ifndef __WRAPPER_UNIONLOWESTLEVELSUGGESTIONOPERATOR_H__
#define __WRAPPER_UNIONLOWESTLEVELSUGGESTIONOPERATOR_H__

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
 * This operator is a suggestion opertor. As of Dec,23rd,2013 this operator is only used when there is
 * a single keyword in the query which is too popular and we decide to return the results of most likely
 * completions instead of returning the results of the query itself. It's also known as H1 heuristic.
 */
class UnionLowestLevelSuggestionOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	/*
	 * This struct is used with a heap to output records sorted by their score
	 */
	struct SuggestionCursorHeapItem{
		SuggestionCursorHeapItem(){
			this->suggestionIndex = 0;
			this->invertedListCursor = 0;
			this->recordId = 0;
			this->score = 0;
			this->termRecordStaticScore = 0;
		}
		SuggestionCursorHeapItem(unsigned suggestionIndex, unsigned invertedListCursor, unsigned recordId,  float score,
				const vector<unsigned>& attributeIdsList, float termRecordStaticScore){
			this->suggestionIndex = suggestionIndex;
			this->invertedListCursor = invertedListCursor;
			this->recordId = recordId;
			this->score = score;
			this->attributeIdsList = attributeIdsList;
			this->termRecordStaticScore = termRecordStaticScore;
		}
		SuggestionCursorHeapItem(const SuggestionCursorHeapItem & src){
			this->suggestionIndex = src.suggestionIndex;
			this->invertedListCursor = src.invertedListCursor;
			this->recordId = src.recordId;
			this->score = src.score;
			this->attributeIdsList = src.attributeIdsList;
			this->termRecordStaticScore = src.termRecordStaticScore;
		}

		bool operator()(const SuggestionCursorHeapItem & left, const SuggestionCursorHeapItem & right){
			return left.score < right.score;
		}

		unsigned suggestionIndex;
		unsigned invertedListCursor;
		unsigned recordId;
		float score;
		vector<unsigned> attributeIdsList ;
		float termRecordStaticScore ;
	};
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionLowestLevelSuggestionOperator();
private:
	UnionLowestLevelSuggestionOperator() ;

	// vector of all suggestions of the keyword
	std::vector<SuggestionInfo > suggestionPairs;
	// inverted lists corresponding to suggestions. We keep them in this vector for improving efficiency.
	std::vector<shared_ptr<vectorview<unsigned> > > suggestionPairsInvertedListReadViews;
	// this heap keeps the most top unread records of inverted lists and always keeps the best one
	// according to runtime score on top
	std::vector<SuggestionCursorHeapItem> recordItemsHeap;

	QueryEvaluatorInternal * queryEvaluatorIntrnal;
	shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    shared_ptr<vectorview<unsigned> > invertedIndexKeywordIdsReadView;
	shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;

	/*
	 * this function iterates on possible suggestions and puts the first record
	 * of each inverted list in a max heap
	 */
	void initializeHeap(Term * term, Ranker * ranker, float prefixMatchPenalty);
	/*
	 * This function returns the top element of recordItemsHeap and uses the same inverted
	 * list to push another record to the heap
	 */
	bool getNextHeapItem(Term * term, Ranker * ranker, float prefixMatchPenalty,
			UnionLowestLevelSuggestionOperator::SuggestionCursorHeapItem & item);


};

class UnionLowestLevelSuggestionOptimizationOperator : public PhysicalPlanOptimizationNode {
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

#endif // __WRAPPER_UNIONLOWESTLEVELSUGGESTIONOPERATOR_H__
