
#include "UnionLowestLevelTermVirtualListOperator.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperator() {
	//TODO
}

UnionLowestLevelTermVirtualListOperator::~UnionLowestLevelTermVirtualListOperator(){
	//TODO
}
bool UnionLowestLevelTermVirtualListOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * histogramManager){
	//TODO
}
PhysicalPlanRecordItem * UnionLowestLevelTermVirtualListOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionLowestLevelTermVirtualListOperator::close(){
	//TODO
}
bool UnionLowestLevelTermVirtualListOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void UnionLowestLevelTermVirtualListOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void UnionLowestLevelTermVirtualListOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelTermVirtualListOptimizationOperator::getType() {
	return PhysicalPlanNode_UnionLowestLevelTermVirtualList;
}
bool UnionLowestLevelTermVirtualListOptimizationOperator::validateChildren(){
	if(getChildrenCount() > 0){ // this operator cannot have any children
		return false;
	}
	return true;
}

}
}
