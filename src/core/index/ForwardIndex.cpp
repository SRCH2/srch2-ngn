// $Id: ForwardIndex.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */

#include "ForwardIndex.h"

#include <vector>
#include <algorithm>
#include <assert.h>
#include <iostream>

#include "util/Logger.h"
#include "util/Assert.h"

#include <boost/array.hpp>

using srch2::util::Logger;
using std::string;
using std::pair;
using std::make_pair;

namespace srch2 {
namespace instantsearch {

bool ForwardList::isAttributeBasedSearch = false;

ForwardIndex::ForwardIndex(const SchemaInternal* schemaInternal) {
    this->forwardListDirectory = new cowvector<ForwardListPtr>();
    this->schemaInternal = schemaInternal;
    this->commited_WriteView = false;
    this->mergeRequired = true;
}

ForwardIndex::ForwardIndex(const SchemaInternal* schemaInternal,
        unsigned expectedNumberOfDocumentsToInitialize) {
    this->forwardListDirectory = new cowvector<ForwardListPtr>(
            expectedNumberOfDocumentsToInitialize);
    this->schemaInternal = schemaInternal;
    this->commited_WriteView = false;
    this->mergeRequired = true;
}

ForwardIndex::~ForwardIndex()
{
    if(this->isCommitted())
        this->forwardListDirectory->merge();
    else
        this->forwardListDirectory->commit();
    for (unsigned forwardIndexIter = 0; forwardIndexIter < this->getTotalNumberOfForwardLists_WriteView(); 
     ++forwardIndexIter) {
        ForwardList *forwardList = this->getForwardList_ForCommit(forwardIndexIter);
        delete forwardList;
    }
    delete this->forwardListDirectory;
}

unsigned ForwardIndex::getTotalNumberOfForwardLists_WriteView() const
{
    return this->forwardListDirectory->getWriteView()->size();
}
    
unsigned ForwardIndex::getTotalNumberOfForwardLists_ReadView() const
{
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->forwardListDirectory->getReadView(readView);
    return readView->size();
}

void ForwardIndex::setDeleteFlag(unsigned internalRecordId)
{
    this->forwardListDirectory->getWriteView()->at(internalRecordId).second = false;
}

void ForwardIndex::resetDeleteFlag(unsigned internalRecordId)
{
    this->forwardListDirectory->getWriteView()->at(internalRecordId).second = true;
}


void printForwardList(unsigned id, const ForwardList *fl , const Schema * schema)
{

    Logger::debug("External ID: %s", (fl->getExternalRecordId()).c_str());
    Logger::debug("RecordBoost: %.3f, Size: %d" , fl->getRecordBoost(), fl->getNumberOfKeywords());
    Logger::debug("nonsearchableAttributeList:");

	for (unsigned idx = 0; idx < schema->getNumberOfNonSearchableAttributes(); idx ++) {
		Logger::debug("[%.5f]", fl->getNonSearchableAttributeValue(idx , schema).c_str());
	}
	//keyword Id list
	Logger::debug("keywordIdList: ");



	//keyword Id list
	for (unsigned idx = 0; idx < fl->getNumberOfKeywords(); idx ++) {
	        Logger::debug("[%d]", fl->getKeywordId(idx));
	}

    // keyword score list
    Logger::debug("keywordRecordStaticScore:");
    for (unsigned idx = 0; idx < fl->getNumberOfKeywords(); idx++) {
        Logger::debug("[%.5f]", fl->getKeywordRecordStaticScore(idx));
    }

    // keyword attribute list
    if (fl->getKeywordAttributeBitmaps() != NULL) {

        Logger::debug("keywordAttributeList: ");
        for (unsigned idx = 0; idx < fl->getNumberOfKeywords(); idx++) {
            Logger::debug("[%d]", fl->getKeywordAttributeBitmap(idx));
        }
    }
}

void printVector(const vector<unsigned> *fl) {
    Logger::debug("( 1o0o1 ) Size: %ld", fl->size());

    for (vector<unsigned>::const_iterator flIter = fl->begin();
            flIter != fl->end(); ++flIter) {
        Logger::debug("[%d]", *flIter);
    }
}

// do binary search to probe in forward list
bool ForwardIndex::haveWordInRange(const unsigned recordId,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore) const {
    ASSERT(minId <= maxId);
    ASSERT(recordId < this->getTotalNumberOfForwardLists_ReadView());

    bool valid = false;
    const ForwardList* fl = this->getForwardList(recordId, valid);

    // Deleted flag is set
    if (valid == false)
        return false;

    return fl->haveWordInRange(this->schemaInternal, minId, maxId,
            termSearchableAttributeIdToFilterTermHits, matchingKeywordId,
            matchingKeywordAttributeBitmap, matchingKeywordRecordStaticScore);
}

bool ForwardIndex::haveWordInRangeWithStemmer(const unsigned recordId,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool &isStemmed) const {
    assert(minId <= maxId);
    ASSERT(recordId < this->getTotalNumberOfForwardLists_ReadView());

    bool valid = false;
    const ForwardList* fl = this->getForwardList(recordId, valid);

    // Deleted flag is set
    if (valid == false)
        return false;

    return fl->haveWordInRangeWithStemmer(this->schemaInternal, minId, maxId,
            termSearchableAttributeIdToFilterTermHits, matchingKeywordId,
            matchingKeywordAttributeBitmap, matchingKeywordRecordStaticScore,
            isStemmed);
}

const ForwardList *ForwardIndex::getForwardList(unsigned recordId, bool &valid) const
{
    // A valid record ID is in the range [0, 1, ..., directorySize - 1]
    if (recordId >= this->getTotalNumberOfForwardLists_ReadView())
    {
        valid = false;
        return NULL;
    }

    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->forwardListDirectory->getReadView(readView);
    valid = readView->getElement(recordId).second;
    return readView->getElement(recordId).first;
}

ForwardList *ForwardIndex::getForwardList_ForCommit(unsigned recordId)
{
    ASSERT (recordId < this->getTotalNumberOfForwardLists_WriteView());
    return  this->forwardListDirectory->getWriteView()->at(recordId).first;
}

//unsigned ForwardList::getForwardListElement(unsigned cursor) const
//{
/*  unsigned offset = this->forwardListDirectory[recordId].offset;

 ASSERT(cursor < this->forwardListDirectory[recordId].numberOfKeywords);
 ASSERT(offset + this->forwardListDirectory[recordId].numberOfKeywords <= this->getTotalLengthOfForwardLists());

 return &(keywordVector[offset + cursor]);*/
//TODO check bounds
//   return this->keywordIdVector->at(cursor);
//}
Score ForwardList::getForwardListNonSearchableAttributeScore(
        const SchemaInternal* schemaInternal,
        unsigned schemaNonSearchableAttributeId) const {

    ASSERT(
            schemaNonSearchableAttributeId
                    < schemaInternal->getNumberOfNonSearchableAttributes());

    FilterType filterType = schemaInternal->getTypeOfNonSearchableAttribute(
            schemaNonSearchableAttributeId);

    Score score;

    switch (filterType) {
		case srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED:
			score.setScore(nonSearchableAttributeValues.getUnsignedAttribute(schemaNonSearchableAttributeId, schemaInternal));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
			score.setScore(nonSearchableAttributeValues.getFloatAttribute(schemaNonSearchableAttributeId, schemaInternal));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
			score.setScore(nonSearchableAttributeValues.getTextAttribute(schemaNonSearchableAttributeId, schemaInternal));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
			score.setScore(nonSearchableAttributeValues.getTimeAttribute(schemaNonSearchableAttributeId, schemaInternal));
			break;
	}

    return score;

}

void ForwardIndex::commit()
{
    if ( !this->isCommitted())
    {
        // make sure the read view is pointing to the write view
        this->forwardListDirectory->commit();

        // writeView->forceCreateCopy();
        this->mergeRequired = false;
    }
}

// WriteView
void ForwardIndex::merge()
{
    if ( this->mergeRequired )
    {
        // make sure the read view is pointing to the write view
        this->forwardListDirectory->merge();

        // writeView->forceCreateCopy();
        this->mergeRequired = false;
    }
}

void ForwardIndex::addRecord(const Record *record, const unsigned recordId,
        KeywordIdKeywordStringInvertedListIdTriple &keywordIdList,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) {
    ASSERT(recordId == this->getTotalNumberOfForwardLists_WriteView());

    /**recordOrder maintains the order in which the records were added to forwardIndex. This is used at the commit stage,
     * to interpret the order of positionIndex entries.
     */

    //An unused position was found
    /*if (!commited_WriteView)
     {
     recordOrder.push_back(recordId);
     }*/
    // We consider KEYWORD_THRESHOLD keywords at most, skip the extra ones
    if (keywordIdList.size() >= KEYWORD_THRESHOLD)
        keywordIdList.resize(KEYWORD_THRESHOLD);

    unsigned keywordListCapacity = keywordIdList.size();

    ForwardList *forwardList = new ForwardList(keywordListCapacity);
    forwardList->setExternalRecordId(record->getPrimaryKey());
    forwardList->setRecordBoost(record->getRecordBoost());
    forwardList->setInMemoryData(record->getInMemoryData());
    forwardList->setNumberOfKeywords(keywordIdList.size());

    //Adding Non searchable Attribute list
    vector<string> nonSearchableAttributeValues;
    for (unsigned iter = 0;
            iter < this->schemaInternal->getNumberOfNonSearchableAttributes();
            ++iter) {

        const string * nonSearchableAttributeValueString = record
                ->getNonSearchableAttributeValue(iter);
        nonSearchableAttributeValues.push_back(*nonSearchableAttributeValueString);
    }
    forwardList->setNonSearchableAttributeValues(this->schemaInternal , nonSearchableAttributeValues );


    // Add KeywordId List
    for (unsigned iter = 0; iter < keywordIdList.size(); ++iter) {
        forwardList->setKeywordId(iter, keywordIdList[iter].first);
    }

    //Add Score List
    for (unsigned iter = 0; iter < keywordIdList.size(); ++iter) {

        map<string, TokenAttributeHits>::const_iterator mapIterator =
                tokenAttributeHitsMap.find(keywordIdList[iter].second.first);
        ASSERT(mapIterator != tokenAttributeHitsMap.end());
        forwardList->setKeywordRecordStaticScore(iter,
                forwardList->computeFieldBoostSummation(this->schemaInternal,
                        mapIterator->second));
    }

    // support attribute-based search
    if (this->schemaInternal->getPositionIndexType()
            == srch2::instantsearch::FIELDBITINDEX) {
        ForwardList::isAttributeBasedSearch = true;
        forwardList->setKeywordAttributeBitmaps(
                new unsigned[keywordListCapacity]);

        for (unsigned iter = 0; iter < keywordIdList.size(); ++iter) {
            map<string, TokenAttributeHits>::const_iterator mapIterator =
                    tokenAttributeHitsMap.find(
                            keywordIdList[iter].second.first);
            ASSERT(mapIterator != tokenAttributeHitsMap.end());
            unsigned bitVector = 0;
            for (unsigned i = 0; i < mapIterator->second.attributeList.size();
                    i++) {
                int attributeId = ((mapIterator->second.attributeList.at(i))
                        >> 24) - 1;
                bitVector |= (1 << attributeId);
            }
            forwardList->setKeywordAttributeBitmap(iter, bitVector);
        }
    }

    ForwardListPtr managedForwardListPtr;
    managedForwardListPtr.first = forwardList;
    managedForwardListPtr.second = true;
    this->forwardListDirectory->getWriteView()->push_back(managedForwardListPtr);

    this->mergeRequired = true;
}

// TODO check if this is still useful
void ForwardIndex::addDummyFirstRecord()			// For Trie bootstrap
{
    ForwardListPtr managedForwardListPtr;
    managedForwardListPtr.first = new ForwardList(0);
    managedForwardListPtr.second = false;
    this->forwardListDirectory->getWriteView()->push_back(managedForwardListPtr);
}

// convert the keyword ids for a given record using the given id mapper
void ForwardIndex::reassignKeywordIds(const unsigned recordId,
        const map<unsigned, unsigned> &keywordIdMapper) {
    bool valid = false;
    // currently the read view and the write view should be the same
    //ForwardList *forwardList = getForwardListToChange(recordId, valid); 
    ForwardList *forwardList = const_cast<ForwardList *>(getForwardList(
            recordId, valid));

    // ingore the deleted records
    if (valid == false)
        return;

    // TODO: CHECK
    vector<NewKeywordIdKeywordOffsetTriple> forwardListReOrderAtCommit;
    this->reorderForwardList(forwardList, keywordIdMapper,
            forwardListReOrderAtCommit);
}

//void ForwardIndex::commit(ForwardList *forwardList, const vector<unsigned> *oldIdToNewIdMap,
void ForwardIndex::commit(ForwardList *forwardList,
        const map<unsigned, unsigned> &oldIdToNewIdMapper,
        vector<NewKeywordIdKeywordOffsetTriple> &forwardListReOrderAtCommit) {
    if (this->commited_WriteView == false) {
        reorderForwardList(forwardList, oldIdToNewIdMapper,
                forwardListReOrderAtCommit);
    }
}

/*
 * Jamshid : Called in ID reassign time
 * forward list is ordered based on keywordId.
 *
 * Basically this function is just replacing old IDs with new IDs and re-sorting the forwardlists. However, since
 * we also keep other stuff like attribute scores, it should make sure the orders remain consistent.
 *
 */
void ForwardIndex::reorderForwardList(ForwardList *forwardList,
        const map<unsigned, unsigned> &oldIdToNewIdMapper,
        vector<NewKeywordIdKeywordOffsetTriple> &forwardListReOrderAtCommit) {

    forwardListReOrderAtCommit.clear();

    unsigned numberOfKeywords = forwardList->getNumberOfKeywords();
    forwardListReOrderAtCommit.resize(numberOfKeywords);

    unsigned keywordOffset = 0;

    // Pack keywordIdList, keywordRecordStaticScore, keywordAttributeList into keywordRichInformation for sorting based on new keywordId
    vector<KeywordRichInformation> keywordRichInformationList(numberOfKeywords);
    for (keywordOffset = 0; keywordOffset < forwardList->getNumberOfKeywords();
            ++keywordOffset) {
        //Get new keywordId
        unsigned keywordId = forwardList->getKeywordId(keywordOffset);
        unsigned newKeywordId;
        map<unsigned, unsigned>::const_iterator mapperIter = oldIdToNewIdMapper
                .find(keywordId);
        if (mapperIter == oldIdToNewIdMapper.end())
            newKeywordId = keywordId;
        else
            newKeywordId = mapperIter->second;

        forwardListReOrderAtCommit[keywordOffset].first = newKeywordId;
        forwardListReOrderAtCommit[keywordOffset].second.first = keywordOffset;
        forwardListReOrderAtCommit[keywordOffset].second.second = keywordId;

        //Add new keyword Id, score, attribute to the new position
        keywordRichInformationList[keywordOffset].keywordId = newKeywordId;
        keywordRichInformationList[keywordOffset].keywordScore = forwardList
                ->getKeywordRecordStaticScore(keywordOffset);

        // support attribute-based search
        if (this->schemaInternal->getPositionIndexType()
                == srch2::instantsearch::FIELDBITINDEX)
            keywordRichInformationList[keywordOffset].keywordAttribute =
                    forwardList->getKeywordAttributeBitmap(keywordOffset);
    }

    // Note: May be not necessary
    std::sort(forwardListReOrderAtCommit.begin(),
            forwardListReOrderAtCommit.end());

    std::sort(keywordRichInformationList.begin(),
            keywordRichInformationList.end());
    //Unpack keywordRichInformation to keywordIdList, keywordRecordStaticScore, keywordAttributeList
    keywordOffset = 0;
    for (vector<KeywordRichInformation>::iterator iter =
            keywordRichInformationList.begin();
            iter != keywordRichInformationList.end(); ++iter) {
        // Copy keywordId
        forwardList->setKeywordId(keywordOffset, iter->keywordId);

        // Copy score
        forwardList->setKeywordRecordStaticScore(keywordOffset,
                iter->keywordScore);

        //copy attribute
        // support attribute-based search
        if (this->schemaInternal->getPositionIndexType()
                == srch2::instantsearch::FIELDBITINDEX) {
            forwardList->setKeywordAttributeBitmap(keywordOffset,
                    iter->keywordAttribute);
        }
        ++keywordOffset;
    }
}

void ForwardIndex::finalCommit() {
    this->commited_WriteView = true;
}

unsigned ForwardIndex::getNumberOfBytes() const {
    unsigned totalSize = 0;

    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->forwardListDirectory->getReadView(readView);

    // add the size of forwardListDirectory                                                                                                                                                                     
    totalSize += readView->size() * sizeof(ForwardListPtr);

    //iterate through the forward list of each record                                                                                                                                                           
    for (unsigned counter = 0; counter < readView->size(); ++counter) {
        const ForwardList* fl = readView->getElement(counter).first;
        totalSize += fl->getNumberOfBytes();
    }

    return totalSize;
}

// Print a forwardIndex to debugging
void ForwardIndex::print_test() {
    Logger::debug("ForwardIndex:");

    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->forwardListDirectory->getReadView(readView);

    Logger::debug("readView size: %d", readView->size());

    for (unsigned counter = 0; counter < readView->size(); ++counter) {
        bool valid = false;
        const ForwardList* fl = this->getForwardList(counter, valid);

        if (valid == false)
            continue;
        printForwardList(counter, fl, this->schemaInternal);
    }
}

void ForwardIndex::print_size() const {
    Logger::debug("Forward Index Size: %d bytes", getNumberOfBytes());
}

/************ForwardList*********************/
//low_bound return a pointer to the first element in the range [first, last) which is less than val
const unsigned* lower_bound(const unsigned* first, const unsigned* last,
        const unsigned val) {
    const unsigned* it;
    unsigned step;
    unsigned count = last - first;
    while (count > 0) {
        step = count / 2;
        it = first + step;
        if (*it < val) {
            first = ++it;
            count -= step + 1;
        } else
            count = step;
    }
    return first;
}

bool ForwardList::getWordsInRange(const SchemaInternal* schema,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        vector<unsigned> &keywordIdsVector) const {
    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd, minId);
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());

