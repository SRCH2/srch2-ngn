/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PhysicalOperators.h"
#include "cmath"
#include "FeedbackRankingOperator.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// sort based on id ////////////////////////////////////////

SortByScoreOperator::SortByScoreOperator() {
}
SortByScoreOperator::~SortByScoreOperator(){
	delete this->feedbackRanker;
}
bool SortByScoreOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);

	if (params.feedbackRanker) {
		// store the ranker object and do not pass it to children.
		this->feedbackRanker = params.feedbackRanker;
		params.feedbackRanker = NULL;
	} else {
		this->feedbackRanker = NULL;
	}

	// open the single child
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator,params);

	// now get all the records from the child and sort them
	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			break;
		}

		if (this->feedbackRanker) {
			float runtimeScore = nextRecord->getRecordRuntimeScore();
			float feedbackBoost = feedbackRanker->getFeedbackBoostForRecord(nextRecord->getRecordId());
			runtimeScore = Ranker::computeFeedbackBoostedScore(runtimeScore, feedbackBoost);
			nextRecord->setRecordRuntimeScore(runtimeScore);
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
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params); // cost(child's open)
	// cost of fetching all the child's records
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	resultCost = resultCost +
			(this->getChildAt(0)->getCostOfGetNext(params).cost) * estimatedNumberOfResults; // (cost(child's getNext) + O(1))*estimatedNumberOfResults
	// sorting
	if(estimatedNumberOfResults < params.k){
		estimatedNumberOfResults = params.k;
	}
	resultCost.cost += estimatedNumberOfResults - params.k;
	resultCost.cost += log2((double)params.k + 1) * params.k;

	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	/*
	 * cost : O(1) + log(estimatedNumberOfResults)
	 */
	PhysicalPlanCost resultCost;
	unsigned estimatedNumberOfResults = this->getLogicalPlanNode()->stats->getEstimatedNumberOfResults();
	if(estimatedNumberOfResults == 0){
		estimatedNumberOfResults = 1;
	}
	if(params.k >= estimatedNumberOfResults){
		resultCost.cost = 1;
		return resultCost;
	}
	resultCost.cost = // amortized cost
			(params.k + (estimatedNumberOfResults - params.k) * log2((double)(estimatedNumberOfResults - params.k))) / estimatedNumberOfResults;
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost SortByScoreOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
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
			case PhysicalPlanNode_RandomAccessGeo:
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
