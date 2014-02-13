
#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// merge when lists are sorted by ID ////////////////////////////

MergeSortedByIDOperator::MergeSortedByIDOperator() {
}

MergeSortedByIDOperator::~MergeSortedByIDOperator(){
}
bool MergeSortedByIDOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	/*
	 * 1. open all children (no parameters known to pass as of now)
	 * 2. initialize nextRecordItems vector.
	 */
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}


	listsHaveMoreRecordsInThem = true;
	previousResultsFound.clear();
	return true;
}
PhysicalPlanRecordItem * MergeSortedByIDOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(listsHaveMoreRecordsInThem == false){
		return NULL;
	}
	/*
	 * 0. i = 0
	 * 1. 'record' = pick a 'record' from the current top of the ith list
	 * 1.1 clear 'recordMatches' vector
	 * 2. for each jth list that j!=i
	 * 2.1. keep getting the 'nextRecord' of the jth list until
	 * ---- either 'nextRecord' is NULL, in which case we return NULL
	 * ---- or'nextRecord'.ID >= 'record'.ID
	 * 2.2. if(nextRecord'.ID == 'record'.ID) then, save 'nextRecord' in 'recordMatches' and go to 2
	 * 2.2. else, 'record' = 'nextrecord' and go to 1.1.
	 * 3. if size of recordMatches == numberOfChildren , prepare and return 'record'
	 * 3. else, i = (i+1) mod numberOfChildren , go to 1
	 *
	 */
	unsigned childToGetNextRecordFrom = 0; // i
	PhysicalPlanRecordItem * record = NULL;// record
	while(true){
		record = this->getPhysicalPlanOptimizationNode()->getChildAt(childToGetNextRecordFrom)->getExecutableNode()->getNext(params);//1.
		if(record == NULL){
			this->listsHaveMoreRecordsInThem = false;
			return NULL;
		}
		if(find(this->previousResultsFound.begin(),this->previousResultsFound.end(), record->getRecordId()) == this->previousResultsFound.end()){
			break;
		}
	}
	while(true){ // 1.1.
		vector<PhysicalPlanRecordItem *> recordMatches;
		bool shouldGoToNextRound = false;
		for(unsigned childOffset /*j*/ = 0 ; childOffset < this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){ // 2.
			if(childOffset == childToGetNextRecordFrom){
				recordMatches.push_back(record);
				// the share of this list in the vector is this record itself
				continue;
			}
			while(true){ //2.1
				PhysicalPlanRecordItem * nextRecord = // record
						this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
				if(nextRecord == NULL){
					this->listsHaveMoreRecordsInThem = false;
					return NULL;
				}
				if(nextRecord->getRecordId() == record->getRecordId()){
					recordMatches.push_back(nextRecord);
					break; // go to 2
				}else if(nextRecord->getRecordId() > record->getRecordId()){
					record = nextRecord;
					childToGetNextRecordFrom = childOffset;
					shouldGoToNextRound = true;//because we want to go to 1.1.
					break;
				}
			}
			if(shouldGoToNextRound == true){
				break;// because we want to go to 1.1.
			}
		}
		if(shouldGoToNextRound == true){
			continue;// go to 1.1.
		}
		//3.
		if(recordMatches.size() == this->getPhysicalPlanOptimizationNode()->getChildrenCount()){ // this record is a result
			//runtime score
			vector<float> runtimeScores;
			for(vector<PhysicalPlanRecordItem *>::iterator match = recordMatches.begin() ; match != recordMatches.end(); ++match){
				runtimeScores.push_back((*match)->getRecordRuntimeScore());
			}
			vector<TrieNodePointer> recordKeywordMatchPrefixes;
			for(vector<PhysicalPlanRecordItem *>::iterator match = recordMatches.begin() ; match != recordMatches.end(); ++match){
				(*match)->getRecordMatchingPrefixes(recordKeywordMatchPrefixes);
			}
			vector<unsigned> recordKeywordMatchEditDistances;
			for(vector<PhysicalPlanRecordItem *>::iterator match = recordMatches.begin() ; match != recordMatches.end(); ++match){
				(*match)->getRecordMatchEditDistances(recordKeywordMatchEditDistances);
			}
			vector<unsigned> recordKeywordMatchBitmaps;
			for(vector<PhysicalPlanRecordItem *>::iterator match = recordMatches.begin() ; match != recordMatches.end(); ++match){
				(*match)->getRecordMatchAttributeBitmaps(recordKeywordMatchBitmaps);
			}
			vector<unsigned> positionIndexOffsets;
			for(vector<PhysicalPlanRecordItem *>::iterator match = recordMatches.begin() ; match != recordMatches.end(); ++match){
				(*match)->getPositionIndexOffsets(positionIndexOffsets);
			}
			// static score, not for now
			record->setRecordRuntimeScore(params.ranker->computeAggregatedRuntimeScoreForAnd(runtimeScores));
			record->setRecordMatchingPrefixes(recordKeywordMatchPrefixes);
			record->setRecordMatchEditDistances(recordKeywordMatchEditDistances);
			record->setRecordMatchAttributeBitmaps(recordKeywordMatchBitmaps);
			record->setPositionIndexOffsets(positionIndexOffsets);
			// static score ignored for now

			// add to previousResultsFound
			this->previousResultsFound.push_back(record->getRecordId());

			return record;
		}else{
			childToGetNextRecordFrom = (childToGetNextRecordFrom + 1) % this->getPhysicalPlanOptimizationNode()->getChildrenCount();
			while(true){
				record = this->getPhysicalPlanOptimizationNode()->getChildAt(childToGetNextRecordFrom)->getExecutableNode()->getNext(params);//1.
				if(record == NULL){
					this->listsHaveMoreRecordsInThem = false;
					return NULL;
				}
				if(find(this->previousResultsFound.begin(),this->previousResultsFound.end(), record->getRecordId()) == this->previousResultsFound.end()){
					break;
				}
			}

		}
	}

	ASSERT(false);
	return NULL;

}
bool MergeSortedByIDOperator::close(PhysicalPlanExecutionParameters & params){

	// close children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	this->listsHaveMoreRecordsInThem = false;
	return true;
}