    bool returnValue = false;

    float matchingKeywordRecordStaticScore = 0.0;
    unsigned matchingKeywordRecordAttributeBitmap = 0;
    while ((vectorIterator != vectorEnd) && (*vectorIterator <= maxId)) {
        if (this->isValidRecordTermHit(schema, (vectorIterator - vectorBegin),
                termSearchableAttributeIdToFilterTermHits,
                matchingKeywordRecordAttributeBitmap,
                matchingKeywordRecordStaticScore)) {
            returnValue = true;
            keywordIdsVector.push_back(*vectorIterator); // found a keyword id in the [minId, maxId] range
        }

        vectorIterator++;
    }

    return returnValue;
}

// Do binary search to probe in the forward list
bool ForwardList::haveWordInRange(const SchemaInternal* schema,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore) const {
    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd, minId);
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());

    bool returnValue = false;
    matchingKeywordRecordStaticScore = 0;
    unsigned tempAttributeBitmap = 0;

    while ((vectorIterator != vectorEnd) && (*vectorIterator <= maxId)) {
        float tempScore = 0;
        if (this->isValidRecordTermHit(schema, (vectorIterator - vectorBegin),
                termSearchableAttributeIdToFilterTermHits, tempAttributeBitmap,
                tempScore)) {
            if (tempScore > matchingKeywordRecordStaticScore) {
                matchingKeywordRecordStaticScore = tempScore;
                matchingKeywordAttributeBitmap = tempAttributeBitmap;
                returnValue = true;
                matchingKeywordId = *vectorIterator; // found a keyword id in the [minId, maxId] range
            }
            break;
        }
        vectorIterator++;
    }

    return returnValue;
}

