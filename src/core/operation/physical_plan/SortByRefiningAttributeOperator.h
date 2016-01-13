/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $



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
	string toString();
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
