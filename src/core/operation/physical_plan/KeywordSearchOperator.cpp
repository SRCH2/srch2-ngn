

#include "KeywordSearchOperator.h"
#include "../QueryEvaluatorInternal.h"

namespace srch2 {
namespace instantsearch {

bool KeywordSearchOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & p){
	//1. Find the right value for K (if search type is topK)
	bool isFuzzy = logicalPlan->isFuzzy();
	// we set fuzzy to false to the first session which is exact
	logicalPlan->setFuzzy(false);
	PhysicalPlanExecutionParameters params(0, logicalPlan->isFuzzy() , logicalPlan->getExactQuery()->getPrefixMatchPenalty(), logicalPlan->getSearchType());

	// TODO : possible optimization: if we save some records from exact session it might help in fuzzy session
	//2. Apply exact/fuzzy policy and run
	vector<unsigned> resultIds;
	for(unsigned fuzzyPolicyIter=0;fuzzyPolicyIter<2;fuzzyPolicyIter++){ // this for is a two iteration loop, to avoid copying the code for exact and fuzzy

		/*
		 * 1. Use CatalogManager to collect statistics and meta data about the logical plan
		 * ---- 1.1. computes and attaches active node sets for each term
		 * ---- 1.2. estimates and saves the number of results of each internal logical operator
		 * ---- 1.3. ...
		 */
		HistogramManager histogramManager(queryEvaluator);
		histogramManager.annotate(logicalPlan);
		/*
		 * 2. Use QueryOptimizer to build PhysicalPlan and optimize it
		 * ---- 2.1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
		 *           and makes sure inputs and outputs of operators are consistent.
		 * ---- 2.2. Applies optimization rules on the physical plan
		 * ---- 2.3. ...
		 */
		QueryOptimizer queryOptimizer(queryEvaluator,logicalPlan);
		PhysicalPlan physicalPlan(queryEvaluator);
		queryOptimizer.buildAndOptimizePhysicalPlan(physicalPlan);

		unsigned numberOfIterations = logicalPlan->offset + logicalPlan->resultsToRetrieve;

		if(physicalPlan.getSearchType() == SearchTypeTopKQuery){
			if(logicalPlan->getPostProcessingInfo() != NULL){
				if(logicalPlan->getPostProcessingInfo()->getfacetInfo() != NULL
						|| logicalPlan->getPostProcessingInfo()->getSortEvaluator() != NULL){
					if(physicalPlan.getPlanTree()->getPhysicalPlanOptimizationNode()->
							getLogicalPlanNode()->stats->getEstimatedNumberOfResults() > 500){
						numberOfIterations = 500;
					}else{
						numberOfIterations = -1; // to set this to a very big number
					}
				}
			}
		}
		params.k = numberOfIterations;
		//1. Open the physical plan by opening the root
		physicalPlan.getPlanTree()->open(queryEvaluator , params);
		//2. call getNext for K times
		for(unsigned i=0;(physicalPlan.getSearchType() == SearchTypeGetAllResultsQuery ? true : (i < numberOfIterations) );i++){
			PhysicalPlanRecordItem * newRecord = physicalPlan.getPlanTree()->getNext( params);
			if(newRecord == NULL){
				break;
			}
			// check if we are in the fuzzyPolicyIter session, if yes, we should not repeat a record
			if(fuzzyPolicyIter > 0){
				if(find(resultIds.begin(),resultIds.end(),newRecord->getRecordId()) != resultIds.end()){
					continue;
				}
			}
			//
			resultIds.push_back(newRecord->getRecordId());
			results.push_back(newRecord);

		}

		if(isFuzzy == false || results.size() >= numberOfIterations){
			break;
		}else{
			logicalPlan->setFuzzy(true);
			params.isFuzzy = true;
		}
	}


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
