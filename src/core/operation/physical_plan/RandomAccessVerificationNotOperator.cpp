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

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PhysicalPlan Random Access Verification Term Operator ////////////////////////////

RandomAccessVerificationNotOperator::RandomAccessVerificationNotOperator() {
}

RandomAccessVerificationNotOperator::~RandomAccessVerificationNotOperator(){
}
bool RandomAccessVerificationNotOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// open all children
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator , params);
	return true;
}
PhysicalPlanRecordItem * RandomAccessVerificationNotOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationNotOperator::close(PhysicalPlanExecutionParameters & params){
	// close the children
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

string RandomAccessVerificationNotOperator::toString(){
	string result = "RandomAccessVerificationNotOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool RandomAccessVerificationNotOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {

	bool resultFromChild = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
	if(resultFromChild == true){
		return false;
	}
	// we clear everything to make sure no junk propagates up
	parameters.attributeIdsList.clear();
	parameters.positionIndexOffsets.clear();
	parameters.prefixEditDistances.clear();
	parameters.termRecordMatchingPrefixes.clear();
	parameters.runTimeTermRecordScore = 0;
	parameters.staticTermRecordScore = 0;
	return true;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params);
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(); // zero cost
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void RandomAccessVerificationNotOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationNotOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// TODO
}
PhysicalPlanNodeType RandomAccessVerificationNotOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessNot;
}
bool RandomAccessVerificationNotOptimizationOperator::validateChildren(){
	ASSERT(this->getChildrenCount() == 1);
	PhysicalPlanNodeType childType = this->getChildAt(0)->getType();
	switch (childType) {
		case PhysicalPlanNode_RandomAccessTerm:
		case PhysicalPlanNode_RandomAccessAnd:
		case PhysicalPlanNode_RandomAccessOr:
		case PhysicalPlanNode_RandomAccessNot:
		case PhysicalPlanNode_RandomAccessGeo:
			return true;
		default:
			return false;
	}
}

}
}
