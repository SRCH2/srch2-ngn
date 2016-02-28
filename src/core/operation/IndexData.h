/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <instantsearch/Constants.h>

#include "index/Trie.h"
#include "index/ForwardIndex.h"
#include "geo/QuadTree.h"
#include "util/RankerExpression.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

using std::vector;
using std::string;
using std::map;

namespace srch2
{
namespace instantsearch
{
/*
 *  These are forward declaration of required classes and typedefs.
 *  It would be nice to move them to a separate forward declaration header file
 */
class InvertedIndex;
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
	void init(InvertedIndex * invertedIndex, ForwardIndex * forwardIndex,
			Trie * trie, QuadTree * quadTree, const Schema * schema){
		this->invertedIndex = invertedIndex;
		this->forwardIndex = forwardIndex;
		this->trie = trie;
		this->quadTree = quadTree;
		this->schema = schema;
	}

    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr trieRootNodeSharedPtr;

    typedef boost::shared_ptr<vectorview<ForwardListPtr> > ForwardIndexReadView;
    ForwardIndexReadView forwardIndexReadViewSharedPtr;

    typedef boost::shared_ptr<vectorview<InvertedListContainerPtr> > InvertedIndexReadView;
    InvertedIndexReadView invertedIndexReadViewSharedPtr;

    typedef boost::shared_ptr<vectorview<unsigned> > InvertedIndexKeywordIdsReadView;
    InvertedIndexKeywordIdsReadView invertedIndexKeywordIdsReadViewSharedPtr;

    typedef boost::shared_ptr<QuadTreeRootNodeAndFreeLists> QuadTreeRootNodeSharedPtr;
    QuadTreeRootNodeSharedPtr quadTreeRootNodeSharedPtr;

    /*
     * When this method is called all shared pointers are reset meaning
     * that reader has lost them.
     */
    void resetSharedPointers(){
    	trieRootNodeSharedPtr.reset();
    	forwardIndexReadViewSharedPtr.reset();
    	invertedIndexReadViewSharedPtr.reset();
    	invertedIndexKeywordIdsReadViewSharedPtr.reset();
    	quadTreeRootNodeSharedPtr.reset();
    }


    /////////////////// Inverted Index Access Methods
    void getInvertedListReadView(const unsigned invertedListId, shared_ptr<vectorview<unsigned> >& invertedListReadView) ;

    // given a forworListId and invertedList offset, return the keyword offset
    unsigned getKeywordOffset(unsigned forwardListId, unsigned invertedListOffset) ;

    bool isValidTermPositionHit(unsigned forwardListId, unsigned keywordOffset,
            const vector<unsigned>& filterAttributesList, ATTRIBUTES_OP attrOp,
            vector<unsigned>& matchingKeywordAttributesList, float &termRecordStaticScore) ;

    ////////////////// Forward Index Access Methods
    const ForwardList *getForwardList(unsigned recordId, bool &valid);

    bool hasAccessToForwardList(unsigned recordId, string &roleId);

    // do binary search to probe in forward list
    bool haveWordInRange(const unsigned recordId, const unsigned minId, const unsigned maxId,
            const vector<unsigned>& filteringAttributesList,
            ATTRIBUTES_OP attrOp,
            unsigned &matchingKeywordId, vector<unsigned>& matchingKeywordAttributesList,
            float &matchingKeywordRecordStaticScore)  ;

    bool getExternalRecordIdFromInternalRecordId(const unsigned internalRecordId, std::string &externalRecordId);

    bool getInternalRecordIdFromExternalRecordId(const std::string &externalRecordId, unsigned &internalRecordId) ;
    /////////////////////// Trie Access Methods
    const TrieNode *getTrieNodeFromUtf8String(const std::string &keywordStr) ;
    void getPrefixString(const TrieNode* trieNode, std::string &in) ;

    void getPrefixString(const TrieNode* trieNode, std::vector<CharType> &in) ;


private:
    InvertedIndex * invertedIndex;
    ForwardIndex * forwardIndex;
    Trie * trie;
    QuadTree * quadTree;
    const Schema * schema;

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

// we use this permission map for deleting a role core. then we can delete this role id from resources' access list
// we don't need to use lock for permission map because only writers use this data and the engine makes
// sure only one writer can access the indexes at any time.
class PermissionMap{
public:
	// Adds resource id to some of the role ids.
	// for each role id if it exists in the permission map it will add this resource id to its vector
	// otherwise it adds new record to the map with this role id and then adds this resource id to it.
	void appendResourceToRoles(const string &resourceId, vector<string> &roleIds);

	// Deletes resource id from the role ids.
	void deleteResourceFromRoles(const string &resourceId, vector<string> &roleIds);

	// return the resource ids for the given role id
	// notice that we can use this without lock because only one writer at a moment use this permission map (we only have one writer in the system)
	vector<string>* getResourceIdsForRole(const string &roleId){
		map<string, vector<string> >::iterator it = permissionMap.find(roleId);
		if( it != permissionMap.end()){
			return &(it->second);
		}else{
			return NULL;
		}
	}