unsigned ForwardList::getKeywordOffset(unsigned keywordId) const {
    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd,
            keywordId);
    return vectorIterator - vectorBegin;
}

/// Added for stemmer
bool ForwardList::haveWordInRangeWithStemmer(const SchemaInternal* schema,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool& isStemmed) const {

    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd, minId);
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());

    bool returnValue = false;

    while ((vectorIterator != vectorEnd) && (*vectorIterator <= maxId)) {
        if (this->isValidRecordTermHitWithStemmer(schema,
                (vectorIterator - vectorBegin),
                termSearchableAttributeIdToFilterTermHits,
                matchingKeywordAttributeBitmap,
                matchingKeywordRecordStaticScore, isStemmed)) {
            returnValue = true;
            matchingKeywordId = *vectorIterator;
            break;
        }
        vectorIterator++;
    }
    return returnValue;
}

float ForwardList::computeFieldBoostSummation(const Schema *schema,
        const TokenAttributeHits &hits) const {
    float sumOfFieldBoost = 0.0;

    for (vector<unsigned>::const_iterator vectorIterator = hits.attributeList
            .begin(); vectorIterator != hits.attributeList.end();
            vectorIterator++) {
        int attributeId = ((*vectorIterator) >> 24) - 1;
        sumOfFieldBoost += schema->getBoostOfSearchableAttribute(attributeId);
    }

    return 1.0 + (sumOfFieldBoost / schema->getBoostSumOfSearchableAttributes());
}

