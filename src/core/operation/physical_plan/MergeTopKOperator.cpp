
#include "PhysicalOperators.h"
#include "MergeTopKOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"
#include <cmath>

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// merge with topK /////////////////////////////////////////

MergeTopKOperator::MergeTopKOperator() {
}

MergeTopKOperator::~MergeTopKOperator(){
}
bool MergeTopKOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

	this->queryEvaluator = queryEvaluator;

	if(this->queryEvaluator != NULL){ // only for mergeTopK ctest queryEvaluator can be NULL
		queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(forwardListDirectoryReadView);
	}

	/*
	 * 0. Cache:
	 * 1. check to see if cache has this query w/o last keyword
	 * 2. if yes, get it from cache and initialize self and children
	 * 3. if no, continue normally
	 */
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	// prepare cache key
	string key;
	this->getUniqueStringForCache(true , key);
	key += params.isFuzzy?"fuzzy":"exact";


	// CHECK CACHE :
	// 1(if a cache hit). USE CACHE HIT TO START FROM MIDDLE OF LAST EXECUTION
	// 2(else). OR JUST START A FRESH NEW EXECUTION
	boost::shared_ptr<PhysicalOperatorCacheObject> cacheHit;
	if(this->queryEvaluator != NULL && // this is for CTEST MergeTopK_Test, in normal cases, queryEvaluator cannot be NULL
			this->queryEvaluator->getCacheManager()->getPhysicalOperatorsCache()->
			getPhysicalOperatorsInfo(key ,  cacheHit)){ // cache has key

		MergeTopKCacheEntry * mergeTopKCacheEntry = (MergeTopKCacheEntry *) cacheHit.get();
		ASSERT(mergeTopKCacheEntry->nextItemsFromChildren.size() == mergeTopKCacheEntry->children.size());
		//1. pass cache to children by putting it in params and opening them
		for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
			params.parentIsCacheEnabled = true; // to tell the child that we are giving cache info and
			// we expect another cache entry in close.
			if(childOffset >= mergeTopKCacheEntry->children.size()){
				params.cacheObject = NULL;
			}else{
				params.cacheObject = mergeTopKCacheEntry->children.at(childOffset);
			}
			this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
		}
		//2. initialize self
		for(unsigned i = 0 ; i < mergeTopKCacheEntry->candidatesList.size() ; ++i){

			if(mergeTopKCacheEntry->children.size() < numberOfChildren){
				bool valid = true;
				std::vector<float> runTimeTermRecordScores;
				std::vector<float>  staticTermRecordScores;
				std::vector<TrieNodePointer> termRecordMatchingKeywords;
				std::vector<unsigned> attributeBitmaps;
				std::vector<unsigned> prefixEditDistances;
				std::vector<unsigned> positionIndexOffsets;
				std::vector<TermType> termTypes;
				// first get all result information which is computed in past
				runTimeTermRecordScores.push_back(mergeTopKCacheEntry->candidatesList.at(i)->getRecordRuntimeScore());
				staticTermRecordScores.push_back(mergeTopKCacheEntry->candidatesList.at(i)->getRecordStaticScore());
				mergeTopKCacheEntry->candidatesList.at(i)->getRecordMatchingPrefixes(termRecordMatchingKeywords);
				mergeTopKCacheEntry->candidatesList.at(i)->getRecordMatchAttributeBitmaps(attributeBitmaps);
				mergeTopKCacheEntry->candidatesList.at(i)->getRecordMatchEditDistances(prefixEditDistances);
				mergeTopKCacheEntry->candidatesList.at(i)->getPositionIndexOffsets(positionIndexOffsets);
				mergeTopKCacheEntry->candidatesList.at(i)->getTermTypes(termTypes);
				// now check the result with the new keyword and if it's a match, append new info to
				// these vectors.
				for(unsigned childOffset = mergeTopKCacheEntry->children.size(); childOffset < numberOfChildren; ++childOffset){
					PhysicalPlanRandomAccessVerificationParameters parameters(params.ranker,
							this->forwardListDirectoryReadView);
					parameters.recordToVerify = mergeTopKCacheEntry->candidatesList.at(i);
					parameters.isFuzzy = params.isFuzzy;
					parameters.prefixMatchPenalty = params.prefixMatchPenalty;
					bool resultOfThisChild =
							this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
					if(resultOfThisChild == false){
						valid = false;
						break;
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
					termTypes.insert(
							termTypes.end(), parameters.termTypes.begin(),parameters.termTypes.end());

				}
				if(valid == false){
					continue;
				}
				// set the members
				mergeTopKCacheEntry->candidatesList.at(i)->setRecordMatchAttributeBitmaps(attributeBitmaps);
				mergeTopKCacheEntry->candidatesList.at(i)->setRecordMatchEditDistances(prefixEditDistances);
				mergeTopKCacheEntry->candidatesList.at(i)->setRecordMatchingPrefixes(termRecordMatchingKeywords);
				mergeTopKCacheEntry->candidatesList.at(i)->setPositionIndexOffsets(positionIndexOffsets);
				// nextRecord->setRecordStaticScore() Should we set static score as well ?
				mergeTopKCacheEntry->candidatesList.at(i)->setRecordRuntimeScore(params.ranker->computeAggregatedRuntimeScoreForAnd( runTimeTermRecordScores));

			}

			candidatesList.push_back(queryEvaluator->getPhysicalPlanRecordItemPool()->
					clone(mergeTopKCacheEntry->candidatesList.at(i)));
			fullCandidatesListForCache.push_back(candidatesList.at(candidatesList.size()-1));
		}

		for(unsigned i = 0 ; i < mergeTopKCacheEntry->nextItemsFromChildren.size() ; ++i){
			if(mergeTopKCacheEntry->nextItemsFromChildren.at(i) == NULL){
				nextItemsFromChildren.push_back(NULL);
			}else{
				nextItemsFromChildren.push_back(queryEvaluator->getPhysicalPlanRecordItemPool()->
						clone(mergeTopKCacheEntry->nextItemsFromChildren.at(i)));
			}
		}
		if(nextItemsFromChildren.size() < this->getPhysicalPlanOptimizationNode()->getChildrenCount()){
			initializeNextItemsFromChildren(params, nextItemsFromChildren.size());
		}
		visitedRecords = mergeTopKCacheEntry->visitedRecords;
		listsHaveMoreRecordsInThem = mergeTopKCacheEntry->listsHaveMoreRecordsInThem;
		childRoundRobinOffset = mergeTopKCacheEntry->childRoundRobinOffset;

	}else{ // there is no cache hit, start a fresh execution
		/*
		 * 1. open all children (no parameters known to pass as of now)
		 * 2. initialize nextRecordItems vector.
		 * 3. candidatesList = empty vector
		 * 4. First assumption is that all lists have records in them.
		 * 5. Round robin should be initialized
		 */
		params.parentIsCacheEnabled = true;
		for(unsigned childOffset = 0 ; childOffset != this->getPhysicalPlanOptimizationNode()->getChildrenCount() ; ++childOffset){
			this->getPhysicalPlanOptimizationNode()->getChildAt(childOffset)->getExecutableNode()->open(queryEvaluator , params);
		}

		initializeNextItemsFromChildren(params);

		// just to make sure
		candidatesList.clear();
		fullCandidatesListForCache.clear();
		listsHaveMoreRecordsInThem = true;
		childRoundRobinOffset = 0;
		visitedRecords.clear();
	}
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

	unsigned numberOfRecordsVisitedForOneResult = 0;
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

		if(visitedRecords.find(nextRecord->getRecordId()) == visitedRecords.end()){
			visitedRecords.insert(nextRecord->getRecordId());
		}else{
			continue;
		}
		numberOfRecordsVisitedForOneResult++;
		//3.
		std::vector<float> runTimeTermRecordScores;
		std::vector<float> staticTermRecordScores;
		std::vector<TrieNodePointer> termRecordMatchingKeywords;
		std::vector<unsigned> attributeBitmaps;
		std::vector<unsigned> prefixEditDistances;
		std::vector<unsigned> positionIndexOffsets;
		std::vector<TermType> termTypes;

		if(verifyRecordWithChildren(nextRecord, childToGetNextRecordFrom,  runTimeTermRecordScores, staticTermRecordScores,
				termRecordMatchingKeywords, attributeBitmaps, prefixEditDistances , positionIndexOffsets, termTypes, params ) == false){
			continue;	// 3.1. and 3.2.
		}


		fullCandidatesListForCache.push_back(nextRecord);
		// from this point, nextRecord is a candidate
		//4.
		// set the members
		nextRecord->setRecordMatchAttributeBitmaps(attributeBitmaps);
		nextRecord->setRecordMatchEditDistances(prefixEditDistances);
		nextRecord->setRecordMatchingPrefixes(termRecordMatchingKeywords);
		nextRecord->setPositionIndexOffsets(positionIndexOffsets);
		nextRecord->setTermTypes(termTypes);
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
	//3. Query evaluator pointer should not be empty in normal query processing. But in CTEST
	//    MergeTopK_Test since only this operator is being tested, this pointer is null. If this pointer is null,
	//    we don't do any caching.
	if(this->queryEvaluator != NULL){

		//3. prepare the cache object of self and add children info to it
		MergeTopKCacheEntry * mergeTopKCacheEntry = new MergeTopKCacheEntry(this->queryEvaluator ,
																		fullCandidatesListForCache ,
																		nextItemsFromChildren,
																		visitedRecords ,
																		listsHaveMoreRecordsInThem ,
																		childRoundRobinOffset);
		for(unsigned i = 0 ; i < childrenCacheEntries.size() ; ++i){
			mergeTopKCacheEntry->children.push_back(childrenCacheEntries.at(i));
		}
		//4. put <key, this object> in the cache
		boost::shared_ptr<PhysicalOperatorCacheObject> cacheEntry;
		cacheEntry.reset(mergeTopKCacheEntry);
		this->queryEvaluator->getCacheManager()->getPhysicalOperatorsCache()->
				setPhysicalOperatosInfo(key , cacheEntry);
	}

	// self closing stuff
	candidatesList.clear();
	childRoundRobinOffset = 0;
	listsHaveMoreRecordsInThem = true;
	nextItemsFromChildren.clear();
	visitedRecords.clear();
	return true;
}

