/*
 * feedbackIndex_test.cpp
 *
 *  Created on: Oct 29, 2014
 *      Author: sbisht
 */

#include "index/FeedbackIndex.h"
#include "util/Assert.h"

using namespace srch2::instantsearch;

bool operator == (UserFeedbackInfo& lhs, UserFeedbackInfo& rhs) {
	if (lhs.recordId == rhs.recordId && lhs.timestamp == rhs.timestamp &&
			lhs.feedbackFrequency == rhs.feedbackFrequency) {
		return true;
	} else {
		return false;
	}
}

/*
 *  Test feedback list logic
 */
void test_feedback_list() {

	unsigned maxFeedbackInfoCountPerQuery = 5;
	FeedbackIndex userFeedbackIndex(maxFeedbackInfoCountPerQuery, 10);

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 1: add 4 initial element. Maximum feedback list size is 5
	 *
	 *----------------------------------------------------------------------------------*/

	//add 4 records to write view
	userFeedbackIndex.addFeedback("trip" , 3, 100);
	userFeedbackIndex.addFeedback("trip" , 5, 101);
	userFeedbackIndex.addFeedback("trip" , 2, 102);
	userFeedbackIndex.addFeedback("trip" , 1, 103);

	vector<UserFeedbackInfo> fetchedFeedbackList;
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);
	ASSERT(fetchedFeedbackList.size() == 0);

	// merge to read view.
	userFeedbackIndex.merge();


	vector<UserFeedbackInfo> expectedFeedbackList;
	UserFeedbackInfo info;
	info.recordId = 1;
	info.feedbackFrequency = 1;
	info.timestamp = 103;
	expectedFeedbackList.push_back(info);

	info.recordId = 2;
	info.feedbackFrequency = 1;
	info.timestamp = 102;
	expectedFeedbackList.push_back(info);

	info.recordId = 3;
	info.feedbackFrequency = 1;
	info.timestamp = 100;
	expectedFeedbackList.push_back(info);

	info.recordId = 5;
	info.feedbackFrequency = 1;
	info.timestamp = 101;
	expectedFeedbackList.push_back(info);

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 2: add one more new element. We are still within the maximum limit.
	 *----------------------------------------------------------------------------------*/

	//  add 5th element
	userFeedbackIndex.addFeedback("trip" , 7, 104);

	// merge to read view.
	userFeedbackIndex.merge();

	info.recordId = 7;
	info.feedbackFrequency = 1;
	info.timestamp = 104;
	expectedFeedbackList.push_back(info);

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}


	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 3: add one more new element. Now we have crossed the maximum limit. The
	 *         oldest feedback record should be discarded
	 *----------------------------------------------------------------------------------*/

	expectedFeedbackList.clear();

	info.recordId = 1;
	info.feedbackFrequency = 1;
	info.timestamp = 103;
	expectedFeedbackList.push_back(info);

	info.recordId = 2;
	info.feedbackFrequency = 1;
	info.timestamp = 102;
	expectedFeedbackList.push_back(info);

	info.recordId = 5;
	info.feedbackFrequency = 1;
	info.timestamp = 101;
	expectedFeedbackList.push_back(info);

	info.recordId = 7;
	info.feedbackFrequency = 1;
	info.timestamp = 104;
	expectedFeedbackList.push_back(info);

	info.recordId = 11;
	info.feedbackFrequency = 1;
	info.timestamp = 105;
	expectedFeedbackList.push_back(info);

	userFeedbackIndex.addFeedback("trip" , 11, 105);

	userFeedbackIndex.merge();

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 4 - Add one existing records. Feedback list is within max limit.
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("trip" , 2, 106);
	userFeedbackIndex.merge();

	expectedFeedbackList.clear();

	info.recordId = 1;
	info.feedbackFrequency = 1;
	info.timestamp = 103;
	expectedFeedbackList.push_back(info);

	info.recordId = 2;
	info.feedbackFrequency = 2;
	info.timestamp = 106;
	expectedFeedbackList.push_back(info);

	info.recordId = 5;
	info.feedbackFrequency = 1;
	info.timestamp = 101;
	expectedFeedbackList.push_back(info);

	info.recordId = 7;
	info.feedbackFrequency = 1;
	info.timestamp = 104;
	expectedFeedbackList.push_back(info);

	info.recordId = 11;
	info.feedbackFrequency = 1;
	info.timestamp = 105;
	expectedFeedbackList.push_back(info);

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 5 - Add 2 existing and 2 new records.  2 oldest entries will be deleted.
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("trip" , 3, 107);
	userFeedbackIndex.addFeedback("trip" , 11, 108);
	userFeedbackIndex.addFeedback("trip" , 13, 109);
	userFeedbackIndex.addFeedback("trip" , 5, 110);
	userFeedbackIndex.merge();

	expectedFeedbackList.clear();

	info.recordId = 2;
	info.feedbackFrequency = 2;
	info.timestamp = 106;
	expectedFeedbackList.push_back(info);

	info.recordId = 3;
	info.feedbackFrequency = 1;
	info.timestamp = 107;
	expectedFeedbackList.push_back(info);

	info.recordId = 5;
	info.feedbackFrequency = 2;
	info.timestamp = 110;
	expectedFeedbackList.push_back(info);

	info.recordId = 11;
	info.feedbackFrequency = 2;
	info.timestamp = 108;
	expectedFeedbackList.push_back(info);

	info.recordId = 13;
	info.feedbackFrequency = 1;
	info.timestamp = 109;
	expectedFeedbackList.push_back(info);

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 6 - Add 1 existing record multiple times.  Existing entry is updated.
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("trip" , 5, 111);
	userFeedbackIndex.addFeedback("trip" , 5, 112);
	userFeedbackIndex.addFeedback("trip" , 5, 113);
	userFeedbackIndex.addFeedback("trip" , 5, 114);
	userFeedbackIndex.merge();

	expectedFeedbackList.clear();

	info.recordId = 2;
	info.feedbackFrequency = 2;
	info.timestamp = 106;
	expectedFeedbackList.push_back(info);

	info.recordId = 3;
	info.feedbackFrequency = 1;
	info.timestamp = 107;
	expectedFeedbackList.push_back(info);

	info.recordId = 5;
	info.feedbackFrequency = 6;
	info.timestamp = 114;
	expectedFeedbackList.push_back(info);

	info.recordId = 11;
	info.feedbackFrequency = 2;
	info.timestamp = 108;
	expectedFeedbackList.push_back(info);

	info.recordId = 13;
	info.feedbackFrequency = 1;
	info.timestamp = 109;
	expectedFeedbackList.push_back(info);

	fetchedFeedbackList.clear();
	userFeedbackIndex.retrieveUserFeedbackInfoForQuery("trip", 	fetchedFeedbackList);

	// Test post conditions
	ASSERT(fetchedFeedbackList.size() == expectedFeedbackList.size());
	for (unsigned i = 0; i < expectedFeedbackList.size(); ++i) {
		ASSERT(fetchedFeedbackList[i] == expectedFeedbackList[i]);
	}

}

