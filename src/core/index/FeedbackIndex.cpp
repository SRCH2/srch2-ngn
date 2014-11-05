/*
 * QueryIndex.cpp
 *
 *  Created on: Oct 20, 2014
 *      Author: Surendra
 */

#include "FeedbackIndex.h"
#include "util/Logger.h"
#include "serialization/Serializer.h"
#include "IndexUtil.h"
#include <sys/stat.h>
#include "operation/IndexerInternal.h"

using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

bool userFeedbackInfoRecordIdComparator(const UserFeedbackInfo& lhs, const UserFeedbackInfo &rhs) {
	// ascending order of record id
	if (lhs.recordId < rhs.recordId) {
		return true;
	}
	else
		return false;
};

bool userFeedbackInfoTimestampComparator(const UserFeedbackInfo& lhs, const UserFeedbackInfo &rhs) {
	//  comparator for min-heap
	if (lhs.timestamp < rhs.timestamp) {
		return false;
	}
	else {
		return true;
	}
};

FeedbackIndex::FeedbackIndex(unsigned maxFeedbackInfoCountPerQuery,
		unsigned maxCountOfFeedbackQueries, Indexer *indexer) {
	this->maxFeedbackInfoCountPerQuery = maxFeedbackInfoCountPerQuery;
	this->maxCountOfFeedbackQueries = maxCountOfFeedbackQueries;
	this->indexer = reinterpret_cast<IndexReaderWriter *>(indexer);
	totalQueryCount = 0;
	queryTrie = new Trie();
	queryTrie->commit();
	feedbackListIndexVector = new cowvector<UserFeedbackList *>();
	feedbackListIndexVector->commit();
	saveIndexFlag = false;
	queryAgeOrder = new DoubleLinkedList[this->maxCountOfFeedbackQueries];
	headId = tailId = -1;
}