unsigned ForwardList::getNumberOfBytes() const {
    unsigned numberOfBytes = sizeof(ForwardList) + this->externalRecordId.size()
            + this->inMemoryData.size()
            + 2 * this->getNumberOfKeywords() * sizeof(unsigned);
    if (this->getKeywordAttributeBitmaps() != NULL)
        numberOfBytes += this->getNumberOfKeywords() * sizeof(unsigned);
    return numberOfBytes;
}

// READER accesses this function
bool ForwardIndex::getExternalRecordId_ReadView(const unsigned internalRecordId,
        std::string &externalRecordId) const {
    ASSERT(internalRecordId < this->getTotalNumberOfForwardLists_ReadView());

    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(internalRecordId,
            valid);

    if (valid == false) {
        externalRecordId = -1;
        return false;
    } else {
        externalRecordId = forwardList->getExternalRecordId();
        return true;
    }
}

bool ForwardList::isValidRecordTermHit(const SchemaInternal *schema,
        unsigned keywordOffset, unsigned searchableAttributeId,
        unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore) const {
    matchingKeywordRecordStaticScore = this->getKeywordRecordStaticScore(
            keywordOffset);
    // support attribute-based search
    if (searchableAttributeId == 0
            || (schema->getPositionIndexType()
                    != srch2::instantsearch::FIELDBITINDEX)) {
        return true;
    } else {
        ASSERT(
                this->getKeywordAttributeBitmaps() != NULL and keywordOffset < this->getNumberOfKeywords());
        bool AND = searchableAttributeId & 0x80000000; // test the highest bit
        matchingKeywordAttributeBitmap = getKeywordAttributeBitmap(
                keywordOffset);
        if (AND) {
            searchableAttributeId &= 0x7fffffff; // turn off the highest bit
            return (matchingKeywordAttributeBitmap & searchableAttributeId)
                    == searchableAttributeId;
        } else {
            return (matchingKeywordAttributeBitmap & searchableAttributeId) != 0;
        }
    }
}

