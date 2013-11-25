
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// merge by moving on shortest list /////////////////////////////

MergeByShortestListOperator::MergeByShortestListOperator() {
	//TODO
}

MergeByShortestListOperator::~MergeByShortestListOperator(){
	//TODO
}
bool MergeByShortestListOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * catalogManager){
	//TODO
}
PhysicalPlanRecordItem * MergeByShortestListOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeByShortestListOperator::close(){
	//TODO
}
bool MergeByShortestListOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeByShortestListOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeByShortestListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeByShortestListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void MergeByShortestListOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// this function doesn't provide any guarantee about order of results.
}
void MergeByShortestListOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no requirement for input
}
PhysicalPlanNodeType MergeByShortestListOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeByShortestList;
}
bool MergeByShortestListOptimizationOperator::validateChildren(){
	unsigned numberOfNonNullChildren = 0;
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();

		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
				break;
			case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
				// this operator cannot have TVL as a child, TVL overhead is not needed for this operator
				return false;
			default:{ // we count the number of non-verification operators.
				numberOfNonNullChildren ++;
				break;
			}
		}

	}
	if(numberOfNonNullChildren != 1){
		return false;
	}


	return true;
}

}
}