void FeedbackIndex::addFeedback(const string& query, const string& externalRecordId, unsigned timestamp) {
	unsigned internalRecordId;
	INDEXLOOKUP_RETVAL retVal = indexer->lookupRecord(externalRecordId, internalRecordId);
	if (retVal == LU_PRESENT_IN_READVIEW_AND_WRITEVIEW) {
		addFeedback(query, internalRecordId, timestamp);
	}
}
void FeedbackIndex::addFeedback(const string& query, unsigned recordId, unsigned timestamp) {
	boost::unique_lock<boost::mutex> lock(writerLock);
	UserFeedbackInfo feedbackInfo = { recordId, 1, timestamp};
	TrieNode *terminalNode;
	unsigned queryId = queryTrie->addKeyword_ThreadSafe(query, &terminalNode);
	unsigned feedbackListOffset = terminalNode->getInvertedListOffset();
	unsigned feedbackListIndexWriteViewSize = feedbackListIndexVector->getWriteView()->size();
	if (feedbackListOffset >= feedbackListIndexWriteViewSize) {

		/*
		 * It is a new query. First check whether the query count is greater than the maximum allowed
		 * queries. If true, then remove the feedback list of the oldest query and the slot of the
		 * oldest query in the feedbackListIndexVector will be reused for the new query.
		 *
		 * e.g
		 *   Max Query in Trie = 5. Current Query to be added = Q5
		 *
		 *   Linked list of queries ( Q1 is oldest entry [HEAD] and Q3 is the latest entry [TAIL] )
		 *
		 *       ^ ------------|      |-------^
		 *       |             V      V       |
		 *       Q0  |  Q1  |  Q2  |  Q3  |  Q4
		 *       ^      |      |              ^
		 *       |------V      V--------------|
		 *
		 *
		 *   Now query Q5 will replace Q1 and Q1's slot "1" will be used for Q6.  Q0 becomes the
		 *   oldest query and Q6 becomes the latest query.
		 *
		 *       ^ ------------|      |-------^
		 *       |             V      V       |
		 *       Q0  |  Q6  |  Q2  |  Q3  |  Q4
		 *              ^      |      |       ^
		 *              |      V------|-------|
		 *              |-------------V
		 *
		 *  If the query count is not more than maximum query then we just simply add the query to
		 *  the end of the linked list and update the latest entry pointer [TAIL].
		 */

		++totalQueryCount;
		if (totalQueryCount > maxCountOfFeedbackQueries) {

			{
				// take RW lock to block readers.( It is automatically unlocked on scope end)
				boost::unique_lock<boost::shared_mutex>  writerLock(readerWriterLock);

				// Logically delete the oldest query by marked inverted list offset as -1.
				TrieNodePath tp;
				tp.path  = new vector<TrieNode *>();
				queryTrie->getKeywordCorrespondingPathToTrieNode_WriteView(
						queryAgeOrder[headId].queryId, &tp);

				if (tp.path->size()) {
					unsigned lastElement = tp.path->size() - 1;
					tp.path->at(lastElement)->setInvertedListOffset(-1);

					// free feedback list
					delete feedbackListIndexVector->getWriteView()->at(headId);
				} else {
					ASSERT(false);
				}
			}

			// current Query's terminal node will reuse the slot of oldest query
			terminalNode->setInvertedListOffset(headId);
			feedbackListOffset = headId;

			// remove the oldest query by moving head to next element in linked list.
			headId =  queryAgeOrder[headId].nextIndexId;
			queryAgeOrder[headId].prevIndexId = -1;

			--totalQueryCount;
		}

		if (headId == -1 && tailId == -1) {
			// this is the first query
			headId = 0;
			tailId = 0;
			ASSERT(feedbackListOffset == 0);
			queryAgeOrder[feedbackListOffset].prevIndexId = -1;
			queryAgeOrder[feedbackListOffset].nextIndexId = -1;
			queryAgeOrder[feedbackListOffset].queryId = queryId;
		} else {
			// append to end of the tail.
			queryAgeOrder[tailId].nextIndexId = feedbackListOffset;
			queryAgeOrder[feedbackListOffset].prevIndexId = tailId;
			queryAgeOrder[feedbackListOffset].nextIndexId = -1;
			queryAgeOrder[feedbackListOffset].queryId = queryId;
			tailId = feedbackListOffset;
		}

		// allocate memory for feedbackInfo list and assign memory. Allocate enough space for write
		// view to grow.
		UserFeedbackList *newList = new cowvector<UserFeedbackInfo>(maxFeedbackInfoCountPerQuery * 1.5);
		// commit to separate read view and write view.
		newList->commit();
		// at() api automatically increases the capacity cowvector array.
		feedbackListIndexVector->getWriteView()->at(feedbackListOffset) = newList;

	} else if (feedbackListOffset != tailId) {
		// it is an existing query and NOT the latest query then put it to the
		// end of the query linked-list
		if (feedbackListOffset == headId) {
			// move head to next element in linked list.
			headId =  queryAgeOrder[feedbackListOffset].nextIndexId;
			queryAgeOrder[headId].prevIndexId = -1;
		} else  {
			// detach itself from linked list by joining previous and next together
			queryAgeOrder[queryAgeOrder[feedbackListOffset].prevIndexId].nextIndexId =
					queryAgeOrder[feedbackListOffset].nextIndexId;
			queryAgeOrder[queryAgeOrder[feedbackListOffset].nextIndexId].prevIndexId =
					queryAgeOrder[feedbackListOffset].prevIndexId;
		}
		// append current element to end of tail.
		queryAgeOrder[tailId].nextIndexId = feedbackListOffset;
		queryAgeOrder[feedbackListOffset].prevIndexId = tailId;
		queryAgeOrder[feedbackListOffset].nextIndexId = -1;
		tailId = feedbackListOffset;
	}

	UserFeedbackList *feedbackList = feedbackListIndexVector->getWriteView()->at(feedbackListOffset);
	boost::shared_ptr<vectorview<UserFeedbackInfo> > feedbackInfoListReadView;
	feedbackList->getReadView(feedbackInfoListReadView);
	unsigned readViewSize = feedbackInfoListReadView->size();
	unsigned writeViewSize = feedbackList->getWriteView()->size();
	unsigned cursor;  //
	for (cursor = readViewSize; cursor < writeViewSize; ++cursor) {
		if (feedbackList->getWriteView()->getElement(cursor).recordId == recordId) {
			// append to existing info for this recordId.
			feedbackList->getWriteView()->at(cursor).feedbackFrequency += 1;
			feedbackList->getWriteView()->at(cursor).timestamp = timestamp;
			break;
		}
	}

	if (cursor == writeViewSize) {
		// we reached the end of the write view.
		// note: "at()" automatically grows the vector.
		feedbackList->getWriteView()->at(cursor) = feedbackInfo;
	}
	feedBackListsToMerge.insert(feedbackListOffset);
	saveIndexFlag = true;
}