/*
 *
 *  ExampleRecord : [Keyword]|[Attribute]|[Stemmed-Yes/No]
 *
 *           R100 : rain|1|Y, rain|3|Y, rain|5|N, rain|6|N
 *
 *  Case 1:
 *  Input: R100, rain, searchableAttributeId = -1
 *  Output:
 *      isValidRecordHit = true
 *      isStemmed = false (Because, the record has keyword "rain" that was a non-stem hit. For the same keyword, Non-Stem hit overrides the Stem Hit)
 *
 *  Case 2:
 *  Input: R100, rain, searchableAttributeId = 5
 *  Output:
 *      isValidRecordHit = true
 *      isStemmed = false
 *
 *  Case 3:
 *  Input: R100, rain, searchableAttributeId = 3
 *  Output:
 *      isValidRecordHit = true
 *      isStemmed = yes
 */
bool ForwardList::isValidRecordTermHitWithStemmer(const SchemaInternal *schema,
        unsigned keywordOffset, unsigned searchableAttributeId,
        unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool &isStemmed) const {
    ASSERT(0);
    return false;
    /*    termRecordStaticScore = 0;
     bool returnValue = false;

     // Case 1 // For stemmer to work, positionIndex must be enabled.
     if (searchableAttributeId == -1) {
     termRecordStaticScore =  this->getTermRecordStaticScore(schema,keywordOffset);
     returnValue = true;
     isStemmed = true;

     pair<unsigned,unsigned> pos(this->getStartEndIndexOfPositionHitsForGivenKeywordOffset(schema, keywordOffset));
     vector<unsigned>::const_iterator vectorIterator = this->keywordIdVector->begin() + pos.first;

     while ((vectorIterator != this->keywordIdVector->end())
     && (vectorIterator != this->keywordIdVector->begin() + pos.second)
     && isStemmed) {
     unsigned pos = ((*vectorIterator) & (0xffffff-1));
     if (pos != (0xffffff-1)) {
     isStemmed = false;
     }
     ++vectorIterator;
     }
     }
     else { // Case 2 and 3
     //Check if the hit is a stemmed hit
     termRecordStaticScore = this->getTermRecordStaticScore(schema,keywordOffset);

     pair<unsigned,unsigned> pos(this->getStartEndIndexOfPositionHitsForGivenKeywordOffset(schema, keywordOffset));
     vector<unsigned>::const_iterator vectorIterator = this->keywordIdVector->begin() + pos.first;
     isStemmed = true;

     while ((vectorIterator != this->keywordIdVector->end())
     && (vectorIterator != this->keywordIdVector->begin() + pos.second)
     && isStemmed) {
     int attributeId = ((*vectorIterator) >> 24) - 1;
     unsigned pos = ((*vectorIterator) & (0xffffff-1));

     if (attributeId == searchableAttributeId) {
     returnValue = true;
     if (pos != (0xffffff-1)) {
     isStemmed = false;
     }
     }
     ++vectorIterator;
     }
     }

     return returnValue;
     */
}

