
// $Id: InvertedIndex.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */

#pragma once
#ifndef __INVERTEDINDEX_H__
#define __INVERTEDINDEX_H__

#include "util/cowvector/cowvector.h"
#include "index/ForwardIndex.h"

//#include <instantsearch/Schema.h>
#include <instantsearch/Ranker.h>
#include "util/Assert.h"
#include "util/Logger.h"
#include "util/RankerExpression.h"
#include "util/half.h"

#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <set>

using std::map;
using std::set;
using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::pair;
using srch2::util::Logger;
using namespace half_float;


namespace srch2
{
namespace instantsearch
{

class Trie;

struct InvertedListElement {
    unsigned recordId;
    unsigned positionIndexOffset;

    InvertedListElement(): recordId(0), positionIndexOffset(0) {};
    InvertedListElement(unsigned _recordId, unsigned _positionIndexOffset): recordId(_recordId), positionIndexOffset(_positionIndexOffset) {};
    InvertedListElement(const InvertedListElement &invertedlistElement)
    {
        if(this != &invertedlistElement)
        {
            this->recordId = invertedlistElement.recordId;
            this->positionIndexOffset = invertedlistElement.positionIndexOffset;
        }
    }
    InvertedListElement& operator=(const InvertedListElement &invertedlistElement)
    {
        if(this != &invertedlistElement)
        {
            this->recordId = invertedlistElement.recordId;
            this->positionIndexOffset = invertedlistElement.positionIndexOffset;
        }
        return *this;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & recordId;
        ar & positionIndexOffset;
    }
};

struct InvertedListIdAndScore {
    unsigned recordId;
    float score;
};


class InvertedListContainer
{
private:
    // Comparator
    class InvertedListElementGreaterThan
    {
    private:
    public:
        InvertedListElementGreaterThan() {
        }

        // this operator should be consistent with two others in TermVirtualList.h and QueryResultsInternal.h
        bool operator() (const InvertedListIdAndScore &lhs, const InvertedListIdAndScore &rhs) const
        {
            return DefaultTopKRanker::compareRecordsGreaterThan(lhs.score,  lhs.recordId,
                    rhs.score, rhs.recordId);
        }
    };

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	// invList should not be NULL. In the debug mode, alert a developer via ASSERT
    	ASSERT(invList != NULL);
        if (invList == NULL)  // In release mode, create new memory.
        	invList = new cowvector<unsigned>();
        // Always use the object reference instead of pointer for boost serialization. During the load phase
        // boost tends to allocate new memory for the pointer leaking the existing one.
        ar & *invList;
    }

public:

    //CowInvertedList *invList;
    cowvector<unsigned> *invList;

    InvertedListContainer() // TODO for serialization. Remove dependency
    {
    	this->invList = new cowvector<unsigned>;
    };

    InvertedListContainer(unsigned capacity)
    {
        this->invList = new cowvector<unsigned>(capacity);
    };

    virtual ~InvertedListContainer()
    {
        delete invList;
    };

    const unsigned getInvertedListElement(unsigned index) const
    {
        shared_ptr<vectorview<unsigned> > readView;
        this->invList->getReadView(readView);
        return readView->getElement(index);
    };

    void getInvertedList(shared_ptr<vectorview<unsigned> >& readview) const
    {
        this->invList->getReadView(readview);
    }

    unsigned getReadViewSize() const
    {
        shared_ptr<vectorview<unsigned> > readView;
        this->invList->getReadView(readView);
        return readView->size();
    };

    unsigned getWriteViewSize() const
    {
        return this->invList->getWriteView()->size();
    };

    void setInvertedListElement(unsigned index, unsigned recordId)
    {
        this->invList->getWriteView()->at(index) = recordId;
    };

    void addInvertedListElement(unsigned recordId)
    {
        this->invList->getWriteView()->push_back(recordId);
    };

    void sortAndMergeBeforeCommit(const unsigned keywordId, const ForwardIndex *forwardIndex, bool needToSortEachInvertedList);

    // return value: # of elements in the final write view
    int sortAndMerge(const unsigned keywordId, ForwardIndex *forwardIndex,
    		shared_ptr<vectorview<ForwardListPtr> >& fwdIdxReadView,
    		vector<InvertedListIdAndScore>& invertedListElements,
    		unsigned totalNumberOfDocuments,
    		RankerExpression *rankerExpression, const Schema *schema);
};

typedef InvertedListContainer* InvertedListContainerPtr;

struct MergeWorkersThreadArgs {
	void* index;        // IndexData pointer used by workers.

	/*
	 * Usage and purpose of perThreadMutex
	 *
	 * 1. Worker locks this mutex when running (awake) and releases it when waiting
	 *    for the condition/signal (sleeping).
	 * 2. The main merge thread MUST acquire this mutex before sending the signal
	 *    to the worker in order to guarantee that the worker is indeed sleeping.
	 *    The main merge thread MUST release this mutex immediately after sending the signal.
	 */
	pthread_mutex_t perThreadMutex;

