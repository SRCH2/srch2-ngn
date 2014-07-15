
// $Id: IndexData.cpp 3480 2013-06-19 08:00:34Z iman $

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

#include "IndexData.h"
#include "record/SchemaInternal.h"
#include "index/IndexUtil.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include "geo/QuadTree.h"
#include <instantsearch/Record.h>
#include <instantsearch/Analyzer.h>
#include "util/FileOps.h"
#include "serialization/Serializer.h"
#include <stdio.h>  /* defines FILENAME_MAX */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <exception>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using std::string;
using std::vector;
using std::map;
using std::pair;
using srch2::util::Logger;
//using std::unordered_set;
using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

IndexData::IndexData(const string &directoryName,
        Analyzer *analyzer,
        const Schema *schema,
        const StemmerNormalizerFlagType &stemmerFlag)
{

    this->directoryName = directoryName;

    if(!checkDirExistence(directoryName.c_str())){
        if (createDir(directoryName.c_str()) == -1){
            throw std::runtime_error("Index Directory can not be created");
        }
    }

    this->schemaInternal = new SchemaInternal( *(dynamic_cast<const SchemaInternal *>(schema)) );

    this->rankerExpression = new RankerExpression(this->schemaInternal->getScoringExpression());

    this->trie = new Trie_Internal();

    this->forwardIndex = new ForwardIndex(this->schemaInternal);
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex) {
        this->invertedIndex =new InvertedIndex(this->forwardIndex);
        this->quadTree = NULL;
    } else {
        this->quadTree = new QuadTree(this->forwardIndex, this->trie);
        this->invertedIndex = NULL;
    }

    this->readCounter = new ReadCounter();
    this->writeCounter = new WriteCounter();
    this->flagBulkLoadDone = false;

    this->mergeRequired = true;
}

IndexData::IndexData(const string& directoryName)
{
    this->directoryName = directoryName;

    if(!checkDirExistence(directoryName.c_str())){
        Logger::error("Given index path %s does not exist", directoryName.c_str());
        throw std::runtime_error("Index load exception ");
    }
    Serializer serializer;
    try{
    	this->schemaInternal = new SchemaInternal();
    	serializer.load(*(this->schemaInternal), this->directoryName + "/" + IndexConfig::schemaFileName);

    	this->rankerExpression = new RankerExpression(this->schemaInternal->getScoringExpression());

    	this->trie = new Trie_Internal();
    	this->forwardIndex = new ForwardIndex(this->schemaInternal);
    	serializer.load(*(this->trie),directoryName + "/" + IndexConfig::trieFileName);
    	if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
            this->invertedIndex = new InvertedIndex(this->forwardIndex);
        else
            this->invertedIndex = NULL;

    	// set if it's a attributeBasedSearch
    	PositionIndexType positionIndexType = this->schemaInternal->getPositionIndexType();
    	if(isEnabledAttributeBasedSearch(positionIndexType))
    		this->forwardIndex->isAttributeBasedSearch = true;

    	serializer.load(*(this->forwardIndex), directoryName + "/" + IndexConfig::forwardIndexFileName);
    	this->forwardIndex->setSchema(this->schemaInternal);

    	if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex){
    		serializer.load(*(this->invertedIndex), directoryName + "/" +  IndexConfig::invertedIndexFileName);
    		this->invertedIndex->setForwardIndex(this->forwardIndex);
                quadTree = NULL;
    	} else {
    		this->quadTree = new QuadTree();
    		serializer.load(*(this->quadTree), directoryName + "/" +  IndexConfig::quadTreeFileName);
    		this->quadTree->setForwardIndex(this->forwardIndex);
    		this->quadTree->setTrie(this->trie);
    		//Logger::debug("QuadTree loaded");
    	}

    	this->loadCounts(directoryName + "/" + IndexConfig::indexCountsFileName);
    	this->flagBulkLoadDone = true;
    }catch(exception& ex){
    	Logger::error("Error while loading the index files ...");
    	throw ex;
    }

    this->mergeRequired = true;
}

IndexData::IndexData(std::istream& inputByteStream, const string& saveDirName) {
    this->directoryName = saveDirName;

    if(!checkDirExistence(directoryName.c_str())){
        Logger::error("Given index path %s does not exist", directoryName.c_str());
        throw std::runtime_error("Index load exception ");
    }
    try {
    	this->_deSerialize(inputByteStream);
    }catch (boost::archive::archive_exception& ex) {
    	cout << ex.what() << endl;
    	throw;
    }
	this->mergeRequired = true;
}

