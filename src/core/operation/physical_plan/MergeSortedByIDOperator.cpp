
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// merge when lists are sorted by ID ////////////////////////////

MergeSortedByIDOperator::MergeSortedByIDOperator() {
	//TODO
}

MergeSortedByIDOperator::~MergeSortedByIDOperator(){
	//TODO
}
bool MergeSortedByIDOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * catalogManager){
	//TODO
}
PhysicalPlanRecordItem * MergeSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeSortedByIDOperator::close(){
	//TODO
}
bool MergeSortedByIDOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeSortedByIDOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeSortedByIDOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeSortedByIDOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void MergeSortedByIDOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// this function keeps the sorted-by-id propert of the inputs
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void MergeSortedByIDOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
PhysicalPlanNodeType MergeSortedByIDOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeSortedById;
}
bool MergeSortedByIDOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
				// this operator cannot have TVL as a child, TVL overhead is not needed for this operator
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
