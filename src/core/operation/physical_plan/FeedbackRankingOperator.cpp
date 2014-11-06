/*
 * FeedbackRankingOperator.cpp
 *
 *  Created on: Oct 21, 2014
 *      Author: Surendra
 */

#include "FeedbackRankingOperator.h"

namespace srch2 {
namespace instantsearch {

bool userFeedbackInfoComparatorForSearch(const UserFeedbackInfo& lhs, const UserFeedbackInfo& rhs) {
	return lhs.recordId < rhs.recordId;
}

FeedbackRanker::FeedbackRanker(const string &query, const FeedbackIndex * feedbackIndex) {
	this->queryString = query;
	this->userFeedbackIndex = feedbackIndex;
}
void FeedbackRanker::init() {
	userFeedbackIndex->retrieveUserFeedbackInfoForQuery(this->queryString, feedbackInfoForQuery);
	queryArrivalTime = time(NULL);

	maxBoostForThisQuery = 1.0;
	for (unsigned i = 0; i < feedbackInfoForQuery.size(); ++i) {
		unsigned feedbackRecencyInSecs = queryArrivalTime - feedbackInfoForQuery[i].timestamp;
		float feedbackBoost = Ranker::computeFeedbackBoost(feedbackRecencyInSecs, feedbackInfoForQuery[i].feedbackFrequency);
		if (feedbackBoost > maxBoostForThisQuery) {
			maxBoostForThisQuery = feedbackBoost;
		}
	}
}

float FeedbackRanker::getMaxBoostForThisQuery() {
	return maxBoostForThisQuery;
}

float FeedbackRanker::getFeedbackBoostForRecord(unsigned recordId) {
	float feedbackBoost = 1.0;

	UserFeedbackInfo searchInfo;
	searchInfo.recordId = recordId;

	vector<UserFeedbackInfo>::iterator iter;
	// the feedback info list is ordered by record-id in ascending order
	iter  = std::lower_bound(feedbackInfoForQuery.begin(), feedbackInfoForQuery.end(),
			searchInfo, userFeedbackInfoComparatorForSearch);
	bool matchFound =  (iter != feedbackInfoForQuery.end() && iter->recordId == recordId) ;
	if (matchFound) {
		unsigned feedbackRecencyInSecs = queryArrivalTime - iter->timestamp;
		feedbackBoost = Ranker::computeFeedbackBoost(feedbackRecencyInSecs, iter->feedbackFrequency);
	}
	return feedbackBoost;
}

} /* namespace instantsearch */
} /* namespace srch2 */
