
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
bool MergeSortedByIDOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * MergeSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeSortedByIDOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeSortedByIDOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeSortedByIDOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeSortedByIDOperator::getCostOfClose() {
	//TODO
}
void MergeSortedByIDOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	// this function keeps the sorted-by-id propert of the inputs
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void MergeSortedByIDOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
PhysicalPlanNodeType MergeSortedByIDOperator::getType() {
	return PhysicalPlanNode_MergeSortedById;
}

}
}
