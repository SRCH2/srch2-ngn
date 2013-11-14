
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
bool UnionSortedByScoreOperatorTopK::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * UnionSortedByScoreOperatorTopK::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionSortedByScoreOperatorTopK::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByScoreOperatorTopK::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByScoreOperatorTopK::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByScoreOperatorTopK::getCostOfClose() {
	//TODO
}
void UnionSortedByScoreOperatorTopK::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	// no guarantee on order
}
void UnionSortedByScoreOperatorTopK::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType UnionSortedByScoreOperatorTopK::getType() {
	return PhysicalPlanNode_UnionSortedByScoreTopK;
}

}
}
