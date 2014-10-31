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

class FeedbackRanker {
	string queryString;
	const FeedbackIndex *userFeedbackIndex;
	vector<UserFeedbackInfo> feedbackInfoForQuery;
	unsigned queryArrivalTime;
public:
	FeedbackRanker(const string &query, const FeedbackIndex * feedbackIndex) ;
	void init();
	float getFeedbackBoostForRecord(unsigned recordId);
};

} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* __FEEDBACKRANKINGOPERATOR_H__ */
