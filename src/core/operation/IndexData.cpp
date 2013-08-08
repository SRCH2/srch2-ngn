
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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include "IndexData.h"
#include "record/SchemaInternal.h"
#include "index/IndexUtil.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"
#include "util/ReadWriteMutex.h"  // for locking
#include "util/Assert.h"
#include "util/Logger.h"
#include "geo/QuadTree.h"
#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"
#include <instantsearch/Record.h>
#include <instantsearch/Analyzer.h>
#include "util/FileOps.h"

#include <stdio.h>  /* defines FILENAME_MAX */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <exception>

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
        Schema *schema,
        const string &trieBootstrapFileNameWithPath,
        const StemmerNormalizerFlagType &stemmerFlag)
{

    this->directoryName = directoryName;

    if(!checkDirExistence(directoryName.c_str())){
		if(createDir(directoryName.c_str()) == -1){
			exit(1);
		}
	}

    /* Create a copy of analyzer as the user created analyzer can be dereferenced by the user any time.
     * Shared pointer could be one solution to overcome this.
     * //TODO Need to serialise Analyzer for stopwords
     */
    // get the analyzer type to instantiate a new analyzer
    this->analyzer = new Analyzer(*analyzer);

    this->schemaInternal = new SchemaInternal( *(dynamic_cast<SchemaInternal *>(schema)) );

    this->rankerExpression = new RankerExpression(this->schemaInternal->getScoringExpression());

    this->trie = new Trie_Internal();

    this->forwardIndex = new ForwardIndex(this->schemaInternal);
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
        this->invertedIndex =new  InvertedIndex(this->forwardIndex);
    else
        this->quadTree = new QuadTree(this->forwardIndex, this->trie);

    //Starting offset
    this->kafkaOffset_CurrentIndexSnapshot = 0;
    this->readCounter = new ReadCounter();
    this->writeCounter = new WriteCounter();
    this->commited = false;

    this->addBootstrapKeywords(trieBootstrapFileNameWithPath);

    this->rwMutexForIdReassign = new ReadWriteMutex(100); // for locking, <= 100 threads
}

