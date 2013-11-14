
#include "PhysicalOperators.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByScoreOperator::SortByScoreOperator() {
	//TODO
}
SortByScoreOperator::~SortByScoreOperator(){
	//TODO
}
bool SortByScoreOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * SortByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool SortByScoreOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned SortByScoreOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned SortByScoreOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned SortByScoreOperator::getCostOfClose() {
	//TODO
}
void SortByScoreOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void SortByScoreOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no input property is required for this operator
}
PhysicalPlanNodeType SortByScoreOperator::getType() {
	return PhysicalPlanNode_SortById;
}


}
}
