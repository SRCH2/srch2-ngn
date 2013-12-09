
#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// merge by moving on shortest list /////////////////////////////

MergeByShortestListOperator::MergeByShortestListOperator() {

}

MergeByShortestListOperator::~MergeByShortestListOperator(){

}
bool MergeByShortestListOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

	this->isShortestListFinished = false;

	this->previousResultsFound.clear();

	this->indexOfShortestListChild =
			((MergeByShortestListOptimizationOperator *)(this->getPhysicalPlanOptimizationNode()))->getShortestListOffsetInChildren();

	// open children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}

	return true;

}
PhysicalPlanRecordItem * MergeByShortestListOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(isShortestListFinished == true){
		return NULL;
	}

	while(true){
		//1. get the next record from shortest list
		PhysicalPlanRecordItem * nextRecord =
				this->getPhysicalPlanOptimizationNode()->getChildAt(this->indexOfShortestListChild)->getExecutableNode()->getNext(params);


		if(nextRecord == NULL){
			this->isShortestListFinished = true;
			return NULL;
		}

		//TODO possible optimization
		if(find(this->previousResultsFound.begin(),this->previousResultsFound.end(), nextRecord->getRecordId()) != this->previousResultsFound.end()){
			continue;
		}
		// validate the record with other children
		//2.
		std::vector<float> runTimeTermRecordScores;
		std::vector<float> staticTermRecordScores;
		std::vector<TrieNodePointer> termRecordMatchingKeywords;
		std::vector<unsigned> attributeBitmaps;
		std::vector<unsigned> prefixEditDistances;
		std::vector<unsigned> positionIndexOffsets;
		if(verifyRecordWithChildren(nextRecord,  runTimeTermRecordScores, staticTermRecordScores,
				termRecordMatchingKeywords, attributeBitmaps, prefixEditDistances , positionIndexOffsets, params ) == false){
			continue;	// 2.1. and 2.2.
		}
		// from this point, nextRecord is a candidate
		//3.
		// set the members
		nextRecord->setRecordMatchAttributeBitmaps(attributeBitmaps);
		nextRecord->setRecordMatchEditDistances(prefixEditDistances);
		nextRecord->setRecordMatchingPrefixes(termRecordMatchingKeywords);
		nextRecord->setPositionIndexOffsets(positionIndexOffsets);
		// nextRecord->setRecordStaticScore() Should we set static score as well ?
		nextRecord->setRecordRuntimeScore(params.ranker->computeAggregatedRuntimeScoreForAnd( runTimeTermRecordScores));
		// save it in previousResultsVector
		this->previousResultsFound.push_back(nextRecord->getRecordId());
		return nextRecord;
	}

	ASSERT(false); // we never reach here
	return NULL; // this return statement is only to suppress compiler warning

}


bool MergeByShortestListOperator::close(PhysicalPlanExecutionParameters & params){
	// close the children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}
	this->isShortestListFinished = false;
	this->previousResultsFound.clear();
	return true;

}
bool MergeByShortestListOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}


