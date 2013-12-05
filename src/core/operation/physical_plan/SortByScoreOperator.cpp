
#include "PhysicalOperators.h"
#include "cmath"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByScoreOperator::SortByScoreOperator() {
}
SortByScoreOperator::~SortByScoreOperator(){
}
bool SortByScoreOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
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
	std::make_heap(records.begin(),records.end(), SortByScoreOperator::SortByScoreRecordMaxHeapComparator());
	return true;
}
PhysicalPlanRecordItem * SortByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(records.size() == 0){
		return NULL;
	}

	// get the next record to return
	PhysicalPlanRecordItem * toReturn = records.front();
	std::pop_heap(records.begin(),records.end(), SortByScoreOperator::SortByScoreRecordMaxHeapComparator());
	records.pop_back();

	return toReturn;
}
bool SortByScoreOperator::close(PhysicalPlanExecutionParameters & params){
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	records.clear();
	return true;
}
bool SortByScoreOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	/*
	 * cost :
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + cost(make_heap)
	 * =
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + estimatedNumberOfResultsOfChild
	 */
	PhysicalPlanCost resultCost(1); // O(1)
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params); // cost(child's open)
	// cost of fetching all the child's records
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost = resultCost +
			(1 + this->getChildAt(0)->getCostOfGetNext(params).cost) * estimatedNumberOfResults; // (cost(child's getNext) + O(1))*estimatedNumberOfResults
	resultCost = resultCost + estimatedNumberOfResults; // make_heap (O(n))

	return resultCost;

}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * cost : O(1) + log(estimatedNumberOfResults)
	 */
	PhysicalPlanCost resultCost(1); // O(1)
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost = resultCost + (unsigned)(log2((double)estimatedNumberOfResults));
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + 1;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void SortByScoreOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void SortByScoreOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no input property is required for this operator
}
PhysicalPlanNodeType SortByScoreOptimizationOperator::getType() {
	return PhysicalPlanNode_SortByScore;
}
bool SortByScoreOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
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