// check whether the keyword id list is sorted. This is called from ASSERT statement below to
// verify the correctness of the assumption that keywordIdList is alphabetaically sorted
bool isSortedAlphabetically(const KeywordIdKeywordStringInvertedListIdTriple& keywordIdList){

	if (keywordIdList.size() < 2)
		return true;   // 0 or 1 element array is considered sorted

	KeywordIdKeywordStringInvertedListIdTriple::const_iterator iter = keywordIdList.begin();
	KeywordIdKeywordStringInvertedListIdTriple::const_iterator previter  = iter;
	++iter;
	while(iter != keywordIdList.end())
	{
		if (iter->second.first.compare(previter->second.first) <= 0)
			return false;
		++iter;
	}
	return true;
}

/// Add a record
INDEXWRITE_RETVAL IndexData::_addRecord(const Record *record, Analyzer *analyzer)
{

    boost::unique_lock< boost::shared_mutex > lock(globalRwMutexForReadersWriters);
    //For M1, since we don't use shared pointers for quad trees, readers
    //and writers need to share the global rwMutex
    // For A1 , we do not need lock , so unlock the lock.
    if(this->schemaInternal->getIndexType() 
            != srch2::instantsearch::LocationIndex) {
        lock.unlock();   // Double unlock is fine. First is here and second will be in destructor of lock
    }
    INDEXWRITE_RETVAL returnValue = OP_FAIL; // not added

    /// Get the internalRecordId
    unsigned internalRecordId;
    //Check for duplicate record
    bool recordPrimaryKeyFound = this->forwardIndex->getInternalRecordIdFromExternalRecordId(record->getPrimaryKey(), internalRecordId);

    // InternalRecordId == ForwardListId
    if (recordPrimaryKeyFound == false)
    {
        this->writeCounter->incWritesCounter();
        this->writeCounter->incDocsCounter();
        returnValue = OP_SUCCESS;

        this->mergeRequired = true;
        /// analyze the record (tokenize it, remove stop words)
        map<string, TokenAttributeHits > tokenAttributeHitsMap;
        analyzer->tokenizeRecord(record, tokenAttributeHitsMap);

        KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

        // only used for committed geo index
        vector<unsigned> *keywordIdVector = NULL;
        if(this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex
                && this->flagBulkLoadDone == true)
        {
            keywordIdVector = new vector<unsigned> ();
        }

        for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
                mapIterator != tokenAttributeHitsMap.end();
                ++mapIterator)
        {
            /// add words to trie

            unsigned invertedIndexOffset = 0;
            unsigned keywordId = 0;

            // two flags that only M1 update needs
            bool isNewTrieNode = false;
            bool isNewInternalTerminalNode = false;
            // the reason we need @var hasExactlyOneChild is that for this kind of prefix on c-filter, we shouldn't change it in-place. Instead, we should add the new prefix when the old one is found on c-filter
            // e.g. Initially on Trie we have "cancer"-4 'c'->'a'->'n'->'c'->'e'->'r'. Each Trie node have the same prefix interval [4, 4]
            //        If we insert a new record with keyword "can"-1, then Trie nodes 'c', 'a' and 'n' will become [1,4], while 'c', 'e' and 'r' will stay the same.
            //        So if we find [4, 4] on c-filter, we should NOT change it to [2, 4]. Instead, we should keep [4, 4] and add [2, 4]
            bool hadExactlyOneChild = false;
            unsigned breakLeftOrRight = 0;
            vector<Prefix> *oldParentOrSelfAndAncs = NULL;

            if (this->flagBulkLoadDone == false) // not committed yet
            	//transform string to vector<CharType>
                keywordId = this->trie->addKeyword(getCharTypeVector(mapIterator->first), invertedIndexOffset);
            else
            {
                if (this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex)
                {
                    oldParentOrSelfAndAncs = new vector<Prefix> ();  // CHEN: store the ancestors (possibly itself) whose interval will change after this insertion
                    breakLeftOrRight = this->trie->ifBreakOldParentPrefix(getCharTypeVector(mapIterator->first), oldParentOrSelfAndAncs, hadExactlyOneChild);
                }  // CHEN: 0 means no break; 1 means break from left; and 2 means break from right
                //transform string to vector<CharType>
                keywordId = this->trie->addKeyword_ThreadSafe(getCharTypeVector(mapIterator->first), invertedIndexOffset, isNewTrieNode, isNewInternalTerminalNode);
            }

            // For the case where the keyword is new, and we do not have the "space" to assign a new id
            // for it, we assign a positive integer to this keyword.  So the returned value
            // should also be valid.
            keywordIdList.push_back( make_pair(keywordId, make_pair(mapIterator->first, invertedIndexOffset) ) );

            if ( this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex ) // A1
            {
                this->invertedIndex->incrementHitCount(invertedIndexOffset);
            }
            else // geo index (M1). Add the flag to the map only if the indexes have been committed
                if (this->flagBulkLoadDone == true)
            {
                unsigned keywordStatus = 0;
                if (isNewTrieNode) // is a new trie leaf node
                    keywordStatus = 1;
                else if (isNewInternalTerminalNode) // an existing trie internal node turned into terminal node
                    keywordStatus = 2;

                keywordIdVector->push_back(keywordId);

                if (keywordStatus > 0 && breakLeftOrRight > 0) // CHEN: is a terminal node, and it breaks the current interval
                    this->quadTree->updateInfoToFixBroadenPrefixesOnFilters(breakLeftOrRight, oldParentOrSelfAndAncs, keywordId, hadExactlyOneChild);
                else
                    delete oldParentOrSelfAndAncs;
            }
        }

        // Commented out the sort statement below. We do not need to sort the keyword List again
        // because keywords are stored in a map which keeps them alphabetically sorted.
        // If this _addRecord() is called BEFORE commit(), these keyword IDs within this list are
        // sorted using the alphabetical order of the keywords, even though they are not sorted
        // based on the integer order. During the commit() phase, we will map these IDs to their
        // final IDs from the trie, and the order of the IDs becomes the integer order automatically.
        // If this _addRecord() is called AFTER commit(), the keyword IDs are sorted using the
        // alphabetical order, which is already consistent with the integer order.
        //
        // std::sort( keywordIdList.begin(), keywordIdList.end());

        // Adding this assert to ensure that keywordIdList is alphabetically sorted. see isSorted()
        // function above.
        ASSERT(isSortedAlphabetically(keywordIdList));

        unsigned internalRecordId;
        this->forwardIndex->appendExternalRecordIdToIdMap(record->getPrimaryKey(), internalRecordId);
        this->forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

        if ( this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex )
        {
            if ( this->flagBulkLoadDone == true )
            {
                const unsigned totalNumberofDocuments = this->forwardIndex->getTotalNumberOfForwardLists_WriteView();
                ForwardList *forwardList = this->forwardIndex->getForwardList_ForCommit(internalRecordId);
                this->invertedIndex->addRecord(forwardList , this->trie, this->rankerExpression,
                        internalRecordId, this->schemaInternal, record, totalNumberofDocuments, keywordIdList);
            }
        }
        else if (this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex ) // geo index
        {
            // for geo index, we don't have inverted index, so we compute TermRecordStaticScores here
            ForwardList *fl = this->forwardIndex->getForwardList_ForCommit(internalRecordId);
            for (unsigned counter = 0; counter < fl->getNumberOfKeywords(); counter++)
            {
                float recordBoost = fl->getRecordBoost();
                float sumOfFieldBoost = fl->getKeywordRecordStaticScore(counter);
                float recordLength = fl->getNumberOfKeywords();
                float idf = 1.0;
                unsigned tf = 1;
                float textRelevance = Ranker::computeRecordTfIdfScore(tf, idf, sumOfFieldBoost);
                float score = rankerExpression->applyExpression(recordLength, recordBoost, textRelevance);
                fl->setKeywordRecordStaticScore(counter, score);
            }

            if ( this->flagBulkLoadDone == false ) // batch load
            {
                this->quadTree->addRecordBeforeCommit(record, internalRecordId);
            }
            else // after commit
            {
                this->quadTree->addRecordAfterCommit(record, internalRecordId, keywordIdVector);
            }
        }
    }
    else
    {
        returnValue = OP_FAIL;
    }
    
    return returnValue;
}

