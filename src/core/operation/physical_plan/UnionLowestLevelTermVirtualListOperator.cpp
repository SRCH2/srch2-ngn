
#include "UnionLowestLevelTermVirtualListOperator.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperator() {
	//TODO
}

UnionLowestLevelTermVirtualListOperator::~UnionLowestLevelTermVirtualListOperator(){
	//TODO
}
bool UnionLowestLevelTermVirtualListOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	//TODO
}
PhysicalPlanIterable * UnionLowestLevelTermVirtualListOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
bool UnionLowestLevelTermVirtualListOperator::close(){
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionLowestLevelTermVirtualListOperator::getCostOfOpen(){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionLowestLevelTermVirtualListOperator::getCostOfGetNext() {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionLowestLevelTermVirtualListOperator::getCostOfClose() {
	//TODO
}
void UnionLowestLevelTermVirtualListOperator::getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void UnionLowestLevelTermVirtualListOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelTermVirtualListOperator::getType() {
	return PhysicalPlanNode_UnionLowestLevelTermVirtualList;
}

}
}
