
// $Id: IndexData.h 3480 2013-06-19 08:00:34Z iman $

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

/*
   *    States of a record in the index.
   *
   *    0. Never exists
   *          - Readview doesn't has this record on ForwardIndex.
   *          - Writeview doesn't has this record on ForwardIndex. 
   *          - The externalToInternalRecordIdMap doesn't have this record on it.
   *
   *    1. Present to reader now and after next merge
   *          - Readview has this record on ForwardIndex with the True flag.
   *          - Writeview has this record on ForwardIndex with the True flag.
   *          - The externalToInternalRecordIdMap has this record on it.
   *
   *    2. Newly inserted, will be present to reader after next merge
   *          - Readview 
   *                - 2.1 doesn't has this record on ForwardIndex.
   *                - 2.2 has this record on ForwardIndex with the False flag.
   *          - Writeview has this record on ForwardIndex with the True flag.
   *          - The externalToInternalRecordIdMap has this record on it.
   *
   *    3. Newly deleted, will be absent to reader after next merge
   *          - Readview has this record on ForwardIndex with the True flag.
   *          - Writeview has this record on ForwardIndex with the False flag.
   *          - The externalToInternalRecordIdMap doesn't have this record on it.
   *
   *    4. Absent to reader now and after next merge
   *          - Readview has this record on ForwardIndex with the False flag.
   *          - Writeview has this record on ForwardIndex with the False flag.
   *          - The externalToInternalRecordIdMap doesn't have this record on it.
   *
   *    (0) --> insert --> (2.1) --> merge --> (1)
   *    (1) --> delete --> (3) --> merge --> (4)
   *    (4) --> insert --> (2.2) --> merge --> (1)
   *
   *    Right now in code, we only define three states in include/instantsearch/Record.h
   *        LU_PRESENT_IN_READVIEW_AND_WRITEVIEW - (1)
   *        LU_TO_BE_INSERTED - (2)
   *        LU_ABSENT_OR_TO_BE_DELETED - (3), (4)
   *
   */

#ifndef __INDEXDATA_H__
#define __INDEXDATA_H__

#include <instantsearch/Indexer.h>
#include <instantsearch/Record.h>

//#include "index/IndexUtil.h"
#include "index/Trie.h"
//#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"
//#include "geo/QuadTree.h"
//#include "record/AnalyzerInternal.h"
//#include "record/SchemaInternal.h"
//#include "license/LicenseVerifier.h"
//#include "operation/Cache.h"
#include "util/RankerExpression.h"

#include <string>
#include <vector>
#include <memory>

using std::vector;
using std::string;

namespace srch2
{
namespace instantsearch
{
/*
 *  Surendra: These are forward declaration of required classes and typedefs.
 *  It would be nice to move them to a separate forward declaration header file
 */
class InvertedIndex;
class QuadTree;
//class ForwardIndex;
class Analyzer;
class SchemaInternal;
class ForwardList;
typedef std::pair<ForwardList*, bool> ForwardListPtr;
class InvertedListContainer;
typedef InvertedListContainer* InvertedListContainerPtr;


typedef Trie Trie_Internal;
//{D-1}: Typedef is not used anywhere
//typedef TrieNode TrieNode_Internal;

struct IndexReadStateSharedPtr_Token
{
    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr trieRootNodeSharedPtr;

    typedef boost::shared_ptr<vectorview<ForwardListPtr> > ForwardIndexReadView;
    ForwardIndexReadView forwardIndexReadViewSharedPtr;

    typedef boost::shared_ptr<vectorview<InvertedListContainerPtr> > InvertedIndexReadView;
    InvertedIndexReadView invertedIndexReadViewSharedPtr;
};

// Uses spinlock and volatile to increment count.
class ReadCounter
{
    public:
        ReadCounter(uint64_t counter = 0)
        {
            pthread_spin_init(&m_spinlock, 0);
            this->counter = counter;
        }
        
        ~ReadCounter()
        {
            pthread_spin_destroy(&m_spinlock);
        }
                
        void increment()
        {
            pthread_spin_lock(&m_spinlock);
            ++this->counter;
            pthread_spin_unlock(&m_spinlock);   
        }
    
        uint64_t getCount() const
        {
            return counter;
        }
            
    private:
        volatile uint64_t counter;
        mutable pthread_spinlock_t m_spinlock;
};

// Assumes the calls to increment are write safe. The caller hold a write lock.
class WriteCounter
{
    public:
        WriteCounter(uint32_t counter = 0, uint32_t numberOfDocumentsIndex = 0)
        {
            this->counter = counter;
            this->numberOfDocumentsIndex = numberOfDocumentsIndex;
        }
        
        ~WriteCounter() { }
                
        void incWritesCounter()
        {
            ++this->counter;
        }

        void decWritesCounter()
        {
            --this->counter;
        }

        void incDocsCounter()
        {
            ++this->numberOfDocumentsIndex;
        }

        void decDocsCounter()
        {
            --this->numberOfDocumentsIndex;
        }
    
        uint32_t getCount() const
        {
            return counter;
        }

        uint32_t getNumberOfDocuments() const
        {
            return numberOfDocumentsIndex;
        }
            