/*
 *   Test query replacement logic based on LRU.
 */
void test_query_replacement() {
	unsigned maxFeedbackInfoCountPerQuery = 5;
	unsigned maxFeedbackQueriesCount = 5;
	FeedbackIndex userFeedbackIndex(maxFeedbackInfoCountPerQuery, maxFeedbackQueriesCount);

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 1 - Add 4 initial queries.
	 *
	 *          recency order ( oldest to latest)
	 *
	 *           query1 -> query2 -> query3 -> query4
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query1", 101, 100);
	userFeedbackIndex.addFeedback("query2", 201, 101);
	userFeedbackIndex.addFeedback("query3", 301, 102);
	userFeedbackIndex.addFeedback("query4", 401, 103);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query2") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == true);

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 2 - Update 1 existing query and add 1 new query. Now query count in trie
	 *          should reach the maximum count. No query should be replaced yet.
	 *
	 *          recency order ( oldest to latest)
	 *
	 *          query2 -> query3 -> query4 -> query1 -> query5
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query1", 102, 104);
	userFeedbackIndex.addFeedback("query5", 501, 105);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query2") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query5") == true);


	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 3 - add 1 new query = "query6". "query2" should be replaced.
	 *
	 *          new recency order ( oldest to latest)
	 *          query3 -> query4 -> query1 -> query5 -> query6
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query6", 601, 106);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query2") == false);

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query5") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query6") == true);


	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 4 - Add 2 existing query. No query should be replaced.
	 *
	 *          new recency order ( oldest to latest)
	 *          query4 -> query5 -> query6 -> query3 -> query1
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query3", 302, 107);
	userFeedbackIndex.addFeedback("query1", 501, 108);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query5") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query6") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == true);

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 5 - Add 2 new query => "query8", and "query9". "query4", "query5" should be replaced.
	 *
	 *          new recency order ( oldest to latest)
	 *          query6 -> query3 -> query1 -> query8 -> query9
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query8", 801, 109);
	userFeedbackIndex.addFeedback("query9", 802, 110);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == false);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query5") == false);

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query6") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query8") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query9") == true);

	/* ----------------------------------------------------------------------------------
	 *
	 * TEST 6 - Add 5 new query. All old queries should be replaced.
	 *
	 *----------------------------------------------------------------------------------*/

	userFeedbackIndex.addFeedback("query10", 111, 110);
	userFeedbackIndex.addFeedback("query11", 112, 111);
	userFeedbackIndex.addFeedback("query2", 201, 112);
	userFeedbackIndex.addFeedback("query4", 402, 113);
	userFeedbackIndex.addFeedback("query5", 502, 114);

	userFeedbackIndex.merge();

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query6") == false);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query3") == false);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query1") == false);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query8") == false);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query9") == false);

	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query10") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query11") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query2") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query4") == true);
	ASSERT(userFeedbackIndex.hasFeedbackDataForQuery("query5") == true);

}

int main() {
	test_feedback_list();
	cout << "test_feedback_list:  Passed " << endl;
	test_query_replacement();
	cout << "test_query_replacement:  Passed " << endl;
}