/**********************************************/
std::string ForwardIndex::getInMemoryData(unsigned internalRecordId) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(internalRecordId,
            valid);
    if (valid == false)
        return string("");
    else
        return forwardList->getInMemoryData();
}

float ForwardIndex::getTermRecordStaticScore(unsigned forwardIndexId,
        unsigned keywordOffset) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardIndexId,
            valid);
    if (valid == false)
        return 0;
    else
        return forwardList->getKeywordRecordStaticScore(keywordOffset);
}

bool ForwardIndex::isValidRecordTermHit(unsigned forwardIndexId,
        unsigned keywordOffset, unsigned searchableAttributeId,
        unsigned &matchingKeywordAttributeBitmap,
        float& matchingKeywordRecordStaticScore) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardIndexId,
            valid);
    if (valid && forwardList != NULL)
        return forwardList->isValidRecordTermHit(this->schemaInternal,
                keywordOffset, searchableAttributeId,
                matchingKeywordAttributeBitmap,
                matchingKeywordRecordStaticScore);
    else
        return false;
}

bool ForwardIndex::isValidRecordTermHitWithStemmer(unsigned forwardIndexId,
        unsigned keywordOffset, unsigned searchableAttributeId,
        unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool &isStemmed) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardIndexId,
            valid);
    if (valid && forwardList != NULL)
        return forwardList->isValidRecordTermHitWithStemmer(
                this->schemaInternal, keywordOffset, searchableAttributeId,
                matchingKeywordAttributeBitmap,
                matchingKeywordRecordStaticScore, isStemmed);
    else
        return false;
}

