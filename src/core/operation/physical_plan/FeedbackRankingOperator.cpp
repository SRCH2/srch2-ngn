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
	// Get the max feedback boost possible for all the feedback records of current query.
	// This max feedback boost is used in tok-k merge operator for setting upper bound.
	// See top-k merge operator's getNext function for more details.
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
