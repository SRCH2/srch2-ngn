//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

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


#ifndef __CORE_POSTPROCESSING_SORTBYREFININGATTRIBUTEOPERATOR_H__
#define __CORE_POSTPROCESSING_SORTBYREFININGATTRIBUTEOPERATOR_H__

#include <vector>
#include <algorithm>
#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"
#include "PhysicalPlan.h"

namespace srch2
{
namespace instantsearch
{

class ResultRefiningAttributeComparator {
private:
    SortEvaluator * evaluator;

public:
    ResultRefiningAttributeComparator(SortEvaluator * evaluator) {
        this->evaluator = evaluator;
    }

    // this operator should be consistent with two others in TermVirtualList.h and QueryResultsInternal.h
    bool operator()(const PhysicalPlanRecordItem * lhs, const PhysicalPlanRecordItem * rhs) const {
        // do the comparison
        return evaluator->compare(lhs->valuesOfParticipatingRefiningAttributes, lhs->getRecordId(),
                rhs->valuesOfParticipatingRefiningAttributes, rhs->getRecordId()) > 0;
    }
};


/*
 * The following two classes implement Sort Filter as a physical operator.
 * this operator is always above the root physical operator and it sorts the
 * input based on the value of a refining attribute given in query and stored
 * in forward index. Results are returned to the parent (which might be another
 * post processing operator) only sorted and untouched in other aspects.
 * Example :
 * q = A AND B OR C & sort=price
 * the core plan for query execution (one possible plan):
 * [OR sorted by ID]___[SORT BY ID]___[SCAN C]
 *       |
 *       |_____________[SORT BY ID]___[Merge TopK]____[TVL A]
 *                                          |_________[TVL B]
 *
 * but the complete plan tree looks like this :
 *
 * [SortByRefiningAttributeOperator price]
 *        |
 * [KeywordSearchOperator]
 * { // the core plan is built and optimized in open function of KeywordSearchOperator
 * [OR sorted by ID]___[SORT BY ID]___[SCAN C]
 *       |
 *       |_____________[SORT BY ID]___[Merge TopK]____[TVL A]
 *                                          |_________[TVL B]
 * }
 *
 * and the results are collected by calling getNext of SortByRefiningAttributeOperator iteratively .
 *
 */

class SortByRefiningAttributeOperator : public PhysicalPlanNode {
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~SortByRefiningAttributeOperator();
	SortByRefiningAttributeOperator(SortEvaluator * sortEvaluator) ;
private:
	void preFilter(QueryEvaluatorInternal *queryEvaluator);
	void doProcessOneResult(const TypedValue & attributeValue, const unsigned facetFieldIndex);

	QueryEvaluatorInternal *queryEvaluatorInternal;
	SortEvaluator * sortEvaluator;

	vector<PhysicalPlanRecordItem *> results;
	unsigned cursorOnResults ;

};

class SortByRefiningAttributeOptimizationOperator : public PhysicalPlanOptimizationNode {
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

#endif //__CORE_POSTPROCESSING_SORTBYREFININGATTRIBUTEOPERATOR_H__
