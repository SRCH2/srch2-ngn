/*
 * FeedbackRankingOperator.h
 *
 *  Created on: Oct 21, 2014
 *      Author: srch2
 */

#ifndef FEEDBACKRANKINGOPERATOR_H_
#define FEEDBACKRANKINGOPERATOR_H_

#include <string>
#include "PhysicalPlan.h"
#include "index/FeedbackIndex.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

//class FeedbackRanker {
//	string queryString;
//	const FeedbackIndex *userFeedbackIndex;
//	vector<UserFeedbackInfo> feedbackInfoForQuery;
//	unsigned queryArrivalTime;
//public:
//	FeedbackRanker(const string &query, const FeedbackIndex * feedbackIndex) {
//		this->queryString = query;
//		this->userFeedbackIndex = feedbackIndex;
//	}
//	void init() {
//		userFeedbackIndex->getUserFeedbackInfoForQuery(this->queryString, feedbackInfoForQuery);
//		queryArrivalTime = time(NULL);
//	}
//
//	float getFeedbackBoostForRecord(unsigned recordId) {
//		for (unsigned i = 0; i < feedbackInfoForQuery.size(); ++i) {
//			if (feedbackInfoForQuery[i].recordId == recordId) {
//				float feedbackBoost;
//				/*
//				 *  Feedback boost for a record found in the user feedback list for a query is
//				 *  calculated as.
//				 *                           1
//				 *   FeedbackBoost = 1 + -------------  X f
//				 *                       1 + (t1 - t2)
//				 *
//				 *   Where t1 = timestamp of query arrival ( time of creation of this operator).
//				 *         t2 = most recent timestamp for a record marked as a feedback for this query.
//				 *         f  = number of times a record was submitted as a feedback for this query.
//				 */
//				float timeStampFactor = 1.0 / (1.0 + ((float)(queryArrivalTime - feedbackInfoForQuery[i].timestamp) / 60.0));
//				float frequencyFactor = (float)feedbackInfoForQuery[i].feedbackFrequency;
//				feedbackBoost = 1.0 + timeStampFactor * frequencyFactor;
//				Logger::console("timestamp factor = %f, frequency factor = %f, boost: %f",timeStampFactor, frequencyFactor, feedbackBoost);
//				return feedbackBoost;
//			}
//		}
//		return 1;
//	}
//
//};

class FeedbackRankingOperator : public PhysicalPlanNode {
	string queryString;
	const FeedbackIndex *userFeedbackIndex;
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
#endif /* FEEDBACKRANKINGOPERATOR_H_ */
