//$Id: IndexerInternal.cpp 3456 2013-06-14 02:11:13Z jiaying $

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


#include "operation/IndexerInternal.h"

namespace srch2
{
namespace instantsearch
{

void IndexReaderWriter::startMergerThreads()
{
    if (not this->mergeThreadStarted)
    {
        this->mergeThreadStarted = true; // Threads running

        pthread_cond_init (&countThresholdConditionVariable, NULL);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        void *(*backgroundMergeThread) (void*);
        backgroundMergeThread = &IndexReaderWriter::startBackgroundMergerThread;

        pthread_create(&mergerThread, &attr, backgroundMergeThread, this);
    }
}
    
INDEXWRITE_RETVAL IndexReaderWriter::commit()
{    
    writelock();
    
    INDEXWRITE_RETVAL commitReturnValue = this->index->_commit();
    this->writesCounter_forMerge = 0;
    
    writeunlock();

    this->startMergerThreads(); // No-op if threads already started.

    return commitReturnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::addRecord(const Record *record, Analyzer* analyzer, const uint64_t kafkaMessageOffset)
{
    writelock();
    this->writesCounter_forMerge++;
    this->kafkaOffset_LatestWriteView = kafkaMessageOffset;
    INDEXWRITE_RETVAL returnValue = this->index->_addRecord(record, analyzer);

    writeunlock();
    
    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::deleteRecord(const std::string &primaryKeyID, const uint64_t kafkaMessageOffset)
{
    writelock();
    this->writesCounter_forMerge++;

    this->kafkaOffset_LatestWriteView = kafkaMessageOffset;
    INDEXWRITE_RETVAL returnValue = this->index->_deleteRecord(primaryKeyID);
    
    writeunlock();
    
    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::deleteRecordGetInternalId(const std::string &primaryKeyID, const uint64_t kafkaMessageOffset, unsigned &internalRecordId)
{
    writelock();
    this->writesCounter_forMerge++;

    this->kafkaOffset_LatestWriteView = kafkaMessageOffset;
    INDEXWRITE_RETVAL returnValue = this->index->_deleteRecordGetInternalId(primaryKeyID, internalRecordId);

    writeunlock();

    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::recoverRecord(const std::string &primaryKeyID, const uint64_t kafkaMessageOffset, unsigned internalRecordId)
{
    writelock();
    this->writesCounter_forMerge++;

    this->kafkaOffset_LatestWriteView = kafkaMessageOffset;
    INDEXWRITE_RETVAL returnValue = this->index->_recoverRecord(primaryKeyID, internalRecordId);

    writeunlock();

    return returnValue;
}

INDEXLOOKUP_RETVAL IndexReaderWriter::lookupRecord(const std::string &primaryKeyID)
{
    // although it's a read-only OP, since we need to check the writeview
    // we need to acquire the writelock
    // but do NOT need to increase the merge counter

    writelock(); 
    
    INDEXLOOKUP_RETVAL returnValue = this->index->_lookupRecord(primaryKeyID);

    writeunlock();

    return returnValue;
}

void IndexReaderWriter::exportData(vector<std::string> &compressedInMemoryRecordStrings)
{
    writelock();

    this->merge();
    writesCounter_forMerge = 0;

    this->index->_setKafkaOffsetOfCurrentIndexSnapshot(this->kafkaOffset_LatestReadView);
    this->index->_exportData(compressedInMemoryRecordStrings);

    writeunlock();
}

void IndexReaderWriter::save()
{
    writelock();

    this->merge();
    writesCounter_forMerge = 0;

    this->index->_setKafkaOffsetOfCurrentIndexSnapshot(this->kafkaOffset_LatestReadView);
    this->index->_save();

    writeunlock();
}

void IndexReaderWriter::save(const std::string& directoryName)
{
    writelock();

    this->merge();
    writesCounter_forMerge = 0;

    this->index->_setKafkaOffsetOfCurrentIndexSnapshot(this->kafkaOffset_LatestReadView);
    this->index->_save(directoryName);

    writeunlock();
}


INDEXWRITE_RETVAL IndexReaderWriter::merge()
{
    if (this->cache != NULL)
        this->cache->clear();

    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);

    INDEXWRITE_RETVAL returnValue = this->index->_merge();

    this->kafkaOffset_LatestReadView = this->kafkaOffset_LatestWriteView;

    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    // std::cout << "{\"merge\":\"success\",\"time\":\""<< time <<"\"}" << std::endl;

    indexHealthInfo.notifyMerge(time,  this->index->_getNumberOfDocumentsInIndex());
    return returnValue;
}

//http://publib.boulder.ibm.com/infocenter/iseries/v5r4/index.jsp?topic=%2Fapis%2Fusers_77.htm
void IndexReaderWriter::mergeThreadLoop()
{
    int               rc;
    struct timespec   ts;
    struct timeval    tp;

    while (1)
    {
        rc =  gettimeofday(&tp, NULL);
        // Convert from timeval to timespec
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += this->mergeEveryNSeconds;

        rwMutexForWriter->cond_timedwait(&countThresholdConditionVariable, &ts);

        if (mergeThreadStarted == false)
            break;
        else
        {
            this->merge();
            writesCounter_forMerge = 0;
            rwMutexForWriter->unlockWrite();
        }
    }
    mergeThreadStarted = true;
    rwMutexForWriter->unlockWrite();
    pthread_exit(0);
}



}
}


