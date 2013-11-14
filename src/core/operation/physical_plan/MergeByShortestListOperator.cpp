
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
bool MergeByShortestListOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * MergeByShortestListOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeByShortestListOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeByShortestListOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeByShortestListOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeByShortestListOperator::getCostOfClose() {
	//TODO
}
void MergeByShortestListOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	// this function doesn't provide any guarantee about order of results.
}
void MergeByShortestListOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no requirement for input
}
PhysicalPlanNodeType MergeByShortestListOperator::getType() {
	return PhysicalPlanNode_MergeByShortestList;
}

}
}
