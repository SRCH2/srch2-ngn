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

using namespace srch2::util;

namespace srch2 {
namespace instantsearch {



ForwardIndex::ForwardIndex(const SchemaInternal* schemaInternal) {
    this->forwardListDirectory = new cowvector<ForwardListPtr>();
    this->schemaInternal = schemaInternal;
    this->commited_WriteView = false;
    this->mergeRequired = true;
    this->isAttributeBasedSearch = false;
}

ForwardIndex::ForwardIndex(const SchemaInternal* schemaInternal,
        unsigned expectedNumberOfDocumentsToInitialize) {
    this->forwardListDirectory = new cowvector<ForwardListPtr>(
            expectedNumberOfDocumentsToInitialize);
    this->schemaInternal = schemaInternal;
    this->commited_WriteView = false;
    this->mergeRequired = true;
    this->isAttributeBasedSearch = false;
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
TypedValue ForwardList::getForwardListNonSearchableAttributeTypedValue(
        const SchemaInternal* schemaInternal,
        unsigned schemaNonSearchableAttributeId) const {

    ASSERT(
            schemaNonSearchableAttributeId
                    < schemaInternal->getNumberOfNonSearchableAttributes());

    FilterType filterType = schemaInternal->getTypeOfNonSearchableAttribute(
            schemaNonSearchableAttributeId);

    TypedValue typedValue;

    switch (filterType) {
		case srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED:
			typedValue.setTypedValue(VariableLengthAttributeContainer::getUnsignedAttribute(schemaNonSearchableAttributeId, schemaInternal , getNonSearchableAttributeValuesDataPointer()));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
			typedValue.setTypedValue(VariableLengthAttributeContainer::getFloatAttribute(schemaNonSearchableAttributeId, schemaInternal, getNonSearchableAttributeValuesDataPointer()));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
			typedValue.setTypedValue(VariableLengthAttributeContainer::getTextAttribute(schemaNonSearchableAttributeId, schemaInternal , getNonSearchableAttributeValuesDataPointer()));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
			typedValue.setTypedValue(VariableLengthAttributeContainer::getTimeAttribute(schemaNonSearchableAttributeId, schemaInternal , getNonSearchableAttributeValuesDataPointer()));
			break;
		case srch2::instantsearch::ATTRIBUTE_TYPE_DURATION:
			ASSERT(false);
			break;
	}

    return typedValue;

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
        KeywordIdKeywordStringInvertedListIdTriple &uniqueKeywordIdList,
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
    if (uniqueKeywordIdList.size() >= KEYWORD_THRESHOLD)
        uniqueKeywordIdList.resize(KEYWORD_THRESHOLD);

    unsigned keywordListCapacity = uniqueKeywordIdList.size();

    ForwardList *forwardList = new ForwardList(keywordListCapacity);
    forwardList->setExternalRecordId(record->getPrimaryKey());
    forwardList->setRecordBoost(record->getRecordBoost());
    forwardList->setInMemoryData(record->getInMemoryData());
    forwardList->setNumberOfKeywords(uniqueKeywordIdList.size());

    //Adding Non searchable Attribute list
    vector<string> nonSearchableAttributeValues;
    for (unsigned iter = 0;
            iter < this->schemaInternal->getNumberOfNonSearchableAttributes();
            ++iter) {

        const string * nonSearchableAttributeValueString = record
                ->getNonSearchableAttributeValue(iter);
        nonSearchableAttributeValues.push_back(*nonSearchableAttributeValueString);
    }

    // Add KeywordId List
    for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {
        forwardList->setKeywordId(iter, uniqueKeywordIdList[iter].first);
    }

    //Add Score List
    for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {

        map<string, TokenAttributeHits>::const_iterator mapIterator =
                tokenAttributeHitsMap.find(uniqueKeywordIdList[iter].second.first);
        ASSERT(mapIterator != tokenAttributeHitsMap.end());
        forwardList->setKeywordRecordStaticScore(iter,
                forwardList->computeFieldBoostSummation(this->schemaInternal,
                        mapIterator->second));
    }

    PositionIndexType positionIndexType = this->schemaInternal->getPositionIndexType();
    // support attribute-based search
    unsigned * keywordAttributeBitMapTemp = NULL;
    if (positionIndexType == POSITION_INDEX_FIELDBIT || positionIndexType == POSITION_INDEX_FULL) {
    	this->isAttributeBasedSearch = true;

    	keywordAttributeBitMapTemp = new unsigned[keywordListCapacity];

    	for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {
    		map<string, TokenAttributeHits>::const_iterator mapIterator =
    				tokenAttributeHitsMap.find(
    						uniqueKeywordIdList[iter].second.first);
    		ASSERT(mapIterator != tokenAttributeHitsMap.end());
    		unsigned bitVector = 0;
    		for (unsigned i = 0; i < mapIterator->second.attributeList.size();
    				i++) {
    			int attributeId = ((mapIterator->second.attributeList.at(i))
    					>> 24) - 1;
    			bitVector |= (1 << attributeId);
    		}
    		keywordAttributeBitMapTemp[iter] = bitVector;
    	}
    }

	// this buffer is temporary storage of variable length byte array, since its
	// size is not known in advance.
	vector<uint8_t> tempVarLenByteBuffer;
    if (this->schemaInternal->getPositionIndexType() == POSITION_INDEX_FULL) {
    	// Add position indexes in forward list
    	typedef map<string, TokenAttributeHits>::const_iterator TokenAttributeHitsIter;
    	// To avoid frequent resizing, reserve space for vector. 10 is random number
    	tempVarLenByteBuffer.reserve(uniqueKeywordIdList.size() * 10);

    	for (unsigned int i = 0; i < uniqueKeywordIdList.size(); ++i) {

    		string keyword = uniqueKeywordIdList[i].second.first;
    		TokenAttributeHitsIter iterator = tokenAttributeHitsMap.find(keyword);
    		ASSERT(iterator != tokenAttributeHitsMap.end());

    		unsigned prevAttributeId = 0;
    		vector<unsigned> positionListVector;

    		for (unsigned j = 0; j < iterator->second.attributeList.size(); ++j) {
    			unsigned attributeId = (iterator->second.attributeList[j] >> 24) - 1; // 0th based
    			unsigned position =  (iterator->second.attributeList[j] & 0xFFFFFF);  // Non Zero

    			// if it is a first element or current attribute is same as
    			// previous attribute id. then continue to push the position
    			// in position list vector
    			if (j == 0 || prevAttributeId == attributeId) {
    				positionListVector.push_back(position);
    			} else {
    				// if the previous attribute is not same as current attribute
    				// then convert the position list vector to variable length byte
    				// array and APPPEND to grand buffer.
    				convertToVarLengthArray(positionListVector, tempVarLenByteBuffer);
    				positionListVector.clear();
    				positionListVector.push_back(position);
    			}
    			prevAttributeId = attributeId;
    		}

    		// convert the position list vector of last attribute to variable
    		// length byte array
    		convertToVarLengthArray(positionListVector, tempVarLenByteBuffer);
    		positionListVector.clear();
    	}
    }

    // set all extra information into the forward list.
    forwardList->setNSAValuesAndAttrBitMapAndPosIndex(this->schemaInternal ,
    		nonSearchableAttributeValues , keywordAttributeBitMapTemp , tempVarLenByteBuffer);

    ForwardListPtr managedForwardListPtr;
    managedForwardListPtr.first = forwardList;
    managedForwardListPtr.second = true;
    this->forwardListDirectory->getWriteView()->push_back(managedForwardListPtr);

    this->mergeRequired = true;
}
/*
 *  This function converts position list vector to variable length byte array and
 *  append it to grand buffer after adding the size of byte array first.
 */
void ForwardIndex::convertToVarLengthArray(const vector<unsigned>& positionListVector,
		vector<uint8_t>& grandBuffer) {

	uint8_t * buffer = 0;
	// convert to vector to variable length byte array
	unsigned lengthOfDeltas = ULEB128::uInt32VectorToVarLenArray(positionListVector, &buffer);

	if (buffer == NULL)
		return;

	uint8_t vlb[5];
	short numberOfBytesForLength;
	// now convert the deltaSize to varable length byte (vlb)
	ULEB128::uInt32ToVarLengthBytes(lengthOfDeltas , vlb, &numberOfBytesForLength);

	// append vlb of deltaSize to grand buffer first
	for (int k =0; k < numberOfBytesForLength; ++k)
		grandBuffer.push_back(vlb[k]);

	// append vlb of delta stored in buffer.
	for (int k =0; k < lengthOfDeltas; ++k)
		grandBuffer.push_back(buffer[k]);

	if (buffer)
		free(buffer);
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
                == srch2::instantsearch::POSITION_INDEX_FIELDBIT ||
                this->schemaInternal->getPositionIndexType()
                                == srch2::instantsearch::POSITION_INDEX_FULL)
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
                == srch2::instantsearch::POSITION_INDEX_FIELDBIT ||
                this->schemaInternal->getPositionIndexType()
                                == srch2::instantsearch::POSITION_INDEX_FULL) {
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
            if (tempScore >= matchingKeywordRecordStaticScore) {
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
bool ForwardIndex::getExternalRecordIdFromInternalRecordId(const unsigned internalRecordId,
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
                    != srch2::instantsearch::POSITION_INDEX_FIELDBIT)) {
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

unsigned getBitSet(unsigned number);
unsigned getBitSetPositionOfAttr(unsigned bitmap, unsigned attribute);

void ForwardList::getKeyWordPostionsInRecordField(unsigned keyOffset, unsigned attributeId,
		unsigned currKeyattributeBitMap, vector<unsigned>& pl) const{

	if (positionIndexSize == 0){
		Logger::warn("Position Index not found in forward index!!");
		return;
	}

	const uint8_t * piPtr = getPositionIndexPointer();  // pointer to position index for the record
	unsigned piOffset = 0;

	if (*(piPtr + positionIndexSize - 1) & 0x80)
	{
		 Logger::error("position index buffer has bad encoding..last byte is not a terminating one");
		 return;
	}

	// get the correct byte array position for current keyword + attribute combination

	for (unsigned j = 0; j < keyOffset ; ++j) {
		currKeyattributeBitMap =
			    getKeywordAttributeBitmapsPointer()[j];
		unsigned count = getBitSet(currKeyattributeBitMap);
		for (unsigned k = 0; k < count; ++k){
			unsigned value;
			short byteRead;
			ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
			piOffset += byteRead + value;
		}
	}
	currKeyattributeBitMap =
		    getKeywordAttributeBitmapsPointer()[keyOffset];
	unsigned totalBitSet = getBitSet(currKeyattributeBitMap);
	unsigned attrBitPosition = getBitSetPositionOfAttr(currKeyattributeBitMap, attributeId);

	ASSERT(totalBitSet > attrBitPosition);
	for (int i = 0; i < totalBitSet; ++i){
		unsigned value;
		short byteRead;
		ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
		if (i == attrBitPosition){
			ULEB128::varLenByteArrayToInt32Vector((uint8_t *)(piPtr + piOffset + byteRead), value, pl);
			break;
		}else {
			piOffset += byteRead + value;
		}
	}
}
// get the count of set bits in he number
// Brian Kernighan's method
// Published in 1988, the C Programming Language 2nd Ed
unsigned getBitSet(unsigned number) {
	unsigned int count; // c accumulates the total bits set in v
	for (count = 0; number; count++)
	{
		number &= number - 1; // clear the least significant bit set
	}
	return count;
}

// get the count of bit set before the bit position of attributeId
unsigned getBitSetPositionOfAttr(unsigned attrBitMap, unsigned attributeId) {
	unsigned shift = 0;
	unsigned count = 0;
	while(shift < attributeId) {
		if ((attrBitMap & (1 << shift)) == 1)
			++count;
		++shift;
	}
	return count;
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
void ForwardIndex::appendExternalRecordIdToIdMap(
        const std::string &externalRecordId, unsigned &internalRecordId) {
    internalRecordId = this->getTotalNumberOfForwardLists_WriteView();
    this->externalToInternalRecordIdMap.setValue(externalRecordId , internalRecordId);
}

// delete a record with a specific id
// WRITER accesses this function
bool ForwardIndex::deleteRecord(const std::string &externalRecordId) {
    unsigned internalRecordId;

    return deleteRecordGetInternalId(externalRecordId,
            internalRecordId);
}

// delete a record with a specific id, return the deleted internalRecordId
// WRITER accesses this function
bool ForwardIndex::deleteRecordGetInternalId(
        const std::string &externalRecordId, unsigned &internalRecordId) {
    bool found = this->getInternalRecordIdFromExternalRecordId(externalRecordId,
            internalRecordId);
    if (found == true) {
        this->setDeleteFlag(internalRecordId);
        this->externalToInternalRecordIdMap.erase(externalRecordId);
        this->mergeRequired = true; // tell the merge thread to merge
    }

    return found;
}

// recover a deleted record with a specific internal id
// WRITER accesses this function
bool ForwardIndex::recoverRecord(const std::string &externalRecordId,
        unsigned internalRecordId) {
    // see if the external record id exists in the externalToInternalRecordIdMap 
    bool found = this->getInternalRecordIdFromExternalRecordId(externalRecordId,
            internalRecordId);
    if (found == false) {
        this->resetDeleteFlag(internalRecordId); // set the flag in the forward index back to true
        this->externalToInternalRecordIdMap.setValue(externalRecordId , internalRecordId); // add the external record id back to the externalToInternalRecordIdMap
        this->mergeRequired = true; // tell the merge thread to merge
    }

    return !found;
}

// check if a record with a specific internal id exists
INDEXLOOKUP_RETVAL ForwardIndex::lookupRecord(
        const std::string &externalRecordId) const {
    if (externalRecordId.empty())
        return LU_ABSENT_OR_TO_BE_DELETED;

    unsigned internalRecordId;
    bool isInMap = this->externalToInternalRecordIdMap.getValue(externalRecordId , internalRecordId);

    if(isInMap == false){
    	return LU_ABSENT_OR_TO_BE_DELETED;
    }

    bool valid = false;
    this->getForwardList(internalRecordId, valid);

    if (valid == false)
        return LU_TO_BE_INSERTED;
    else
        return LU_PRESENT_IN_READVIEW_AND_WRITEVIEW;
}

bool ForwardIndex::getInternalRecordIdFromExternalRecordId(
        const std::string &externalRecordId, unsigned &internalRecordId) const {
    if (externalRecordId.empty())
        return false;

    return this->externalToInternalRecordIdMap.getValue(externalRecordId , internalRecordId);

}

unsigned ForwardIndex::getKeywordOffset(unsigned forwardListId,
        unsigned keywordId) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardListId, valid);
    return forwardList->getKeywordOffset(keywordId);
}


void ForwardIndex::exportData(ForwardIndex &forwardIndex, const string &exportedDataFileName)
{
    ofstream out(exportedDataFileName.c_str());
    // If the forwardIndex needs to merge, we do the merge it first
    if(forwardIndex.mergeRequired)
        forwardIndex.merge();
    shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
    forwardIndex.forwardListDirectory->getReadView(forwardListDirectoryReadView);
    string uncompressedInMemoryRecordString;
    // loop all the forwardList Index
    for (unsigned counter = 0; counter < forwardListDirectoryReadView->size(); ++counter) {
        bool valid = false;
        const ForwardList* fl = forwardIndex.getForwardList(counter, valid);
        // ignore the invalid record
        if (valid == false)
            continue;
        const string& compressedInMemoryRecordString = fl->getInMemoryData();
        snappy::Uncompress(compressedInMemoryRecordString.c_str(), compressedInMemoryRecordString.size(), &uncompressedInMemoryRecordString);
        out << uncompressedInMemoryRecordString << endl;
    }
    out.close();
}

}
}
