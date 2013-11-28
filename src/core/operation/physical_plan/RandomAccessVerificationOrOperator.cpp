
#include "PhysicalOperators.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PhysicalPlan Random Access Verification Term Operator ////////////////////////////

RandomAccessVerificationOrOperator::RandomAccessVerificationOrOperator() {
	//TODO
}

RandomAccessVerificationOrOperator::~RandomAccessVerificationOrOperator(){
	//TODO
}
bool RandomAccessVerificationOrOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = queryEvaluator;
	// open all children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}
	return true;
}
PhysicalPlanRecordItem * RandomAccessVerificationOrOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationOrOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = NULL;
	// close the children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	return true;
}
bool RandomAccessVerificationOrOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	// move on children and if at least on of them verifies the record return true
	bool verified = false;
	vector<float> runtimeScore;
	// static score is ignored for now
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		bool resultOfThisChild =
				this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
		runtimeScore.push_back(parameters.runTimeTermRecordScore);
		if(resultOfThisChild == true){
			verified = true;
		}
	}
	if(verified == true){ // so we need to aggregate runtime and static score
		parameters.runTimeTermRecordScore = computeAggregatedRuntimeScoreForOr(runtimeScore);
	}
	return verified;
}


float RandomAccessVerificationOrOperator::computeAggregatedRuntimeScoreForOr(std::vector<float> runTimeTermRecordScores){

	// max
	float resultScore = -1;

	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
		if((*score) > resultScore){
			resultScore = (*score);
		}
	}
	return resultScore;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned RandomAccessVerificationOrOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned RandomAccessVerificationOrOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned RandomAccessVerificationOrOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void RandomAccessVerificationOrOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationOrOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// TODO
}
PhysicalPlanNodeType RandomAccessVerificationOrOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessOr;
}
bool RandomAccessVerificationOrOptimizationOperator::validateChildren(){
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
