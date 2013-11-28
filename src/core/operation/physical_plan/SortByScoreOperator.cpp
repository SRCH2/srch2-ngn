
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByScoreOperator::SortByScoreOperator() {
	//TODO
}
SortByScoreOperator::~SortByScoreOperator(){
	//TODO
}
bool SortByScoreOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = queryEvaluator;

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
	records.clear();
	this->queryEvaluator = NULL;
	return true;
}
bool SortByScoreOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned SortByScoreOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned SortByScoreOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned SortByScoreOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
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
