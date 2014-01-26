
#include "PhysicalOperators.h"
#include "cmath"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByScoreOperator::SortByScoreOperator() {
}
SortByScoreOperator::~SortByScoreOperator(){
}
bool SortByScoreOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);

	// open the single child
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator,params);

	// now get all the records from the child and sort them
	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			break;
		}
		// if topKBestRecords has less than K records insert this record into topKBestRecords
		// ---- and if the size becomes K heapify it
		// else
		// check to see if this record is smaller than top record of topKBestRecords heap (minHeap)
		// if it's smaller, then insert it in recordsAfterTopK,
		// otherwise, pop the smallest element and insert it into recordsAfterTopK and insert this new record in topKBestRecords
		if(topKBestRecords.size() < params.k){
			topKBestRecords.push_back(nextRecord);
			if(topKBestRecords.size() == params.k){
				// heapify
				std::make_heap(topKBestRecords.begin(),topKBestRecords.end(), SortByScoreOperator::SortByScoreRecordMinHeapComparator() );
			}
		}else{ // we must decide whether we want to add this record to top K records or not
			PhysicalPlanRecordItem * kthBestRecordSoFar = topKBestRecords.front();
			if(SortByScoreOperator::SortByScoreRecordMinHeapComparator()(kthBestRecordSoFar , nextRecord) == true){  // kthBestRecordSoFar > nextRecord
				recordsAfterTopK.push_back(nextRecord);
			}else{ // kthBestRecordSoFar < nextRecord
				std::pop_heap (topKBestRecords.begin(),topKBestRecords.end() , SortByScoreOperator::SortByScoreRecordMinHeapComparator());
				topKBestRecords.pop_back();
				topKBestRecords.push_back(nextRecord);
				std::push_heap (topKBestRecords.begin(),topKBestRecords.end(), SortByScoreOperator::SortByScoreRecordMinHeapComparator());
				recordsAfterTopK.push_back(kthBestRecordSoFar);
			}
		}
	}


	// sort the topKBestRecords vector in ascending order (we should use maxHeap comparator)
	// and we get the records from the tail because it's easier to remove the last element of vector
	std::sort(topKBestRecords.begin(), topKBestRecords.end() , SortByScoreOperator::SortByScoreRecordMaxHeapComparator() );

	isRecordsAfterTopKVectorSorted = false;
	return true;
}
PhysicalPlanRecordItem * SortByScoreOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(topKBestRecords.size() > 0){
		// get the next record to return
		PhysicalPlanRecordItem * toReturn = topKBestRecords.at(topKBestRecords.size()-1);
		topKBestRecords.pop_back();
		return toReturn;
	}
	// topKBestRecords vector was exhausted just in the last getNext call
	// so now we should heapify the rest of records one time
	if(isRecordsAfterTopKVectorSorted == false){

		// make a max heap of these records
		std::make_heap(recordsAfterTopK.begin(),recordsAfterTopK.end(), SortByScoreOperator::SortByScoreRecordMaxHeapComparator());
		isRecordsAfterTopKVectorSorted = true;
	}

	if(recordsAfterTopK.size() == 0){
		return NULL;
	}

	// get the next record to return
	PhysicalPlanRecordItem * toReturn = recordsAfterTopK.front();
	std::pop_heap(recordsAfterTopK.begin(),recordsAfterTopK.end(), SortByScoreOperator::SortByScoreRecordMaxHeapComparator());
	recordsAfterTopK.pop_back();

	return toReturn;
}
bool SortByScoreOperator::close(PhysicalPlanExecutionParameters & params){
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	recordsAfterTopK.clear();
	return true;
}

string SortByScoreOperator::toString(){
	string result = "SortByScoreOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool SortByScoreOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	/*
	 * cost :
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + cost(make_heap)
	 * =
	 * O(1) + cost(child's open) + (cost(child's getNext) + O(1))*estimatedNumberOfResults + estimatedNumberOfResultsOfChild
	 */
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(2);
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params); // cost(child's open)
	// cost of fetching all the child's records
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost.addFunctionCallCost(2 * estimatedNumberOfResults);
	resultCost.addInstructionCost(estimatedNumberOfResults);
	resultCost = resultCost +
			(this->getChildAt(0)->getCostOfGetNext(params).cost) * estimatedNumberOfResults; // (cost(child's getNext) + O(1))*estimatedNumberOfResults
	// sorting
	resultCost.addMediumFunctionCost(); // sort
	resultCost.addSmallFunctionCost(estimatedNumberOfResults - params.k); // we assume make_heap calls estimatedNumberOfResults small functions
	resultCost.addSmallFunctionCost(log2((double)params.k + 1) * params.k);

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * cost : O(1) + log(estimatedNumberOfResults)
	 */
	PhysicalPlanCost resultCost;
	resultCost.addSmallFunctionCost(5);
	resultCost.addInstructionCost();
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	if(params.k < estimatedNumberOfResults){
		estimatedNumberOfResults = params.k;
	}
	resultCost.addSmallFunctionCost((unsigned)(log2((double)estimatedNumberOfResults + 1))); // + 1 is to avoid 0
	// we assume make_heap calls estimatedNumberOfResults small functions
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost.addFunctionCallCost(2);
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(2);
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void SortByScoreOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void SortByScoreOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// no input property is required for this operator
}
PhysicalPlanNodeType SortByScoreOptimizationOperator::getType() {
	return PhysicalPlanNode_SortByScore;
}
bool SortByScoreOptimizationOperator::validateChildren(){
	for(unsigned i = 0 ; i < getChildrenCount() ; i++){
		PhysicalPlanOptimizationNode * child = getChildAt(i);
		PhysicalPlanNodeType childType = child->getType();
		switch (childType) {
			case PhysicalPlanNode_RandomAccessTerm:
			case PhysicalPlanNode_RandomAccessAnd:
			case PhysicalPlanNode_RandomAccessOr:
			case PhysicalPlanNode_RandomAccessNot:
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