// delete a record with a specific id //TODO Give the correct return message for delete pass/fail
INDEXWRITE_RETVAL IndexData::_deleteRecord(const std::string &externalRecordId)
{
    INDEXWRITE_RETVAL success = this->forwardIndex->deleteRecord(externalRecordId)  ? OP_SUCCESS: OP_FAIL;

    if (success == OP_SUCCESS) {
        this->mergeRequired = true; // need to tell the merge thread to merge
        this->writeCounter->decDocsCounter();
        this->writeCounter->incWritesCounter();
    }

    return success;
}

// delete a record with a specific id //TODO Give the correct return message for delete pass/fail
// get the deleted internal recordID
INDEXWRITE_RETVAL IndexData::_deleteRecordGetInternalId(const std::string &externalRecordId, unsigned &internalRecordId)
{
    INDEXWRITE_RETVAL success = this->forwardIndex->deleteRecordGetInternalId(externalRecordId, internalRecordId)  ? OP_SUCCESS: OP_FAIL;

    if (success == OP_SUCCESS) {
        this->mergeRequired = true; // need to tell the merge thread to merge
        this->writeCounter->decDocsCounter();
        this->writeCounter->incWritesCounter();
    }

    return success;
}

// recover the deleted record
INDEXWRITE_RETVAL IndexData::_recoverRecord(const std::string &externalRecordId, unsigned internalRecordId)
{
    INDEXWRITE_RETVAL success = this->forwardIndex->recoverRecord(externalRecordId, internalRecordId)  ? OP_SUCCESS: OP_FAIL;

    if (success == OP_SUCCESS) {
        this->mergeRequired = true; // need to tell the merge thread to merge

        // recover the changes to the counters made by _deleteRecordGetInternalId
        this->writeCounter->incDocsCounter();
        this->writeCounter->decWritesCounter();
    }

    return success;
}

