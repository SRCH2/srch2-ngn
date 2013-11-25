
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Union lists sorted by score  ////////////////////////////////
UnionSortedByScoreOperator::UnionSortedByScoreOperator(){
	//TODO
}
UnionSortedByScoreOperator::~UnionSortedByScoreOperator(){
	//TODO
}
bool UnionSortedByScoreOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * histogramManager){
	//TODO
}
PhysicalPlanRecordItem * UnionSortedByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionSortedByScoreOperator::close(){
	//TODO
}
bool UnionSortedByScoreOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByScoreOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByScoreOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByScoreOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void UnionSortedByScoreOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// no guarantee on order
}
void UnionSortedByScoreOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType UnionSortedByScoreOptimizationOperator::getType() {
	return PhysicalPlanNode_UnionSortedByScore;
}
bool UnionSortedByScoreOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_UnionLowestLevelSimpleScanOperator:
				// since this operator needs the input to be sorted, it must only connect to
				// invertedIndex by TVL
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