/********-record-id-converter-**********/
// WRITER accesses this function
void ForwardIndex::appendExternalRecordId_WriteView(
        const std::string &externalRecordId, unsigned &internalRecordId) {
    //bool returnValue = false;
    /*if ( this->getInternalRecordId(externalRecordId, internalRecordId) == false)
     {*/
    //this->internalToExternalRecordIdVector.push_back(std::make_pair(externalRecordId,true)); // Added in ForwardIndex::addRecord(...)
    internalRecordId = this->getTotalNumberOfForwardLists_WriteView();
    this->externalToInternalRecordIdMap_WriteView[externalRecordId] =
            internalRecordId;
    //returnValue = true;
    /*}
     else
     {
     internalRecordId = (unsigned)(-1);
     }*/
    //return returnValue;
}

// delete a record with a specific id
// WRITER accesses this function
bool ForwardIndex::deleteRecord_WriteView(const std::string &externalRecordId) {
    unsigned internalRecordId;

    return deleteRecordGetInternalId_WriteView(externalRecordId,
            internalRecordId);
}

// delete a record with a specific id, return the deleted internalRecordId
// WRITER accesses this function
bool ForwardIndex::deleteRecordGetInternalId_WriteView(
        const std::string &externalRecordId, unsigned &internalRecordId) {
    bool found = this->getInternalRecordId_WriteView(externalRecordId,
            internalRecordId);
    if (found == true) {
        this->setDeleteFlag(internalRecordId);
        this->externalToInternalRecordIdMap_WriteView.erase(externalRecordId);
        this->mergeRequired = true; // tell the merge thread to merge
    }

    return found;
}

