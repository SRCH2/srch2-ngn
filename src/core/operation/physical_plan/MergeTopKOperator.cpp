
#include "PhysicalOperators.h"
#include "MergeTopKOperator.h"
#include "operation/QueryEvaluatorInternal.h"

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
bool MergeTopKOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

	this->queryEvaluator = queryEvaluator;

	/*
	 * 1. open all children (no parameters known to pass as of now)
	 * 2. initialize nextRecordItems vector.
	 * 3. candidatesList = empty vector
	 * 4. First assumption is that all lists have records in them.
	 * 5. Round robin should be initialized
	 */
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
	}

	initializeNextItemsFromChildren(params);

	// just to make sure
	candidatesList.clear();

	listsHaveMoreRecordsInThem = true;
	childRoundRobinOffset = 0;
	visitedRecords.clear();
	return true;
}
PhysicalPlanRecordItem * MergeTopKOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * PhysicalPlanRecordItem * topRecordToReturn = NULL;
	 *
	 * Part 1:
	 * 1. sort the candidatesList
	 * 2. set 'topRecordToReturn' = best record in candidatesList
	 * 3. If part 2 is disabled, return topRecordToReturn
	 *
	 * Part 2:
	 * 1. child = getNextChildForSequentialAccess() gives the offset of the next child to get next result from
	 * ----- right now it implements Round Robin
	 * 2. get the next 'record' of 'child' by using 'getNextRecordOfChild(...)'
	 * 2.1. if 'record' is NULL, disable part 2, return 'topRecordToReturn'
	 * 2.2. if 'record' is in 'visitedRecords', go to 1, else, insert it into 'visitedRecords'
	 * 3. Verify the 'record' on the rest of children by using their verifyByRandomAccess(...) API
	 * 3.1. If the 'record' is verified, move to 4.
	 * 3.2. else, move to 1.
	 * 4. prepare the record item (score should be calculated here)
	 * 4.1. if 'topRecordToReturn' == NULL, 'topRecordToReturn' = 'record'
	 * 4.2. else,
	 * 4.2.1. if 'topRecordToReturn'.score < 'record'.score, add 'topRecordToReturn' to candidatesList and
	 * ---------- set 'topRecordToReturn' = 'record'
	 * 4.2.2. else, just add 'record' to candidatesList
	 * 5. maxScore = getMaximumScoreOfUnvisitedRecords()
	 * 5.1. if maxScore < 'topRecordToReturn'.score, STOP, return 'topRecordToReturn'
	 * 5.2. else, go to 1
	 */

	PhysicalPlanRecordItem * topRecordToReturn = NULL;

	// Part 1.
	if(candidatesList.size() > 0){
		// 1.
		std::sort(candidatesList.begin() ,candidatesList.end() , PhysicalPlanRecordItemComparator()); // change candidatesList to a priority queue
		// 2.
		topRecordToReturn= candidatesList.at(0);
		candidatesList.erase(candidatesList.begin());
		float maxScore = 0;
		if( getMaximumScoreOfUnvisitedRecords(maxScore) == false){
			listsHaveMoreRecordsInThem = false;
		}
		if(maxScore < topRecordToReturn->getRecordRuntimeScore()){
			return topRecordToReturn;
		}
	}

	// 3.
	if( listsHaveMoreRecordsInThem == false ) {
		return topRecordToReturn;
	}

	// Part2.
	while(true){
		//1.
		unsigned childToGetNextRecordFrom = getNextChildForSequentialAccess(); // this function implements Round robin
		//2.
		PhysicalPlanRecordItem * nextRecord = getNextRecordOfChild(childToGetNextRecordFrom,params);
		//2.1.
		if(nextRecord == NULL){
			listsHaveMoreRecordsInThem = false;
			return topRecordToReturn;
		}
		//2.2.
		if(std::find(visitedRecords.begin(),visitedRecords.end(),nextRecord->getRecordId()) == visitedRecords.end()){
			visitedRecords.push_back(nextRecord->getRecordId());
		}else{ // already visited
			continue;
		}

		//3.
        std::vector<float> runTimeTermRecordScores;
        std::vector<float> staticTermRecordScores;
        std::vector<TrieNodePointer> termRecordMatchingKeywords;
        std::vector<unsigned> attributeBitmaps;
        std::vector<unsigned> prefixEditDistances;
        std::vector<unsigned> positionIndexOffsets;
		if(verifyRecordWithChildren(nextRecord, childToGetNextRecordFrom,  runTimeTermRecordScores, staticTermRecordScores,
				termRecordMatchingKeywords, attributeBitmaps, prefixEditDistances , positionIndexOffsets, params ) == false){
			continue;	// 3.1. and 3.2.
		}
		// from this point, nextRecord is a candidate
		//4.
		// set the members
		nextRecord->setRecordMatchAttributeBitmaps(attributeBitmaps);
		nextRecord->setRecordMatchEditDistances(prefixEditDistances);
		nextRecord->setRecordMatchingPrefixes(termRecordMatchingKeywords);
		nextRecord->setPositionIndexOffsets(positionIndexOffsets);
		// nextRecord->setRecordStaticScore() Should we set static score as well ?
		nextRecord->setRecordRuntimeScore(computeAggregatedRuntimeScoreForAnd( runTimeTermRecordScores));

		// 4.1
		if(topRecordToReturn == NULL){
			topRecordToReturn = nextRecord;
		}else{ // 4.2.
			if(topRecordToReturn->getRecordRuntimeScore() < nextRecord->getRecordRuntimeScore()){//4.2.1.
				candidatesList.push_back(topRecordToReturn);
				topRecordToReturn = nextRecord;
			}else{ // 4.2.2.
				candidatesList.push_back(nextRecord);
			}
		}

		//5.
		float maxScore = 0;
		if( getMaximumScoreOfUnvisitedRecords(maxScore) == false){
			listsHaveMoreRecordsInThem = false;
			break;
		}
		if(maxScore < topRecordToReturn->getRecordRuntimeScore()){ // 5.1
			break;
		}
		// 5.2: go to the beginning of the loop again
	}
	return topRecordToReturn;

}
bool MergeTopKOperator::close(PhysicalPlanExecutionParameters & params){
	queryEvaluator = NULL;
	candidatesList.clear();
	childRoundRobinOffset = 0;
	listsHaveMoreRecordsInThem = true;
	nextItemsFromChildren.clear();
	queryEvaluator = NULL;
	visitedRecords.clear();

	// close the children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}

	return true;
}
bool MergeTopKOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {

	// move on children and if at least on of them verifies the record return true
	vector<float> runtimeScore;
	// static score is ignored for now
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		bool resultOfThisChild =
				this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
		runtimeScore.push_back(parameters.runTimeTermRecordScore);
		if(resultOfThisChild == false){
			return false;
		}
	}
	parameters.runTimeTermRecordScore = computeAggregatedRuntimeScoreForAnd(runtimeScore);

	return true;
}