	pthread_cond_t waitConditionVar;  // condition variable to wake up worker thread.
	unsigned workerId;   // worker Id
	bool isDataReady;    // flag when set true indicates inverted list queue is ready.
	bool stopExecuting;  // flag when set true stops the worker threads.
	bool workerReady;    // flag used to indicate whether worker thread is ready to accept task
	                     // from main merge thread.
};
// Queue which holds data for merge workers.
struct MergeWorkersSharedQueue {
    // Array of pairs of inverted list Id and keyword Id
    pair<unsigned, unsigned> *invertedListKeywordIds;
    unsigned dataLen;   // max size of the array.
    unsigned cursor;    // max value of the array index processed by threads
    boost::mutex _lock;
    MergeWorkersSharedQueue() {
        invertedListKeywordIds = NULL;
        cursor = 0;
        dataLen = 0;
    }
};

class InvertedIndex
{
public:

    //InvertedIndex(PositionIndex *positionIndex);
    InvertedIndex(ForwardIndex *forwardIndex);

    virtual ~InvertedIndex();

    /**
     *
     * Reader Functions called by IndexSearcherInternal and TermVirtualList
     *
     */

    /* Gets the @param invertedListElement in the @param invertedList. Internally, we first get the sub-sequence of invertedIndexVector represented by
     * [invertedList.offset,invertedList.currentHitCount] and return the InvertedListElement at invertedList.offset + cursor.
     * We make assertions to check if offset, offset + currentHitCount is within getTotalLengthOfInvertedLists(). Also, assert to check if currentHitCount > cursor.
     */
    void getInvertedListReadView(shared_ptr<vectorview<InvertedListContainerPtr> > & invertedListDirectoryReadView,
    		const unsigned invertedListId, shared_ptr<vectorview<unsigned> >& readview) const;
    void getInvertedIndexDirectory_ReadView(shared_ptr<vectorview<InvertedListContainerPtr> > & invertedListDirectoryReadView) const;
    void getInvertedIndexKeywordIds_ReadView(shared_ptr<vectorview<unsigned> > & invertedIndexKeywordIdsReadView) const;
    unsigned getInvertedListSize_ReadView(const unsigned invertedListId) const;

    unsigned getRecordNumber() const;

    bool isValidTermPositionHit(shared_ptr<vectorview<ForwardListPtr> > & forwardIndexDirectoryReadView,
    		unsigned forwardListId,
    		unsigned keywordOffset,
    		const vector<unsigned>& filterAttributesList, ATTRIBUTES_OP attrOp,
    		vector<unsigned>& matchingKeywordAttributesList,
            float &termRecordStaticScore) const;

    // return FORWARDLIST_NOTVALID if the forward list is not valid (e.g., already deleted)
    unsigned getKeywordOffset(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
    		shared_ptr<vectorview<unsigned> > & invertedIndexKeywordIdsReadView,
    		unsigned forwardListId, unsigned invertedListOffset) const;

    /**
     * Writer Functions called by IndexerInternal
     */
    void incrementHitCount(unsigned invertedIndexDirectoryIndex);
    void incrementDummyHitCount(unsigned invertedIndexDirectoryIndex);// For Trie Bootstrap
    void addRecord(ForwardList* forwardList, Trie * trie, RankerExpression *rankerExpression,
            const unsigned forwardListOffset, const SchemaInternal *schema,
            const Record *record, const unsigned totalNumberOfDocuments, const KeywordIdKeywordStringInvertedListIdTriple &keywordIdList);

    void initialiseInvertedIndexCommit();
    void commit( ForwardList *forwardList, RankerExpression *rankerExpression,
            const unsigned forwardListOffset, const unsigned totalNumberOfDocuments,
            const Schema *schema, const vector<NewKeywordIdKeywordOffsetTriple> &newKeywordIdKeywordOffsetTriple);

    // When we construct the inverted index from a set of records, in the commit phase we need to sort each inverted list,
    // i.e., needToSortEachInvertedList = true.
	// When we load the inverted index from disk, we do NOT need to sort each inverted list since it's already sorted,
    // i.e., needToSortEachInvertedList = false.
    void finalCommit(bool needToSortEachInvertedList = true);
    void merge(RankerExpression *rankerExpression,  unsigned totalNumberOfDocuments, const Schema *schema, Trie *trie);
    void parallelMerge();
    unsigned workerMergeTask(RankerExpression *rankerExpression,  unsigned totalNumberOfDocuments,
    		const Schema *schema, Trie *trie);
    // Array of per thread arguments. It will be allocated and freed by the main merge thread at runtime.
    MergeWorkersThreadArgs *mergeWorkersArgs;
    MergeWorkersSharedQueue  mergeWorkersSharedQueue;
    // condition variable on which main merge thread waits for workers to finish.
	pthread_cond_t dispatcherConditionVar;
	// main merge thread uses this lock to coordinate with worker threads. When the main thread is
	// waiting on condition then this lock is released. Workers acquire this lock to signal the
	// condition. This is necessary to avoid loss of condition signal.
	pthread_mutex_t dispatcherMutex;
	// the number of threads dedicated for merging inverted lists in parallel.
	unsigned int mergeWorkersCount;
    void setForwardIndex(ForwardIndex *forwardIndex) {
        this->forwardIndex = forwardIndex;
    }

