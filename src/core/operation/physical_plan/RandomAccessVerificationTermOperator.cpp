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

RandomAccessVerificationTermOperator::RandomAccessVerificationTermOperator() {
}

RandomAccessVerificationTermOperator::~RandomAccessVerificationTermOperator(){
}
bool RandomAccessVerificationTermOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// random access needs no caching.
	this->queryEvaluator = queryEvaluator;
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 0);
	return true;
}
PhysicalPlanRecordItem * RandomAccessVerificationTermOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationTermOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = NULL;
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 0);
	return true;
}

string RandomAccessVerificationTermOperator::toString(){
	string result = "RandomAccessVerificationTermOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool RandomAccessVerificationTermOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	  //do the verification
	boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet =
			this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(parameters.isFuzzy);

	Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(parameters.isFuzzy);;

	return verifyByRandomAccessHelper(this->queryEvaluator, prefixActiveNodeSet.get(), term, parameters);

}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	unsigned estimatedNumberOfActiveNodes = 0;

	Term * term = this->getLogicalPlanNode()->getTerm(params.isFuzzy);
	if(term->getTermType() == TERM_TYPE_COMPLETE){
		LeafNodeSetIteratorForComplete iter(this->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy).get()
						, term->getThreshold());
		estimatedNumberOfActiveNodes = iter.size();
	}else{ // prefix
		estimatedNumberOfActiveNodes = this->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy)->getNumberOfActiveNodes();
	}
	PhysicalPlanCost resultCost;
	resultCost.cost = estimatedNumberOfActiveNodes * log2(85.0);
	return resultCost;
}
void RandomAccessVerificationTermOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationTermOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel NULL operator will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType RandomAccessVerificationTermOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessTerm;
}
bool RandomAccessVerificationTermOptimizationOperator::validateChildren(){
	// shouldn't have any child
	ASSERT(getChildrenCount() == 0);
	if(getChildrenCount() > 0){
		return false;
	}
	return true;
}

}
}