void FeedbackIndex::addFeedback(const string& query, unsigned recordId) {
	addFeedback(query, recordId, time(NULL));
}

// This API determines whether a query is present in the readview of
// the query-trie.
bool FeedbackIndex::hasFeedbackDataForQuery(const string& query) {

	if (query.size() == 0)
		return false;

	boost::shared_lock<boost::shared_mutex>  readerLock(readerWriterLock);

	boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNodeReadView;
	queryTrie->getTrieRootNode_ReadView(trieRootNodeReadView);

	const TrieNode *terminalNode = queryTrie->getTrieNodeFromUtf8String(trieRootNodeReadView->root, query);

	if (!terminalNode) {
		// Terminal node is NULL. Query does not exist in the trie.
		return false;
	}

	// Query exists in the trie but it is logically deleted. Trie will be cleaned up later.
	if (terminalNode->getInvertedListOffset() == -1)
		return false;

	return true;
}

// This API returns a list of feedback info for a query
void FeedbackIndex::retrieveUserFeedbackInfoForQuery(const string& query,
		vector<UserFeedbackInfo>& feedbackInfo) const{

	if (query.size() == 0)
		return;

	boost::shared_lock<boost::shared_mutex>  readerLock(readerWriterLock);

	boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNodeReadView;
	queryTrie->getTrieRootNode_ReadView(trieRootNodeReadView);

	const TrieNode *terminalNode = queryTrie->getTrieNodeFromUtf8String(trieRootNodeReadView->root, query);

	if (!terminalNode) {
		return;
	}
	boost::shared_ptr<vectorview<UserFeedbackList *> > feedbackListIndexVectorReadView;
	feedbackListIndexVector->getReadView(feedbackListIndexVectorReadView);
	UserFeedbackList *feedbackList = NULL;
	if (terminalNode->invertedListOffset < feedbackListIndexVectorReadView->size()) {
		feedbackList = feedbackListIndexVectorReadView->getElement(terminalNode->invertedListOffset);
	} else {
		ASSERT(false);
	}

	if (!feedbackList)
		return;

	boost::shared_ptr<vectorview<UserFeedbackInfo> > feedbackInfoListReadView;
	feedbackList->getReadView(feedbackInfoListReadView);
	for (unsigned i = 0; i < feedbackInfoListReadView->size(); ++i) {
		feedbackInfo.push_back(feedbackInfoListReadView->getElement(i));
	}

}

void FeedbackIndex::merge() {
	/*
	 *  Conceptual view of query feedback indexing
	 *
	 *                    /\
	 *                   /  \
	 *                  /    \
	 *                 /      \
	 *                /        \
	 *               /          \
	 *              / queryTrie  \
	 *             /              \
	 *        ----------------------------
	 *         |
	 *         V
	 *        ----------------------------
	 *        | feedbackListIndexVector  |
	 *        ----------------------------
	 *         |
	 *         V
	 *        | |
	 *        | |
	 *        | |   <--- User Feedback List
	 *        | |
	 *        ---
	 *
	 *  merge data structure starting from low-level towards top-level.
	 */

	boost::unique_lock<boost::mutex> lock(writerLock);
	//1. merge each feedback list from the feedBackListsToMerge set.
	std::set<unsigned>::iterator iter = feedBackListsToMerge.begin();
	while (iter != feedBackListsToMerge.end()) {
		mergeFeedbackList(feedbackListIndexVector->getWriteView()->getElement(*iter));
		++iter;
	}
	feedBackListsToMerge.clear();

	//2. Now merge feedbackListIndexVector
	feedbackListIndexVector->merge();

	if (queryTrie->needToReassignKeywordIds()) {

		// reassign id is not thread safe so we need to grab an exclusive lock
		boost::unique_lock<boost::shared_mutex> lock(readerWriterLock);
		map<TrieNode *, unsigned> trieNodeIdMapper;
		queryTrie->reassignKeywordIds(trieNodeIdMapper);
		for (map<TrieNode *, unsigned>::iterator iter = trieNodeIdMapper.begin();
				iter != trieNodeIdMapper.end(); ++iter) {
			TrieNode *node = iter->first;
			unsigned newKeywordId = iter->second;
			node->setId(newKeywordId);

			if (node->getInvertedListOffset() < maxCountOfFeedbackQueries)
				queryAgeOrder[node->getInvertedListOffset()].queryId =  newKeywordId;
			else
				ASSERT(false);
		}
	}

	//3. Trie should be merged last because it is a top level data structure.
	queryTrie->merge(NULL, NULL, 0, false);
}

