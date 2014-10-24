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

using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

bool userFeedbackInfoComparator(const UserFeedbackInfo& lhs, const UserFeedbackInfo &rhs) {
	// ascending order of record id
	if (lhs.recordId < rhs.recordId) {
		return true;
	}
//	else if (lhs.recordId == rhs.recordId) {
//		return lhs.feedbackFrequency > rhs.feedbackFrequency;
//	}
	else
		return false;
};

FeedbackIndex::FeedbackIndex() {
	maxFedbackInfoCountPerQuery = 20;  // TODO: create new config setting
	queryTrie = new Trie();
	queryTrie->commit();
	feedbackListIndexVector = new cowvector<UserFeedbackList>();
	feedbackListIndexVector->commit();
	saveIndexFlag = false;
}

void FeedbackIndex::addFeedback(const string& query, unsigned recordId, unsigned timestamp) {
	boost::unique_lock<boost::mutex> lock(writerLock);
	UserFeedbackInfo feedbackInfo = { recordId, 1, timestamp};
	unsigned feedbackListIndex;
	queryTrie->addKeyword_ThreadSafe(query, feedbackListIndex);
	unsigned feedbackListIndexWriteViewSize = feedbackListIndexVector->getWriteView()->size();
	if (feedbackListIndex >= feedbackListIndexWriteViewSize) {
		UserFeedbackList newList = new cowvector<UserFeedbackInfo>(maxFedbackInfoCountPerQuery * 1.5);
		// commit to separate read view and write view.
		newList->commit();
		feedbackListIndexVector->getWriteView()->at(feedbackListIndex) = newList;

	}

	UserFeedbackList feedbackList = feedbackListIndexVector->getWriteView()->at(feedbackListIndex);
	boost::shared_ptr<vectorview<UserFeedbackInfo> > feedbackInfoListReadView;
	feedbackList->getReadView(feedbackInfoListReadView);
	unsigned readViewSize = feedbackInfoListReadView->size();
	unsigned writeViewSize = feedbackList->getWriteView()->size();
	unsigned idx;
	for (idx = readViewSize; idx < writeViewSize; ++idx) {
		if (feedbackList->getWriteView()->getElement(idx).recordId == recordId) {
			// append to existing info for this recordId.
			feedbackList->getWriteView()->at(idx).feedbackFrequency += 1;
			feedbackList->getWriteView()->at(idx).timestamp = timestamp;
			break;
		}
	}

	if (idx == writeViewSize) {
		feedbackList->getWriteView()->at(idx) = feedbackInfo;
	}
	feedBackListsToMerge.insert(feedbackListIndex);
	saveIndexFlag = true;
}

void FeedbackIndex::addFeedback(const string& query, unsigned recordId) {
	addFeedback(query, recordId, time(NULL));
}

bool FeedbackIndex::hasFeedbackDataForQuery(const string& query) {

	boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNodeReadView;
	queryTrie->getTrieRootNode_ReadView(trieRootNodeReadView);

	const TrieNode *terminalNode = queryTrie->getTrieNodeFromUtf8String(trieRootNodeReadView->root, query);

	if (!terminalNode) {
		return false;
	}
	return true;
}

void FeedbackIndex::getUserFeedbackInfoForQuery(const string& query,
		vector<UserFeedbackInfo>& feedbackInfo) const{

	boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNodeReadView;
	queryTrie->getTrieRootNode_ReadView(trieRootNodeReadView);

	const TrieNode *terminalNode = queryTrie->getTrieNodeFromUtf8String(trieRootNodeReadView->root, query);

	if (!terminalNode) {
		return;
	}
	boost::shared_ptr<vectorview<UserFeedbackList> > feedbackListIndexVectorReadView;
	feedbackListIndexVector->getReadView(feedbackListIndexVectorReadView);
	UserFeedbackList feedbackList = NULL;
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
	boost::unique_lock<boost::mutex> lock(writerLock);
	feedbackListIndexVector->merge();
	// merge each feedback list
	std::set<unsigned>::iterator iter = feedBackListsToMerge.begin();
	while (iter != feedBackListsToMerge.end()) {
		mergeFeedbackList(feedbackListIndexVector->getWriteView()->getElement(*iter));
		++iter;
	}
	feedBackListsToMerge.clear();
	queryTrie->merge(NULL, NULL, 0, false);
}

void FeedbackIndex::mergeFeedbackList(UserFeedbackList feedbackList) {
	boost::shared_ptr<vectorview<UserFeedbackInfo> > feedbackInfoListReadView;
	feedbackList->getReadView(feedbackInfoListReadView);
	unsigned readViewSize = feedbackInfoListReadView->size();
	vectorview<UserFeedbackInfo>* feedbackInfoListWriteView  = feedbackList->getWriteView();
	unsigned writeViewSize = feedbackInfoListWriteView->size();

	ASSERT(writeViewSize > 0);

	if (readViewSize == writeViewSize)
		return;

	// if readview and writeview share same array, then make a new copy of the array.
    if (feedbackInfoListWriteView->getArray() == feedbackInfoListReadView->getArray())
    	feedbackInfoListWriteView->forceCreateCopy();

	//sort the write view
	UserFeedbackInfo *arrayOfUserFeedbackInfo = feedbackInfoListWriteView->getArray()->extent;
	std::sort(arrayOfUserFeedbackInfo + readViewSize, arrayOfUserFeedbackInfo + writeViewSize,
			userFeedbackInfoComparator);

    // sort-merge readview and write part of the array.
    std::inplace_merge (arrayOfUserFeedbackInfo,
    		arrayOfUserFeedbackInfo + readViewSize,
    		arrayOfUserFeedbackInfo + writeViewSize,
    		userFeedbackInfoComparator);

    // combined duplicate entries.
    unsigned writeIdx = 0;
    unsigned prevRecId = arrayOfUserFeedbackInfo[0].recordId;
    for (unsigned i = 1; i < writeViewSize; ++i) {

    	if (arrayOfUserFeedbackInfo[i].recordId == prevRecId) {
    		//accumulate the frequency.
    		arrayOfUserFeedbackInfo[writeIdx].feedbackFrequency += arrayOfUserFeedbackInfo[i].feedbackFrequency;
    		if (arrayOfUserFeedbackInfo[writeIdx].timestamp < arrayOfUserFeedbackInfo[i].timestamp) {
    			arrayOfUserFeedbackInfo[writeIdx].timestamp =
    					arrayOfUserFeedbackInfo[i].timestamp;
    		}
    	} else {
    		++writeIdx;
    		if (writeIdx < i)
    			arrayOfUserFeedbackInfo[writeIdx] = arrayOfUserFeedbackInfo[i];
    		prevRecId = arrayOfUserFeedbackInfo[writeIdx].recordId;
    	}
    }

    if (writeViewSize > maxFedbackInfoCountPerQuery) {
    	feedbackInfoListWriteView->setSize(maxFedbackInfoCountPerQuery);
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
	oa << maxFedbackInfoCountPerQuery;
	oa << feedbackListIndexVector;
	for (unsigned i = 0; i < feedbackListIndexVector->getWriteView()->size(); ++i) {
		oa << feedbackListIndexVector->getWriteView()->at(i);
	}
	ofs.close();
}

void FeedbackIndex::load(const string& directoryName) {

	// serialize the data structures to disk
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
		ia >> maxFedbackInfoCountPerQuery;
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
