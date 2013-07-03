
// $Id: ForwardIndex.h 3480 2013-06-19 08:00:34Z jiaying $
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

 * Copyright © 2010 SRCH2 Inc. All rights reserved */

#pragma once
#ifndef __FORWARDINDEX_H__
#define __FORWARDINDEX_H__

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include <instantsearch/Record.h>
#include "record/SchemaInternal.h"
#include "analyzer/AnalyzerInternal.h"
#include "util/cowvector/cowvector.h"
#include "util/Log.h"
#include "util/encoding.h"
#include "util/half.h"

using std::vector;
using std::fstream;
using std::string;
using std::map;
using std::pair;
using half_float::half;

// The upper bound of the number of keywords in a record is FFFFFF
#define KEYWORD_THRESHOLD ((1<<24) - 1)
// The upper bound of the number of sortable attributes in a record is FF
#define SORTABLE_ATTRIBUTES_THRESHOLD ((1<<8) - 1)

namespace srch2
{
namespace instantsearch
{

typedef vector< pair<unsigned, pair<string, unsigned> > > KeywordIdKeywordStringInvertedListIdTriple;

// for reordering keyword ids
struct KeywordRichInformation
{
    unsigned keywordId;
    float keywordScore;
    unsigned keywordAttribute;
    bool operator < (const KeywordRichInformation& keyword) const
    {
        return keywordId < keyword.keywordId;
    }
};

typedef pair<unsigned, pair<unsigned,unsigned> > NewKeywordIdKeywordOffsetTriple;
struct NewKeywordIdKeywordOffsetPairGreaterThan
{
    bool operator() (const NewKeywordIdKeywordOffsetTriple &lhs, const NewKeywordIdKeywordOffsetTriple &rhs) const
    {
        return lhs.first > rhs.first;
    }
};

class ForwardList
{
public:

	//getter and setter
	// keep numberOfKeywords and numberOfSortableAttributes in an unsigned
	// |numberOfKeywords : numberOfSortableAttributes |
	// | 24 : 8 |
	unsigned getNumberOfKeywords() const
	{
		return (numberOfKeywordsAndNumberOfSortableAttributes & 0xFFFFFF00) >> 8;
	}

	void setNumberOfKeywords(unsigned numberOfKeywords)
	{
		// we consider at most KEYWORD_THRESHOLD keywords
		if(numberOfKeywords <= KEYWORD_THRESHOLD)
			this->numberOfKeywordsAndNumberOfSortableAttributes |= (numberOfKeywords << 8);
		else
			this->numberOfKeywordsAndNumberOfSortableAttributes |= (KEYWORD_THRESHOLD << 8);
	}

	unsigned getNumberOfSortableAttributes() const
	{
		return numberOfKeywordsAndNumberOfSortableAttributes & 0xFF;
	}

	void setNumberOfSortableAttributes(unsigned numberOfSortableAttributes)
	{
		// we consider at most SORTABLE_ATTRIBUTES_THRESHOLD sortableAttributes
		if(numberOfSortableAttributes <= SORTABLE_ATTRIBUTES_THRESHOLD)
			this->numberOfKeywordsAndNumberOfSortableAttributes |= numberOfSortableAttributes;
		else
			this->numberOfKeywordsAndNumberOfSortableAttributes |= SORTABLE_ATTRIBUTES_THRESHOLD;
	}

	float getRecordBoost() const
	{
		return recordBoost;
	}

	void setRecordBoost(float recordBoost)
	{
		this->recordBoost = recordBoost;
	}

	std::string getExternalRecordId() const
	{
		return externalRecordId;
	}

	void setExternalRecordId(std::string externalRecordId)
	{
		this->externalRecordId = externalRecordId;
	}

	std::string getInMemoryData() const
	{
		return inMemoryData;
	}

	void setInMemoryData(std::string inMemoryData)
	{
		this->inMemoryData = inMemoryData;
	}

	const float getSortableAttribute(unsigned iter) const
	{
		return sortableAttributeScores[iter];
	}

	void setSortableAttribute(unsigned iter, float sortableAttributeScore)
	{
		if(iter <= SORTABLE_ATTRIBUTES_THRESHOLD)
			this->sortableAttributeScores[iter] = sortableAttributeScore;
	}

	const unsigned* getKeywordIds() const
	{
		return keywordIds;
	}

	const unsigned getKeywordId(unsigned iter) const
	{
		return keywordIds[iter];
	}

