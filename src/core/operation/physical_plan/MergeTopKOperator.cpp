
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// merge with topK /////////////////////////////////////////

MergeTopKOperator::MergeTopKOperator() {
	//TODO
}

MergeTopKOperator::~MergeTopKOperator(){
	//TODO
}
bool MergeTopKOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * catalogManager){
	//TODO
}
PhysicalPlanRecordItem * MergeTopKOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeTopKOperator::close(){
	//TODO
}
bool MergeTopKOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeTopKOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeTopKOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeTopKOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void MergeTopKOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void MergeTopKOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by score
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType MergeTopKOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeTopK;
}
bool MergeTopKOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_UnionLowestLevelSimpleScanOperator:
				// TopK should connect to InvertedIndex only by TVL
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
