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
#include "FilterQueryOperator.h"
#include "PhysicalOperators.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "operation/QueryEvaluatorInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"
#include <vector>
#include "string"
#include "util/RecordSerializerUtil.h"
using namespace srch2::util;

namespace srch2
{
namespace instantsearch
{

bool FilterQueryOperator::open(QueryEvaluatorInternal * queryEvaluatorInternal, PhysicalPlanExecutionParameters & params){
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluatorInternal, params);
	this->queryEvaluatorInternal = queryEvaluatorInternal;
	return true;
}
PhysicalPlanRecordItem * FilterQueryOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			return NULL;
		}
		const Schema * schema = queryEvaluatorInternal->getSchema();
		if(hasAccessToRecord(nextRecord->getRecordId()) && doPass(schema, nextRecord)){
			return nextRecord;
		}
	}
	ASSERT(false); // we can never reach to this point
	return NULL;
}
bool FilterQueryOperator::close(PhysicalPlanExecutionParameters & params){
	this->filterQueryEvaluator = NULL;
	this->queryEvaluatorInternal = NULL;
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

string FilterQueryOperator::toString(){
	string result;
	if(this->filterQueryEvaluator != NULL)
		result = "filterQueryOperator" + this->filterQueryEvaluator->toString() + this->roleId ;
	else
		result = this->roleId;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool FilterQueryOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
}
FilterQueryOperator::~FilterQueryOperator(){}

FilterQueryOperator::FilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator, string &roleId) {
	this->filterQueryEvaluator = filterQueryEvaluator;
	this->roleId = roleId;
}

bool FilterQueryOperator::doPass(const Schema * schema, PhysicalPlanRecordItem * record){
	// Because we use this operator for filters and for access control. When we just have access control the filter query evaluator is NULL
	if(this->filterQueryEvaluator == NULL) // filterQueryEvaluator is null. So we don't have any filter
		return true;
    // fetch the names and ids of non searchable attributes from schema
    vector<string> attributes;
    for(map<string,unsigned>::const_iterator attr = schema->getRefiningAttributes()->begin();
            attr != schema->getRefiningAttributes()->end() ; ++attr ){
        attributes.push_back(attr->first);
    }

    // now fetch the values of different attributes from forward index
    vector<TypedValue> typedValues;
    bool isValid = false;
    const ForwardList * list = this->queryEvaluatorInternal->indexReadToken.getForwardList(record->getRecordId() , isValid);
    // return false if this record is not valid (i.e., already deleted)
    if (!isValid)
      return false;
    StoredRecordBuffer refiningAttributesData = list->getInMemoryData();
    RecordSerializerUtil::getBatchOfAttributes(attributes, schema,refiningAttributesData.start.get() ,&typedValues);

    // now call the evaluator to see if this record passes the criteria or not
    // A criterion can be for example price:12 or price:[* TO 100]
    map<string, TypedValue> valuesForEvaluation;
    // prepare the evaluator input
    unsigned scoresIndex =0;
    for(vector<string>::iterator attr = attributes.begin() ; attr != attributes.end() ; ++attr ){
        valuesForEvaluation[*attr] = typedValues.at(scoresIndex);
        scoresIndex++;
    }
    return this->filterQueryEvaluator->evaluate(valuesForEvaluation);
}

bool FilterQueryOperator::hasAccessToRecord(unsigned recordId){
	if(roleId == "") // it means that we don't have any access control check
		return true;

	return this->queryEvaluatorInternal->indexReadToken.hasAccessToForwardList(recordId, this->roleId);
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfOpen(params);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfGetNext(params);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfClose(params);
}
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	return this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
}
void FilterQueryOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	return;
}
void FilterQueryOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	return ;
}
PhysicalPlanNodeType FilterQueryOptimizationOperator::getType() {
	return PhysicalPlanNode_FilterQuery;
}
bool FilterQueryOptimizationOperator::validateChildren(){
	return true;
}

}
}