// check if the record exists
INDEXLOOKUP_RETVAL IndexData::_lookupRecord(const std::string &externalRecordId) const
{
    return this->forwardIndex->lookupRecord(externalRecordId);
}

/* build the index. After commit(), no more records can be added.
 *
 * There has to be at least one record in Index for the commit to succeed.
 * Returns 1, if the committing succeeded.
 * Returns 0, if the committing failed, which is in the following two cases.
 *    a) No records in index.
 *    b) Index had been already commited.
 */
INDEXWRITE_RETVAL IndexData::finishBulkLoad()
{
    bool isLocational = false;
    if(this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex)
        isLocational = true;

    if (this->flagBulkLoadDone == false){
        /*
         * For the text only Index:
         * 1. Initialize the size of Inverted Index vector as size of Forward Index vector.
         * 2. For each ForwardIndex Element, get PositionIndex Element.
         * 3. Add ForwardIndex and Position Index information as entry into InvertedIndex.
         * 4. Commit Inverted Index, by traversing Trie by Depth First.
         * 5. For each Terminal Node, add InvertedIndex offset information in it.
         *
         * For the text+Geo Index, two differences:
         * 1. There is no InvertedIndex.
         * 2. Need to go to the QuadTree to build filters.
         */
        const unsigned totalNumberofDocuments = this->forwardIndex->getTotalNumberOfForwardLists_WriteView();

        // Note: we should commit even if totalNumberofDocuments = 0

        this->forwardIndex->commit();
        this->trie->commit();
        //this->trie->print_Trie();
        const vector<unsigned> *oldIdToNewIdMapVector = this->trie->getOldIdToNewIdMapVector();

        map<unsigned, unsigned> oldIdToNewIdMapper;
        for (unsigned i = 0; i < oldIdToNewIdMapVector->size(); i++)
          oldIdToNewIdMapper[i] = oldIdToNewIdMapVector->at(i);

        if(!isLocational)
            this->invertedIndex->initialiseInvertedIndexCommit();

        for (unsigned forwardIndexIter = 0; forwardIndexIter < totalNumberofDocuments; ++forwardIndexIter){
            ForwardList *forwardList = this->forwardIndex->getForwardList_ForCommit(forwardIndexIter);
            vector<NewKeywordIdKeywordOffsetTriple> newKeywordIdKeywordOffsetTriple;
            //this->forwardIndex->commit(forwardList, oldIdToNewIdMapVector, newKeywordIdKeywordOffsetTriple);
            this->forwardIndex->commit(forwardList, oldIdToNewIdMapper, newKeywordIdKeywordOffsetTriple);
            if(!isLocational)
                this->invertedIndex->commit(forwardList, this->rankerExpression,
                        forwardIndexIter, totalNumberofDocuments, this->schemaInternal, newKeywordIdKeywordOffsetTriple);
        }
        this->forwardIndex->finalCommit();
//        this->forwardIndex->print_size();
        if (isLocational){
            //time_t begin,end;
            //time(&begin);
            this->quadTree->createFilters();
            //time(&end);
            //std::cout << "CFilters and OFilters creating time elapsed: " << difftime(end, begin) << " seconds"<< std::endl;
        }else{
            this->invertedIndex->setForwardIndex(this->forwardIndex);
            this->invertedIndex->finalCommit();
        }

        // delete the keyword mapper (from the old ids to the new ids) inside the trie
        this->trie->deleteOldIdToNewIdMapVector();


        /*
         * Since we don't have inverted index for M1, we send NULL.
         * NULL will make the component to compute simple frequency
         * (vs. integration of frequency and recordStaticScores) for nodeSubTrieValue of trie nodes.
         */
        if (isLocational){
			this->trie->finalCommit_finalizeHistogramInformation(NULL , NULL, 0);
        }else{
			this->trie->finalCommit_finalizeHistogramInformation(this->invertedIndex ,
					this->forwardIndex,
					this->forwardIndex->getTotalNumberOfForwardLists_ReadView());
        }
        this->flagBulkLoadDone = true;
        return OP_SUCCESS;
    }else{
        return OP_FAIL;
    }
}

