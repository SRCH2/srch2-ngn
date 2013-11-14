
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Union lists sorted by score  ////////////////////////////////

UnionSortedByScoreOperator::UnionSortedByScoreOperator() {
	//TODO
}
UnionSortedByScoreOperator::~UnionSortedByScoreOperator(){
	//TODO
}
bool UnionSortedByScoreOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * UnionSortedByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionSortedByScoreOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByScoreOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByScoreOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByScoreOperator::getCostOfClose() {
	//TODO
}
void UnionSortedByScoreOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	// no guarantee on order
}
void UnionSortedByScoreOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType UnionSortedByScoreOperator::getType() {
	return PhysicalPlanNode_UnionSortedByScore;
}


}
}
