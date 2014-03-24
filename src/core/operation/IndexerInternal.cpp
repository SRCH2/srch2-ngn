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

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */


#include "operation/IndexerInternal.h"
#include "util/Logger.h"

namespace srch2
{
namespace instantsearch
{
    
INDEXWRITE_RETVAL IndexReaderWriter::commit()
{    
    rwMutexForWriter->lockWrite();
    INDEXWRITE_RETVAL commitReturnValue;
    if (!this->index->isBulkLoadDone()) {
    	commitReturnValue = this->index->finishBulkLoad();
    } else {
    	/*
    	 *  If bulk load is done, then we are in past bulk load stage. We should call merge function
    	 *  even if dedicated merge thread running. The rwMutexLock will take care of concurrency.
    	 */
    	bool updateHistogramFlag = shouldUpdateHistogram();
    	if(updateHistogramFlag == true){
    		this->resetMergeCounterForHistogram();
    	}
    	commitReturnValue =this->merge(updateHistogramFlag);

    }
    this->writesCounterForMerge = 0;
    
    rwMutexForWriter->unlockWrite();

    return commitReturnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::addRecord(const Record *record, Analyzer* analyzer)
{
    rwMutexForWriter->lockWrite();
    INDEXWRITE_RETVAL returnValue = this->index->_addRecord(record, analyzer);
    if (returnValue == OP_SUCCESS) {
    	this->writesCounterForMerge++;
    	this->needToSaveIndexes = true;
    	if (this->mergeThreadStarted && writesCounterForMerge >= mergeEveryMWrites) {
    		rwMutexForWriter->cond_signal(&countThresholdConditionVariable);
    	}
    }

    rwMutexForWriter->unlockWrite();
    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::deleteRecord(const std::string &primaryKeyID)
{
    rwMutexForWriter->lockWrite();

    INDEXWRITE_RETVAL returnValue = this->index->_deleteRecord(primaryKeyID);
    if (returnValue == OP_SUCCESS) {
    	this->writesCounterForMerge++;
    	this->needToSaveIndexes = true;
    	if (this->mergeThreadStarted && writesCounterForMerge >= mergeEveryMWrites) {
    		rwMutexForWriter->cond_signal(&countThresholdConditionVariable);
    	}
    }
    
    rwMutexForWriter->unlockWrite();
    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::deleteRecordGetInternalId(const std::string &primaryKeyID, unsigned &internalRecordId)
{
    rwMutexForWriter->lockWrite();

    INDEXWRITE_RETVAL returnValue = this->index->_deleteRecordGetInternalId(primaryKeyID, internalRecordId);
    if (returnValue == OP_SUCCESS) {
    	this->writesCounterForMerge++;
    	this->needToSaveIndexes = true;
    	if (this->mergeThreadStarted && writesCounterForMerge >= mergeEveryMWrites){
    		rwMutexForWriter->cond_signal(&countThresholdConditionVariable);
    	}
    }

    rwMutexForWriter->unlockWrite();
    return returnValue;
}

INDEXWRITE_RETVAL IndexReaderWriter::recoverRecord(const std::string &primaryKeyID, unsigned internalRecordId)
{
    rwMutexForWriter->lockWrite();

    INDEXWRITE_RETVAL returnValue = this->index->_recoverRecord(primaryKeyID, internalRecordId);
    if (returnValue == OP_SUCCESS) {
    	this->writesCounterForMerge++;
    	this->needToSaveIndexes = true;
    	if (this->mergeThreadStarted && writesCounterForMerge >= mergeEveryMWrites) {
    		rwMutexForWriter->cond_signal(&countThresholdConditionVariable);
    	}
    }

    rwMutexForWriter->unlockWrite();
    return returnValue;
}

INDEXLOOKUP_RETVAL IndexReaderWriter::lookupRecord(const std::string &primaryKeyID)
{
    // although it's a read-only OP, since we need to check the writeview
    // we need to acquire the writelock
    // but do NOT need to increase the merge counter

    rwMutexForWriter->lockWrite();
    
    INDEXLOOKUP_RETVAL returnValue = this->index->_lookupRecord(primaryKeyID);

    rwMutexForWriter->unlockWrite();

    return returnValue;
}

void IndexReaderWriter::exportData(const string &exportedDataFileName)
{
    // add write lock
    rwMutexForWriter->lockWrite();

    // merge the index
    // we don't have to update histogram information when we want to export.
    this->merge(false);
    writesCounterForMerge = 0;

    //get the export data
    this->index->_exportData(exportedDataFileName);

    // free write lock
    rwMutexForWriter->unlockWrite();
}

void IndexReaderWriter::save()
{
    rwMutexForWriter->lockWrite();

    // If no insert/delete/update is performed, we don't need to save.
    if(this->needToSaveIndexes == false){
    	rwMutexForWriter->unlockWrite();
    	return;
    }

    // we don't have to update histogram information when we want to export.
    this->merge(false);
    writesCounterForMerge = 0;

    srch2::util::Logger::console("Saving Indexes ...");
    this->index->_save();

    // Since one save is done, we need to set needToSaveIndexes back to false
    // we need this line because of bulk-load. Because in normal save, the engine will be killed after save
    // so we don't need this flag (the engine will die anyways); but in the save which happens after bulk-load,
    // we should set this flag back to false for future save calls.
    this->needToSaveIndexes = false;

    rwMutexForWriter->unlockWrite();
}

void IndexReaderWriter::save(const std::string& directoryName)
{
	rwMutexForWriter->lockWrite();

    // we don't have to update histogram information when we want to export.
    this->merge(false);
    writesCounterForMerge = 0;

    this->index->_save(directoryName);

    rwMutexForWriter->unlockWrite();
}

/*
 *  This function is not thread safe. Caller of this function should hold valid lock. This
 *  function should not be exposed outside core.
 */

INDEXWRITE_RETVAL IndexReaderWriter::merge(bool updateHistogram)
{
    if (this->cache != NULL && this->index->isMergeRequired())
        this->cache->clear();

    // increment the mergeCounterForUpdatingHistogram
    this->mergeCounterForUpdatingHistogram ++;

    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);

    INDEXWRITE_RETVAL returnValue = this->index->_merge(updateHistogram);

    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    indexHealthInfo.getLatestHealthInfo(this->index->_getNumberOfDocumentsInIndex());
    return returnValue;
}

void * dispatchMergeThread(void *indexer) {
	(reinterpret_cast <IndexReaderWriter *>(indexer))->startMergeThreadLoop();
	pthread_exit(0);
}

IndexReaderWriter::IndexReaderWriter(IndexMetaData* indexMetaData, Analyzer *analyzer, Schema *schema)
{
     // CREATE NEW Index
     this->index =  IndexData::create(indexMetaData->directoryName,
     		                          analyzer,
                                      schema,
                                      srch2::instantsearch::DISABLE_STEMMER_NORMALIZER
                                      );
     this->initIndexReaderWriter(indexMetaData);
     // start merge threads after commit
 }

IndexReaderWriter::IndexReaderWriter(IndexMetaData* indexMetaData)
{
    // LOAD Index
    this->index = IndexData::load(indexMetaData->directoryName);
    this->initIndexReaderWriter(indexMetaData);
    //this->startMergerThreads();
}

void IndexReaderWriter::initIndexReaderWriter(IndexMetaData* indexMetaData)
 {
     this->cache = dynamic_cast<CacheManager*>(indexMetaData->cache);
     this->mergeEveryNSeconds = indexMetaData->mergeEveryNSeconds;
     this->mergeEveryMWrites = indexMetaData->mergeEveryMWrites;
     this->updateHistogramEveryPMerges = indexMetaData->updateHistogramEveryPMerges;
     this->updateHistogramEveryQWrites = indexMetaData->updateHistogramEveryQWrites;
     this->writesCounterForMerge = 0;
     this->mergeCounterForUpdatingHistogram = 0;
     this->needToSaveIndexes = false;

     this->mergeThreadStarted = false; // No threads running
     this->rwMutexForWriter = new ReadWriteMutex(100);
 }

uint32_t IndexReaderWriter::getNumberOfDocumentsInIndex() const
{
    return this->index->_getNumberOfDocumentsInIndex();
}



pthread_t IndexReaderWriter::createAndStartMergeThreadLoop() {
	pthread_attr_init(&mergeThreadAttributes);
	pthread_attr_setdetachstate(&mergeThreadAttributes, PTHREAD_CREATE_JOINABLE);
	pthread_create(&mergerThread, &mergeThreadAttributes, dispatchMergeThread, this);
	pthread_attr_destroy(&mergeThreadAttributes);
	return mergerThread;
}

//http://publib.boulder.ibm.com/infocenter/iseries/v5r4/index.jsp?topic=%2Fapis%2Fusers_77.htm
void IndexReaderWriter::startMergeThreadLoop()
{
    int               rc;
    struct timespec   ts;
    struct timeval    tp;

    /*
     *  There should be only one merger thread per indexer object
     */
    rwMutexForWriter->lockWrite();
    if (mergeThreadStarted) {
    	rwMutexForWriter->unlockWrite();
    	Logger::warn("Only one merge thread per index is supported!");
    	return;
    }
    if (!this->index->isBulkLoadDone()) {
        	rwMutexForWriter->unlockWrite();
        	Logger::warn("Merge thread can be called only after first commit!");
        	return;
    }
    rwMutexForWriter->unlockWrite();
    mergeThreadStarted = true;
    /*
     *  Initialize condition variable for the first time before loop starts.
     */
    pthread_cond_init(&countThresholdConditionVariable, NULL);

    while (1)
    {
        rc =  gettimeofday(&tp, NULL);
        // Convert from timeval to timespec
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += this->mergeEveryNSeconds;

        rwMutexForWriter->writeLockWithCondTimedWait(&countThresholdConditionVariable, &ts);

        if (mergeThreadStarted == false)
            break;
        else
        {
        	// check to see if it's the time to update histogram information
        	// if so, reset the merge counter for future.
            bool updateHistogramFlag = shouldUpdateHistogram();
            if(updateHistogramFlag == true){
            	this->resetMergeCounterForHistogram();
            }
            this->merge(updateHistogramFlag);
            writesCounterForMerge = 0;
            rwMutexForWriter->unlockWrite();
        }
    }
    pthread_cond_destroy(&countThresholdConditionVariable);
    rwMutexForWriter->unlockWrite();
    return;
}



}
}


