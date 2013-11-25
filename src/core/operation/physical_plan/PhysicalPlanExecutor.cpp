
#include "PhysicalPlanExecutor.h"



namespace srch2 {
namespace instantsearch {

PhysicalPlanExecutor::PhysicalPlanExecutor(const LogicalPlan * logicalPlan,
		PhysicalPlan & physicalPlan): logicalPlan(logicalPlan),physicalPlan(physicalPlan){
	///TODO
}

PhysicalPlanExecutor::~PhysicalPlanExecutor(){
	// TODO
}

/*
 * Executes the PhysicalPlan and returns the results by saving them in QueryResults
 */
void PhysicalPlanExecutor::execute(QueryResults *queryResults){
	//1. Find the right value for K (if search type is topK)
	unsigned K = findRightValueForK();

	//2. Apply exact/fuzzy policy and run
	//TODO
}

/*
 * Uses k, offset and other information such as searchType and post-processing to calculate the right value for k
 */
unsigned PhysicalPlanExecutor::findRightValueForK(){
	// TODO
}


void PhysicalPlanExecutor::run(bool exactOnly, vector<PhysicalPlanRecordItem *> * results){
	//TODO
	// 1. prepare the instance of PhysicalPlanExecutionParameters
	// 2. keep calling getNext(...) on the root until it returns null and save the records in results vector
}


}
}
