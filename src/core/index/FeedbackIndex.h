/*
 * QueryIndex.h
 *
 *  Created on: Oct 20, 2014
 *      Author: srch2
 */

#ifndef __CORE_INDEX_FEEDBACKINDEX_H__
#define __CORE_INDEX_FEEDBACKINDEX_H__

#include <string>
#include <set>
#include "util/cowvector/cowvector.h"
#include "Trie.h"
#include <boost/serialization/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

using namespace std;

namespace srch2 {
namespace instantsearch {

struct UserFeedbackInfo;
typedef cowvector<UserFeedbackInfo>* UserFeedbackList;

#define CURRENT_FEEDBACK_LIST_INDEX_VERSION 0

class FeedbackIndex {
	Trie *queryTrie;
    cowvector<UserFeedbackList> * feedbackListIndexVector;
    unsigned maxFedbackInfoCountPerQuery;
    boost::mutex writerLock;
    std::set<unsigned> feedBackListsToMerge;
    bool saveIndexFlag;
public:
    //writers
    void addFeedback(const string& query, unsigned recordId, unsigned timestamp);
    void addFeedback(const string& query, unsigned recordId);

    // readers
    void getUserFeedbackInfoForQuery(const string& query, vector<UserFeedbackInfo>& feedbackInfo) const;

    bool hasFeedbackDataForQuery(const string& query);

    void merge();

    void save(const string& directoryName);

    void load(const string& directoryName);

    bool shouldSaveIndex() {
    	return saveIndexFlag;
    }

    void shouldSaveIndex(bool flag) {
    	saveIndexFlag = flag;
    }

    // constructor
    FeedbackIndex();
    // destructor
	virtual ~FeedbackIndex();
private:
	void mergeFeedbackList(UserFeedbackList feedbackList);
};

struct UserFeedbackInfo{
	unsigned recordId;           // selected record for a query
	unsigned feedbackFrequency;  // frequency of selecting this record for a query
	unsigned timestamp;          // epoch time

	template <class Archive>
	void serialize(Archive& ar, int version) {
		ar & recordId;
		ar & feedbackFrequency;
		ar & timestamp;
	}
};

} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* __CORE_INDEX_FEEDBACKINDEX_H__ */
