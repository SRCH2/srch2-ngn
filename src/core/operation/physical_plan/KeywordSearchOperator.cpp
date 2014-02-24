

#include "KeywordSearchOperator.h"
#include "../QueryEvaluatorInternal.h"
//#include <gperftools/profiler.h>

namespace srch2 {
namespace instantsearch {

bool KeywordSearchOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & p){

//	struct timespec tstart;
//	clock_gettime(CLOCK_REALTIME, &tstart);

	//1. Find the right value for K (if search type is topK)
	bool isFuzzy = logicalPlan->isFuzzy();

	// we set fuzzy to false to the first session which is exact
	logicalPlan->setFuzzy(false);
	PhysicalPlanExecutionParameters params(0, logicalPlan->isFuzzy() , logicalPlan->getExactQuery()->getPrefixMatchPenalty(), logicalPlan->getQueryType());
	params.totalNumberOfRecords = queryEvaluator->getForwardIndex()->getTotalNumberOfForwardLists_ReadView();
	//2. Apply exact/fuzzy policy and run
	vector<unsigned> resultIds;
	 // this for is a two iteration loop, to avoid copying the code for exact and fuzzy
	for(unsigned fuzzyPolicyIter = 0 ; fuzzyPolicyIter < 2 ; fuzzyPolicyIter++ ){

//		if(fuzzyPolicyIter == 0){
//			cout << "Exact:\t";
//		}else{
//			cout << "Fuzzy:\t";
//		}
		unsigned numberOfIterations = logicalPlan->offset + logicalPlan->numberOfResultsToRetrieve;
//		for(unsigned planOffset = 0 ; planOffset < 7 ; planOffset ++){
//			//start the timer for search
//			struct timespec tstart;
//			clock_gettime(CLOCK_REALTIME, &tstart);
			/*
			 * 1. Use CatalogManager to collect statistics and meta data about the logical plan
			 * ---- 1.1. computes and attaches active node sets for each term
			 * ---- 1.2. estimates and saves the number of results of each internal logical operator
			 * ---- 1.3. ...
			 */
			//2. Apply exact/fuzzy policy and run
//			vector<unsigned> resultIds;
//			results.clear();

			HistogramManager histogramManager(queryEvaluator);
			histogramManager.annotate(logicalPlan);
			/*
			 * 2. Use QueryOptimizer to build PhysicalPlan and optimize it
			 * ---- 2.1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
			 *           and makes sure inputs and outputs of operators are consistent.
			 * ---- 2.2. Applies optimization rules on the physical plan
			 * ---- 2.3. ...
			 */
			QueryOptimizer queryOptimizer(queryEvaluator);
			PhysicalPlan physicalPlan(queryEvaluator);

			if(physicalPlan.getSearchType() == SearchTypeTopKQuery){
				if(logicalPlan->getPostProcessingInfo() != NULL){
					if(logicalPlan->getPostProcessingInfo()->getfacetInfo() != NULL
							|| logicalPlan->getPostProcessingInfo()->getSortEvaluator() != NULL
							|| logicalPlan->getPostProcessingInfo()->getPhraseSearchInfoContainer() != NULL){
						if(physicalPlan.getPlanTree()->getPhysicalPlanOptimizationNode()->
								getLogicalPlanNode()->stats->getEstimatedNumberOfResults() > 500){
							numberOfIterations = 500;
						}else{
							numberOfIterations = -1; // to set this to a very big number
						}
					}
				}
			}else if(physicalPlan.getSearchType() == SearchTypeGetAllResultsQuery){
				if(physicalPlan.getPlanTree()->getPhysicalPlanOptimizationNode()->
						getLogicalPlanNode()->stats->getEstimatedNumberOfResults() >
				queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->getAllMaximumNumberOfResults){
					numberOfIterations = queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->getAllTopKReplacementK;
				}
			}
			params.k = numberOfIterations;
			params.cacheObject = NULL;
			physicalPlan.setExecutionParameters(&params);

			queryOptimizer.buildAndOptimizePhysicalPlan(physicalPlan,logicalPlan,0);
			if(physicalPlan.getPlanTree() == NULL){
				return true;
			}

			//1. Open the physical plan by opening the root
			physicalPlan.getPlanTree()->open(queryEvaluator , params);
			//2. call getNext for K times
			while(true){
				PhysicalPlanRecordItem * newRecord = physicalPlan.getPlanTree()->getNext( params);
				if(newRecord == NULL){
					break;
				}
				if(find(resultIds.begin(),resultIds.end(),newRecord->getRecordId()) != resultIds.end()){
					continue;
				}
				//
				resultIds.push_back(newRecord->getRecordId());
				results.push_back(newRecord);

				if(physicalPlan.getSearchType() != SearchTypeGetAllResultsQuery){
					if(results.size() >= numberOfIterations){
						break;
					}
				}

			}

			physicalPlan.getPlanTree()->close(params);


//			// compute elapsed time in ms , end the timer
//			struct timespec tend;
//			clock_gettime(CLOCK_REALTIME, &tend);
//			unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000000
//					+ (tend.tv_nsec - tstart.tv_nsec) / 1000;
//			cout << "Plan" << planOffset << "(" << ts1*1.0/1000 << ")\t" ;
//			queryEvaluator->getPhysicalPlanRecordItemFactory()->refresh();
////			cout << ts1/1000 << endl;
//
//		}
////		cout << endl;

		if(fuzzyPolicyIter == 0){
			if(isFuzzy == true && results.size() < numberOfIterations){
				logicalPlan->setFuzzy(true);
				params.isFuzzy = true;
			}else{
				break;
			}
		}

	}

//	// compute elapsed time in ms , end the timer
//	struct timespec tend;
//	clock_gettime(CLOCK_REALTIME, &tend);
//	unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000000
//			+ (tend.tv_nsec - tstart.tv_nsec) / 1000;
//////			cout << "Plan" << planOffset << "(" << ts1*1.0/1000 << ")\t" ;
//	cout << ts1/1000 << endl ;
	cursorOnResults = 0;
	return true;
}
PhysicalPlanRecordItem * KeywordSearchOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	if(cursorOnResults >= results.size()){
		return NULL;
	}

	return results[cursorOnResults++];
}
bool KeywordSearchOperator::close(PhysicalPlanExecutionParameters & params){
	results.clear();
	cursorOnResults = 0;
	logicalPlan = NULL;
	return true;
}

//As of now, cache implementation doesn't need this function for this operator.
// This code is here only if we want to implement a
//cache module in future that needs it.
string KeywordSearchOperator::toString(){
	ASSERT(false);
	string result = "KeywordSearchOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool KeywordSearchOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	ASSERT(false);
	return false;
}
KeywordSearchOperator::~KeywordSearchOperator(){
	return;
}
KeywordSearchOperator::KeywordSearchOperator(LogicalPlan * logicalPlan){
	this->logicalPlan = logicalPlan;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost KeywordSearchOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost KeywordSearchOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost KeywordSearchOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
PhysicalPlanCost KeywordSearchOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	ASSERT(false);
	return PhysicalPlanCost(0);
}
void KeywordSearchOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	ASSERT(false);
	return;
}
void KeywordSearchOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	ASSERT(false);
	return;
}
PhysicalPlanNodeType KeywordSearchOptimizationOperator::getType(){
	return PhysicalPlanNode_KeywordSearch;
}
bool KeywordSearchOptimizationOperator::validateChildren(){
	ASSERT(false);
	return true;
}


}
}