bool MergeByShortestListOperator::verifyRecordWithChildren(PhysicalPlanRecordItem * recordItem ,
					std::vector<float> & runTimeTermRecordScores,
					std::vector<float> & staticTermRecordScores,
					std::vector<TrieNodePointer> & termRecordMatchingKeywords,
					std::vector<unsigned> & attributeBitmaps,
					std::vector<unsigned> & prefixEditDistances,
					std::vector<unsigned> & positionIndexOffsets,
					const PhysicalPlanExecutionParameters & params){

	// move on children and call verifyByRandomAccess
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		if(childOffset == this->indexOfShortestListChild){
			runTimeTermRecordScores.push_back(recordItem->getRecordRuntimeScore());
			staticTermRecordScores.push_back(recordItem->getRecordStaticScore());
			vector<TrieNodePointer> matchingPrefixes;
			recordItem->getRecordMatchingPrefixes(matchingPrefixes);
			termRecordMatchingKeywords.insert(termRecordMatchingKeywords.end(),matchingPrefixes.begin(),matchingPrefixes.end());
			vector<unsigned> recordAttributeBitmaps;
			recordItem->getRecordMatchAttributeBitmaps(recordAttributeBitmaps);
			attributeBitmaps.insert(attributeBitmaps.end(),recordAttributeBitmaps.begin(),recordAttributeBitmaps.end());
			vector<unsigned> recordPrefixEditDistances;
			recordItem->getRecordMatchEditDistances(recordPrefixEditDistances);
			prefixEditDistances.insert(prefixEditDistances.end(),recordPrefixEditDistances.begin(),recordPrefixEditDistances.end());
			vector<unsigned> recordPositionIndexOffsets;
			recordItem->getPositionIndexOffsets(recordPositionIndexOffsets);
			positionIndexOffsets.insert(positionIndexOffsets.end(),recordPositionIndexOffsets.begin(),recordPositionIndexOffsets.end());
		}else{
			PhysicalPlanRandomAccessVerificationParameters parameters(params.ranker);
			parameters.recordToVerify = recordItem;
			parameters.isFuzzy = params.isFuzzy;
			parameters.prefixMatchPenalty = params.prefixMatchPenalty;
			bool resultOfThisChild =
					this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
			if(resultOfThisChild == false){
				return false;
			}
			// append new information to the output
			runTimeTermRecordScores.push_back(parameters.runTimeTermRecordScore);
			staticTermRecordScores.push_back(parameters.staticTermRecordScore);
			termRecordMatchingKeywords.insert(
					termRecordMatchingKeywords.end(),parameters.termRecordMatchingPrefixes.begin(),parameters.termRecordMatchingPrefixes.end());
			attributeBitmaps.insert(
					attributeBitmaps.end(),parameters.attributeBitmaps.begin(),parameters.attributeBitmaps.end());
			prefixEditDistances.insert(
					prefixEditDistances.end(),parameters.prefixEditDistances.begin(),parameters.prefixEditDistances.end());
			positionIndexOffsets.insert(
					positionIndexOffsets.end(),parameters.positionIndexOffsets.begin(),parameters.positionIndexOffsets.end());
		}
	}

	return true;

}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * If shortest list has 10 records, and the estimated number of results for this AND
	 * is 5, it means 2 records are processed in average per getNext(...)
	 * So, the cost of getNext is
	 * (estimated length of shortest list / estimated number of results of this and) * ( child's getNextCost + sum of verification costs + O(1) ) + O(1)
	 */
	unsigned indexOfShortestList = getShortestListOffsetInChildren();
	unsigned estimatedLengthOfShortestList = this->getChildAt(indexOfShortestList)->getLogicalPlanNode()->stats->estimatedNumberOfResults;
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	if(estimatedNumberOfResults == 0){
		estimatedNumberOfResults = 1;
	}
	// initialize record processing cost by cost of child's get next
	PhysicalPlanCost recordProcessingCost = this->getChildAt(indexOfShortestList)->getCostOfGetNext(params);
	// cost of verification on other children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		if(childOffset == indexOfShortestList){
			continue;
		}
		recordProcessingCost = recordProcessingCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
	}
	recordProcessingCost = recordProcessingCost + 1;


	recordProcessingCost.cost = (unsigned)( recordProcessingCost.cost * ( (estimatedLengthOfShortestList * 1.0) / (estimatedNumberOfResults) ));

	// O(1)
	recordProcessingCost = recordProcessingCost + 1;

	return recordProcessingCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
}
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
		resultCost = resultCost + 1; // O(1)
	}

	return resultCost;
}
void MergeByShortestListOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// this function doesn't provide any guarantee about order of results.
}
void MergeByShortestListOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no requirement for input
}
PhysicalPlanNodeType MergeByShortestListOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeByShortestList;
}
bool MergeByShortestListOptimizationOperator::validateChildren(){
	unsigned numberOfNonNullChildren = 0;
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();

		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
				break;
			case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
				// this operator cannot have TVL as a child, TVL overhead is not needed for this operator
				return false;
			default:{ // we count the number of non-verification operators.
				numberOfNonNullChildren ++;
				break;
			}
		}

	}
	if(numberOfNonNullChildren != 1){
		return false;
	}


	return true;
}

unsigned MergeByShortestListOptimizationOperator::getShortestListOffsetInChildren(){
	unsigned numberOfNonNullChildren = 0;
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();

		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
				break;
			default:{
				return i;
			}
		}

	}
	ASSERT(false);
	return 0;
}


}
}
