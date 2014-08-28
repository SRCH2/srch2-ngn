//$Id: Indexer.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __INDEXER_H__
#define __INDEXER_H__

#include <instantsearch/platform.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Schema.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/Record.h>
#include <instantsearch/Constants.h>

#include <string>
#include <stdint.h>

namespace srch2
{
namespace instantsearch
{

class Analyzer;
class Schema;
class Record;
class GlobalCache;

class IndexMetaData
{
public:
    IndexMetaData( GlobalCache *_cache,
                   unsigned _mergeEveryNSeconds,
                   unsigned _mergeEveryMWrites,
                   unsigned _updateHistogramEveryPMerges,
                   unsigned _updateHistogramEveryQWrites,
                   const std::string &_directoryName)
    {
        cache = _cache;

        if (_mergeEveryNSeconds == 0)
        {
            _mergeEveryNSeconds = 5;
        }
        mergeEveryNSeconds = _mergeEveryNSeconds;

        if (_mergeEveryMWrites == 0)
        {
            _mergeEveryMWrites = 5;
        }
        mergeEveryMWrites = _mergeEveryMWrites;

        // we are going to update the histogram information every 10 merges.
        if(_updateHistogramEveryPMerges == 0){
        	_updateHistogramEveryPMerges = 10;
        }
        updateHistogramEveryPMerges = _updateHistogramEveryPMerges;

        // we are going to update the histogram information every 50 writes
        if(_updateHistogramEveryQWrites == 0){
        	_updateHistogramEveryQWrites = 50;
        }
        updateHistogramEveryQWrites = _updateHistogramEveryPMerges * _mergeEveryMWrites;


        directoryName = _directoryName;
    }
    
    ~IndexMetaData()
    {
        delete cache;
    }

    std::string directoryName;
    GlobalCache *cache;
    unsigned mergeEveryNSeconds;
    unsigned mergeEveryMWrites;
    unsigned updateHistogramEveryPMerges;
    unsigned updateHistogramEveryQWrites;
};




class Indexer
{
public:
    static Indexer* create(IndexMetaData* index, Analyzer *analyzer, Schema *schema);
    static Indexer* load(IndexMetaData* index);

    virtual ~Indexer() {};

    /*virtual const Index *getReadView_NoToken() = 0;*/
    /*virtual GlobalCache *getCache() = 0;*/

    /*
    * Adds a record. If primary key is duplicate, insert fails and -1 is returned. Otherwise, 0 is returned.*/
    virtual INDEXWRITE_RETVAL addRecord(const Record *record, Analyzer *analyzer) = 0;

    virtual INDEXWRITE_RETVAL aclRoleAdd(const std::string &primaryKeyID, vector<string> &roleIds) = 0;

    virtual INDEXWRITE_RETVAL aclRoleDelete(const std::string &primaryKeyID, vector<string> &roleIds) = 0;

    virtual INDEXWRITE_RETVAL deleteRoleRecord(const std::string &primaryKeyID) = 0;

    /*
    * Deletes all the records.*/
    virtual INDEXWRITE_RETVAL deleteRecord(const std::string &primaryKeyID) = 0;

    /*
    * Deletes all the records.
      Get deleted internal record id */
    virtual INDEXWRITE_RETVAL deleteRecordGetInternalId(const std::string &primaryKeyID, unsigned &internalRecordId) = 0;

    /*
      Resume a deleted record */
    virtual INDEXWRITE_RETVAL recoverRecord(const std::string &primaryKeyID, unsigned internalRecordId) = 0;

    /*
      Check if a record exists */
    virtual INDEXLOOKUP_RETVAL lookupRecord(const std::string &primaryKeyID) = 0;

    virtual uint32_t getNumberOfDocumentsInIndex() const = 0;

    virtual const std::string getIndexHealth() const = 0;

    virtual const srch2::instantsearch::Schema *getSchema() const = 0;

    // get schema, which can be modified without rebuilding the index
    virtual srch2::instantsearch::Schema *getSchema() = 0;

    virtual StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const = 0;

    /**
     * Builds the index. The records are made searchable after the first commit.
     * It is advised to call the commit in a batch mode. The first commit should be called when
     * the bulk loading of initial records is done. Subsequent commits should be called based on
     * different criteria. For example, we may call "commit()" after a certain number of records
     * have been added to indexes (not yet searchable) or another certain number of seconds have
     * passed since last the commit or when a certain event occurs.
     * Note:- In order to avoid explicit commits after the first commit, we could choose to call
     * startMergeThreadLoop() function.
     */
    virtual INDEXWRITE_RETVAL commit() = 0;
    
    virtual const bool isCommited() const = 0;

    /*
     * Saves the indexes to disk, in the "directoryName" folder.*/

    virtual void save(const std::string& directoryName) = 0;
    virtual void save() = 0;

    virtual void exportData(const string &exportedDataFileName) = 0;

    /*For testing purpose only. Do not use in wrapper code*/
    virtual void merge_ForTesting() = 0;
    /*
    * Deletes all the records.*/
    /*virtual int deleteAll() = 0;*/
    //virtual int merge() = 0;

    /*
     *  Starts a conditional wait loop and merges all the incremental changes to indexes when a
     *  certain condition occurs.
     *  Condition:  n records have been added or t seconds have passed ( whichever occurs first)
     *  Note: This function starts a separate dedicated thread and returns thread id
     */
    virtual pthread_t createAndStartMergeThreadLoop() = 0;
};

}}

#endif //__INDEXER_H__