IndexData::IndexData(const string& directoryName)
{
    this->directoryName = directoryName;

    if(!checkDirExistence(directoryName.c_str())){
		if(createDir(directoryName.c_str()) == -1){
			exit(1);
		}
	}

    std::ifstream ifs((directoryName+"/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);
	AnalyzerType analyzerType;
	ia >> analyzerType;

	this->analyzer = new Analyzer(
	                    DISABLE_STEMMER_NORMALIZER,
                        "",
                        "",
                        "",
                        SYNONYM_DONOT_KEEP_ORIGIN,
                        "",
                        analyzerType);

    this->analyzer->load(ia);

    ifs.close();
    //this->analyzerInternal->setIndexDirectory(directoryName);

    this->schemaInternal = new SchemaInternal();
    SchemaInternal::load(*(this->schemaInternal), directoryName + "/" + string(IndexConfig::schemaFileName));

    this->rankerExpression = new RankerExpression(this->schemaInternal->getScoringExpression());

    this->trie = new Trie_Internal();
    this->forwardIndex = new ForwardIndex(this->schemaInternal);
    Trie_Internal::load(*(this->trie),directoryName + "/" + IndexConfig::trieFileName);
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
        this->invertedIndex =new  InvertedIndex(this->forwardIndex);

    // set if it's a attributeBasedSearch
    if(this->schemaInternal->getPositionIndexType() == srch2::instantsearch::FIELDBITINDEX)
    	ForwardList::isAttributeBasedSearch = true;

    ForwardIndex::load(*(this->forwardIndex), directoryName + "/" + IndexConfig::forwardIndexFileName);
    this->forwardIndex->setSchema(this->schemaInternal);
    this->forwardIndex->merge();// to force create a separate view for writes.

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
    {
        InvertedIndex::load(*(this->invertedIndex), directoryName + "/" +  IndexConfig::invertedIndexFileName);
        this->invertedIndex->setForwardIndex(this->forwardIndex);
    }
    else
    {
        this->quadTree = new QuadTree();
        QuadTree::load(*(this->quadTree), directoryName + "/" +  IndexConfig::quadTreeFileName);
        this->quadTree->setForwardIndex(this->forwardIndex);
        this->quadTree->setTrie(this->trie);
        //cout << "QuadTree loaded" << endl;
    }
    
    this->loadCounts(directoryName + "/" + IndexConfig::indexCountsFileName);
    this->commited = true;

    this->rwMutexForIdReassign = new ReadWriteMutex(100); // for locking, <= 100 threads
}

/// Add a record
INDEXWRITE_RETVAL IndexData::_addRecord(const Record *record)
{
    INDEXWRITE_RETVAL returnValue = OP_FAIL; // not added

    /// Get the internalRecordId
    unsigned internalRecordId;
    //Check for duplicate record
    bool recordPrimaryKeyFound = this->forwardIndex->getInternalRecordId_WriteView(record->getPrimaryKey(), internalRecordId);

    // InternalRecordId == ForwardListId
    if (recordPrimaryKeyFound == false)
    {
        this->writeCounter->incWritesCounter();
        this->writeCounter->incDocsCounter();
        returnValue = OP_SUCCESS;

        this->mergeRequired = true;
        /// analyze the record (tokenize it, remove stop words)
        map<string, TokenAttributeHits > tokenAttributeHitsMap;
        this->analyzer->tokenizeRecord(record, tokenAttributeHitsMap);

        KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

        // only used for committed geo index
        vector<unsigned> *keywordIdVector = NULL;
        if(this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex
                && this->commited == true)
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

            if (this->commited == false) // not committed yet
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
                if (this->commited == true)
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
        // Sort keywordList
        std::sort( keywordIdList.begin(), keywordIdList.end() );

        unsigned internalRecordId;
        this->forwardIndex->appendExternalRecordId_WriteView(record->getPrimaryKey(), internalRecordId);
        this->forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

        if ( this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex )
        {
            if ( this->commited == true )
            {
                const unsigned totalNumberofDocuments = this->forwardIndex->getTotalNumberOfForwardLists_WriteView();
                ForwardList *forwardList = this->forwardIndex->getForwardList_ForCommit(internalRecordId);
                this->invertedIndex->addRecord(forwardList, this->rankerExpression,
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

            if ( this->commited == false ) // batch load
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

void IndexData::addBootstrapKeywords(const string &trieBootstrapFileNameWithPath)
{
    try
    {
        std::ifstream infile;
        infile.open (trieBootstrapFileNameWithPath.c_str());
        if (infile.good())
        {
            std::string line;
            while ( std::getline(infile, line) )
            {
                std::vector<std::string> keywords;
                //char c = '.';
//                this->analyzerInternal->tokenizeQuery(line, keywords); iman: previous one
                this->analyzer->tokenizeQuery(line, keywords);

                for (std::vector<std::string>::const_iterator kiter = keywords.begin();
                            kiter != keywords.end();
                            ++kiter)
                {
                    /// add words to trie
                    unsigned invertedIndexOffset = 0;
                    unsigned keywordId = 0;
                    keywordId = this->trie->addKeyword(getCharTypeVector(*kiter), invertedIndexOffset);
                    this->invertedIndex->incrementDummyHitCount(invertedIndexOffset);
                }
            }

            // All the dummy keywords that are used to bootstrap trie have a unique keywordId and hence a invertedList.
            // The invertedList cannot be of size 0 and also, it is initialised to have "0"s.
            // We create a dummy first record to occupy internalRid "0" in forwardList.

            this->forwardIndex->addDummyFirstRecord();
        }
    }
    catch (std::exception& e)
    {
        Logger::error("trie bootstrap with english dictionary failed. File read error");
    }
}

// delete a record with a specific id //TODO Give the correct return message for delete pass/fail
INDEXWRITE_RETVAL IndexData::_deleteRecord(const std::string &externalRecordId)
{
    INDEXWRITE_RETVAL success = this->forwardIndex->deleteRecord_WriteView(externalRecordId)  ? OP_SUCCESS: OP_FAIL;

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
    INDEXWRITE_RETVAL success = this->forwardIndex->deleteRecordGetInternalId_WriteView(externalRecordId, internalRecordId)  ? OP_SUCCESS: OP_FAIL;

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
    INDEXWRITE_RETVAL success = this->forwardIndex->recoverRecord_WriteView(externalRecordId, internalRecordId)  ? OP_SUCCESS: OP_FAIL;

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
    return this->forwardIndex->lookupRecord_WriteView(externalRecordId);
}

/* build the index. After commit(), no more records can be added.
 *
 * There has to be at least one record in Index for the commit to succeed.
 * Returns 1, if the committing succeeded.
 * Returns 0, if the committing failed, which is in the following two cases.
 *    a) No records in index.
 *    b) Index had been already commited.
 */
INDEXWRITE_RETVAL IndexData::_commit()
{
    bool isLocational = false;
    if(this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex)
        isLocational = true;

    if (this->commited == false)
    {
        //cout << "here: index commit" << endl;
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

        /*{
                struct timespec tstart;
                clock_gettime(CLOCK_REALTIME, &tstart);
*/
        this->forwardIndex->merge();

/*
            struct timespec tend;
            clock_gettime(CLOCK_REALTIME, &tend);
            unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
            cout << time << "FL commit" << endl;
        }
*/

        const unsigned totalNumberofDocuments = this->forwardIndex->getTotalNumberOfForwardLists_WriteView();

        //Check for case, where in commit() is called without any records added to the index.
        if (totalNumberofDocuments == 0)
            return OP_FAIL;//Failed

/*
        {
            struct timespec tstart;
            clock_gettime(CLOCK_REALTIME, &tstart);
*/

        this->trie->commit();

        //this->trie->print_Trie();

/*
            struct timespec tend;
            clock_gettime(CLOCK_REALTIME, &tend);
            unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
            cout << time << "KYindex commit" << endl;
        }
*/

        const vector<unsigned> *oldIdToNewIdMapVector = this->trie->getOldIdToNewIdMapVector();

        map<unsigned, unsigned> oldIdToNewIdMapper;
        for (unsigned i = 0; i < oldIdToNewIdMapVector->size(); i++)
          oldIdToNewIdMapper[i] = oldIdToNewIdMapVector->at(i);

        if(!isLocational)
            this->invertedIndex->initialiseInvertedIndexCommit();

        // Measuring the time to change the keyword ids in the forward index
        // struct timespec tstart;
        // clock_gettime(CLOCK_REALTIME, &tstart);

        for (unsigned forwardIndexIter = 0; forwardIndexIter < totalNumberofDocuments; ++forwardIndexIter)
        {
            ForwardList *forwardList = this->forwardIndex->getForwardList_ForCommit(forwardIndexIter);
            vector<NewKeywordIdKeywordOffsetTriple> newKeywordIdKeywordOffsetTriple;
            //this->forwardIndex->commit(forwardList, oldIdToNewIdMapVector, newKeywordIdKeywordOffsetTriple);
            this->forwardIndex->commit(forwardList, oldIdToNewIdMapper, newKeywordIdKeywordOffsetTriple);
            if(!isLocational)
                this->invertedIndex->commit(forwardList, this->rankerExpression,
                        forwardIndexIter, totalNumberofDocuments, this->schemaInternal, newKeywordIdKeywordOffsetTriple);

            /*
            if (forwardIndexIter%1000 == 0)
            {
              std::cout << "\rPass 2: Indexing  - " << forwardIndexIter;
            }
            if (forwardIndexIter%99999 == 0)
            {
                std::cout << "\r";
                }*/
        }

        // struct timespec tend;
        // clock_gettime(CLOCK_REALTIME, &tend);
        // unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + 
        //   (double) (tend.tv_nsec - tstart.tv_nsec) / (double)1000000L;
        // cout << "Commit phase: time spent to reassign keyword IDs in the forward index (ms): " << time << endl;

/*
        {
                struct timespec tstart;
                clock_gettime(CLOCK_REALTIME, &tstart);
*/

        this->forwardIndex->finalCommit();

//        this->forwardIndex->print_size();
/*

            struct timespec tend;
            clock_gettime(CLOCK_REALTIME, &tend);
            unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
            cout << time << "FL commit" << endl;
        }
*/

/*
        {
            struct timespec tstart;
            clock_gettime(CLOCK_REALTIME, &tstart);
*/

        if (isLocational)
        {
            //time_t begin,end;
            //time(&begin);
            this->quadTree->createFilters();
            //time(&end);
            //std::cout << "CFilters and OFilters creating time elapsed: " << difftime(end, begin) << " seconds"<< std::endl;
        }
        else
        {
            this->invertedIndex->setForwardIndex(this->forwardIndex);
            this->invertedIndex->finalCommit();
        }

/*
            struct timespec tend;
            clock_gettime(CLOCK_REALTIME, &tend);
            unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
            cout << time << "IL commit" << endl;
        }
*/

/*

        {
            struct timespec tstart;
            clock_gettime(CLOCK_REALTIME, &tstart);
*/


        // delete the keyword mapper (from the old ids to the new ids) inside the trie
        this->trie->deleteOldIdToNewIdMapVector();

/*
            struct timespec tend;
            clock_gettime(CLOCK_REALTIME, &tend);
            unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
            cout << time << "KYindex clean" << endl;
        }
*/

        //this->trie->print_Trie();
        this->commited = true;
        return OP_SUCCESS;
    }
    else
    {
        return OP_FAIL;
    }
}

INDEXWRITE_RETVAL IndexData::_merge()
{
    Logger::debug("Merge begins--------------------------------"); 

    if (!this->mergeRequired)
        return OP_FAIL;
    
    // struct timespec tstart;
    // clock_gettime(CLOCK_REALTIME, &tstart);
    
    this->trie->merge();
    
    // struct timespec tend;
    // clock_gettime(CLOCK_REALTIME, &tend);
    // unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    // cout << time << "-trie merge" << endl;
    
    this->forwardIndex->merge();

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
        this->invertedIndex->merge();
    
    // check if we need to reassign some keyword ids
    if (this->trie->needToReassignKeywordIds()) {

        // struct timespec tstart;
        // clock_gettime(CLOCK_REALTIME, &tstart);


    	// reassign id is not thread safe so we need to grab an exclusive lock
    	// NOTE : all index structure commits are happened before reassign id phase. Only QuadTree is left
    	//        because we need new ids in quadTree commit phase.
        this->rwMutexForIdReassign->lockWrite(); // need locking to block other readers
        this->reassignKeywordIds();
        this->rwMutexForIdReassign->unlockWrite();
      
        // struct timespec tend;
        // clock_gettime(CLOCK_REALTIME, &tend);
        // unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 +
        // (double) (tend.tv_nsec - tstart.tv_nsec) / (double)1000000L;
        // cout << "Commit phase: time spent to reassign keyword IDs in the forward index (ms): " << time << endl;
    }
    
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::LocationIndex)
        this->quadTree->merge();

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
        this->invertedIndex->getInvertedListReadView(invertedListId, readview);
        unsigned invertedListSize = readview->size();
        // go through each record id on the inverted list
        InvertedListElement invertedListElement;
        for (unsigned i = 0; i < invertedListSize; i ++) {
            /*if (invertedListElement == NULL)
                continue;*/
            unsigned recordId = readview->getElement(i);

            // re-map it only it is not done before
            if (processedRecordIds.find (recordId) == processedRecordIds.end()) {

                this->forwardIndex->reassignKeywordIds(recordId, keywordIdMapper);
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

    for (map<unsigned, unsigned>::const_iterator citer = recordIdsToProcess.begin();
            citer != recordIdsToProcess.end(); ++ citer)
        this->forwardIndex->reassignKeywordIds(citer->first, keywordIdMapper);

}

void IndexData::_save(const string &directoryName) const
{
    // serialize the data structures to disk
    Trie_Internal::save(*this->trie, directoryName + "/" + IndexConfig::trieFileName);
    //this->forwardIndex->print_test();
    //this->invertedIndex->print_test();
    ForwardIndex::save(*this->forwardIndex, directoryName + "/" + IndexConfig::forwardIndexFileName);
    SchemaInternal::save(*this->schemaInternal, directoryName + "/" + IndexConfig::schemaFileName);
    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
        InvertedIndex::save(*this->invertedIndex, directoryName + "/" +  IndexConfig::invertedIndexFileName);
    else
        QuadTree::save(*this->quadTree, directoryName + "/" + IndexConfig::quadTreeFileName);
    std::ofstream ofs((directoryName+"/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << this->analyzer->getAnalyzerType();
    this->analyzer->save(oa);
    ofs.close();
    this->saveCounts(directoryName + "/" + IndexConfig::indexCountsFileName);
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

const Analyzer* IndexData::getAnalyzer() const
{
    return this->analyzer;
}

const Schema* IndexData::getSchema() const
{
    return dynamic_cast<const Schema *>(this->schemaInternal);
}

IndexData::~IndexData()
{
    delete this->analyzer;
    delete this->trie;
    delete this->forwardIndex;

    if (this->schemaInternal->getIndexType() == srch2::instantsearch::DefaultIndex)
    {
        delete this->invertedIndex;
    }
    else
        delete this->quadTree;

    delete this->schemaInternal;
    delete this->rwMutexForIdReassign;
}

}}