INDEXWRITE_RETVAL IndexData::_merge(bool updateHistogram){
    Logger::debug("Merge begins--------------------------------"); 

    if (!this->mergeRequired)
        return OP_FAIL;
    
    // struct timespec tstart;
    // clock_gettime(CLOCK_REALTIME, &tstart);

    // struct timespec tend;
    // clock_gettime(CLOCK_REALTIME, &tend);
    // unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    // cout << time << "-trie merge" << endl;
    
    this->forwardIndex->merge();
    if (this->forwardIndex->hasDeletedRecords()) {
      // free the space for deleted records.
      // need the global lock to block other readers
      boost::unique_lock< boost::shared_mutex > lock(globalRwMutexForReadersWriters);
      this->forwardIndex->freeSpaceOfDeletedRecords();
    }

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
        this->invertedIndex->merge();
    
    // Since trie is the entry point of every search, trie merge should be done after all other merges.
    // If forwardIndex or invertedIndex is merged before trie, then users can see an inconsistent state of
    // the index.
    // if it is the case of M1 (geo), invertedIndex is passed as a NULL, so that histogram information is calculated only
    // using frequencies. Otherwise, the invertedIndex will be used to integrate its information with frequencies.
    const InvertedIndex * invertedIndex = NULL;
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex){
    	invertedIndex = this->invertedIndex;
    }

    //Need to block reader for both trie reassignment and quadtree merge in
    //M1
    bool haveGlobalLockForM1 = true;
    boost::unique_lock< boost::shared_mutex > lock(globalRwMutexForReadersWriters);
    if (this->schemaInternal->getIndexType() !=
        srch2::instantsearch::LocationIndex) {
      lock.unlock();
      haveGlobalLockForM1 = false;
    }

    // check if we need to reassign some keyword ids
    if (this->trie->needToReassignKeywordIds()) {

        // struct timespec tstart;
        // clock_gettime(CLOCK_REALTIME, &tstart);


    	// reassign id is not thread safe so we need to grab an exclusive lock
    	// NOTE : all index structure commits are happened before reassign id phase. Only QuadTree is left
    	//        because we need new ids in quadTree commit phase.
        if(!haveGlobalLockForM1) // need locking to block other readers
        	lock.lock();
        this->reassignKeywordIds();
        if(!haveGlobalLockForM1) 
        	lock.unlock();
      
        // struct timespec tend;
        // clock_gettime(CLOCK_REALTIME, &tend);
        // unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 +
        // (double) (tend.tv_nsec - tstart.tv_nsec) / (double)1000000L;
        // cout << "Commit phase: time spent to reassign keyword IDs in the forward index (ms): " << time << endl;
    }

    this->trie->merge(invertedIndex , this->forwardIndex,
    		this->forwardIndex->getTotalNumberOfForwardLists_ReadView() , updateHistogram);
    
    if (this->schemaInternal->getIndexType() == 
        srch2::instantsearch::LocationIndex) {
      this->quadTree->merge();
    }

    this->mergeRequired = false;

    Logger::debug("Merge ends--------------------------------"); 

    return OP_SUCCESS;
}
  
