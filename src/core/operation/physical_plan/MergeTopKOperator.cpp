
#include "PhysicalOperators.h"
#include "MergeTopKOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// merge with topK /////////////////////////////////////////

MergeTopKOperator::MergeTopKOperator() {
}

MergeTopKOperator::~MergeTopKOperator(){
}
bool MergeTopKOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

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
		nextRecord->setRecordRuntimeScore(params.ranker->computeAggregatedRuntimeScoreForAnd( runTimeTermRecordScores));

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
	candidatesList.clear();
	childRoundRobinOffset = 0;
	listsHaveMoreRecordsInThem = true;
	nextItemsFromChildren.clear();
	visitedRecords.clear();

	// close the children
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
	}

	return true;
}
bool MergeTopKOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
}


PhysicalPlanRecordItem * MergeTopKOperator::getNextRecordOfChild(unsigned childOffset , const PhysicalPlanExecutionParameters & params){
	ASSERT(childOffset < this->nextItemsFromChildren.size());
	PhysicalPlanRecordItem * toReturn = nextItemsFromChildren.at(childOffset);
    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);
	nextItemsFromChildren.at(childOffset) = this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->getNext(params);
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    cout << "TVL get next : " << ts1 << endl;
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
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(2 + 2 * this->getChildrenCount()); // 2 + number of open calls + number of getNext calls
	resultCost.addSmallFunctionCost(2); // clear()
	resultCost.addFunctionCallCost(8 * this->getChildrenCount()); // 4 * (  number of open calls + number of getNext calls

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfOpen(params);
	}

	// cost of initializing nextItems vector
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfGetNext(params);
	}

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * ***** cost =
	 *  costOfVisitingOneRecord * estimatedNumberOfRecordsToVisitForOneCandidate * estimatedNumberOfCandidatesToFindForTop1
	 *
	 * that :
	 * costOfVisitingOneRecord = 1 getNext + (X-1) verifications , i.e. that X is the number of children
	 * NOTE: since getNext cost is different for different children, we use the average. So :
	 * ***** costOfVisitingOneRecord =
	 *       SUM [1<=i<=X]( getNext(i) + SUM[1<=j<=X,j != i](verify(j))) / X
	 * ***** estimatedNumberOfRecordsToVisitForOneCandidate =
	 *       SUM[1<=i<=X](length(i)) / estimatedTotalNumberOfCandidates
	 * ***** estimatedNumberOfCandidatesToFindForTop1 = This value depends on scores, for now we assume it's 10
	 * So :
	 * ******* cost =
	 * 				( ( SUM [1<=i<=X]( getNext(i) + SUM[1<=j<=X,j != i](verify(j))) / X ) + O(1) ) *
	 * 				( (SUM[1<=i<=X](length(i)) / estimatedTotalNumberOfCandidates) + O(1) ) *
	 * 				10
	 */
	double costOfVisitingOneRecord = 0;
	for(unsigned childOffset = 0 ; childOffset < this->getChildrenCount() ; ++childOffset){
		costOfVisitingOneRecord = costOfVisitingOneRecord + this->getChildAt(childOffset)->getCostOfGetNext(params).cost;
		for(unsigned childOffset2 = 0 ; childOffset2 != this->getChildrenCount() ; ++childOffset2){
			if(childOffset == childOffset2){
				continue;
			}
			costOfVisitingOneRecord = costOfVisitingOneRecord + this->getChildAt(childOffset2)->getCostOfVerifyByRandomAccess(params).cost;
		}
	}
	ASSERT(this->getChildrenCount() != 0);
	costOfVisitingOneRecord = costOfVisitingOneRecord / this->getChildrenCount();

	unsigned minOfChildrenLenghts =  this->getChildAt(0)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	for(unsigned childOffset = 1 ; childOffset < this->getChildrenCount() ; ++childOffset){
		if(minOfChildrenLenghts > this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults()){
			minOfChildrenLenghts = this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
		}
	}

	unsigned estimatedTotalNumberOfCandidates = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	double estimatedNumberOfRecordsToVisitForOneCandidate = ((minOfChildrenLenghts * 1.0) / (estimatedTotalNumberOfCandidates + 1)) *
			this->getChildrenCount();

	PhysicalPlanCost resultCost;
	resultCost = resultCost + (unsigned )( ( costOfVisitingOneRecord + 50 ) * estimatedNumberOfRecordsToVisitForOneCandidate );


	resultCost.addMediumFunctionCost(); // finding the records
	return resultCost;

}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(2 + this->getChildrenCount()); // 3 + number of open calls
	resultCost.addSmallFunctionCost(3); // clear()
	resultCost.addFunctionCallCost(4 * this->getChildrenCount()); // 2 + number of open calls

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.addSmallFunctionCost();

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
	}

	return resultCost;
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
