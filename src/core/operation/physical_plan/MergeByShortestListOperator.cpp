
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
	this->queryEvaluator = queryEvaluator;

	// prepare the cache key
	string key;
	// "true" means we want to ignore the last leaf node
	// for example for query "terminator AND movie AND trailer"
	// we get the cache key of "terminator AND movie"
	this->getUniqueStringForCache(true , key);
	key += params.isFuzzy?"fuzzy":"exact";


	// CHECK CACHE :
	// 1(if a cache hit). USE CACHE HIT TO START FROM MIDDLE OF LAST EXECUTION
	// 2(else). OR JUST START A FRESH NEW EXECUTION
	// For example :
	// suppose previously we had a query q1: terminator AND movie
	// now if we have a query q2 : terminator AND movie AND trailer
	// we can use the cache entry of "terminator AND movie" to incrementally
	// compute the results for "terminator AND movie AND trailer".
	// If a cache entry exists for q1, we can first verify all the past results with
	// new keyword "trailer", return the verified ones, and then continue the iteration
	// on the shortest list and verify those records with "movie" and "trailer".
	// If the cache entry for q1 doesn't exist, we just start fresh and iterate "terminator"
	// records (assuming it's chosen as the shortest list) and verify those records with
	// "movie" and "trailer".
	// A complete documentation about Cache is here :
	// https://docs.google.com/a/srch2.com/document/d/1Zw4MKSeimsAhbFAWb0VJTB5Msrq_HocVm9Xer4McJuU/edit?disco=AAAAAEouq-w#heading=h.v4ed6yoj3ddf
	ts_shared_ptr<PhysicalOperatorCacheObject> cacheHit;
	if(this->queryEvaluator != NULL && // this is for CTEST ShortestList_Test, in normal cases, queryEvaluator cannot be NULL
			this->queryEvaluator->getCacheManager()->getPhysicalOperatorsCache()->
			getPhysicalOperatorsInfo(key ,  cacheHit)){ // cache has key
		MergeByShortestListCacheEntry * mergeShortestCacheEntry = (MergeByShortestListCacheEntry *) cacheHit.get();
		this->isShortestListFinished = mergeShortestCacheEntry->isShortestListFinished;
		this->indexOfShortestListChild = mergeShortestCacheEntry->indexOfShortestListChild;
		ASSERT(((MergeByShortestListOptimizationOperator *)(this->getPhysicalPlanOptimizationNode()))->getShortestListOffsetInChildren()
				== this->indexOfShortestListChild);
		this->indexOfCandidateListFromCache = 0;

		for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
			params.parentIsCacheEnabled = true; // to tell the child that we are giving cache info and
			if(childOffset >= mergeShortestCacheEntry->children.size()){
				params.cacheObject = NULL;
			}else{
				params.cacheObject = mergeShortestCacheEntry->children.at(childOffset);
			}
			this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
		}

		// get candidate lists from cache
		for(unsigned candidateOffset = 0 ; candidateOffset < mergeShortestCacheEntry->candidatesList.size() ; ++candidateOffset){
			candidateListFromCache.push_back(queryEvaluator->getPhysicalPlanRecordItemFactory()->
								clone(mergeShortestCacheEntry->candidatesList.at(candidateOffset)));
		}
	}else{
		params.parentIsCacheEnabled = true;
		this->isShortestListFinished = false;
		this->indexOfCandidateListFromCache = 0;
		this->indexOfShortestListChild =
				((MergeByShortestListOptimizationOperator *)(this->getPhysicalPlanOptimizationNode()))->getShortestListOffsetInChildren();

		// open children
		for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
			this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
		}
	}


	return true;

}
PhysicalPlanRecordItem * MergeByShortestListOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(isShortestListFinished == true &&
			indexOfCandidateListFromCache >= candidateListFromCache.size()){
		return NULL;
	}

	while(true){
		PhysicalPlanRecordItem * nextRecord = NULL;
		bool recordComesFromCache = false;
		//1. get the next record from shortest list
		if(indexOfCandidateListFromCache < candidateListFromCache.size()){ // get it from cache candidates
			nextRecord = candidateListFromCache.at(indexOfCandidateListFromCache++);
			recordComesFromCache = true;
		}else{ // get a new record from the shortest list
			nextRecord =
					this->getPhysicalPlanOptimizationNode()->
					getChildAt(this->indexOfShortestListChild)->getExecutableNode()->getNext(params);
		}


		if(nextRecord == NULL){
			this->isShortestListFinished = true;
			return NULL;
		}

		// validate the record with other children
		//2.
		std::vector<float> runTimeTermRecordScores;
		std::vector<float> staticTermRecordScores;
		std::vector<TrieNodePointer> termRecordMatchingKeywords;
		std::vector<unsigned> attributeBitmaps;
		std::vector<unsigned> prefixEditDistances;
		std::vector<unsigned> positionIndexOffsets;
		if(recordComesFromCache){ // if record is from cache, it has some result info in it
			runTimeTermRecordScores.push_back(nextRecord->getRecordRuntimeScore());
			staticTermRecordScores.push_back(nextRecord->getRecordStaticScore());
			nextRecord->getRecordMatchingPrefixes(termRecordMatchingKeywords);
			nextRecord->getRecordMatchAttributeBitmaps(attributeBitmaps);
			nextRecord->getRecordMatchEditDistances(prefixEditDistances);
			if(verifyRecordWithChildren(nextRecord,  runTimeTermRecordScores, staticTermRecordScores,
					termRecordMatchingKeywords, attributeBitmaps, prefixEditDistances , positionIndexOffsets, params,
					this->getPhysicalPlanOptimizationNode()->getChildrenCount() - 1) == false){
				continue;	// 2.1. and 2.2.
			}
		}else{
			if(verifyRecordWithChildren(nextRecord,  runTimeTermRecordScores, staticTermRecordScores,
					termRecordMatchingKeywords, attributeBitmaps, prefixEditDistances , positionIndexOffsets, params ) == false){
				continue;	// 2.1. and 2.2.
			}
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
		candidateListForCache.push_back(nextRecord);
		return nextRecord;
	}

	ASSERT(false); // we never reach here
	return NULL; // this return statement is only to suppress compiler warning

}