// recover a deleted record with a specific internal id
// WRITER accesses this function
bool ForwardIndex::recoverRecord_WriteView(const std::string &externalRecordId,
        unsigned internalRecordId) {
    // see if the external record id exists in the externalToInternalRecordIdMap 
    bool found = this->getInternalRecordId_WriteView(externalRecordId,
            internalRecordId);
    if (found == false) {
        this->resetDeleteFlag(internalRecordId); // set the flag in the forward index back to true
        this->externalToInternalRecordIdMap_WriteView[externalRecordId] = internalRecordId; // add the external record id back to the externalToInternalRecordIdMap
        this->mergeRequired = true; // tell the merge thread to merge
    }

    return !found;
}

// check if a record with a specific internal id exists
// WRITER accesses this function
INDEXLOOKUP_RETVAL ForwardIndex::lookupRecord_WriteView(
        const std::string &externalRecordId) const {
    if (externalRecordId.empty())
        return LU_ABSENT_OR_TO_BE_DELETED;

    std::map<string, unsigned>::const_iterator mapIter = this
            ->externalToInternalRecordIdMap_WriteView.find(externalRecordId);

    if (mapIter == this->externalToInternalRecordIdMap_WriteView.end())
        return LU_ABSENT_OR_TO_BE_DELETED;

    bool valid = false;
    this->getForwardList(mapIter->second, valid);

    if (valid == false)
        return LU_TO_BE_INSERTED;
    else
        return LU_PRESENT_IN_READVIEW_AND_WRITEVIEW;
}

// WRITER accesses this function
bool ForwardIndex::getInternalRecordId_WriteView(
        const std::string &externalRecordId, unsigned &internalRecordId) const {
    if (externalRecordId.empty())
        return false;

    std::map<string, unsigned>::const_iterator mapIter = this
            ->externalToInternalRecordIdMap_WriteView.find(externalRecordId);
    if (mapIter != this->externalToInternalRecordIdMap_WriteView.end()) {
        internalRecordId = mapIter->second;
        //ASSERT(internalRecordId < this->getTotalNumberOfForwardLists_WriteView());
        return true;
    } else {
        return false;
    }
}

unsigned ForwardIndex::getKeywordOffset(unsigned forwardListId,
        unsigned keywordId) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardListId, valid);
    //assert(valid == true);
    return forwardList->getKeywordOffset(keywordId);
}

}
}
