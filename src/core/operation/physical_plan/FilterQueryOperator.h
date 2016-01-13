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


#ifndef __CORE_OPERATION_PHYSICALPLAN_FILTERQUERYOPERATOR_H__
#define __CORE_OPERATION_PHYSICALPLAN_FILTERQUERYOPERATOR_H__

#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/TypedValue.h"
#include "PhysicalPlan.h"

namespace srch2
{
namespace instantsearch
{



/*
 * The following two classes implement filter_query as a physical operator.
 * This operator only exists if a filter_query request comes with the query.
 * For example query : q = A AND B OR C & fq = "model='TOYOTA'"
 * If this operator exists, it goes exactly one top of lowest level
 * term nodes so that it filters the records as soon as the come out of
 * inverted index.
 * The core physical plan of this example is :
 *
 *
 * [OR sorted by ID]___[SORT BY ID]____[FilterQueryOperator]____[SCAN C]
 *       |
 *       |_____________[SORT BY ID]___[Merge TopK]____[FilterQueryOperator]____[TVL A]
 *                                          |_________[FilterQueryOperator]____[TVL B]
 *
 */
class FilterQueryOperator : public PhysicalPlanNode {
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~FilterQueryOperator();
	FilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator, string &roleId) ;
private:
	bool doPass(const Schema * schema, PhysicalPlanRecordItem * record);
	bool hasAccessToRecord(unsigned recordId); // check the access of the role to this record
	RefiningAttributeExpressionEvaluator * filterQueryEvaluator;
	QueryEvaluatorInternal * queryEvaluatorInternal;
	string roleId;   // role id for access control
};

class FilterQueryOptimizationOperator : public PhysicalPlanOptimizationNode {
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
#endif // __CORE_OPERATION_PHYSICALPLAN_FILTERQUERYOPERATOR_H__
