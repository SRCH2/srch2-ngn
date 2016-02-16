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

class Indexer;
class IndexReaderWriter;

struct UserFeedbackInfo;
typedef cowvector<UserFeedbackInfo> UserFeedbackList;

#define CURRENT_FEEDBACK_LIST_INDEX_VERSION 0

struct DoubleLinkedListElement {
	unsigned queryId;
	unsigned prevIndexId;
	unsigned nextIndexId;
};
class FeedbackIndex {
	// The record index structure pointer
	IndexReaderWriter *indexer;
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
    // max number of feedback queries in the trie
    unsigned maxCountOfFeedbackQueries;
    // Linked list to track the age/recency of the queries.
    DoubleLinkedListElement * queryAgeOrder;
    // Head of the list points to oldest query
    unsigned headId;
    // Tail of the list points to recent query
    unsigned tailId;

    // Existing count of queries in query trie index;
    unsigned totalQueryCount;

    // flag to indicate wether merge is required.
    bool mergeRequired;


public:
    //writers
    void addFeedback(const string& query, const string& externalRecordId, unsigned timestamp);
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

    bool isMergeRequired() { return mergeRequired; }

    // constructor
    FeedbackIndex(unsigned maxFeedbackInfoCountPerQuery, unsigned maxCountOfFeedbackQueries,
    		Indexer *indexer);
    // destructor
    virtual ~FeedbackIndex();

    bool isTerminalNodeValid(const TrieNode *terminalNode) const{
    	return terminalNode->getInvertedListOffset() != -1;
    }

    void finalize() {
    	queryTrie->finalCommit_finalizeHistogramInformation(NULL, NULL,0);
    }

private:
    // private merge function. Writer lock should be acquired before calling it.
    void _merge();
	void mergeFeedbackList(UserFeedbackList *feedbackList);
	void reassignQueryIdsInFeedbackIndex();
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
