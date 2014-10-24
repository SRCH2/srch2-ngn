/*
 * QueryIndex.cpp
 *
 *  Created on: Oct 20, 2014
 *      Author: srch2
 */

#include "FeedbackIndex.h"

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

} /* namespace instantsearch */
} /* namespace srch2 */