	void setKeywordId(unsigned iter, unsigned keywordId)
	{
		if(iter <= KEYWORD_THRESHOLD)
			this->keywordIds[iter] = keywordId;
	}

	const float getKeywordRecordStaticScore(unsigned iter) const
	{
		return keywordRecordStaticScores[iter];
	}

	void setKeywordRecordStaticScore(unsigned iter, float keywordScore)
	{
		if(iter <= KEYWORD_THRESHOLD)
			this->keywordRecordStaticScores[iter] = keywordScore;
	}

	unsigned* getKeywordAttributeBitmaps() const
	{
		return keywordAttributeBitmaps;
	}

	void setKeywordAttributeBitmaps(unsigned * keywordAttributeBitmaps)
	{
		this->keywordAttributeBitmaps = keywordAttributeBitmaps;
	}

	unsigned getKeywordAttributeBitmap(unsigned iter) const
	{
		return keywordAttributeBitmaps[iter];
	}

	void setKeywordAttributeBitmap(unsigned iter, unsigned keywordAttributeBitmap)
	{
		if(iter <= KEYWORD_THRESHOLD)
			this->keywordAttributeBitmaps[iter] = keywordAttributeBitmap;
	}

	//set the size of keywordIds and keywordRecordStaticScores to keywordListCapacity
    ForwardList(int sortableAttributeListCapacity=0, int keywordListCapacity=0)
    {
    	// we consider at most SORTABLE_ATTRIBUTES_THRESHOLD sortableAttributes
    	if(sortableAttributeListCapacity > SORTABLE_ATTRIBUTES_THRESHOLD)
    		sortableAttributeListCapacity = SORTABLE_ATTRIBUTES_THRESHOLD;
    	// we consider at most KEYWORD_THRESHOLD keywords
    	if(keywordListCapacity > KEYWORD_THRESHOLD)
    		keywordListCapacity = KEYWORD_THRESHOLD;
    	numberOfKeywordsAndNumberOfSortableAttributes = 0;
    	numberOfKeywordsAndNumberOfSortableAttributes |= sortableAttributeListCapacity;
        recordBoost = 0.5;
        inMemoryData = "";
        keywordIds = new unsigned[keywordListCapacity];
        keywordRecordStaticScores = new half[keywordListCapacity];
        sortableAttributeScores = new float[sortableAttributeListCapacity];
        keywordAttributeBitmaps = NULL;
    }

    virtual ~ForwardList()
    {
    	if(keywordIds != NULL)
    		delete keywordIds;
    	if(keywordRecordStaticScores != NULL)
    		delete keywordRecordStaticScores;
        if (sortableAttributeScores != NULL)
            delete sortableAttributeScores;
        if (keywordAttributeBitmaps != NULL)
            delete keywordAttributeBitmaps;
    }


    float computeFieldBoostSummation(const Schema *schema, const TokenAttributeHits &hits) const;

    //unsigned getForwardListElement(unsigned cursor) const;
    float getForwardListSortableAttributeScore(const SchemaInternal* schemaInternal, unsigned schemaSortableAttributeId) const;

    bool haveWordInRangeWithStemmer(const SchemaInternal* schema, const unsigned minId, 
                    const unsigned maxId, const unsigned termSearchableAttributeIdToFilterTermHits, unsigned &matchingKeywordId,
                    unsigned &matchingKeywordAttributeBitmap, float &matchingKeywordRecordStaticScore, bool &isStemmed) const;
    bool haveWordInRange(const SchemaInternal* schema, const unsigned minId, const unsigned maxId, 
                 const unsigned termSearchableAttributeIdToFilterTermHits, unsigned &keywordId, unsigned &termAttributeBitmap,
                 float &termRecordStaticScore) const;

    unsigned getKeywordOffset(unsigned keywordId) const;

    bool getWordsInRange(const SchemaInternal* schema, const unsigned minId, const unsigned maxId, 
                 const unsigned termSearchableAttributeIdToFilterTermHits, vector<unsigned> &keywordIdsVector) const;

    /**************************PositionIndex****************/
    float getTermRecordStaticScore(unsigned forwardIndexId, unsigned keywordOffset) const;
    /**
     * Input @param tokenAttributeHitsMap is a map of keywordTokens to a "vector of positions" for the given record.
     * The PositionIndex is ZERO terminated, i.e. ZERO is the end flag for each position list.
     * So, keyword positions in each record attribute start from 1.
     * For example,
     * Consider a record, a1 a2 a1 a1 a3 a1 a3
     * keywordIdMap has the following key,value pairs:
     * a1: [1, 3, 4, 7]
     * a2: [2]
     * a3: [6, 8]
     * Here, a1,a2,a3 are keywordTokesn.
     *
     * Internally, addRecord iterates over the keys and for each key, the "positions" are first appended and then a [0] is appended to the positionVector.
     * In the above case, for key a1, we append [1, 3, 4, 7, 0].
     * for key a2, we append [2, 0].
     * for key a3, we append [6, 8, 0].
     */

