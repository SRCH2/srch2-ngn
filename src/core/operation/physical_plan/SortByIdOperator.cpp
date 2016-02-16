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

#include "PhysicalOperators.h"
#include "algorithm"
#include "cmath"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByIdOperator::SortByIdOperator() {
}
SortByIdOperator::~SortByIdOperator(){
}
bool SortByIdOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);

	// open the single child
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator,params);

	// now get all the records from the child and sort them
	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			break;
		}
		records.push_back(nextRecord);
	}

	// heapify the records to get the smallest one on top
	/*
	 * Instead of sorting the results and then returning them one by one, we heapify all of them
	 * and then after returning each one we do one more heapification.
	 * If the user needs K results, the time complexity is klog(N) + N (because bulk heapify is N)
	 * while normal sort approach is NlogN which is larger
	 */
	std::make_heap(records.begin(),records.end(), SortByIdOperator::SortByIdRecordMinHeapComparator());
	return true;
}
PhysicalPlanRecordItem * SortByIdOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(records.size() == 0){
		return NULL;
	}

	// get the next record to return
	PhysicalPlanRecordItem * toReturn = records.front();
	std::pop_heap(records.begin(),records.end(), SortByIdOperator::SortByIdRecordMinHeapComparator());
	records.pop_back();

	return toReturn;
}
bool SortByIdOperator::close(PhysicalPlanExecutionParameters & params){
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	records.clear();
	return true;
}

string SortByIdOperator::toString(){
	string result = "SortByIdOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool SortByIdOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost SortByIdOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	/*
	 * cost :
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + cost(make_heap)
	 * =
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + estimatedNumberOfResultsOfChild
	 */
	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params); // cost(child's open)
	// cost of fetching all the child's records
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost = resultCost +
			(this->getChildAt(0)->getCostOfGetNext(params).cost) * estimatedNumberOfResults; // (cost(child's getNext) + O(1))*estimatedNumberOfResults
	// sorting
	// making heap = O(N)
	resultCost = resultCost + estimatedNumberOfResults;

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost SortByIdOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * cost : log(estimatedNumberOfResults)
	 */
	PhysicalPlanCost resultCost;
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost = resultCost + (unsigned)(log2((double)estimatedNumberOfResults + 1)); // + 1 is to avoid 0
	return resultCost;
}

// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost SortByIdOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost SortByIdOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void SortByIdOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void SortByIdOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no input property is required for this operator
}
PhysicalPlanNodeType SortByIdOptimizationOperator::getType() {
	return PhysicalPlanNode_SortById;
}
bool SortByIdOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_RandomAccessGeo:
				return false;
			default:{
				continue;
			}
		}
	}
	return true;
}

}
}