    /**
     * Adds a @param invertedListElement to the @param invertedList. Internally, we first get the sub-sequence of invertedIndexVector represented by
     * [invertedList.offset,invertedList.currentHitCount] and scan for the first InvertedListElement = {0,0}, and then insert @param invertedListElement to it.
     * When we do not find an empty space (which is unlikely in our implementation of index construction), we raise a assertion panic.
     */
    void  addInvertedListElement(unsigned invertedListDirectoryPosition, unsigned recordId);
    unsigned getTotalNumberOfInvertedLists_ReadView() const;

    /**
     * Prints the invertedIndexVector. Used for testing purposes.
     */
    void print_test() const;
    int getNumberOfBytes() const;

    void printInvList(const unsigned invertedListId) const
    {
        shared_ptr<vectorview<InvertedListContainerPtr> > invertedIndexVectorReadView;
        this->invertedIndexVector->getReadView(invertedIndexVectorReadView);
		shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;
		this->forwardIndex->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);

        unsigned readViewListSize = invertedIndexVectorReadView->getElement(invertedListId)->getReadViewSize();

        Logger::error("Print invListid: %d size %d" , invertedListId, readViewListSize);
        InvertedListElement invertedListElement;
        for (unsigned i = 0; i < readViewListSize; i++)
        {
        	invertedIndexVectorReadView->getElement(invertedListId)->getInvertedListElement(i);
            unsigned recordId = invertedListElement.recordId;
            unsigned positionIndexOffset = invertedListElement.positionIndexOffset;
            float score;
            vector<unsigned> tempMatchedAttrsList;
            vector<unsigned> filterAttributes;
            if (isValidTermPositionHit(forwardIndexDirectoryReadView, recordId,
            		positionIndexOffset, filterAttributes, ATTRIBUTES_OP_AND, tempMatchedAttrsList, score) ){
                Logger::debug("%d | %d | %.5f", recordId, positionIndexOffset, score);
            }
        }
    };
    cowvector<unsigned> *getKeywordIds()
	{
    	return keywordIds;
	}

    // Merge is required if the list is not empty
    bool mergeRequired() const  { return !(invertedListKeywordSetToMerge.empty()); }
    /*
     *   This API appends the inverted lists supplied as an input to a set of inverted list ids
     *   that need to be merged.
     */
    void appendInvertedListKeywordIdsForMerge(const vector<pair<unsigned, unsigned> >& invertedListKeywordIds);

private:

    float getIdf(const unsigned totalNumberOfDocuments, const unsigned keywordId) const;
    float computeRecordStaticScore(RankerExpression *rankerExpression, const float recordBoost,
                       const float recordLength, const float idf,
                       const float tfBoostProduct) const;

    cowvector<InvertedListContainerPtr> *invertedIndexVector;
    ReadViewManager<InvertedListContainerPtr> invertedIndexVectorReadViewsMgr;

    //mapping from keywordOffset to keywordId
    cowvector<unsigned> *keywordIds;
    ReadViewManager<unsigned> keywordIdsReadViewsMgr;

    bool commited_WriteView;

    ForwardIndex *forwardIndex; //Not serialised, must be assigned after every load and save.

    // Index Build time
    vector<unsigned> invertedListSizeDirectory;

    // Used by InvertedIndex::merge(). The first unsigned is invertedListId,
    // the second unsigned is keywordId.
    set<pair<unsigned, unsigned> > invertedListKeywordSetToMerge;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
    	//invertedIndexVector should no be NULL. In debug mode,Alert developer that it is NULL
    	ASSERT(invertedIndexVector != NULL);
        ar & *invertedIndexVector;
        //invertedIndexVector should no be NULL. In debug mode,Alert developer that it is NULL
        ASSERT(keywordIds != NULL);
        ar & *keywordIds;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        if (invertedIndexVector == NULL)
            invertedIndexVector = new cowvector<InvertedListContainerPtr>();
        ar & *invertedIndexVector;
        if (keywordIds == NULL)
            keywordIds = new cowvector<unsigned>();
        ar & *keywordIds;
        commited_WriteView = true;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version)
    {
        boost::serialization::split_member(ar, *this, file_version);
    }
};

}}


#endif //__INVERTEDINDEX_H__