	// deletes a role id from the permission map
	void deleteRole(const string &roleId){
		map<string, vector<string> >::iterator it = permissionMap.find(roleId);
		if( it != permissionMap.end() ){
			permissionMap.erase(it);
		}
	}

private:
	map<string, vector<string> > permissionMap;

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & this->permissionMap;
	}
};

class CacheManager;

class IndexData
{
private:

    ///Added for stemmer integration
    IndexData(const string& directoryName, Analyzer *analyzer, Schema *schema,
            const StemmerNormalizerFlagType &stemmerFlag);
            
    IndexData(const string& directoryName);

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

    // This function is called when a necessary lock is already acquired (in M1) or no lock is acquired (in A1).
    INDEXWRITE_RETVAL _addRecordWithoutLock(const Record *record, Analyzer *analyzer);
public:
    
    inline static IndexData* create(const string& directoryName,
    			Analyzer *analyzer,
                Schema *schema,
                const StemmerNormalizerFlagType &stemmerFlag = srch2::instantsearch::DISABLE_STEMMER_NORMALIZER)
    { 
        return new IndexData(directoryName, analyzer,schema, stemmerFlag );
    }
    
    inline static IndexData* load(const string& directoryName)
    {
        return new IndexData(directoryName);
    }

    Trie_Internal *trie;
    InvertedIndex *invertedIndex;

    QuadTree *quadTree;

    ForwardIndex *forwardIndex;
    SchemaInternal *schemaInternal;
    
    AttributeAccessControl *attributeAcl;

    RankerExpression *rankerExpression;

    // we store a map from role ids to resource ids. then when we delete a record from a role core
    // we can use this map to delete the id of this record from the access lists of the resource records
    // Notice that this map is only used by a writer, and it is never used by a reader. So concurrency control is simple.
    PermissionMap* permissionMap;

    // a global RW lock for readers and writers;
    // Used in several places, such as KeywordIdReassign and memory
    // recollection for deleted records
    mutable boost::shared_mutex globalRwMutexForReadersWriters;
    

    inline bool isMergeRequired() const{
    	return mergeRequired;
    }

    virtual ~IndexData();

    void getReadView(IndexReadStateSharedPtr_Token &readToken);
    void initializeIndexReadTokenHolder(IndexReadStateSharedPtr_Token & token) const;

    // add a record
    INDEXWRITE_RETVAL _addRecord(const Record *record, Analyzer *analyzer);
    
    // Edit role ids of a record's access list based on command type
    INDEXWRITE_RETVAL _aclModifyRecordAccessList(const std::string& resourcePrimaryKeyID, vector<string> &roleIds, RecordAclCommandType commandType);

    // Deletes the role id from the permission map
    // we use this function for deleting a record from a role core
    // then we need to delete this record from the permission map of the resource cores of this core
    INDEXWRITE_RETVAL _aclRoleRecordDelete(const std::string& rolePrimaryKeyID);

    inline uint64_t _getReadCount() const { return this->readCounter->getCount(); }
    inline uint32_t _getWriteCount() const { return this->writeCounter->getCount(); }
    inline uint32_t _getNumberOfDocumentsInIndex() const { return this->writeCounter->getNumberOfDocuments(); }
    
    // merge the index
    INDEXWRITE_RETVAL _merge(CacheManager *cache, bool updateHistogram);

     // delete a record with a specific id
    INDEXWRITE_RETVAL _deleteRecord(const std::string &externalRecordId);

    INDEXWRITE_RETVAL _deleteRecordGetInternalId(const std::string &externalRecordId, unsigned &internalRecordId);
    
    // recover a deleted record
    INDEXWRITE_RETVAL _recoverRecord(const std::string &externalRecordId, unsigned internalRecordId);

    // check if a record exists
    INDEXLOOKUP_RETVAL _lookupRecord(const std::string &externalRecordId, unsigned& internalRecordId) const;

    // build the index. After commit(), no more records can be added
    INDEXWRITE_RETVAL finishBulkLoad();

    const bool isBulkLoadDone() const { return this->flagBulkLoadDone; }

    void _exportData(const string& exportedDataFileName) const;

    void _save(CacheManager *cache) const { this->_save(cache, this->directoryName); }

    void _save(CacheManager *cache, const std::string &directoryName) const;
    
    const Schema* getSchema() const;

    Schema* getSchema();

    const std::string& getLicenseFileNameWithPath() const { return this->licenseFileNameWithPath; }

    StoredRecordBuffer getInMemoryData(unsigned internalRecordId) const
    {
        return this->forwardIndex->getInMemoryData(internalRecordId);
    }

    void printNumberOfBytes() const;

    void reassignKeywordIds();
    void changeKeywordIdsOnForwardLists(const map<TrieNode *, unsigned> &trieNodeIdMapper,
                                        const map<unsigned, unsigned> &keywordIdMapper,
                                        map<unsigned, unsigned> &processedRecordIds);
};

}}

#endif /* __INDEXDATA_H__ */