    bool isValidRecordTermHit(const SchemaInternal *schema, unsigned keywordOffset, unsigned searchableAttributeId, unsigned &termAttributeBitVec, float& termRecordStaticScore) const;
    bool isValidRecordTermHitWithStemmer(const SchemaInternal *schema, unsigned keywordOffset, unsigned searchableAttributeId,  unsigned &matchingKeywordAttributeBitmap, float &termRecordStaticScore, bool &isStemmed) const;

    unsigned getNumberOfBytes() const;

    // is attribute based search or not
    static bool isAttributeBasedSearch;

    //void mapOldIdsToNewIds();

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	typename Archive::is_loading load;
        ar & this->numberOfKeywordsAndNumberOfSortableAttributes;
        ar & this->recordBoost;
        // In loading process, we need to allocate space for the members first.
        if(load)
        {
        	this->sortableAttributeScores = new float[this->getNumberOfSortableAttributes()];
        	this->keywordIds = new unsigned[this->getNumberOfKeywords()];
        	this->keywordRecordStaticScores = new half[this->getNumberOfKeywords()];
        	// check if it's an attribute based search
        	if (ForwardList::isAttributeBasedSearch)
        	{
        		this->keywordAttributeBitmaps = new unsigned[this->getNumberOfKeywords()];
        	}
        }
        ar & boost::serialization::make_array(this->sortableAttributeScores, this->getNumberOfSortableAttributes());
        ar & boost::serialization::make_array(this->keywordIds, this->getNumberOfKeywords());
        ar & boost::serialization::make_array(this->keywordRecordStaticScores, this->getNumberOfKeywords());
        // check if it's an attribute based search
        if (ForwardList::isAttributeBasedSearch)
        {
        	ar & boost::serialization::make_array(this->keywordAttributeBitmaps, this->getNumberOfKeywords());
        }
        ar & this->externalRecordId;
        ar & this->inMemoryData;
    }

    // members
	unsigned numberOfKeywordsAndNumberOfSortableAttributes;
	half recordBoost;
	std::string externalRecordId;
	std::string inMemoryData;
	unsigned* keywordIds;
	half* keywordRecordStaticScores;

	float* sortableAttributeScores;
	unsigned* keywordAttributeBitmaps;

};

typedef std::pair<ForwardList*, bool> ForwardListPtr;

/**
 * A forward index includes a forward list for each record.  The list (sorted)
 * includes the keywords in the record, represented as keywordIds, representing the trie leaf node
 * corresponding to the keyword.
 */
class  ForwardIndex
{
private:
    const static unsigned MAX_SCORE = unsigned(-1);

    ///vector of forwardLists, where RecordId is the element index.
    cowvector< ForwardListPtr > *forwardListDirectory;

    //Used only in WriteView
    map<std::string,unsigned> externalToInternalRecordIdMap_WriteView;

    // Build phase structure
    // Stores the order of records, by which it was added to forward index. Used in bulk initial insert
    //std::vector<unsigned> recordOrder;

    // Set to true when the commit function is called. Set to false, when the addRecord function is called.
    bool commited_WriteView;
    bool mergeRequired_WriteView;

    // Initialised in constructor and used in calculation of offset in filterAttributesVector. This is lighter than serialising the schema itself.
    const SchemaInternal *schemaInternal;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & forwardListDirectory;
        ar & externalToInternalRecordIdMap_WriteView;
        ar & commited_WriteView;
    }

    //helper functions
    float _getSortableAttributeValue( unsigned sortableAttributeIndex, const string* sortableAttributeValueString) const;
    void _getPositionListFromTokenAttributesMap(KeywordIdKeywordStringInvertedListIdTriple &keywordIdList, map<string, TokenAttributeHits > &tokenAttributeHitsMap, vector<unsigned>& positionList);

public:

    ForwardIndex( const SchemaInternal* schemaInternal);
    ForwardIndex( const SchemaInternal* schemaInternal, unsigned expectedNumberOfDocumentsToInitialize);
    virtual ~ForwardIndex();