bool MergeByShortestListOperator::close(PhysicalPlanExecutionParameters & params){


	// prepare cache entry, first prepare key recursively
	string key;
	this->getUniqueStringForCache(false, key);
	key += params.isFuzzy?"fuzzy":"exact";
	vector<PhysicalOperatorCacheObject *> childrenCacheEntries;
	// close the children
	params.parentIsCacheEnabled = true;
	params.cacheObject = NULL;
	for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
		this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->close(params);
		childrenCacheEntries.push_back(params.cacheObject);
		params.cacheObject = NULL;
	}

	// cache
	//1. cache stuff of children is returned through params
	//2. prepare key
	if(this->queryEvaluator != NULL){

		//3. prepare the cache object of self and add children info to it
		MergeByShortestListCacheEntry * mergeShortestCacheEntry = new MergeByShortestListCacheEntry(this->queryEvaluator ,
																	this->indexOfShortestListChild,
																	this->isShortestListFinished,
																	this->candidateListForCache);
		for(unsigned i = 0 ; i < childrenCacheEntries.size() ; ++i){
			mergeShortestCacheEntry->children.push_back(childrenCacheEntries.at(i));
		}
		//4. put <key, this object> in the cache
		ts_shared_ptr<PhysicalOperatorCacheObject> cacheEntry;
		cacheEntry.reset(mergeShortestCacheEntry);
		this->queryEvaluator->getCacheManager()->getPhysicalOperatorsCache()->
				setPhysicalOperatosInfo(key , cacheEntry);
	}


	this->isShortestListFinished = false;
	return true;

}

string MergeByShortestListOperator::toString(){
	string result = "MergeByShortestListOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
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
					const PhysicalPlanExecutionParameters & params, unsigned onlyThisChild){

	// move on children and call verifyByRandomAccess
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	unsigned startChildOffset = 0;
	if(onlyThisChild != -1){
		startChildOffset = onlyThisChild;
	}
	unsigned endChildOffset = numberOfChildren;
	if(onlyThisChild != -1){
		endChildOffset = onlyThisChild  + 1;
	}
	for(unsigned childOffset = startChildOffset; childOffset < endChildOffset; ++childOffset){
		if(childOffset == this->indexOfShortestListChild){
			/*
			 * No verification is needed for the shortest list itself, we should only
			 * copy the vefirication info (like matching prefix and editdistance) to the output
			 */
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
			/*
			 * We should verify this record with all children (except for the shortest list one) and if all of them
			 * pass, then we should copy the verification info.
			 */
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
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * If shortest list has 10 records, and the estimated number of results for this AND
	 * is 5, it means 2 records are processed in average per getNext(...)
	 * So, the cost of getNext is
	 * (estimated length of shortest list / estimated number of results of this and) * ( child's getNextCost + sum of verification costs + O(1) ) + O(1)
	 */
	unsigned indexOfShortestList = getShortestListOffsetInChildren();
	unsigned estimatedLengthOfShortestList = this->getChildAt(indexOfShortestList)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
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
		recordProcessingCost = recordProcessingCost +
				this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
	}
	recordProcessingCost.addMediumFunctionCost(); // cost of verify record with children
	recordProcessingCost.addFunctionCallCost(15); // function calls
	recordProcessingCost.addInstructionCost(4); // simple instructions and conditions
	recordProcessingCost.addSmallFunctionCost(5); // small functions like push_back


	recordProcessingCost.cost = (unsigned)( recordProcessingCost.cost * ( (estimatedLengthOfShortestList * 1.0) / (estimatedNumberOfResults) ));
	// we assume results are distributed normally in the shortest list


	return recordProcessingCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addInstructionCost(3 + this->getChildrenCount()); // 3 + number of open calls
	resultCost.addSmallFunctionCost(); // clear()
	resultCost.addFunctionCallCost(2 + 4 * this->getChildrenCount()); // 2 + number of open calls

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost MergeByShortestListOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){

	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(2);
	resultCost.addSmallFunctionCost();

	// cost of opening children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params);
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
