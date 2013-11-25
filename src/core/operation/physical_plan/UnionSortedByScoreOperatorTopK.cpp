
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Union lists sorted by score top K ////////////////////////////

UnionSortedByScoreOperatorTopK::UnionSortedByScoreOperatorTopK() {
	//TODO
}
UnionSortedByScoreOperatorTopK::~UnionSortedByScoreOperatorTopK(){
	//TODO
}
bool UnionSortedByScoreOperatorTopK::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * histogramManager){
	//TODO
}
PhysicalPlanRecordItem * UnionSortedByScoreOperatorTopK::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionSortedByScoreOperatorTopK::close(){
	//TODO
}
bool UnionSortedByScoreOperatorTopK::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByScoreOptimizationOperatorTopK::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByScoreOptimizationOperatorTopK::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByScoreOptimizationOperatorTopK::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void UnionSortedByScoreOptimizationOperatorTopK::getOutputProperties(IteratorProperties & prop){
	// no guarantee on order
}
void UnionSortedByScoreOptimizationOperatorTopK::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType UnionSortedByScoreOptimizationOperatorTopK::getType() {
	return PhysicalPlanNode_UnionSortedByScoreTopK;
}
bool UnionSortedByScoreOptimizationOperatorTopK::validateChildren(){
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