/*
 *
 */
void IndexData::reassignKeywordIds()
{
    map<TrieNode *, unsigned> trieNodeIdMapper; //
    this->trie->reassignKeywordIds(trieNodeIdMapper);

    // Generating an ID mapper by iterating through the set of trie nodes whose
    // ids need to be reassigned
    // a map from temperory id to new ids, this map is used for changing forwardIndex and quadTree
    map<unsigned, unsigned> keywordIdMapper;
    for (map<TrieNode *, unsigned>::iterator iter = trieNodeIdMapper.begin();
            iter != trieNodeIdMapper.end(); ++ iter)
    {
        TrieNode *node = iter->first;
        unsigned newKeywordId = iter->second;

        keywordIdMapper[node->getId()] = newKeywordId;

        node->setId(newKeywordId); // set the new keyword Id
    }

    // TODO: change it to an unordered set
    //std::unordered_set<unsigned> processedRecordIds; // keep track of records that have been converted
    map<unsigned, unsigned> processedRecordIds; // keep track of records that have been converted

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex) // if it's A1 index
    {
        // Now we have the ID mapper.  We want to go through the trie nodes one by one.
        // For each of them, access its inverted list.  For each record,
        // use the id mapper to change the integers on the forward list.
        changeKeywordIdsOnForwardLists(trieNodeIdMapper, keywordIdMapper, processedRecordIds);
    }
    else // if it's M1 index
    {
        changeKeywordIdsOnForwardListsAndOCFilters(keywordIdMapper, processedRecordIds);
        this->quadTree->fixReassignedIds(keywordIdMapper);
    }

}

/*
 * Jamshid : uses the id mapped to replace old ids to new ids in forward list.
 * since we use inverted index to go through all records of a keyword it is possible to visit a record more than once
 * so we use processedRecordIds to remember what records have been reassigned.
 */
void IndexData::changeKeywordIdsOnForwardLists(const map<TrieNode *, unsigned> &trieNodeIdMapper,
                                               const map<unsigned, unsigned> &keywordIdMapper,
                                               map<unsigned, unsigned> &processedRecordIds)
{
	vectorview<unsigned>* &keywordIDsWriteView = this->invertedIndex->getKeywordIds()->getWriteView();

	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
    this->forwardIndex->getForwardListDirectory_ReadView(forwardListDirectoryReadView);

    for (map<TrieNode *, unsigned>::const_iterator iter = trieNodeIdMapper.begin();
            iter != trieNodeIdMapper.end(); ++ iter)
    {
        TrieNode *node = iter->first;

        // the following code is based on TermVirtualList.cpp
        unsigned invertedListId = node->getInvertedListOffset();
        // change the keywordId for given invertedListId
        map<unsigned, unsigned>::const_iterator keywordIdMapperIter = keywordIdMapper.find(invertedListId);
        keywordIDsWriteView->at(invertedListId) = keywordIdMapperIter->second;
        // Jamshid : since it happens after the commit of other index structures it uses read view
        shared_ptr<vectorview<unsigned> > readview;
    	shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    	this->invertedIndex->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
        this->invertedIndex->getInvertedListReadView(invertedListDirectoryReadView,
        		invertedListId, readview);
        unsigned invertedListSize = readview->size();
        // go through each record id on the inverted list
        InvertedListElement invertedListElement;
        for (unsigned i = 0; i < invertedListSize; i ++) {
            /*if (invertedListElement == NULL)
                continue;*/
            unsigned recordId = readview->getElement(i);

            // re-map it only it is not done before
            if (processedRecordIds.find (recordId) == processedRecordIds.end()) {

                this->forwardIndex->reassignKeywordIds(forwardListDirectoryReadView, recordId,keywordIdMapper);
                processedRecordIds[recordId] = 0; // add it to the set 
            }
        }
    }

}
/*
 * Jamshid : uses the IDMapper to change old temperory ids in quadTree to new correct ids
 *
 */
void IndexData::changeKeywordIdsOnForwardListsAndOCFilters(map<unsigned, unsigned> &keywordIdMapper,
                                                           map<unsigned, unsigned> &recordIdsToProcess)
{
    this->quadTree->gatherForwardListsAndAdjustOCFilters(keywordIdMapper, recordIdsToProcess);

	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
    this->forwardIndex->getForwardListDirectory_ReadView(forwardListDirectoryReadView);

    for (map<unsigned, unsigned>::const_iterator citer = recordIdsToProcess.begin();
            citer != recordIdsToProcess.end(); ++ citer)
        this->forwardIndex->reassignKeywordIds(forwardListDirectoryReadView, citer->first, keywordIdMapper);

}