PhysicalPlanRecordItem * MergeTopKOperator::getNextRecordOfChild(unsigned childOffset , const PhysicalPlanExecutionParameters & params){
	ASSERT(childOffset < this->nextItemsFromChildren.size());
	PhysicalPlanRecordItem * toReturn = nextItemsFromChildren.at(childOffset);
	nextItemsFromChildren.at(childOffset) = this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
	return toReturn;
}

unsigned MergeTopKOperator::getNextChildForSequentialAccess(){
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	unsigned toReturn = this->childRoundRobinOffset;
	this->childRoundRobinOffset = ( this->childRoundRobinOffset + 1 ) % numberOfChildren;
	return toReturn;
}


bool MergeTopKOperator::verifyRecordWithChildren(PhysicalPlanRecordItem * recordItem, unsigned childOffsetOfRecord ,
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
		if(childOffset == childOffsetOfRecord){
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
			PhysicalPlanRandomAccessVerificationParameters parameters;
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

    bool validForwardList;
    this->queryEvaluator->getForwardIndex()->getForwardList(recordItem->getRecordId(), validForwardList);
    if (validForwardList) {
    	return true;
    }
	return false;

}

bool MergeTopKOperator::getMaximumScoreOfUnvisitedRecords(float & score){
	// we just get the summation of all nextRecords in nextItemsFromChildren
	score = 0;
	for(vector<PhysicalPlanRecordItem * >::iterator nextRecord = nextItemsFromChildren.begin() ;
			nextRecord != nextItemsFromChildren.end(); ++nextRecord){
		if(*nextRecord == NULL){
			return false;
		}
		score += (*nextRecord)->getRecordRuntimeScore();
	}
	return true;
}

float MergeTopKOperator::computeAggregatedRuntimeScoreForAnd(std::vector<float> runTimeTermRecordScores){

	float resultScore = 0;

	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
		resultScore += *(score);
	}
	return resultScore;
}

void MergeTopKOperator::initializeNextItemsFromChildren(PhysicalPlanExecutionParameters & params){
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		PhysicalPlanRecordItem * recordItem =
				this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
		if(recordItem == NULL){
			listsHaveMoreRecordsInThem = false;
		}
		this->nextItemsFromChildren.push_back(recordItem);
	}
}

//###################################### Optimization Node ####################################################//
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned MergeTopKOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	//TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned MergeTopKOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned MergeTopKOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	//TODO
}
void MergeTopKOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void MergeTopKOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be sorted by score
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
PhysicalPlanNodeType MergeTopKOptimizationOperator::getType() {
	return PhysicalPlanNode_MergeTopK;
}
bool MergeTopKOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
			case PhysicalPlanNode_UnionLowestLevelSimpleScanOperator:
				// TopK should connect to InvertedIndex only by TVL
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
