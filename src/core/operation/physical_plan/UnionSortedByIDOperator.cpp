
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
bool UnionSortedByIDOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = queryEvaluator;
	/*
	 * 1. open all children (no parameters known to pass as of now)
	 * 2. initialize nextRecordItems vector.
	 */
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}

	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		PhysicalPlanRecordItem * recordItem =
				this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
		this->nextItemsFromChildren.push_back(recordItem);
	}

	listsHaveMoreRecordsInThem = true;
	return true;

}
PhysicalPlanRecordItem * UnionSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(listsHaveMoreRecordsInThem == false){
		return NULL;
	}

	/*
	 * 1. iterate on nextItemsFromChildren
	 * 2. find the min ID on top,
	 * 3. remove all top records that have the same ID as min ID
	 * 4. return the min ID (which has the most Score in ties)
	 */
	// find the min ID
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	PhysicalPlanRecordItem * minIDRecord = NULL;
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		PhysicalPlanRecordItem * recordItem = this->nextItemsFromChildren.at(childOffset);
		if(recordItem == NULL){
			continue;
		}
		if(minIDRecord == NULL){
			minIDRecord = recordItem;
		}else if((minIDRecord->getRecordId() > recordItem->getRecordId()) // find the 1. min ID and 2. max score
				|| (minIDRecord->getRecordId() == recordItem->getRecordId() &&
						minIDRecord->getRecordRuntimeScore() < recordItem->getRecordRuntimeScore())){
			minIDRecord = recordItem;
		}

	}
	if(minIDRecord == NULL){
		this->listsHaveMoreRecordsInThem = false;
		return NULL;
	}
	// now remove all minID records from all lists
	vector<float> runtimeScores;
	vector<TrieNodePointer> recordKeywordMatchPrefixes;
	vector<unsigned> recordKeywordMatchEditDistances;
	vector<unsigned> recordKeywordMatchBitmaps;
	vector<unsigned> positionIndexOffsets;
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		if(this->nextItemsFromChildren.at(childOffset)->getRecordId() == minIDRecord->getRecordId()){
			this->nextItemsFromChildren.at(childOffset)->getRecordMatchingPrefixes(recordKeywordMatchPrefixes);
			this->nextItemsFromChildren.at(childOffset)->getRecordMatchEditDistances(recordKeywordMatchEditDistances);
			this->nextItemsFromChildren.at(childOffset)->getRecordMatchAttributeBitmaps(recordKeywordMatchBitmaps);
			this->nextItemsFromChildren.at(childOffset)->getPositionIndexOffsets(positionIndexOffsets);
			// static score, not for now
			// and remove it from this list by substituting it by the next one
			this->nextItemsFromChildren.at(childOffset) =
					this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
		}
	}
	// prepare the record and return it
	minIDRecord->setRecordMatchingPrefixes(recordKeywordMatchPrefixes);
	minIDRecord->setRecordMatchEditDistances(recordKeywordMatchEditDistances);
	minIDRecord->setRecordMatchAttributeBitmaps(recordKeywordMatchBitmaps);
	minIDRecord->setPositionIndexOffsets(positionIndexOffsets);

	return minIDRecord;
}
bool UnionSortedByIDOperator::close(PhysicalPlanExecutionParameters & params){
	// close children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	this->queryEvaluator = NULL;
	this->listsHaveMoreRecordsInThem = true;
	return true;
}
bool UnionSortedByIDOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	// move on children and if at least on of them verifies the record return true
	bool verified = false;
	vector<float> runtimeScore;
	// static score is ignored for now
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		bool resultOfThisChild =
				this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
		runtimeScore.push_back(parameters.runTimeTermRecordScore);
		if(resultOfThisChild == true){
			verified = true;
		}
	}
	if(verified == true){ // so we need to aggregate runtime and static score
		parameters.runTimeTermRecordScore = computeAggregatedRuntimeScoreForOr(runtimeScore);
	}
	return verified;
}

float UnionSortedByIDOperator::computeAggregatedRuntimeScoreForOr(std::vector<float> runTimeTermRecordScores){

	// max
	float resultScore = -1;

	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
		if((*score) > resultScore){
			resultScore = (*score);
		}
	}
	return resultScore;
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionSortedByIDOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionSortedByIDOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionSortedByIDOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void UnionSortedByIDOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void UnionSortedByIDOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
PhysicalPlanNodeType UnionSortedByIDOptimizationOperator::getType() {
	return PhysicalPlanNode_UnionSortedById;
}
bool UnionSortedByIDOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
				// this operator cannot have TVL as a child, TVL overhead is not needed for this operator
				return false;
			default:{
				continue;
			}
		}
	}
	return true;
}

}
}