void IndexData::_exportData(const string &exportedDataFileName) const
{
    ForwardIndex::exportData(*this->forwardIndex, exportedDataFileName);
}

void IndexData::_deSerialize(std::istream& inputStream) {
	boost::archive::binary_iarchive ia(inputStream);
	IndexVersion storedIndexVersion;
	ia >> storedIndexVersion;
	if (storedIndexVersion != IndexVersion::currentVersion){
		// throw invalid index file exception
		Logger::error("Invalid serialized shard. Either shard was built with a previous version"
				"of engine or migrated from a different machine/architecture.");
		throw exception();
	}

	this->trie = new Trie_Internal();
	ia >> *(this->trie);

	this->schemaInternal = new SchemaInternal();
	ia >> *(this->schemaInternal);

	this->rankerExpression = new RankerExpression(this->schemaInternal->getScoringExpression());

	this->forwardIndex = new ForwardIndex(this->schemaInternal);
	ia >> *(this->forwardIndex);

	this->forwardIndex->setSchema(this->schemaInternal);

	PositionIndexType positionIndexType = this->schemaInternal->getPositionIndexType();
	if(isEnabledAttributeBasedSearch(positionIndexType))
		this->forwardIndex->isAttributeBasedSearch = true;

	if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex){
		this->invertedIndex = new InvertedIndex(this->forwardIndex);
		ia >> *(this->invertedIndex);
		this->invertedIndex->setForwardIndex(this->forwardIndex);
		quadTree = NULL;
	} else {
		this->quadTree = new QuadTree();
		ia >> *(this->quadTree);
		this->quadTree->setForwardIndex(this->forwardIndex);
		this->quadTree->setTrie(this->trie);
		this->invertedIndex = NULL;
	}

    uint64_t readCount_tmp;
    uint32_t writeCount_tmp, numDocs_tmp;
    ia >> readCount_tmp;
    ia >> writeCount_tmp;
    ia >> numDocs_tmp;
    this->readCounter = new ReadCounter(readCount_tmp);
    this->writeCounter = new WriteCounter(writeCount_tmp, numDocs_tmp);

	this->flagBulkLoadDone = true;
}

void IndexData::_serialize(std::ostream& outputStream) const{
	boost::archive::binary_oarchive oa(outputStream);
	//1. Index Version
	oa << IndexVersion::currentVersion;

	if (this->trie->isMergeRequired())
		this->trie->merge(NULL , NULL,  0 , false);

	//2. Trie
	oa << *this->trie;


	//3. Schema Internal
	oa << *this->schemaInternal;

	if(this->forwardIndex->isMergeRequired()) {
		this->forwardIndex->merge();
		if (this->forwardIndex->hasDeletedRecords()) {
			// free the space for deleted records.
			// need the global lock to block other readers
			boost::unique_lock< boost::shared_mutex > lock(globalRwMutexForReadersWriters);
			this->forwardIndex->freeSpaceOfDeletedRecords();
		}
	}
	//4. Forward Index
	oa << *this->forwardIndex;

	//5. Inverted Index or Quad Tree
	if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex) {
		if(this->invertedIndex->mergeRequired())
			this->invertedIndex->merge();
		oa << *this->invertedIndex;

	}else {
		oa << *this->quadTree;
	}

	uint64_t readCount_tmp = this->readCounter->getCount();
	uint32_t writeCount_tmp = this->writeCounter->getCount();
	uint32_t numDocs_tmp = this->writeCounter->getNumberOfDocuments();
	oa << readCount_tmp;
	oa << writeCount_tmp;
	oa << numDocs_tmp;
}