void FeedbackIndex::mergeFeedbackList(UserFeedbackList *feedbackList) {
	boost::shared_ptr<vectorview<UserFeedbackInfo> > feedbackInfoListReadView;
	feedbackList->getReadView(feedbackInfoListReadView);
	unsigned readViewSize = feedbackInfoListReadView->size();
	vectorview<UserFeedbackInfo>* feedbackInfoListWriteView  = feedbackList->getWriteView();
	unsigned writeViewSize = feedbackInfoListWriteView->size();

	ASSERT(writeViewSize > 0);

	// defensive check. If true then return because there is nothing to merge.
	if (readViewSize == writeViewSize)
		return;

	// if readview and writeview share same array, then make a new copy of the array.
    if (feedbackInfoListWriteView->getArray() == feedbackInfoListReadView->getArray())
    	feedbackInfoListWriteView->forceCreateCopy();

	//sort the write view
	UserFeedbackInfo *arrayOfUserFeedbackInfo = feedbackInfoListWriteView->getArray()->extent;
	std::sort(arrayOfUserFeedbackInfo + readViewSize, arrayOfUserFeedbackInfo + writeViewSize,
			userFeedbackInfoRecordIdComparator);

    // sort-merge readview and delta-part of the array.
    std::inplace_merge (arrayOfUserFeedbackInfo,
    		arrayOfUserFeedbackInfo + readViewSize,
    		arrayOfUserFeedbackInfo + writeViewSize,
    		userFeedbackInfoRecordIdComparator);

    // combined duplicate entries because we can have a new entry with a record id which is
    // already in the read view. Since the array is sorted, duplicate entries will be in
    // consecutive locations.
    unsigned writeCursor = 0;
    unsigned prevRecId = arrayOfUserFeedbackInfo[0].recordId;
    for (unsigned i = 1; i < writeViewSize; ++i) {

    	if (arrayOfUserFeedbackInfo[i].recordId == prevRecId) {
    		//accumulate the frequency.
    		arrayOfUserFeedbackInfo[writeCursor].feedbackFrequency += arrayOfUserFeedbackInfo[i].feedbackFrequency;
    		if (arrayOfUserFeedbackInfo[writeCursor].timestamp < arrayOfUserFeedbackInfo[i].timestamp) {
    			arrayOfUserFeedbackInfo[writeCursor].timestamp =
    					arrayOfUserFeedbackInfo[i].timestamp;
    		}
    	} else {
    		++writeCursor;
    		if (writeCursor < i)
    			arrayOfUserFeedbackInfo[writeCursor] = arrayOfUserFeedbackInfo[i];
    		prevRecId = arrayOfUserFeedbackInfo[writeCursor].recordId;
    	}
    }

    writeViewSize = writeCursor + 1;
    if (writeViewSize > maxFeedbackInfoCountPerQuery) {

    	// 1. calculate count of extra entries
    	// 2. make min-heap of existing array based on timestamp. So oldest entry will be on top.
    	// 3. pop_heap N times. where N = count of extra entries
    	// 4. re-sort the remaining array based on record-id

    	unsigned entriesToRemove = writeViewSize - maxFeedbackInfoCountPerQuery;
    	make_heap(arrayOfUserFeedbackInfo, arrayOfUserFeedbackInfo + writeViewSize,
    			userFeedbackInfoTimestampComparator);
    	for (unsigned i = 0; i < entriesToRemove; ++i) {
    		// pop_heap copies the top entry to the end of the array and heapify rest of the
    		// array.
    		pop_heap(arrayOfUserFeedbackInfo, arrayOfUserFeedbackInfo + writeViewSize - i,
    				userFeedbackInfoTimestampComparator);
    	}
    	std::sort(arrayOfUserFeedbackInfo, arrayOfUserFeedbackInfo + maxFeedbackInfoCountPerQuery,
    			userFeedbackInfoRecordIdComparator);
    	feedbackInfoListWriteView->setSize(maxFeedbackInfoCountPerQuery);

    } else {
    	feedbackInfoListWriteView->setSize(writeViewSize);
    }
    //reset readview to writeview and create new write view
    feedbackList->merge();
}

