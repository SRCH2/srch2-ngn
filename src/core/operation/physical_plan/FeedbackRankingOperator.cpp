/*
 * FeedbackRankingOperator.cpp
 *
 *  Created on: Oct 21, 2014
 *      Author: Surendra
 */

#include "FeedbackRankingOperator.h"

namespace srch2 {
namespace instantsearch {

bool FeedbackRankingOperator::open(QueryEvaluatorInternal * queryEvaluator,
		PhysicalPlanExecutionParameters & params) {

	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	userFeedbackIndex->retrieveUserFeedbackInfoForQuery(this->queryString, feedbackInfoForQuery);
	queryArrivalTime = time(NULL);
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator,
			params);
}

PhysicalPlanRecordItem * FeedbackRankingOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	PhysicalPlanRecordItem * nextRecord =
			this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
	if(nextRecord == NULL){
		return NULL;
	}
	float runtimeScore = nextRecord->getRecordRuntimeScore();
	unsigned recordId = nextRecord->getRecordId();
	float feedbackBoost = getFeedbackBoostForRecord(recordId);
	runtimeScore *= feedbackBoost;
	nextRecord->setRecordRuntimeScore(runtimeScore);
	return nextRecord;

}

bool userFeedbackInfoComparatorForSearch(const UserFeedbackInfo& lhs, const UserFeedbackInfo& rhs) {
	return lhs.recordId < rhs.recordId;
}
float FeedbackRankingOperator::getFeedbackBoostForRecord(unsigned recordId) {

	float feedbackBoost = 1.0;

	UserFeedbackInfo searchInfo;
	searchInfo.recordId = recordId;

	vector<UserFeedbackInfo>::iterator iter;
	iter  = std::lower_bound(feedbackInfoForQuery.begin(), feedbackInfoForQuery.end(),
			searchInfo, userFeedbackInfoComparatorForSearch);
	bool matchFound =  (iter != feedbackInfoForQuery.end() && iter->recordId == recordId) ;
	if (matchFound) {
		unsigned feedbackRecencyInSecs = queryArrivalTime - iter->timestamp;
		feedbackBoost = Ranker::computeFeedbackBoost(feedbackRecencyInSecs, iter->feedbackFrequency);
	}
	return feedbackBoost;
}

bool FeedbackRankingOperator::close(PhysicalPlanExecutionParameters & params) {
	return true;
}
string FeedbackRankingOperator::toString() {
	return queryString;
}

bool FeedbackRankingOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	ASSERT(false);
	return true;
}
FeedbackRankingOperator::FeedbackRankingOperator(const string &query,
		const FeedbackIndex *feedbackIndex) {
	this->queryString = query;
	this->userFeedbackIndex = feedbackIndex;
}

FeedbackRankingOperator::~FeedbackRankingOperator() {
}



// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost FeedbackRankingOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost FeedbackRankingOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost FeedbackRankingOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
PhysicalPlanCost FeedbackRankingOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	return PhysicalPlanCost(1);
}
void FeedbackRankingOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	return;
}
void FeedbackRankingOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	return;
}
PhysicalPlanNodeType FeedbackRankingOptimizationOperator::getType() {
	return PhysicalPlanNode_FeedbackRanker;
}
bool FeedbackRankingOptimizationOperator::validateChildren(){
	return true;
}


} /* namespace instantsearch */
} /* namespace srch2 */
