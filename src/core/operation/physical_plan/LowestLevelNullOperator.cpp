
#include "PhysicalOperators.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

LowestLevelNullOperator::LowestLevelNullOperator() {
	//TODO
}

LowestLevelNullOperator::~LowestLevelNullOperator(){
	//TODO
}
bool LowestLevelNullOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * LowestLevelNullOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool LowestLevelNullOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned LowestLevelNullOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned LowestLevelNullOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned LowestLevelNullOperator::getCostOfClose() {
	//TODO
}
void LowestLevelNullOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	// No output property because any operator can be attached to NULL operator
	// Also note that since nothing can be used as input to this operator, we don't care about inputProp
	return;
}
void LowestLevelNullOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel NULL operator will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType LowestLevelNullOperator::getType() {
	return PhysicalPlanNode_LowestLevelNull;
}

}
}