    private:
        uint32_t counter; // number of documents ever added
        uint32_t numberOfDocumentsIndex;
};

class IndexData
{
private:

    ///Added for stemmer integration
    IndexData(const string& directoryName, const Schema *schema);
            
    //To save the directory name to save the trieIndex
    string directoryName;
    bool flagBulkLoadDone;
    bool mergeRequired;
    std::string licenseFileNameWithPath;

    
    ReadCounter *readCounter;
    WriteCounter *writeCounter;

    
    /**
     * Internal functions
     */
    void loadCounts(const std::string &indeDataPathFileName);
    void saveCounts(const std::string &indeDataPathFileName) const;

public:
    
    inline static IndexData* create(const string& directoryName,
                const Schema *schema)
    { 
        return new IndexData(directoryName, schema);
    }
    
    inline static IndexData* create(const string& directoryName)
    {
        return new IndexData(directoryName, NULL);
    }

//    inline static IndexData* load(std::istream& inputByteStream, const string& directoryName)
//    {
//    	return new IndexData(inputByteStream, directoryName);
//    }

    string getStoredIndexDirectory() {
    	return directoryName;
    }

    Trie_Internal *trie;
    InvertedIndex *invertedIndex;
    QuadTree *quadTree;

    ForwardIndex *forwardIndex;
    SchemaInternal *schemaInternal;
    bool hasSchema;
    
    RankerExpression *rankerExpression;
    // a global RW lock for readers and writers;
    // Used in several places, such as KeywordIdReassign and memory
    // recollection for deleted records
    mutable boost::shared_mutex globalRwMutexForReadersWriters;
    
    inline bool isMergeRequired() const{
    	return mergeRequired;
    }

    virtual ~IndexData();

    void getReadView(IndexReadStateSharedPtr_Token &readToken)
    {
        this->trie->getTrieRootNode_ReadView(readToken.trieRootNodeSharedPtr);
        this->readCounter->increment();
    }

    // add a record
    INDEXWRITE_RETVAL _addRecord(const Record *record, Analyzer *analyzer);
    
    inline uint64_t _getReadCount() const { return this->readCounter->getCount(); }
    inline uint32_t _getWriteCount() const { return this->writeCounter->getCount(); }
    inline uint32_t _getNumberOfDocumentsInIndex() const { return this->writeCounter->getNumberOfDocuments(); }
    
    // merge the index
    INDEXWRITE_RETVAL _merge(bool updateHistogram);

     // delete a record with a specific id
    INDEXWRITE_RETVAL _deleteRecord(const std::string &externalRecordId);

    INDEXWRITE_RETVAL _deleteRecordGetInternalId(const std::string &externalRecordId, unsigned &internalRecordId);
    
    // recover a deleted record
    INDEXWRITE_RETVAL _recoverRecord(const std::string &externalRecordId, unsigned internalRecordId);

    // check if a record exists
    INDEXLOOKUP_RETVAL _lookupRecord(const std::string &externalRecordId) const;

    // build the index. After commit(), no more records can be added
    INDEXWRITE_RETVAL finishBulkLoad();

    const bool isBulkLoadDone() const { return this->flagBulkLoadDone; }

    void _exportData(const string& exportedDataFileName) const;

    void _save() const { this->_save(this->directoryName); }

    void _save(const std::string &directoryName) const;
    
    void _bootStrapFromDisk();
    void _bootStrapComponentFromByteSteam(std::istream& inputStream, const string& componentName);
    void _deSerializeSchema(std::istream& inputStream);
    void _deSerializeTrie(std::istream& inputStream);
    void _deSerializeForwardIndex(std::istream& inputStream);
    void _deSerializeInvertedIndex(std::istream& inputStream);
    void _deSerializeLocationIndex(std::istream& inputStream);
    void _deSerializeIndexCounts(std::istream& inputStream);

    void _serialize(std::ostream& outputStream) const;

    const Schema* getSchema() const;

    Schema* getSchema();

    bool containsSchema() { return hasSchema; }

    const std::string& getLicenseFileNameWithPath() const { return this->licenseFileNameWithPath; }

    StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const
    {
        return this->forwardIndex->getInMemoryData(internalRecordId);
    }

    void printNumberOfBytes() const;
    // Surendra: this function is not used anywhere. It creates unnecessary dependency on
    // Trie, InvertedIndex, and ForwardIndex. If you use to wish this. Please move it to the cpp file
    /*void print_Index() const
    {
        this->trie->print_Trie();
        this->invertedIndex->print_test();
        this->forwardIndex->print_test();
    }*/

    void reassignKeywordIds();
    void changeKeywordIdsOnForwardLists(const map<TrieNode *, unsigned> &trieNodeIdMapper,
                                        const map<unsigned, unsigned> &keywordIdMapper,
                                        map<unsigned, unsigned> &processedRecordIds);

    void changeKeywordIdsOnForwardListsAndOCFilters(map<unsigned, unsigned> &keywordIdMapper,
                                                    map<unsigned, unsigned> &recordIdsToProcess);
};

}}

#endif /* __INDEXDATA_H__ */
