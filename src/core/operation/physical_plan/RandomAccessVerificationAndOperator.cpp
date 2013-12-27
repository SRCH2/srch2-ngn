
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

void RandomAccessVerificationAndOperator::getUniqueStringForCache(bool ignoreLastLeafNode, string & uniqueString){

}

bool RandomAccessVerificationAndOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(2 + 2 * this->getChildrenCount()); // 2 + number of open calls + number of getNext calls
	resultCost.addSmallFunctionCost(2); // clear()
	resultCost.addFunctionCallCost(8 * this->getChildrenCount()); // 4 * (  number of open calls + number of getNext calls

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
	}

	// cost of initializing nextItems vector
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfGetNext(params);
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1); // cost is zero
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(this->getChildrenCount()); // 3 + number of close calls
	resultCost.addFunctionCallCost(4 * this->getChildrenCount()); // 2 + number of close calls

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationAndOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.addSmallFunctionCost();
	resultCost.addFunctionCallCost();

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
				continue;
			default:
				return false;
		}
	}
	return true;
}

}
}
