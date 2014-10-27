/*
 * FeedbackRankingOperator.h
 *
 *  Created on: Oct 21, 2014
 *      Author: srch2
 */

#ifndef __FEEDBACKRANKINGOPERATOR_H__
#define __FEEDBACKRANKINGOPERATOR_H__

#include <string>
#include "PhysicalPlan.h"
#include "index/FeedbackIndex.h"

using namespace std;
namespace srch2 {
namespace instantsearch {


class FeedbackRankingOperator : public PhysicalPlanNode {
	string queryString;
	const FeedbackIndex *userFeedbackIndex;
	// size should not be more than maxFeedbackInfoCountPerQuery
	vector<UserFeedbackInfo> feedbackInfoForQuery;
	unsigned queryArrivalTime;
public:
	FeedbackRankingOperator(const string &query, const FeedbackIndex * feedbackIndex);
	virtual ~FeedbackRankingOperator();
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
private:
	float getFeedbackBoostForRecord(unsigned recordId);
};

class FeedbackRankingOptimizationOperator : public PhysicalPlanOptimizationNode {
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* __FEEDBACKRANKINGOPERATOR_H__ */
