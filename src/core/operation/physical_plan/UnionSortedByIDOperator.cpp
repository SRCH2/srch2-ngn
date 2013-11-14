
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Union lists sorted by ID /////////////////////////////////////

UnionSortedByIDOperator::UnionSortedByIDOperator() {
	//TODO
}
UnionSortedByIDOperator::~UnionSortedByIDOperator(){
	//TODO
}
bool UnionSortedByIDOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * UnionSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionSortedByIDOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByIDOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByIDOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByIDOperator::getCostOfClose() {
	//TODO
}
void UnionSortedByIDOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void UnionSortedByIDOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
PhysicalPlanNodeType UnionSortedByIDOperator::getType() {
	return PhysicalPlanNode_UnionSortedById;
}


}
}
