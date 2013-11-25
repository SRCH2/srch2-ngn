
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
bool SortByScoreOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * histogramManager){
	//TODO
}
PhysicalPlanRecordItem * SortByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool SortByScoreOperator::close(){
	//TODO
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