string MergeSortedByIDOperator::toString(){
	string result = "MergeSortedByIDOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool MergeSortedByIDOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost MergeSortedByIDOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(3 + this->getChildrenCount()); // 3 + number of open calls
	resultCost.addSmallFunctionCost(); // clear()
	resultCost.addFunctionCallCost(2 + this->getChildrenCount()); // 2 + number of open calls

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost MergeSortedByIDOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * We suppose all cursors move with the same speed
	 * total cost for each getNext is :
	 * sum(over all lists I) : (lengthOfListI / estimatedNumberOfResults) * getNextCost of listI
	 *
	 */
	unsigned numberOfChildren = this->getChildrenCount();
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	if(estimatedNumberOfResults == 0){
		estimatedNumberOfResults = 1;
	}
	unsigned numberOfGetNextCallsOfChildren = 0;
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		unsigned thisChildsLength = this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
		numberOfGetNextCallsOfChildren +=
				((thisChildsLength * 1.0) / estimatedNumberOfResults) * this->getChildAt(childOffset)->getCostOfGetNext(params).cost;
	}
	PhysicalPlanCost resultCost ;
	// cost of other instructions in getNext
	resultCost.addSmallFunctionCost(numberOfGetNextCallsOfChildren);
	resultCost.addMediumFunctionCost(); // copying the result


	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost MergeSortedByIDOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(3 + this->getChildrenCount()); // 3 + number of open calls
	resultCost.addSmallFunctionCost(); // clear()
	resultCost.addFunctionCallCost(2 + this->getChildrenCount()); // 2 + number of open calls

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost MergeSortedByIDOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(2);
	resultCost.addSmallFunctionCost();

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
	}

	return resultCost;
}
void MergeSortedByIDOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// this function keeps the sorted-by-id propert of the inputs
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
void MergeSortedByIDOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by ID
	prop.addProperty(PhysicalPlanIteratorProperty_SortById);
}
PhysicalPlanNodeType MergeSortedByIDOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeSortedById;
}
bool MergeSortedByIDOptimizationOperator::validateChildren(){
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
