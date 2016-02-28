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
#include "PhysicalOperatorsHelper.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PhysicalPlan Random Access Verification Term Operator ////////////////////////////

RandomAccessVerificationAndOperator::RandomAccessVerificationAndOperator() {
}

RandomAccessVerificationAndOperator::~RandomAccessVerificationAndOperator(){
}
bool RandomAccessVerificationAndOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// open all children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}
	return true;
}
PhysicalPlanRecordItem * RandomAccessVerificationAndOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationAndOperator::close(PhysicalPlanExecutionParameters & params){
	// close the children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	return true;
}

string RandomAccessVerificationAndOperator::toString(){
	string result = "RandomAccessVerificationAndOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool RandomAccessVerificationAndOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(0); // cost is zero
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;

	// cost of verifying children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
	}

	return resultCost;
}
void RandomAccessVerificationAndOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationAndOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// TODO
}
PhysicalPlanNodeType RandomAccessVerificationAndOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessAnd;
}
bool RandomAccessVerificationAndOptimizationOperator::validateChildren(){
	// all it's children must be RandomAccessNodes
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_RandomAccessGeo:
				continue;
			default:
				return false;
		}
	}
	return true;
}

}
}