    unsigned getTotalNumberOfForwardLists_ReadView() const;
    unsigned getTotalNumberOfForwardLists_WriteView() const;


    void addDummyFirstRecord();// For Trie bootstrap

    /**
     * Add a new record, represented by a list of keyword IDs. The keywordIds are unique and have no order.
     * If the record ID already exists, do nothing. Else, add the record.
     */
    void addRecord(const Record *record, const unsigned recordId, KeywordIdKeywordStringInvertedListIdTriple &keywordIdList, map<string, TokenAttributeHits > &tokenAttributeHitsMap);

    /**
     * Set the deletedFlag on the forwardList, representing record deletion.
     */
    void setDeleteFlag(unsigned internalRecordId);

    /**
     * Resume the deletedFlag on the forwardList, reversing record deletion.
     */
    void resetDeleteFlag(unsigned internalRecordId);

    /**
     * Verify if given interval is present in the forward list
     */
    //bool havePrefixInForwardList(const unsigned recordId, const unsigned minId, const unsigned maxId, const int termSearchableAttributeIdToFilterTermHits, float &score) const;

    /**
     * Check if the ForwardList of recordId has a keywordId in range [minId, maxId]. Note that this is closed range.
     * If the recordId does not exist, return false.
     */
//    bool haveWordInRange(const unsigned recordId, const unsigned minId, const unsigned maxId, const int termSearchableAttributeIdToFilterTermHits, unsigned &keywordId, float &score) const;

    ///Added for stemmer
    bool haveWordInRangeWithStemmer(const unsigned recordId, const unsigned minId, const unsigned maxId, const unsigned termSearchableAttributeIdToFilterTermHits,
    		unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap, float &matchingKeywordRecordStaticScore, bool &isStemmed) const;
    bool haveWordInRange(const unsigned recordId, const unsigned minId, const unsigned maxId, const unsigned termSearchableAttributeIdToFilterTermHits,
    		unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap, float &matchingKeywordRecordStaticScore) const;

    /**
         * Check if the ForwardList of recordId has a keywordId in range [minId, maxId]. Note that this is closed range.
         * If the recordId does not exist, return false. - STEMMER VERSION
         */
    //bool haveWordInRangeWithStemmer(const unsigned recordId, const unsigned minId, const unsigned maxId, const int termSearchableAttributeIdToFilterTermHits, unsigned &keywordId, float &termRecordStaticScore, bool& isStemmed);

    /**
     * Returns the number of bytes occupied by the ForwardIndex
     */
    unsigned getNumberOfBytes() const;


