
// $Id: IndexerInternal.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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
#ifndef __INDEXERINTERNAL_H__
#define __INDEXERINTERNAL_H__

#include <instantsearch/Indexer.h>
#include "operation/CacheManager.h"
#include "operation/IndexData.h"
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include "util/mypthread.h"

using std::vector;
using std::string;

namespace srch2
{
namespace instantsearch
{
class CacheManager;
class GlobalCache;

struct IndexHealthInfo
{
    //std::string lastWriteTimeString;
    std::string lastMergeTimeString;
    unsigned doc_count;

    IndexHealthInfo()
    {
        //this->notifyWrite();
        this->getLatestHealthInfo(0);
    }

    void getString(struct std::tm *timenow, string &in)
    {
        char buffer [80];
        strftime (buffer,80,"%x %X",timenow);
        in = string(buffer);
    }

    /*void notifyWrite()
    {
        time_t timer = time(NULL);
        struct std::tm* timenow = gmtime(&timer);
        IndexHealthInfo::getString(timenow, this->lastWriteTimeString);
    }*/

    void getLatestHealthInfo(unsigned doc_count)
    {
        time_t timer = time(NULL);
        struct std::tm* timenow = gmtime(&timer);
        IndexHealthInfo::getString(timenow, this->lastMergeTimeString);
        this->doc_count = doc_count;
    }

    const std::string getIndexHealthString() const
    {
        std::stringstream returnString;
        //returnString << "\"last_insert\":\"" << lastWriteTimeString << "\"";
        //returnString << ",\"last_merge\":\"" << lastMergeTimeString << "\"";
        returnString << "\"last_merge\":\"" << lastMergeTimeString << "\"";
        returnString << ",\"doc_count\":\"" << doc_count << "\"";
        return returnString.str();
    }
};

class IndexReaderWriter: public Indexer
{
public:

    //TODO put it as private
    pthread_mutex_t lockForWriters;

    IndexReaderWriter(IndexMetaData* indexMetaData, Analyzer *analyzer, Schema *schema);

    IndexReaderWriter(IndexMetaData* indexMetaData);

    void initIndexReaderWriter(IndexMetaData* indexMetaData);
    virtual ~IndexReaderWriter()
    {
    	if (this->mergeThreadStarted == true) {
	  pthread_mutex_lock(&lockForWriters);
	  this->mergeThreadStarted = false;
	  pthread_cond_signal(&countThresholdConditionVariable);
	  pthread_mutex_unlock(&lockForWriters);
        
	  pthread_join(mergerThread, NULL); // waiting to JOINABLE merge thread.
    	}
        delete this->index;
    };

    uint32_t getNumberOfDocumentsInIndex() const;

    /**
     * Builds the index. After commit(), the records are made searchable after the first commit.
     *
     * After the first commit, the call to commit does nothing.
     *
     */
    INDEXWRITE_RETVAL commit();

    /**
     * Adds a record. If primary key is duplicate, insert fails and -1 is returned. Otherwise, 0 is returned.
     */
    INDEXWRITE_RETVAL addRecord(const Record *record, Analyzer *analyzer);

    /**
     * Deletes all the records.
     */
    INDEXWRITE_RETVAL deleteRecord(const std::string &primaryKeyID);

    INDEXWRITE_RETVAL deleteRecordGetInternalId(const std::string &primaryKeyID, unsigned &internalRecordId);

    INDEXWRITE_RETVAL recoverRecord(const std::string &primaryKeyID, unsigned internalRecordId);

    INDEXLOOKUP_RETVAL lookupRecord(const std::string &primaryKeyID);

    inline const IndexData *getReadView(IndexReadStateSharedPtr_Token &readToken)
    {
        this->index->getReadView(readToken);
        return this->index;
    }

    inline const srch2::instantsearch::Schema *getSchema() const
    {
        return this->index->getSchema();
    }

    inline srch2::instantsearch::Schema *getSchema()
    {
        return this->index->getSchema();
    }

    inline StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const
    {
        return this->index->getInMemoryData(internalRecordId);
    }

    const AttributeAccessControl & getAttributeAcl() const {
    	return *(this->index->attributeAcl);
    }
    void exportData(const string &exportedDataFileName);

    void save();

    void save(const std::string& directoryName);

    inline GlobalCache *getCache()
    {
        return this->cache;
    }

    inline const string getIndexHealth() const
    {
        std::stringstream str;
        str << "\"engine_status\":{";
        str << "\"search_requests\":\"" << this->index->_getReadCount() << "\",";
        str << "\"write_requests\":\"" <<  this->index->_getWriteCount() << "\",";
        str << "\"docs_in_index\":\"" << this->index->_getNumberOfDocumentsInIndex() << "\",";
        str << this->indexHealthInfo.getIndexHealthString() << "}";
        return str.str();
    }
    
    inline const bool isCommited() const { return this->index->isBulkLoadDone(); }


    // histogram update is triggered if :
    // A : we have had updateHistogramEveryQWrites writes since the last histogram update
    // or B : we have had updateHistogramEveryPMerges merges since the last histogram update
    inline bool shouldUpdateHistogram(){
    	if(writesCounterForMerge >= this->updateHistogramEveryQWrites ||
    			mergeCounterForUpdatingHistogram >= this->updateHistogramEveryPMerges){
    		return true;
    	}
    	return false;
    }

    inline void resetMergeCounterForHistogram(){
    	this->mergeCounterForUpdatingHistogram = 0;
    }

    inline void merge_ForTesting()
    {
        this->merge(false);
    }

    inline QuadTree *getQuadTree() const { return this->index->quadTree; }
    inline ForwardIndex * getForwardIndex() const { return this->index->forwardIndex; }

    pthread_t createAndStartMergeThreadLoop();
    void startMergeThreadLoop();

private:
    IndexData *index;
    CacheManager *cache;

    IndexHealthInfo indexHealthInfo;

    pthread_cond_t countThresholdConditionVariable;
    volatile bool mergeThreadStarted;

	pthread_t mergerThread;  // stores thread identifier.
	pthread_attr_t mergeThreadAttributes;  // store thread attributes

    volatile unsigned writesCounterForMerge;
    bool needToSaveIndexes;
    unsigned mergeEveryNSeconds;
    unsigned mergeEveryMWrites;

    unsigned updateHistogramEveryPMerges;
    unsigned updateHistogramEveryQWrites;
    volatile unsigned mergeCounterForUpdatingHistogram;

    INDEXWRITE_RETVAL merge(bool updateHistogram);


};

}}

#endif /* __INDEXERINTERNAL_H__ */