string MergeTopKOperator::toString(){
	string result = "MergeTopKOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool MergeTopKOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return verifyByRandomAccessAndHelper(this->getPhysicalPlanOptimizationNode(), parameters);
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
					std::vector<TermType>& termTypes,
					const PhysicalPlanExecutionParameters & params){

	// move on children and call verifyByRandomAccess
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	for(unsigned childOffset = 0; childOffset < numberOfChildren; ++childOffset){
		if(childOffset == childOffsetOfRecord){
			runTimeTermRecordScores.push_back(recordItem->getRecordRuntimeScore());
			staticTermRecordScores.push_back(recordItem->getRecordStaticScore());
			recordItem->getRecordMatchingPrefixes(termRecordMatchingKeywords);
			recordItem->getRecordMatchAttributeBitmaps(attributeBitmaps);
			recordItem->getRecordMatchEditDistances(prefixEditDistances);
			recordItem->getPositionIndexOffsets(positionIndexOffsets);
			recordItem->getTermTypes(termTypes);
		}else{
			PhysicalPlanRandomAccessVerificationParameters parameters(params.ranker,
					this->forwardListDirectoryReadView);
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
			termTypes.insert(termTypes.end(),parameters.termTypes.begin(),parameters.termTypes.end());
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

void MergeTopKOperator::initializeNextItemsFromChildren(PhysicalPlanExecutionParameters & params, unsigned fromIndex){
	unsigned numberOfChildren = this->getPhysicalPlanOptimizationNode()->getChildrenCount();
	for(unsigned childOffset = fromIndex; childOffset < numberOfChildren; ++childOffset){
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
	 * Theory :
	 * Constants and notation:
	 * T = number of terms
	 * N = total number of records in the data
	 * K = number of top results to find (we calculate the score for K and then divide the result by K)
	 * M = cursor value after termination (it means M records from each list are read)
	 * P[] = the probability array of children (P[i]*N = estimated length of child i)
	 * R = estimated total number of results from these children (R ~= P[0] * ... P[T-1] * N )
	 * Li = the name we use to mention list of child i, i.e. P(Li) = P[i]
	 * Qi = the name of the top portion of list Li that has M records in it,
	 * 					 i.e. Qi is a subset of Li, P(Qi) = M/N
	 * U(A,B) = A|B = the union of lists A and B
	 * I(A,B) = A^B = the intersection of lists A and B, i.e. I(Li) is the intersection of all L1 to LT-1
	 *
	 * Scn[i] = cost of scanning one record from child i
	 * Ran[i] = cost of doing random access for a record on child i
	 *
	 *
	 * How to calculate this cost :
	 *
	 * If min(|Li|) < K, // it means we will finish the shortest list anyways
	 *     M = min(|Li|)
	 *
	 * CN: total number of candidate records visited until cursor = M
	 * CN = |U(Qi)| * P( I(Li) | U(Qi) )
	 * (records which are in the intersection of all lists with the condition that they come from
	 * the top M records of a list)
	 * P( I(Li) | U(Qi) ) = P( I(Li) ^ U(Qi) ) / P( U(Qi) )                                        (1)
	 * I(Li) ^ U(Qi) = (L1 ^ ... ^ Lt-1) ^ (Q1 | ... | Qt-1) =
	 *                 (Q1 ^ L2 ^ L3 ^ ... ^ Lt-1) | .... | (L1 ^ L2 ^ ... ^ Lt-2 ^ Qt-1)
	 * (using the fact that Qi is a subset of Li and pushing intersection into union)
	 *
	 * now if we define Xi = P(L1 ^ L2 ^ ... ^ Li-1 ^ Qi ^ Li+1 ^ ... ^ Lt-1) =
	 * 						 P[0] * P[1] * ... * P[i-1] * M/N * P[i+1] * ... P[t-1]
	 * then P( I(Li) ^ U(Qi) ) = JointProbability(X0, X1, ..., XT)                                 (2)
	 * and we know  P( U(Qi) ) = JointProbability(P(Q0), ..., P(Qt-1)) =
	 * 							 JointProbability(M/N, M/N, ..., M/N)  = 1 - (1 - M/N)^T           (3)
	 * so from (1), (2) and (3) :
	 * P( I(Li) | U(Qi) ) = JointProbability(X0, X1, ..., XT) / (1 - (1 - M/N)^T)                  (4)
	 * and therefore CN = |U(Qi)| * (4) = N * JointProbability(X0, X1, ..., XT)
	 *
	 * NCN = total number of non-candidate records visited until cursor is M
	 * NCN = |U(Qi)| - CN
	 * using (3) :
	 * NCN = N * (1 - (1 - M/N)^T) - CN                                                            (5)
	 *
	 *
	 * Computing Costs :
	 *
	 * C-CAN[i] =
	 * 		the cost of visiting one record from child i when we know the record is a
	 * 		candidate and all forward index verifications return true
	 * C-CAN[i] = Scn[i] + 	Ran[0] + Ran[1] + ... + Ran[i-1] + Ran[i+1] + ... + Ran[T-1]
	 * P-CAN[i] = probability that a record from child i is a candidate  =
	 * 			  R / |Li| = (P[0] * ... * P[T-1] * N ) / (P[i] * N) =
	 * 			  P[0] * ... P[i-1] * P[i+1] * ... P[T-1]                                           (6)
	 * NormP-CAN[i] = probability that a candidate record is from child i =
	 * 			   P-CAN[i] / (P-CAN[0] + ... + P-CAN[T-1])
	 * CAN[i] = number of visited candidates from child i =
	 *          total number of visited candidates * NormP-CAN[i]
	 *          CN * NormP-CAN[i]
	 *
	 * Cost_candidates = cost of visiting all candidate records =
	 *          C-CAN[0] * CAN[0] + .... + C-CAN[T-1] * CAN[T-1]
	 *
	 * C-NCAN[i] =
	 *      the cost of visiting one record from child i when we know the record is not
	 *      a candidate and at least one of the random access verifications will fail.
	 *
	 * C-NCAN[i] = (1-P[0])Rnd[0] +
	 *             P[0]*(1-P[1])*(Rnd[0] + Rnd[1]) +
	 *             P[0]*P[1]*(1-P[2])*(Rnd[0] + Rnd[1 + Rnd[2]]) +
	 *             ... +
	 *             P[0]*...*P[T-2]*(1-P[T-1])*(Rnd[0] + ... + Rnd[T-1])
	 *             - ( P[0] * P[1] * ... * P[i-1] * (1 - P[i]) * (Rnd[0] + ... + Rnd[i]) )
	 *             + Scn[i]  // for this formula, we set P[i] = 1 temporary
	 *                       // because when record comes from P[i] we don't check
	 *                       // child i for random access so we always pass it
	 * P-NCAN[i] = probability that a record from child i is not a candidate  =
	 *             1 - P-CAN[i] = 1 - (6)
	 * NormP-NCAN[i] = probability that a non-candidate record is from child i =
	 *             P-NCAN[i] / (P-NCAN[0] + ... + P-NCAN[T-1])
	 * NCAN[i] = number of visited non-candidates from child i =
	 *           total number of visited non-candidate records * NormP-NCAN[i] =
	 *           NCN * NormP-NCAN[i]
	 *
	 * Cost_noncandidates = cost of visiting all non-candidate records =
	 *         C-NCAN[0] * NCAN[0] + ... + C-NCAN[T-1] * NCAN[T-1]
	 *
	 * cost = (Cost_candidates + Cost_noncandidates) / K
	 *
	 * ----
	 *
	 * But we don't know about M (the cursor value at the end) yet. so we cannot actually
	 * compute the value of costs using this formula. To solve this problem, we should
	 * note that if K+1 results are present in all top-M portions of lists (all Qis) then
	 * we can prove that the score of these K+1 records must be higher than the max combined
	 * score of unvisited records, which means early termination should have happened earlier
	 * and it's a contradiction. Therefore, the maximum size of intersection of all top-M
	 * parts is no more than K. So
	 * |I( Qi )| <= K                                                      -->
	 *                               |Q0 ^ ... ^ Qt-1| <= K                -->
	 *                               N * (M/N * M/N * M/N ... * M/N) <= K  -->
	 *                               M^T / (N^(T-1)) <= K                  -->
	 *                               M <= (K * N^(T-1))^1/T                                         (7)
	 *
	 * so (7) gives the upper bound of M and using this upper bound we can find the upper-bound for cost.
	 *
	 *
	 */


	/*
	 * Constants and notation:
	 * T = number of terms
	 */
	unsigned T = this->getChildrenCount();
	 /*
	  * N = total number of records in the data
	  */
	unsigned N = params.totalNumberOfRecords;
	 /*
	  * R = estimated total number of results from these children (R ~= P[0] * ... P[T-1] * N )
	  */
	unsigned R = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();

	 /*
	  *
	  * K = number of top results to find (we calculate the score for K and then divide the result by K)
	  */
	unsigned K = R/2;
	if(K == 0){
		K = 1;
	}
	 /*
	  * M = cursor value after termination (it means M records from each list is read)
	  *   = (K * N^(T-1))^1/T   (Read the last part of the comment in the beginning of this function)
	  */
	unsigned M = (unsigned)pow(K * pow((double)N, (double)T-1), 1.0 / T);
	 /*
	  * P[] = the probability array of children (P[i]*N = estimated length of child i)
	  */
	vector<float> P;
	unsigned estimatedLengthOfShortestList = -1; // -1 is a very big number
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		P.push_back(this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedProbability());
//		cout << unsigned(P[P.size()-1]*N) << "\t";
		if(estimatedLengthOfShortestList >
				this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults()){
			estimatedLengthOfShortestList =
					this->getChildAt(childOffset)->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
		}
	}

	 /* Li = the name we use to mention list of child i, i.e. P(Li) = P[i]
	 * Qi = the name of the top portion of list Li that has M records in it,
	 * 					 i.e. Qi is a subset of Li, P(Qi) = M/N
	 * U(A,B) = A|B = the union of lists A and B
	 * I(A,B) = A^B = the intersection of lists A and B, i.e. I(Li) is the intersection of all L1 to LT-1
	 *
	 * Scn[i] = cost of scanning one record from child i
	 * Rnd[i] = cost of doing random access for a record on child i
	 */
	vector<double> Scn;
	vector<double> Rnd;
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		Scn.push_back(this->getChildAt(childOffset)->getCostOfGetNext(params).cost);
		Rnd.push_back(this->getChildAt(childOffset)->getCostOfVerifyByRandomAccess(params).cost);
	}

	 /*
	 * How to calculate this cost :
	 *
	 * If min(|Li|) < K, // it means we will finish the shortest list anyways
	 *	M = min(|Li|)
	 */
	if(R <= K || M > estimatedLengthOfShortestList){
		M = estimatedLengthOfShortestList;
	}
	 /*
	 * CN: total number of candidate records visited until cursor = M
	 * CN = |U(Qi)| * P( I(Li) | U(Qi) )
	 * (records which are in the intersection of all lists with the condition that they come from
	 * the top M records of a list)
	 * P( I(Li) | U(Qi) ) = P( I(Li) ^ U(Qi) ) / P( U(Qi) )                                        (1)
	 * I(Li) ^ U(Qi) = (L1 ^ ... ^ Lt-1) ^ (Q1 | ... | Qt-1) =
	 *                 (Q1 ^ L2 ^ L3 ^ ... ^ Lt-1) | .... | (L1 ^ L2 ^ ... ^ Lt-2 ^ Qt-1)
	 * (using the fact that Qi is a subset of Li and pushing intersection into union)
	 *
	 * now if we define Xi = P(L1 ^ L2 ^ ... ^ Li-1 ^ Qi ^ Li+1 ^ ... ^ Lt-1) =
	 * 						 P[0] * P[1] * ... * P[i-1] * M/N * P[i+1] * ... P[t-1]
	 */
	vector<float> X;
	float PiP = 1; // multiplication of all probabilities
	for(unsigned c = 0 ; c < P.size() ; ++c){
		PiP *= P[c];
	}
	for(unsigned c = 0 ; c < P.size() ; ++c){
		X.push_back(((PiP / P[c]) * M) / N);
	}
	 /*
	  * then P( I(Li) ^ U(Qi) ) = JointProbability(X0, X1, ..., XT)                                 (2)
	  */
	float JointProb_X = 0;
	for(unsigned c = 0; c < P.size(); ++c){
		JointProb_X = JointProb_X + X[c] - JointProb_X * X[c];
	}
	 /* and we know  P( U(Qi) ) = JointProbability(P(Q0), ..., P(Qt-1)) =
	 * 							 JointProbability(M/N, M/N, ..., M/N)  = 1 - (1 - M/N)^T           (3)
	 * so from (1), (2) and (3) :
	 * P( I(Li) | U(Qi) ) = JointProbability(X0, X1, ..., XT) / (1 - (1 - M/N)^T)                  (4)
	 * and therefore CN = |U(Qi)| * (4) = N * JointProbability(X0, X1, ..., XT)
	 */
	float JointProb_Q = 1 - pow(1-(M*1.0/N) , (double)T);
	unsigned CN = N * JointProb_X ;
	 /* NCN = total number of non-candidate records visited until cursor is M
	 * NCN = |U(Qi)| - CN
	 * using (3) :
	 * NCN = N * (1 - (1 - M/N)^T) - CN                                                            (5)
	 */
	unsigned NCN = N *JointProb_Q - CN;
	 /*
	 * Computing Costs :
	 *
	 * C-CAN[i] =
	 * 		the cost of visiting one record from child i when we know the record is a
	 * 		candidate and all forward index verifications return true
	 * C-CAN[i] = Scn[i] + 	Rnd[0] + Rnd[1] + ... + Rnd[i-1] + Rnd[i+1] + ... + Rnd[T-1]
	 */
	double SigmaRnd = 0;
	for(unsigned c = 0; c < P.size(); ++c){
		SigmaRnd += Rnd[c];
	}
	vector<double> C_CAN;
	for(unsigned c = 0; c < P.size(); ++c){
		C_CAN.push_back(SigmaRnd - Rnd[c] + Scn[c]);
	}
	 /* P-CAN[i] = probability that a record from child i is a candidate  =
	 * 			  R / |Li| = (P[0] * ... * P[T-1] * N ) / (P[i] * N) =
	 * 			  P[0] * ... P[i-1] * P[i+1] * ... P[T-1]                                           (6)
	 */
	vector<float> P_CAN;
	float SigmaP_CAN = 0;
	for(unsigned c = 0; c < P.size(); ++c){
		P_CAN.push_back(PiP / P[c]);
		SigmaP_CAN += P_CAN[P_CAN.size()-1];
	}
	 /* NormP-CAN[i] = probability that a candidate record is from child i =
	 * 			   P-CAN[i] / (P-CAN[0] + ... + P-CAN[T-1])
	 */
	vector<float> NormP_CAN;
	for(unsigned c = 0; c < P.size(); ++c){
		NormP_CAN.push_back(P_CAN[c] / SigmaP_CAN);
	}
	 /* CAN[i] = number of visited candidates from child i =
	 *          total number of visited candidates * NormP-CAN[i]
	 *          CN * NormP-CAN[i]
	 */
	vector<unsigned> CAN;
	for(unsigned c = 0; c < P.size(); ++c){
		CAN.push_back(CN * NormP_CAN[c]);
	}
	 /* Cost_candidates = cost of visiting all candidate records =
	 *          C-CAN[0] * CAN[0] + .... + C-CAN[T-1] * CAN[T-1]
	 */
	double cost_candidates = 0;
	for(unsigned c = 0; c < P.size(); ++c){
		cost_candidates += C_CAN[c] * CAN[c];
	}
	 /* C-NCAN[i] =
	 *      the cost of visiting one record from child i when we know the record is not
	 *      a candidate and at least one of the random access verifications will fail.
	 *
	 * C-NCAN[i] = (1-P[0])Rnd[0] +
	 *             P[0]*(1-P[1])*(Rnd[0] + Rnd[1]) +
	 *             P[0]*P[1]*(1-P[2])*(Rnd[0] + Rnd[1 + Rnd[2]]) +
	 *             ... +
	 *             P[0]*...*P[T-2]*(1-P[T-1])*(Rnd[0] + ... + Rnd[T-1])
	 *             - ( P[0] * P[1] * ... * P[i-1] * (1 - P[i]) * (Rnd[0] + ... + Rnd[i]) )
	 *             + Scn[i]  // for this formula, we set P[i] = 1 temporary
	 *                       // because when record comes from P[i] we don't check
	 *                       // child i for random access so we always pass it
	 */
	vector<double> C_NCAN;
	for(unsigned c = 0; c < P.size(); ++c){
		double C_NCAN_c = 0;
		float PcBackup = P[c];
		P[c] = 1;
		float PPart = 1;
		unsigned RndPart = 0;
		for(unsigned d = 0; d < P.size(); ++d){
			RndPart += Rnd[d];
			if(d != c){
				C_NCAN_c += PPart * (1 - P[d]) * RndPart;
			}
			PPart *= P[d];
		}
		C_NCAN_c += Scn[c];
		C_NCAN.push_back(C_NCAN_c);
		//
		P[c] = PcBackup;
	}
	 /* P-NCAN[i] = probability that a record from child i is not a candidate  =
	 *             1 - P-CAN[i] = 1 - (6)
	 */
	vector<float> P_NCAN;
	float SigmaP_NCAN = 0 ;
	for(unsigned c = 0; c < P.size(); ++c){
		P_NCAN.push_back(1 - PiP / P[c]);
		SigmaP_NCAN += P_NCAN[P_NCAN.size()-1];
	}
	 /* NormP-NCAN[i] = probability that a non-candidate record is from child i =
	 *             P-NCAN[i] / (P-NCAN[0] + ... + P-NCAN[T-1])
	 */
	vector<float> NormP_NCAN;
	for(unsigned c = 0; c < P.size(); ++c){
		NormP_NCAN.push_back(P_NCAN[c] / SigmaP_NCAN);
	}
	 /* NCAN[i] = number of visited non-candidates from child i =
	 *           total number of visited non-candidate records * NormP-NCAN[i] =
	 *           NCN * NormP-NCAN[i]
	 */
	vector<unsigned> NCAN;
	for(unsigned c = 0; c < P.size(); ++c){
		NCAN.push_back(NCN * NormP_NCAN[c]);
	}
	 /* Cost_noncandidates = cost of visiting all non-candidate records =
	 *         C-NCAN[0] * NCAN[0] + ... + C-NCAN[T-1] * NCAN[T-1]
	 */
	double cost_noncandidates = 0;
	for(unsigned c = 0; c < P.size(); ++c){
		cost_noncandidates += C_NCAN[c] * NCAN[c];
	}
	 /* cost = (Cost_candidates + Cost_noncandidates) / K
	 *
	 */
	PhysicalPlanCost resultCost;
	resultCost.cost = (cost_candidates + cost_noncandidates) / K;
	return resultCost;

}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;

	// cost of closing children
	for(unsigned childOffset = 0 ; childOffset != this->getChildrenCount() ; ++childOffset){
		resultCost = resultCost + this->getChildAt(childOffset)->getCostOfClose(params);
	}

	return resultCost;
}
PhysicalPlanCost MergeTopKOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;

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
