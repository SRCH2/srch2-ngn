
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
bool MergeTopKOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * MergeTopKOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool MergeTopKOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeTopKOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeTopKOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeTopKOperator::getCostOfClose() {
	//TODO
}
void MergeTopKOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void MergeTopKOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by score
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType MergeTopKOperator::getType() {
	return PhysicalPlanNode_MergeTopK;
}


}
}