    // Boost load and save functions
    static void load(ForwardIndex &forwardIndex, const std::string &forwardIndexFullPathFileName)
    {
        // read the ForwardIndex from the file
    	//struct timespec tstart;
		//clock_gettime(CLOCK_REALTIME, &tstart);
        std::ifstream ifs(forwardIndexFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> forwardIndex;
        ifs.close();
        //struct timespec tend;
        //clock_gettime(CLOCK_REALTIME, &tend);
        //unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
        //std::cout << "Forward index create time :" << time << std::endl;
    };

    static void save(const ForwardIndex &forwardIndex, const std::string &forwardIndexFullPathFileName)
    {
        std::ofstream ofs(forwardIndexFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_oarchive oa(ofs);
        oa << forwardIndex;
        ofs.close();
    };

    /**
     * Build Phase functions
     */
    const ForwardList *getForwardList(unsigned recordId, bool &valid) const;
    //ForwardList *getForwardListToChange(unsigned recordId, bool &valid); // CHENLI
    ForwardList *getForwardList_ForCommit(unsigned recordId);

    //ForwardListElement *getForwardListElementByOffset(unsigned forwardIndexOffset);

    /** COMMITING FORWARDINDEX
     * 1. Get the invertedIndex.
     * 2. Set all hitCounts in ForwardIndexDirectory to zero.
     * 3. For each invertedListDirectory element -> keywordId, get newKeywordId, i.e
     *      1. For each RecordId in Inverted List
     *          1. Insert into forwardIndex using the following steps.
     *              1. Look up ForwardIndexDirectory and get offset, hitcount.
     *              2. Insert at offset+hitcount, the newKeywordId
     *              3. Increment hitcount
     * 4. Sort each ForwardList based on newKeywordIds
     */
    void merge();
    //void commit(ForwardList *forwardList, const vector<unsigned> *oldIdToNewIdMap, vector<NewKeywordIdKeywordOffsetTriple> &forwardListReOrderAtCommit );
    void commit(ForwardList *forwardList, const map<unsigned, unsigned> &oldIdToNewIdMapper, vector<NewKeywordIdKeywordOffsetTriple> &forwardListReOrderAtCommit );

    void reorderForwardList(ForwardList *forwardList, const map<unsigned, unsigned> &oldIdToNewIdMapper,
                vector<NewKeywordIdKeywordOffsetTriple> &forwardListReOrderAtCommit);
    void finalCommit();
    //void commit(const std::vector<unsigned> *oldIdToNewIdMap);

    bool isCommitted() const{
        return this->commited_WriteView;
    }

    /*std::vector<unsigned>* getRecordOrderVector() {
        return &recordOrder;
    };*/

    void setSchema(SchemaInternal *schema)
    {
        this->schemaInternal = schema;
    }

    const SchemaInternal *getSchema() const
    {
        return this->schemaInternal;
    }

    //void print_test() const;
    void print_test() ;
    void print_size() const; 

    float getTermRecordStaticScore(unsigned forwardIndexId, unsigned keywordOffset) const;
    /**
     * Input @param tokenAttributeHitsMap is a map of keywordTokens to a "vector of positions" for the given record.
     * The PositionIndex is ZERO terminated, i.e. ZERO is the end flag for each position list.
     * So, keyword positions in each record attribute start from 1.
     * For example,
     * Consider a record, a1 a2 a1 a1 a3 a1 a3
     * keywordIdMap has the following key,value pairs:
     * a1: [1, 3, 4, 7]
     * a2: [2]
     * a3: [6, 8]
     * Here, a1,a2,a3 are keywordTokesn.
     *
     * Internally, addRecord iterates over the keys and for each key, the "positions" are first appended and then a [0] is appended to the positionVector.
     * In the above case, for key a1, we append [1, 3, 4, 7, 0].
     * for key a2, we append [2, 0].
     * for key a3, we append [6, 8, 0].
     */
    bool isValidRecordTermHit(unsigned forwardIndexId, unsigned keywordOffset, unsigned searchableAttributeId, unsigned &termAttributeBitmap, float& termRecordStaticScore) const;
    bool isValidRecordTermHitWithStemmer(unsigned forwardIndexId, unsigned keywordOffset, unsigned searchableAttributeId,
    		unsigned &matchingKeywordAttributeBitmap, float &matchingKeywordRecordStaticScore, bool &isStemmed) const;

    unsigned getKeywordOffset(unsigned forwardListId, unsigned keywordId) const;


    /*****-record-id-converter*****/
    /**
     * Append @param[in] externalRecordId to the RecordIdVector
     * @param[out] internalRecordId is the internal recordID corresponding to externalRecordId.
     * Internally, internalRecordId is the uint value of the position where the externalRecordId was appended, i.e, "RecordIdVector.size() - 1".
     *
     * If the externalRecordId already exists, "false" is returned. internalRecordId is set to a default (unsigned)(-1) in this case.
     */
    //bool appendExternalRecordId(unsigned externalRecordId, unsigned &internalRecordId);// Goes to both map-write and forwardIndex
    void appendExternalRecordId_WriteView(const std::string &externalRecordId, unsigned &internalRecordId);

    bool deleteRecord_WriteView(const std::string &externalRecordId);
    bool deleteRecordGetInternalId_WriteView(const std::string &externalRecordId, unsigned &internalRecordId);
    //bool deleteExternalRecordId(unsigned externalRecordId); // Goes to forwardIndex and map-write

    bool recoverRecord_WriteView(const std::string &externalRecordId, unsigned internalRecordId);

    INDEXLOOKUP_RETVAL lookupRecord_WriteView(const std::string &externalRecordId) const;

    void reassignKeywordIds(const unsigned recordId, const map<unsigned, unsigned> &keywordIdMapper);
    //void reassignKeywordIds(map<unsigned, unsigned> &reassignedKeywordIdMapper);



    /**
     * For the given internalRecordId, returns the ExternalRecordId. ASSERT if the internalRecordId
     * is out of bound of RecordIdVector.
     */
    bool getExternalRecordId_ReadView(const unsigned internalRecordId, std::string &externalRecordId) const; // Goes to forwardIndex-read

    bool getInternalRecordId_WriteView(const std::string &externalRecordId, unsigned &internalRecordId) const; // Goes to recordIdConverterMap-read

    /**
     * Access the InMemoryData of a record using InternalRecordId
     */
    std::string getInMemoryData(unsigned internalRecordId) const;
};


}}

#endif //__FORWARDINDEX_H__