void IndexData::_save(const string &directoryName) const
{
	Serializer serializer;
    if (this->trie->isMergeRequired())
        this->trie->merge(NULL , NULL,  0 , false);
    // serialize the data structures to disk
    try {
        serializer.save(*this->trie, directoryName + "/" + IndexConfig::trieFileName);
    } catch (exception &ex) {
        Logger::error("Error writing trie index file: %s/%s", directoryName.c_str(), IndexConfig::trieFileName);
	// can keep running - don't rethrow exception
    }

    if(this->forwardIndex->isMergeRequired()) {
        this->forwardIndex->merge();
        if (this->forwardIndex->hasDeletedRecords()) {
        	// free the space for deleted records.
        	// need the global lock to block other readers
        	boost::unique_lock< boost::shared_mutex > lock(globalRwMutexForReadersWriters);
        	this->forwardIndex->freeSpaceOfDeletedRecords();
        }
    }

    try {
        serializer.save(*this->forwardIndex, directoryName + "/" + IndexConfig::forwardIndexFileName);
    } catch (exception &ex) {
        Logger::error("Error writing forward index file: %s/%s", directoryName.c_str(), IndexConfig::forwardIndexFileName);
    }

    try {
        serializer.save(*this->schemaInternal, directoryName + "/" + IndexConfig::schemaFileName);
    } catch (exception &ex) {
        Logger::error("Error writing schema index file: %s/%s", directoryName.c_str(), IndexConfig::schemaFileName);
    }

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex) {
    	 if(this->invertedIndex->mergeRequired())
    		 this->invertedIndex->merge();
	 try {
	     serializer.save(*this->invertedIndex, directoryName + "/" +  IndexConfig::invertedIndexFileName);
	 } catch (exception &ex) {
	     Logger::error("Error writing inverted index file: %s/%s", directoryName.c_str(), IndexConfig::invertedIndexFileName);
	 }
    }
    else {
        try {
	    serializer.save(*this->quadTree, directoryName + "/" + IndexConfig::quadTreeFileName);
	} catch (exception &ex) {
	    Logger::error("Error writing quad tree index file: %s/%s", directoryName.c_str(), IndexConfig::quadTreeFileName);
	}
    }

    try {
        this->saveCounts(directoryName + "/" + IndexConfig::indexCountsFileName);
    } catch (exception &ex) {
        Logger::error("Error writing index counts file: %s/%s", directoryName.c_str(), IndexConfig::indexCountsFileName);
    }
}

void IndexData::printNumberOfBytes() const
{
    Logger::debug("Number Of Bytes:");
    Logger::debug("Trie:\t\t %d bytes\t %.5f MB", this->trie->getNumberOfBytes(), (float)this->trie->getNumberOfBytes()/1048576);
    Logger::debug("ForwardIndex:\t %d bytes\t %.5f MB", this->forwardIndex->getNumberOfBytes(), (float)this->forwardIndex->getNumberOfBytes()/1048576);
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex){
        Logger::debug("InvertedIndex:\t %d bytes\t %.5f MB", this->invertedIndex->getNumberOfBytes(), (float)this->invertedIndex->getNumberOfBytes()/1048576);
    }
}

const Schema* IndexData::getSchema() const
{
    return dynamic_cast<const Schema *>(this->schemaInternal);
}

Schema* IndexData::getSchema()
{
    return dynamic_cast<Schema *>(this->schemaInternal);
}


void IndexData::loadCounts(const std::string &indeDataPathFileName)
{
    std::ifstream ifs(indeDataPathFileName.c_str(), std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    uint64_t readCount_tmp;
    uint32_t writeCount_tmp, numDocs_tmp;
    ia >> readCount_tmp;
    ia >> writeCount_tmp;
    ia >> numDocs_tmp;
    this->readCounter = new ReadCounter(readCount_tmp);
    this->writeCounter = new WriteCounter(writeCount_tmp, numDocs_tmp);
    ifs.close();
}

void IndexData::saveCounts(const std::string &indeDataPathFileName) const
{
    std::ofstream ofs(indeDataPathFileName.c_str(), std::ios::binary);
if (! ofs.good()) throw std::runtime_error("Error opening " + indeDataPathFileName);
    boost::archive::binary_oarchive oa(ofs);
    uint64_t readCount_tmp = this->readCounter->getCount();
    uint32_t writeCount_tmp = this->writeCounter->getCount();
    uint32_t numDocs_tmp = this->writeCounter->getNumberOfDocuments();
    oa << readCount_tmp;
    oa << writeCount_tmp;
    oa << numDocs_tmp;
    ofs.close();
}

IndexData::~IndexData()
{
    delete this->trie;
    delete this->forwardIndex;

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
    {
        delete this->invertedIndex;
    }
    else
        delete this->quadTree;

    delete this->schemaInternal;
    delete this->readCounter;
    delete this->writeCounter;
    delete this->rankerExpression;
}




}}
