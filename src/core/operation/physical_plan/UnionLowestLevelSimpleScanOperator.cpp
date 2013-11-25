
#include "UnionLowestLevelSimpleScanOperator.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelSimpleScanOperator::UnionLowestLevelSimpleScanOperator() {
	//TODO
}

UnionLowestLevelSimpleScanOperator::~UnionLowestLevelSimpleScanOperator(){
	//TODO
}
bool UnionLowestLevelSimpleScanOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * histogramManager){
	//TODO
}
PhysicalPlanRecordItem * UnionLowestLevelSimpleScanOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionLowestLevelSimpleScanOperator::close(){
	//TODO
}
bool UnionLowestLevelSimpleScanOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionLowestLevelSimpleScanOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionLowestLevelSimpleScanOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionLowestLevelSimpleScanOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void UnionLowestLevelSimpleScanOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// no output property
}
void UnionLowestLevelSimpleScanOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelSimpleScanOptimizationOperator::getType() {
	return PhysicalPlanNode_UnionLowestLevelSimpleScanOperator;
}
bool UnionLowestLevelSimpleScanOptimizationOperator::validateChildren(){
	if(getChildrenCount() > 0){ // this operator cannot have any children
		return false;
	}
	return true;
}

}
}
