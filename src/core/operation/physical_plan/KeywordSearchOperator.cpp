

#include "KeywordSearchOperator.h"
#include "../QueryEvaluatorInternal.h"
//#include <gperftools/profiler.h>

namespace srch2 {
namespace instantsearch {

void freeStatsOfLogicalPlanTree(LogicalPlanNode * node) {
	if(node == NULL){
		return;
	}
	delete node->stats;
	for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
		freeStatsOfLogicalPlanTree(*child);
	}
}
bool KeywordSearchOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & p){

	struct timespec tstart;
    struct timespec tend;
	clock_gettime(CLOCK_REALTIME, &tstart);

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

			/*
			 * 1. Use CatalogManager to collect statistics and meta data about the logical plan
			 * ---- 1.1. computes and attaches active node sets for each term
			 * ---- 1.2. estimates and saves the number of results of each internal logical operator
			 * ---- 1.3. ...
			 */

			HistogramManager histogramManager(queryEvaluator);
			if (fuzzyPolicyIter > 0) {
				/*
				 *   For the fuzzy run, free the old 'stats' accumulated during the exact run.
				 */
				freeStatsOfLogicalPlanTree(logicalPlan->getTree());
			}
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

			params.k = numberOfResultsToRetrievePolicy(queryEvaluator);

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

				if(resultIds.size() >= params.k  ){
					break;
				}

			}

			physicalPlan.getPlanTree()->close(params);



		if(fuzzyPolicyIter == 0){
			if(isFuzzy == true && results.size() < params.k){
				logicalPlan->setFuzzy(true);
				params.isFuzzy = true;
			}else{
				break;
			}
		}

	}

	clock_gettime(CLOCK_REALTIME, &tend);
	unsigned searchTime = (tend.tv_sec - tstart.tv_sec) * 1000
			+ (tend.tv_nsec - tstart.tv_nsec) / 1000000;
	cout << "\tTime:" << searchTime << endl;


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

/*
 * This function determines the number of times that getNext should be called
 * on the physical plan build inside open of this function. This number depends
 * on our policy related to 1.searchType 2.Having or not having Facet/Sort.
 */
unsigned KeywordSearchOperator::numberOfResultsToRetrievePolicy(QueryEvaluatorInternal * queryEvaluator){
	unsigned decidedNumberOfResultsToFetch = 0;

	// if search type is searchTopK, we only fetch offset + rows results no matter we have facet or not
	if(logicalPlan->getQueryType() == SearchTypeTopKQuery){

		decidedNumberOfResultsToFetch = logicalPlan->offset + logicalPlan->numberOfResultsToRetrieve;

	// if search type is getAll,
	// if expected number of results is greater than a constant we only fetch some number of results
	// otherwise, we get everything.
	}else if(logicalPlan->getQueryType() == SearchTypeGetAllResultsQuery){
		decidedNumberOfResultsToFetch = -1; // a very big number which results in fetching all possible results.

		// but if the expected total number of results is more than a threshold we only search for
		// a certain number of results to keep performance.
		if(logicalPlan->getTree()->stats->getEstimatedNumberOfResults() >
		queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->getAllMaximumNumberOfResults){
			decidedNumberOfResultsToFetch = queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->getAllTopKReplacementK;
		}
	}

	return decidedNumberOfResultsToFetch;
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