FeedbackIndex::~FeedbackIndex() {
	delete queryTrie;
	for (unsigned i = 0; i < feedbackListIndexVector->getWriteView()->size(); ++i) {
		delete feedbackListIndexVector->getWriteView()->getElement(i);
	}
	delete feedbackListIndexVector;
}

void FeedbackIndex::save(const string& directoryName) {

	if (this->queryTrie->isMergeRequired())
		this->queryTrie->merge(NULL, NULL, 0, false);

	// serialize the data structures to disk
	Serializer serializer;
	string queryTrieFilePath = directoryName + "/" + IndexConfig::queryTrieFileName;
	try {
		serializer.save(*this->queryTrie, queryTrieFilePath);
	} catch (exception &ex) {
		Logger::error("Error writing query index file: %s", queryTrieFilePath.c_str());
	}

	string userfeedbackListFilePath = directoryName + "/" + IndexConfig::queryFeedbackFileName;
	std::ofstream ofs(userfeedbackListFilePath.c_str(), std::ios::binary);
	if (!ofs.good())
		throw std::runtime_error("Error opening " + userfeedbackListFilePath);
	boost::archive::binary_oarchive oa(ofs);
	oa << IndexVersion::currentVersion;
	oa << maxFeedbackInfoCountPerQuery;
	oa << maxCountOfFeedbackQueries;
	oa << totalQueryCount;
	oa << headId;
	oa << tailId;
	for (unsigned i = 0; i < totalQueryCount; ++i) {
		oa << queryAgeOrder[i].nextIndexId;
		oa << queryAgeOrder[i].prevIndexId;
		oa << queryAgeOrder[i].queryId;
	}
	oa << feedbackListIndexVector;
	for (unsigned i = 0; i < feedbackListIndexVector->getWriteView()->size(); ++i) {
		oa << feedbackListIndexVector->getWriteView()->at(i);
	}
	ofs.close();
}

void FeedbackIndex::load(const string& directoryName) {

	// serialize the data structures from disk
	Serializer serializer;
	string queryTrieFilePath = directoryName + "/" + IndexConfig::queryTrieFileName;
	struct stat fileStat;

	if (::stat(queryTrieFilePath.c_str(), &fileStat) == -1) {
		Logger::info("query index not found during load.");
		return;
	}

	try {
		serializer.load(*this->queryTrie, queryTrieFilePath);
	} catch (exception &ex) {
		Logger::error("Error writing query index file: %s", queryTrieFilePath.c_str());
	}

	string userfeedbackListFilePath = directoryName + "/" + IndexConfig::queryFeedbackFileName;
	std::ifstream ifs(userfeedbackListFilePath.c_str(), std::ios::binary);
	if (!ifs.good()) {
		Logger::info("query index not found during load.");
	}
	boost::archive::binary_iarchive ia(ifs);
	IndexVersion storedIndexVersion;
	ia >> storedIndexVersion;
	if (IndexVersion::currentVersion == storedIndexVersion) {
		ia >> maxFeedbackInfoCountPerQuery;
		ia >> maxCountOfFeedbackQueries;
		ia >> totalQueryCount;
		ia >> headId;
		ia >> tailId;
		delete queryAgeOrder;
		queryAgeOrder = new DoubleLinkedList[this->maxCountOfFeedbackQueries];
		ASSERT(totalQueryCount <= maxCountOfFeedbackQueries);
		for (unsigned i = 0; i < totalQueryCount; ++i) {
			ia >> queryAgeOrder[i].nextIndexId;
			ia >> queryAgeOrder[i].prevIndexId;
			ia >> queryAgeOrder[i].queryId;
		}
		ia >> feedbackListIndexVector;
		for (unsigned i = 0; i < feedbackListIndexVector->getWriteView()->size(); ++i) {
			ia >> feedbackListIndexVector->getWriteView()->at(i);
		}
		ifs.close();
	} else {
		ifs.close();
		Logger::error(
				"Invalid index file. Either index files are built with a previous version"
				" of the engine or copied from a different machine/architecture.");
		throw exception();
	}
}

} /* namespace instantsearch */
} /* namespace srch2 */
