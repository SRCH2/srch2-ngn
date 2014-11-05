/*
 * QueryIndex.h
 *
 *  Created on: Oct 20, 2014
 *      Author: Surendra
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
typedef cowvector<UserFeedbackInfo> UserFeedbackList;

#define CURRENT_FEEDBACK_LIST_INDEX_VERSION 0

struct DoubleLinkedList {
	unsigned queryId;
	unsigned prevIndexId;
	unsigned nextIndexId;
};
class FeedbackIndex {
	// Trie which stores all queries
	Trie *queryTrie;
	// index vector for the Feedback list pointers.
    cowvector<UserFeedbackList *> * feedbackListIndexVector;
    // max feedback info per feedback list
    unsigned maxFeedbackInfoCountPerQuery;
    // write lock
    boost::mutex writerLock;
    // reader/writer lock.
    mutable boost::shared_mutex readerWriterLock;
    // set which stores list to be merged by merge thread.
    std::set<unsigned> feedBackListsToMerge;
    // flag to indicate whether the index has changed and needs to be saved to disk.
    bool saveIndexFlag;
    // max feedback queries in the trie
    unsigned maxCountOfFeedbackQueries;
    // Linked list to track the age/recency of the queries.
    DoubleLinkedList * queryAgeOrder;
    // Head of the list points to oldest query
    unsigned headId;
    // Tail of the list points to recent query
    unsigned tailId;

    // running count of queries in index;
    unsigned totalQueryCount;


public:
    //writers
    void addFeedback(const string& query, unsigned recordId, unsigned timestamp);
    void addFeedback(const string& query, unsigned recordId);

    // readers
    void retrieveUserFeedbackInfoForQuery(const string& query, vector<UserFeedbackInfo>& feedbackInfo) const;

    bool hasFeedbackDataForQuery(const string& query);

    void merge();

    void save(const string& directoryName);

    void load(const string& directoryName);

    bool getSaveIndexFlag() {
    	return saveIndexFlag;
    }

    void setSaveIndexFlag(bool flag) {
    	saveIndexFlag = flag;
    }

    // constructor
    FeedbackIndex(unsigned maxFeedbackInfoCountPerQuery, unsigned maxCountOfFeedbackQueries);
    // destructor
    virtual ~FeedbackIndex();
private:
	void mergeFeedbackList(UserFeedbackList *feedbackList);
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
