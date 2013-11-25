
#include "PhysicalOperators.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PhysicalPlan Random Access Verification Term Operator ////////////////////////////

RandomAccessVerificationTermOperator::RandomAccessVerificationTermOperator() {
	//TODO
}

RandomAccessVerificationTermOperator::~RandomAccessVerificationTermOperator(){
	//TODO
}
bool RandomAccessVerificationTermOperator::open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, HistogramManager * catalogManager){
	//TODO
}
PhysicalPlanRecordItem * RandomAccessVerificationTermOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationTermOperator::close(){
	//TODO
}
bool RandomAccessVerificationTermOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	//TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned RandomAccessVerificationTermOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned RandomAccessVerificationTermOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned RandomAccessVerificationTermOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void RandomAccessVerificationTermOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationTermOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel NULL operator will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType RandomAccessVerificationTermOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessTerm;
}
bool RandomAccessVerificationTermOptimizationOperator::validateChildren(){
	// shouldn't have any child
	ASSERT(getChildrenCount() == 0);
	if(getChildrenCount() > 0){
		return false;
	}
	return true;
}

}
}
