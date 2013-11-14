
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByIdOperator::SortByIdOperator() {
	//TODO
}
SortByIdOperator::~SortByIdOperator(){
	//TODO
}
bool SortByIdOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * SortByIdOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool SortByIdOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned SortByIdOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned SortByIdOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned SortByIdOperator::getCostOfClose() {
	//TODO
}
void SortByIdOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void SortByIdOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no input property is required for this operator
}
PhysicalPlanNodeType SortByIdOperator::getType() {
	return PhysicalPlanNode_SortById;
}


}
}
