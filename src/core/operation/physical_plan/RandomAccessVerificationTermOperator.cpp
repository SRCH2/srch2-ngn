
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
bool RandomAccessVerificationTermOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	  //do the verification
	ts_shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet =
			this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(parameters.isFuzzy);

	Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(parameters.isFuzzy);;

	return verifyByRandomAccessHelper(this->queryEvaluator, prefixActiveNodeSet.get(), term, parameters);

}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(2);
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost();
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost();
	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationTermOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(5);
	resultCost.addMediumFunctionCost(estimatedNumberOfTerminalNodes);
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
