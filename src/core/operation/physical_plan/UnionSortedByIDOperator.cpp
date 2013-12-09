
#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Union lists sorted by ID /////////////////////////////////////

UnionSortedByIDOperator::UnionSortedByIDOperator() {}
UnionSortedByIDOperator::~UnionSortedByIDOperator(){}

bool UnionSortedByIDOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
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
	visitedRecords.clear();
	return true;

}
PhysicalPlanRecordItem * UnionSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(listsHaveMoreRecordsInThem == false){
		return NULL;
	}

	/*
	 * 0. Iterate on nextItemsFromChildren and getNext until an unvisited
	 * ---- record comes up
	 * 1. iterate on nextItemsFromChildren
	 * 2. find the min ID on top,
	 * 3. remove all top records that have the same ID as min ID
	 * 4. return the min ID (which has the most Score in ties)
	 */
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();

	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		while(true){
			PhysicalPlanRecordItem * recordItem = this->nextItemsFromChildren.at(childOffset);
			if(recordItem == NULL){
				break;;
			}
			if(find(this->visitedRecords.begin(), this->visitedRecords.end(), recordItem->getRecordId()) == this->visitedRecords.end()){
				break;
			}else{
				this->nextItemsFromChildren.at(childOffset) =
						this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
			}
		}
	}

	// find the min ID
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
		if(this->nextItemsFromChildren.at(childOffset) != NULL &&
				this->nextItemsFromChildren.at(childOffset)->getRecordId() == minIDRecord->getRecordId()){
			runtimeScores.push_back( this->nextItemsFromChildren.at(childOffset)->getRecordRuntimeScore() );
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
	minIDRecord->setRecordRuntimeScore(params.ranker->computeAggregatedRuntimeScoreForOr(runtimeScores));
	minIDRecord->setRecordMatchingPrefixes(recordKeywordMatchPrefixes);
	minIDRecord->setRecordMatchEditDistances(recordKeywordMatchEditDistances);
	minIDRecord->setRecordMatchAttributeBitmaps(recordKeywordMatchBitmaps);
	minIDRecord->setPositionIndexOffsets(positionIndexOffsets);

	this->visitedRecords.push_back(minIDRecord->getRecordId());

	return minIDRecord;
}
bool UnionSortedByIDOperator::close(PhysicalPlanExecutionParameters & params){
	// close children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	this->listsHaveMoreRecordsInThem = true;
	visitedRecords.clear();
	return true;
}
bool UnionSortedByIDOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessOrHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost UnionSortedByIDOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
		resultCost = resultCost + 1; // O(1)
	}

	// cost of initializing nextItems vector
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfGetNext(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost UnionSortedByIDOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * cost : sum of size of all children / estimatedNumberOfResults
	 */
	unsigned sumOfAllLengths = 0;
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		sumOfAllLengths += this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	}
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	return PhysicalPlanCost((unsigned)(((sumOfAllLengths*1.0)/estimatedNumberOfResults) + 1));
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost UnionSortedByIDOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
}
PhysicalPlanCost UnionSortedByIDOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
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
