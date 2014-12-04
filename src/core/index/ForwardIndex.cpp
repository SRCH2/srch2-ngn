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

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */

#include "ForwardIndex.h"

#include <vector>
#include <algorithm>
#include <assert.h>
#include <iostream>

#include "util/Logger.h"
#include "util/Assert.h"

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include <boost/array.hpp>
#include "util/RecordSerializerUtil.h"
#include <instantsearch/Ranker.h>

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

/*
 * this function uses forward index lock inside and it's expensive, so it should be used carefully inside a loop
 */
unsigned ForwardIndex::getTotalNumberOfForwardLists_WriteView() const
{
    return this->forwardListDirectory->getWriteView()->size();
}
/*
 * this function uses forward index lock inside and it's expensive, so it should be used carefully inside a loop
 */
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
    Logger::debug("refiningAttributeList:");

	for (unsigned idx = 0; idx < schema->getNumberOfRefiningAttributes(); idx ++) {
//		Logger::debug("[%.5f]", fl->getRefiningAttributeValue(idx , schema).c_str());
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
    Logger::debug("keywordAttributeList: ");

    vector<vector<unsigned> > attrsLists;
    fl->getKeywordAttributeIdsLists(fl->getNumberOfKeywords(), attrsLists);
    for(vector<vector<unsigned> >::iterator itLists = attrsLists.begin(); itLists != attrsLists.end();itLists++){
        for(vector<unsigned>::iterator it = (*itLists).begin();it!=(*itLists).end();it++){
            Logger::debug("[%d]", *it);
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
bool ForwardIndex::haveWordInRange(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const unsigned recordId,
        const unsigned minId, const unsigned maxId,
        const vector<unsigned>& filteringAttributesList,
        ATTRIBUTES_OP attrOp,
        unsigned &matchingKeywordId, vector<unsigned>& matchingKeywordAttributesList,
        float &matchingKeywordRecordStaticScore) const {
    ASSERT(minId <= maxId);
    ASSERT(recordId < this->getTotalNumberOfForwardLists_ReadView());

    bool valid = false;
    const ForwardList* fl = this->getForwardList(forwardListDirectoryReadView, recordId, valid);

    // Deleted flag is set
    if (valid == false)
        return false;

    return fl->haveWordInRange(this->schemaInternal, minId, maxId,
    		filteringAttributesList, attrOp, matchingKeywordId,
    		matchingKeywordAttributesList, matchingKeywordRecordStaticScore);
}

bool ForwardIndex::haveWordInRangeWithStemmer(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const unsigned recordId,
        const unsigned minId, const unsigned maxId,
        const unsigned termSearchableAttributeIdToFilterTermHits,
        unsigned &matchingKeywordId, unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool &isStemmed) const {
    assert(minId <= maxId);
    ASSERT(recordId < this->getTotalNumberOfForwardLists_ReadView());
    bool valid = false;
    const ForwardList* fl = this->getForwardList(forwardListDirectoryReadView, recordId, valid);

    // Deleted flag is set
    if (valid == false)
        return false;

    return fl->haveWordInRangeWithStemmer(this->schemaInternal, minId, maxId,
            termSearchableAttributeIdToFilterTermHits, matchingKeywordId,
            matchingKeywordAttributeBitmap, matchingKeywordRecordStaticScore,
            isStemmed);
}

const ForwardList *ForwardIndex::getForwardList(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		unsigned recordId, bool &valid) const
{
    // A valid record ID is in the range [0, 1, ..., directorySize - 1]
    if(recordId >= forwardListDirectoryReadView->size()){
    	valid = false;
    	return NULL;
    }

    ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);
    valid = flPtr.second;

    return flPtr.first;
}

bool ForwardIndex::hasAccessToForwardList(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		unsigned recordId, string &roleId)
{
	if(recordId >= forwardListDirectoryReadView->size()){
		return false;
	}

	ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);
	if(flPtr.second){
		return flPtr.first->accessibleByRole(roleId);
	}
	return false;
}

// returnValue: true if record with this primaryKey exists and false otherwise.
bool ForwardIndex::appendRoleToResource(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const string& resourcePrimaryKeyID, vector<string> &roleIds){
	unsigned recordId;
	bool hasRecord = getInternalRecordIdFromExternalRecordId(resourcePrimaryKeyID, recordId);

	if(hasRecord == false)
		return false;

	ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);

	if(flPtr.second){
		flPtr.first->appendRolesToResource(roleIds);
		return true;
	}
	return false;
}

// returnValue: true if record with this primaryKey exists and false otherwise.
bool ForwardIndex::deleteRoleFromResource(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const string& resourcePrimaryKeyID, vector<string> &roleIds){
	unsigned recordId;
	bool hasRecord = getInternalRecordIdFromExternalRecordId(resourcePrimaryKeyID, recordId);

	if(hasRecord == false)
		return false;

	ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);

	if(flPtr.second){
		flPtr.first->deleteRolesFromResource(roleIds);
		return true;
	}
	return false;
}

bool ForwardIndex::deleteRoleFromResource(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const string& resourcePrimaryKeyID, const string &roleId){
	unsigned recordId;
	bool hasRecord = getInternalRecordIdFromExternalRecordId(resourcePrimaryKeyID, recordId);

	if(hasRecord == false)
		return false;

	ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);

	if(flPtr.second){
		flPtr.first->deleteRoleFromResource(roleId);
		return true;
	}
	return false;
}

RecordAcl* ForwardIndex::getRecordAccessList(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const string& resourcePrimaryKeyID){
	unsigned recordId;
	bool hasRecord = getInternalRecordIdFromExternalRecordId(resourcePrimaryKeyID, recordId);

	if(hasRecord == false)
		return NULL;

	ForwardListPtr flPtr = forwardListDirectoryReadView->getElement(recordId);

	if(flPtr.second){
		return flPtr.first->getAccessList();
	}
	return NULL;
}

ForwardList *ForwardIndex::getForwardList_ForCommit(unsigned recordId)
{
    ASSERT (recordId < this->getTotalNumberOfForwardLists_WriteView());
    return  this->forwardListDirectory->getWriteView()->at(recordId).first;
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
    if ( this->mergeRequired ) {
        // make sure the read view is pointing to the write view
        this->forwardListDirectory->merge();
        // writeView->forceCreateCopy();
        this->mergeRequired = false;
    }
}

void ForwardIndex::freeSpaceOfDeletedRecords() {
  vectorview<ForwardListPtr> *writeView = this->forwardListDirectory->getWriteView();
  for(boost::unordered_set<unsigned>::iterator iter = this->deletedRecordInternalIds.begin();
      iter != this->deletedRecordInternalIds.end(); ++ iter) {
        unsigned internalRecordId = *iter;
        // free the memory if it's no longer valid
        ASSERT(writeView->at(internalRecordId).second == false);
        ASSERT(writeView->at(internalRecordId).first != NULL);
        delete writeView->at(internalRecordId).first;
        writeView->at(internalRecordId).first = NULL;
    }
  // clear the set
  this->deletedRecordInternalIds.clear();
}

void ForwardIndex::addRecord(const Record *record, const unsigned recordId,
        KeywordIdKeywordStringInvertedListIdTriple &uniqueKeywordIdList,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) {
    ASSERT(recordId == this->getTotalNumberOfForwardLists_WriteView());

    /**recordOrder maintains the order in which the records were added to forwardIndex. This is used at the commit stage,
     * to interpret the order of positionIndex entries.
     */

    // We consider KEYWORD_THRESHOLD keywords at most, skip the extra ones
    if (uniqueKeywordIdList.size() >= KEYWORD_THRESHOLD)
        uniqueKeywordIdList.resize(KEYWORD_THRESHOLD);

    unsigned keywordListCapacity = uniqueKeywordIdList.size();

    ForwardList *forwardList = new ForwardList(keywordListCapacity);
    forwardList->setExternalRecordId(record->getPrimaryKey());
    forwardList->setRecordBoost(record->getRecordBoost());
    forwardList->setInMemoryData(record->getInMemoryData());
    // now forward Index took the ownership of this pointer. Record object should not free it.
    // casting away constness to circumvent compiler error/design issue.
    ((Record *)record)->disownInMemoryData();
    forwardList->setNumberOfKeywords(uniqueKeywordIdList.size());

    forwardList->appendRolesToResource(*(record->getRoleIds()));

    PositionIndexType positionIndexType = this->schemaInternal->getPositionIndexType();
    bool shouldAttributeBitMapBeAllocated = false;
    if (isEnabledAttributeBasedSearch(positionIndexType)) {
    	shouldAttributeBitMapBeAllocated = true;
    }

	// this buffer is temporary storage of variable length byte array, since its
	// size is not known in advance.
	vector<uint8_t> tempPositionIndexBuffer;
	vector<uint8_t> tempOffsetBuffer;
	vector<uint8_t> tempcharLenBuffer;

	vector<uint8_t> tempSynonymBitMapBuffer;

    if (isEnabledWordPositionIndex(positionIndexType) || isEnabledCharPositionIndex(positionIndexType)) {
    	// Add position indexes in forward list
    	typedef map<string, TokenAttributeHits>::const_iterator TokenAttributeHitsIter;
    	// To avoid frequent resizing, reserve space for vector. 10 is random number
    	tempPositionIndexBuffer.reserve(uniqueKeywordIdList.size() * 10);
    	tempOffsetBuffer.reserve(uniqueKeywordIdList.size() * 10);

    	for (unsigned int i = 0; i < uniqueKeywordIdList.size(); ++i) {

    		string keyword = uniqueKeywordIdList[i].second.first;
    		TokenAttributeHitsIter iterator = tokenAttributeHitsMap.find(keyword);
    		ASSERT(iterator != tokenAttributeHitsMap.end());

    		unsigned prevAttributeId = 0;
    		vector<unsigned> positionListVector;
    		vector<unsigned> offsetVector;
    		vector<unsigned> synonymOriginalTokenLenArray;
    		vector<uint8_t> synonymBitFlagArray;
    		unsigned bitMapCursor = 0;

    		for (unsigned j = 0; j < iterator->second.attributeIdList.size(); ++j) {
    			unsigned attributeId = iterator->second.attributeIdList[j]; // 0th based
    			unsigned position =  iterator->second.positionsOfTermInAttribute[j];  // Non Zero
    			unsigned offset =  iterator->second.charOffsetsOfTermInAttribute[j];  // Non Zero
    			AnalyzedTokenType type =  (iterator->second.typesOfTermInAttribute[j]);
    			unsigned charLen =  (iterator->second.charLensOfTermInAttribute[j]);
    			// if it is a first element or current attribute is same as
    			// previous attribute id. then continue to push the position
    			// in position list vector
    			if (j == 0 || prevAttributeId == attributeId) {
    				if (isEnabledWordPositionIndex(positionIndexType))
    					positionListVector.push_back(position);
    				if (isEnabledCharPositionIndex(positionIndexType)){
    					offsetVector.push_back(offset);
    					//add a new byte to bitMap vector
    					if (bitMapCursor / 8 >= synonymBitFlagArray.size()) {
    						synonymBitFlagArray.push_back(0);
    					}
    					if (type == ANALYZED_SYNONYM_TOKEN){
    						// For synonym token set its bit flag to 1
    						// and also push the original token character len in
    						// synonymOriginalTokenLenArray
    						unsigned byte = bitMapCursor / 8;
    						uint8_t mask = 1;
    						mask = mask << (bitMapCursor % 8);
    						synonymBitFlagArray[byte] = synonymBitFlagArray[byte] | mask;
    						synonymOriginalTokenLenArray.push_back(charLen);
    					}
    					++bitMapCursor;
    				}
    			} else {
    				// if the previous attribute is not same as current attribute
    				// then convert the position list vector to variable length byte
    				// array and APPPEND to grand buffer.
    				if (isEnabledWordPositionIndex(positionIndexType)) { 
    					convertToVarLengthArray(positionListVector, tempPositionIndexBuffer);
    				} 
                                if (isEnabledCharPositionIndex(positionIndexType)) { 
    					convertToVarLengthArray(offsetVector, tempOffsetBuffer);
    					convertToVarLengthArray(synonymOriginalTokenLenArray, tempcharLenBuffer);
    					convertToVarLengthBitMap(synonymBitFlagArray, tempSynonymBitMapBuffer);
                                } 

    				positionListVector.clear();
    				offsetVector.clear();
    				synonymOriginalTokenLenArray.clear();
    				synonymBitFlagArray.clear();
    				bitMapCursor = 0;

    				if (isEnabledWordPositionIndex(positionIndexType))
    					positionListVector.push_back(position);
    				if (isEnabledCharPositionIndex(positionIndexType)) {
    					offsetVector.push_back(offset);
    					if (bitMapCursor / 8 >= synonymBitFlagArray.size()) {
    						synonymBitFlagArray.push_back(0);
    					}
    					if (type == ANALYZED_SYNONYM_TOKEN){
    						unsigned byte = bitMapCursor / 8;
    						uint8_t mask = 1;
    						mask = mask << (bitMapCursor % 8);
    						synonymBitFlagArray[byte] = synonymBitFlagArray[byte] | mask;
    						synonymOriginalTokenLenArray.push_back(charLen);
    					}
    					++bitMapCursor;
    				}
    			}
    			prevAttributeId = attributeId;
    		}

    		// convert the position list vector of last attribute to variable
    		// length byte array
    		if (isEnabledWordPositionIndex(positionIndexType)) { 
    			convertToVarLengthArray(positionListVector, tempPositionIndexBuffer);
    		} 
                if (isEnabledCharPositionIndex(positionIndexType)) { 
    			convertToVarLengthArray(offsetVector, tempOffsetBuffer);
    			convertToVarLengthArray(synonymOriginalTokenLenArray, tempcharLenBuffer);
    			convertToVarLengthBitMap(synonymBitFlagArray, tempSynonymBitMapBuffer);
                } 

    		positionListVector.clear();
    		offsetVector.clear();
    		synonymOriginalTokenLenArray.clear();
    		synonymBitFlagArray.clear();
    	}
    }

    // support attribute-based search
    /*
     *  Go over the each keyword in the current record and make a vector of attribute ids.
     *  attributeIdList can have duplicate attributes. Once all unique attributes of a
     *  given keyword are accumulated. Store the list of attribute ids as VLB array.
     */
    vector<uint8_t> tempAttributeIdBuffer;
    if (isEnabledAttributeBasedSearch(positionIndexType)) {
    	this->isAttributeBasedSearch = true;
    	vector<unsigned> attributeIds;
    	for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {
    		map<string, TokenAttributeHits>::const_iterator mapIterator =
    				tokenAttributeHitsMap.find(
    						uniqueKeywordIdList[iter].second.first);
    		ASSERT(mapIterator != tokenAttributeHitsMap.end());
    		unsigned prevAttrId = 0;
    		for (unsigned i = 0; i < mapIterator->second.attributeIdList.size();
    				i++) {
    			unsigned attributeId = mapIterator->second.attributeIdList[i];
    			if (i == 0 || attributeId != prevAttrId){
    				attributeIds.push_back(attributeId);
    				prevAttrId = attributeId;
    			}
    		}
    		convertToVarLengthArray(attributeIds, tempAttributeIdBuffer);
    		attributeIds.clear();
    	}
    }

    // set all extra information into the forward list.
    forwardList->allocateSpaceAndSetNSAValuesAndPosIndex(this->schemaInternal ,
    		tempAttributeIdBuffer , tempPositionIndexBuffer,
    		tempOffsetBuffer, tempcharLenBuffer, tempSynonymBitMapBuffer);

    // Add KeywordId List
    for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {
        forwardList->setKeywordId(iter, uniqueKeywordIdList[iter].first);
    }

    // Get term frequency list for all keywords
    vector<float> tfList;
    forwardList->getTermFrequency(uniqueKeywordIdList.size(), tfList);
    ASSERT(uniqueKeywordIdList.size() == tfList.size());

    //Add Score List
    for (unsigned iter = 0; iter < uniqueKeywordIdList.size(); ++iter) {

        map<string, TokenAttributeHits>::const_iterator mapIterator =
                tokenAttributeHitsMap.find(uniqueKeywordIdList[iter].second.first);
        ASSERT(mapIterator != tokenAttributeHitsMap.end());
        //Get sumOfFieldBoost
        float boostSum = forwardList->computeFieldBoostSummation(this->schemaInternal,
                mapIterator->second);
        //Get term frequency
        float tf = tfList[iter];
        float tfBoostProduct = Ranker::computeRecordTfBoostProdcut(tf, boostSum);
        forwardList->setKeywordTfBoostProduct(iter, tfBoostProduct);    //TF * sumOfFieldBoosts
    }



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

	if (buffer == NULL) {
		ASSERT(lengthOfDeltas == 0);
		lengthOfDeltas = 0;
	}

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
		delete [] buffer;
}
void ForwardIndex::convertToVarLengthBitMap(const vector<uint8_t>& bitMapVector,
		vector<uint8_t>& grandBuffer)
{
	unsigned sizeOfBitMap = bitMapVector.size();
	uint8_t vlb[5];
	short numberOfBytesForLength;
	ULEB128::uInt32ToVarLengthBytes(sizeOfBitMap , vlb, &numberOfBytesForLength);
	for (int k =0; k < numberOfBytesForLength; ++k)
		grandBuffer.push_back(vlb[k]);
	for (int k =0; k < bitMapVector.size(); ++k)
		grandBuffer.push_back(bitMapVector[k]);
}

// TODO check if this is still useful
void ForwardIndex::addDummyFirstRecord()			// For Trie bootstrap
{
    ForwardListPtr managedForwardListPtr;
    managedForwardListPtr.first = new ForwardList(0);
    managedForwardListPtr.second = false;
    this->forwardListDirectory->getWriteView()->push_back(managedForwardListPtr);
}

void ForwardIndex::getForwardListDirectory_ReadView(shared_ptr<vectorview<ForwardListPtr> > & readView) const{
	this->forwardListDirectory->getReadView(readView);
}

// convert the keyword ids for a given record using the given id mapper
void ForwardIndex::reassignKeywordIds(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const unsigned recordId,
        const map<unsigned, unsigned> &keywordIdMapper) {
    bool valid = false;
    // currently the read view and the write view should be the same
    //ForwardList *forwardList = getForwardListToChange(recordId, valid); 
    ForwardList *forwardList = const_cast<ForwardList *>(getForwardList(forwardListDirectoryReadView,recordId, valid));

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

    /*
     *  Go over the keyword-ids in the forward list and verify -
     *  a) whether the keyword-ids have changed or not.
     *  b) If the keyword-ids have changed, then whether the numerical order of keyword-ids have
     *     changed or not.
     *
     *  if only a) is true then we need to change only the keyword-ids in the forward list. Other
     *  VLB array information in the forward list should remain unchanged.
     *
     *  if both a) and b) are true then we have to reorder forward list completely.
     *
     *  Note: b) occurs only when a) occurs
     */
    bool reorderRequired = false;
    bool keywordIdChanged = false;
    unsigned prevNewKeywordId = 0;
    vector<unsigned> newKeywordIdsList;
    for (keywordOffset = 0; keywordOffset < forwardList->getNumberOfKeywords();
            ++keywordOffset) {
    	unsigned keywordId = forwardList->getKeywordId(keywordOffset);
    	unsigned newKeywordId;
    	map<unsigned, unsigned>::const_iterator mapperIter = oldIdToNewIdMapper
    			.find(keywordId);
    	if (mapperIter == oldIdToNewIdMapper.end())
    		newKeywordId = keywordId;
    	else {
    		keywordIdChanged = true;
    		newKeywordId = mapperIter->second;
    	}
    	newKeywordIdsList.push_back(newKeywordId);
    	if (prevNewKeywordId > newKeywordId) {
    		reorderRequired = true;
    		newKeywordIdsList.clear();
    		keywordOffset = 0;
    		break;
    	}
    	prevNewKeywordId = newKeywordId;

        forwardListReOrderAtCommit[keywordOffset].first = newKeywordId;
        forwardListReOrderAtCommit[keywordOffset].second.first = keywordOffset;
        forwardListReOrderAtCommit[keywordOffset].second.second = keywordId;
    }

    if (!reorderRequired) {
    	// re-ordering of forward list is not required. Check whether keyword ids changed
    	if (keywordIdChanged) {
    		ASSERT(newKeywordIdsList.size() == forwardList->getNumberOfKeywords());
    		for (unsigned i = 0; i < newKeywordIdsList.size(); ++i)
    			forwardList->setKeywordId(i, newKeywordIdsList[i]);
    	}
    	// the sort forwardListReOrderAtCommit is not required because ordering check is
    	// done above.
    	return;
    }
    // Else reorder forward list
    forwardListReOrderAtCommit.clear();
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
        keywordRichInformationList[keywordOffset].keywordTfBoostProduct =
                forwardList->getKeywordTfBoostProduct(keywordOffset);
        keywordRichInformationList[keywordOffset].keywordScore = forwardList
                ->getKeywordRecordStaticScore(keywordOffset);

        // support attribute-based search
        vector<uint8_t>& keywordAttributesVLB = keywordRichInformationList[keywordOffset].keywordAttribute;
        if ( isEnabledAttributeBasedSearch(this->schemaInternal->getPositionIndexType())) {
        	// fetch VLB representation of attribute Ids for the current keyword based on its offset.
            forwardList->getKeywordAttributeIdsByteArray(keywordOffset, keywordAttributesVLB);
        }

        if (isEnabledWordPositionIndex(this->schemaInternal->getPositionIndexType())){
        	vector<uint8_t>& keywordPositionsVLB = keywordRichInformationList[keywordOffset].keywordPositionsInAllAttribute;
        	// fetch VLB representation of all positions (in all attributes) for the
        	// current keyword based on its offset.
        	forwardList->getKeyWordPostionsByteArray(keywordOffset, keywordPositionsVLB);
        }

        if (isEnabledCharPositionIndex(this->schemaInternal->getPositionIndexType())){

        	vector<uint8_t>& charOffsetPositionsVLB = keywordRichInformationList[keywordOffset].keywordOffsetsInAllAttribute;
        	// fetch VLB representation of all char offsets (in all attributes) for the
        	// current keyword based on its offset.
        	forwardList->getKeyWordOffsetsByteArray(keywordOffset, charOffsetPositionsVLB);

        	vector<uint8_t>& synonymBitMapVLB =  keywordRichInformationList[keywordOffset].keywordSynonymBitMapInAllAttribute;
        	// fetch VLB representation of all synonym bitmaps (in all attributes) for the
        	// current keyword based on its offset.
        	forwardList->getKeywordSynonymsBitMapByteArray(keywordOffset, synonymBitMapVLB);

        	vector<uint8_t>& charLenVectorVLB = keywordRichInformationList[keywordOffset].keywordSynonymCharLenInAllAttribute;
        	// fetch VLB representation of all synonym original char lengths (in all attributes) for the
        	// current keyword based on its offset.
        	forwardList->getSynonymCharLensByteArray(keywordOffset, charLenVectorVLB);
        }

    }

    // Note: May be not necessary
    std::sort(forwardListReOrderAtCommit.begin(),
            forwardListReOrderAtCommit.end());

    std::sort(keywordRichInformationList.begin(),
            keywordRichInformationList.end());
    //Unpack keywordRichInformation to keywordIdList, keywordRecordStaticScore, keywordAttributeList
    keywordOffset = 0;
    vector<uint8_t> tempAttributeIdBuffer;
    vector<uint8_t> tempKeywordPositionsBuffer;
    vector<uint8_t> tempKeywordCharOffsetsBuffer;
    vector<uint8_t> tempKeywordSynonymBitMapBuffer;
    vector<uint8_t> tempKeywordSynonymCharLenBuffer;
    for (vector<KeywordRichInformation>::iterator iter =
            keywordRichInformationList.begin();
            iter != keywordRichInformationList.end(); ++iter) {
        // Copy keywordId
        forwardList->setKeywordId(keywordOffset, iter->keywordId);

        // Copy tf * sumOfFieldBoosts
        forwardList->setKeywordTfBoostProduct(keywordOffset,
                iter->keywordTfBoostProduct);

        // Copy score
        forwardList->setKeywordRecordStaticScore(keywordOffset,
                iter->keywordScore);

        //Build VLB array of attributes
        if (isEnabledAttributeBasedSearch(this->schemaInternal->getPositionIndexType())) {
        	// append current keyword;s attribute VLB array to temp buffer
        	tempAttributeIdBuffer.insert(tempAttributeIdBuffer.end(), iter->keywordAttribute.begin(),
        	        			iter->keywordAttribute.end());
        }
        //Build VLB array of word positions
        if (isEnabledWordPositionIndex(this->schemaInternal->getPositionIndexType())){
        	// append current keyword's positions VLB array to temp buffer
        	tempKeywordPositionsBuffer.insert(tempKeywordPositionsBuffer.end(),
        			iter->keywordPositionsInAllAttribute.begin(),
        			iter->keywordPositionsInAllAttribute.end());
        }
        //Build VLB array of char offsets
        if (isEnabledCharPositionIndex(this->schemaInternal->getPositionIndexType())){
        	// append current keyword's char offsets VLB array to temp buffer
        	tempKeywordCharOffsetsBuffer.insert(tempKeywordCharOffsetsBuffer.end(),
        			iter->keywordOffsetsInAllAttribute.begin(),
        			iter->keywordOffsetsInAllAttribute.end());
        }

        //Build VLB array of synonym bit map
        if (isEnabledCharPositionIndex(this->schemaInternal->getPositionIndexType())){
        	// append current keyword's synonym bitmaps VLB array to temp buffer
        	tempKeywordSynonymBitMapBuffer.insert(tempKeywordSynonymBitMapBuffer.end(),
        			iter->keywordSynonymBitMapInAllAttribute.begin(),
        			iter->keywordSynonymBitMapInAllAttribute.end());
        }

        //Build VLB array of char offsets
        if (isEnabledCharPositionIndex(this->schemaInternal->getPositionIndexType())){
        	// append current keyword's synonym original char lens VLB array to temp buffer
        	tempKeywordSynonymCharLenBuffer.insert(tempKeywordSynonymCharLenBuffer.end(),
        			iter->keywordSynonymCharLenInAllAttribute.begin(),
        			iter->keywordSynonymCharLenInAllAttribute.end());
        }

        ++keywordOffset;
    }
    // finally copy VLB array of attributes,  in-attribute positions and char offset to Forward list.
    forwardList->copyByteArraysToForwardList(tempAttributeIdBuffer, tempKeywordPositionsBuffer,
    		tempKeywordCharOffsetsBuffer, tempKeywordSynonymBitMapBuffer, tempKeywordSynonymCharLenBuffer);
}

void ForwardIndex::finalCommit() {
    this->commited_WriteView = true;
}

/*
 * this function uses forward index lock inside and it's expensive, so it should be used carefully inside a loop
 */
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
        const ForwardList* fl = this->getForwardList(readView, counter, valid);

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
        const vector<unsigned>& filteringAttributesList,
        ATTRIBUTES_OP attrOps,
        vector<unsigned> &keywordIdsVector) const {
    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd, minId);
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());

    bool returnValue = false;

    float matchingKeywordRecordStaticScore = 0.0;
    vector<unsigned> matchedAttributesList;
    while ((vectorIterator != vectorEnd) && (*vectorIterator <= maxId)) {
        if (this->isValidRecordTermHit(schema, (vectorIterator - vectorBegin),
        		filteringAttributesList, attrOps,
        		matchedAttributesList,
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
        const vector<unsigned>& filteringAttributesList, ATTRIBUTES_OP attrOp,
        unsigned &matchingKeywordId, vector<unsigned>& matchingKeywordAttributesList,
        float &matchingKeywordRecordStaticScore) const {
    const unsigned* vectorBegin = this->getKeywordIds();
    const unsigned* vectorEnd = this->getKeywordIds()
            + this->getNumberOfKeywords();
    const unsigned* vectorIterator = lower_bound(vectorBegin, vectorEnd, minId);
    ASSERT(vectorEnd - vectorBegin == this->getNumberOfKeywords());

    bool returnValue = false;
    matchingKeywordRecordStaticScore = 0;
    vector<unsigned> matchedAttributesList;

    while ((vectorIterator != vectorEnd) && (*vectorIterator <= maxId)) {
        float tempScore = 0;
        if (this->isValidRecordTermHit(schema, (vectorIterator - vectorBegin),
        		filteringAttributesList, attrOp, matchedAttributesList,
                tempScore)) {
            if (tempScore >= matchingKeywordRecordStaticScore) {
                matchingKeywordRecordStaticScore = tempScore;
                matchingKeywordAttributesList = matchedAttributesList;
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

unsigned ForwardList::getKeywordOffsetByLinearScan(unsigned keywordId) const {
	const unsigned* vectorBegin = this->getKeywordIds();
	const unsigned* vectorEnd = vectorBegin + this->getNumberOfKeywords();
	const unsigned* vectorIter = vectorBegin;
	while (vectorIter != vectorEnd) {
		if (*vectorIter == keywordId) {
			break;
		}
		++vectorIter;
	}
	return vectorIter - vectorBegin;
}

/*
 *   Populate the term frequency (TF) calculated as square root of all
 *   term occurrences in all attributes. If keyword is not found then
 *   set to 0.0. If position index is not enabled then set to 1.0.
 */
void ForwardList::getTermFrequency(const unsigned numOfKeywords,
        vector<float> & keywordTfList) const {
	ASSERT(numOfKeywords <= this->getNumberOfKeywords());

    this->getKeywordTfListInRecordField(keywordTfList);

    //Keyword not found, set to 0
    for(int i = keywordTfList.size(); i < numOfKeywords; i++ ){
        keywordTfList.push_back(0.0);
    }
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

    for (vector<unsigned>::const_iterator vectorIterator = hits.attributeIdList
            .begin(); vectorIterator != hits.attributeIdList.end();
            vectorIterator++) {
        int attributeId = *vectorIterator;
        sumOfFieldBoost += schema->getBoostOfSearchableAttribute(attributeId);
    }

    return 1.0 + (sumOfFieldBoost / schema->getBoostSumOfSearchableAttributes());
}

unsigned ForwardList::getNumberOfBytes() const {
    unsigned numberOfBytes = sizeof(ForwardList)
            + this->inMemoryDataLen + this->dataSize;
    return numberOfBytes;
}

// READER accesses this function
bool ForwardIndex::getExternalRecordIdFromInternalRecordId(
		shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		const unsigned internalRecordId,
        std::string &externalRecordId) const {
    ASSERT(internalRecordId < this->getTotalNumberOfForwardLists_ReadView());

    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardListDirectoryReadView, internalRecordId,valid);

    if (valid == false) {
        externalRecordId = -1;
        return false;
    } else {
        externalRecordId = forwardList->getExternalRecordId();
        return true;
    }
}

bool ForwardList::isValidRecordTermHit(const SchemaInternal *schema,
        unsigned keywordOffset, 
        const vector<unsigned>& filteringAttributesList,
        ATTRIBUTES_OP attrOp,
        vector<unsigned> &matchingKeywordAttributesList,
        float &matchingKeywordRecordStaticScore) const {
    matchingKeywordRecordStaticScore = this->getKeywordRecordStaticScore(
            keywordOffset);
    // support attribute-based search. Here we check if attribute search
    // is disabled, ie. the POSITION_INDEX_TYPE is neither FIELDBIT nor
    // FULL. In this case the the hit is always valid.
    if (!isEnabledAttributeBasedSearch(schema->getPositionIndexType())) {
        return true;
    } else if (filteringAttributesList.size() == 0){
    	// Check if the filtering attributes list is of size 0 ( which means all attributes)
    	if (attrOp == ATTRIBUTES_OP_NAND) {
    		// if operation is NAND (which means not match in all attributes), we should return false
    		return false;
    	} else if(attrOp == ATTRIBUTES_OP_OR) {
    		// if operation is OR then return true because term is present in the record.
    		return true;
    	} else {
    		ASSERT(false);  // cannot be AND
    		return false;
    	}
    }else {
        ASSERT(
                this->getKeywordAttributeIdsPointer()!= NULL and keywordOffset < this->getNumberOfKeywords());

        getKeywordAttributeIdsList(
                keywordOffset, matchingKeywordAttributesList);
        if (attrOp == ATTRIBUTES_OP_AND) {

            /*
             *  Mask the record's keyword attribute bit map with query's keyword attribute bit map.
             *  e.g:
             *  if a keyword python is in attributes title, tags, and body. Whereas a user searched
             *  python only in attribute body. Then we should set matchingKeywordAttributeBitmap to
             *  only have 'body' attribute set.
             */
            vector<unsigned> commonAttributesList;
            fetchCommonAttributes(matchingKeywordAttributesList, filteringAttributesList,
            		commonAttributesList);
            matchingKeywordAttributesList = commonAttributesList;
            /*
             *  For AND condition on attributes e.g  title and body : python
             *  The masked bitmap should be same as query bit map to be considered as a valid hit.
             */
            return isAttributesListsMatching(commonAttributesList
                    , filteringAttributesList);
        } else if (attrOp == ATTRIBUTES_OP_NAND) {
        	vector<unsigned> differenceAttributesList;
        	// check whether matching attributes for this term are subset of filter attributes.
        	// if yes then return false otherwise return true.
        	// e.g
        	//  filtering list = [a b c] , operation NAND ( not in any attributes)
        	//  1. matching list = [b ,c]  then return false
        	//  2. matching list = [c ,d ] then return true
        	//
        	set_difference(matchingKeywordAttributesList.begin(), matchingKeywordAttributesList.end(),
        			filteringAttributesList.begin(), filteringAttributesList.end(),
        			back_inserter(differenceAttributesList));

        	matchingKeywordAttributesList = differenceAttributesList;
        	return differenceAttributesList.size() != 0;

        }else { // OR operation
        	vector<unsigned> commonAttributesList;
        	fetchCommonAttributes(matchingKeywordAttributesList, filteringAttributesList,
        			commonAttributesList);
        	/*
        	 *  For OR condition on attributes e.g  title or body : python
        	 *  The masked bitmap should have atleast one query attribute set to be considered as
        	 *  a valid hit.
        	 */
        	matchingKeywordAttributesList = commonAttributesList;
            return (commonAttributesList.size() != 0);
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

//Get all the attribute id lists in one loop
void ForwardList::getKeywordAttributeIdsLists(const unsigned numOfKeywords,
        vector<vector<unsigned> > & attributeIdsLists) const {

    const uint8_t * piPtr = getKeywordAttributeIdsPointer(); // pointer to position index for the record

    attributeIdsLists.clear();

    unsigned piOffset = 0;
    unsigned value;
    short byteRead;
    vector<unsigned> attributeIdsList;
    // Get all keyword's attribute.
    for (unsigned j = 0; j < numOfKeywords; ++j) {
        attributeIdsList.clear();
        if (attributeIdsIndexSize == 0) {
            Logger::warn("Attribute Index not found in forward index!!");
            attributeIdsLists.push_back(attributeIdsList);
            continue;
        }

        if (piPtr == NULL) {
            attributeIdsLists.push_back(attributeIdsList);
            continue;
        }

        if (*(piPtr + attributeIdsIndexSize - 1) & 0x80) {
            attributeIdsLists.push_back(attributeIdsList);
            Logger::error(
                    "Attribute Ids index buffer has bad encoding..last byte is not a terminating one");
            continue;
        }

        ULEB128::varLengthBytesToUInt32(piPtr + piOffset, &value, &byteRead);
        ULEB128::varLenByteArrayToInt32Vector(
                (uint8_t *) (piPtr + piOffset + byteRead), value,
                attributeIdsList);
        attributeIdsLists.push_back(attributeIdsList);
        piOffset += byteRead + value;
    }
}

void ForwardList::getKeywordAttributeIdsList(unsigned keywordOffset, vector<unsigned>& attributeIdsList) const{

	if (attributeIdsIndexSize == 0){
		Logger::warn("Attribute Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getKeywordAttributeIdsPointer();  // pointer to position index for the record

	if (piPtr == NULL) {
		return;
	}

	if (*(piPtr + attributeIdsIndexSize - 1) & 0x80)
	{
		Logger::error("Attribute Ids index buffer has bad encoding..last byte is not a terminating one");
		return;
	}
	attributeIdsList.clear();
	unsigned piOffset = 0;
	unsigned value;
	short byteRead;
	// First jump over all the attribute ids of other keywords.
	for (unsigned j = 0; j < keywordOffset ; ++j) {
		ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
		piOffset += byteRead + value;
	}
	// Now read the length of current keyword's attribute.
	ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
	ULEB128::varLenByteArrayToInt32Vector((uint8_t *)(piPtr + piOffset + byteRead), value, attributeIdsList);
}

void ForwardList::getKeywordTfListInRecordField(
        vector<float> & keywordTfList) const {

    //If position index is not enabled then set all term frequency to 1.0.
    if (positionIndexSize == 0) {
        for (int i = 0; i < this->getNumberOfKeywords(); i++) {
            keywordTfList.push_back(1.0);
        }
        Logger::warn("Position Index not found in forward index!!");
        return;
    }

    const uint8_t * piPtr = getPositionIndexPointer();  // pointer to position index for the record

    if (*(piPtr + positionIndexSize - 1) & 0x80) {
        for (int i = 0; i < this->getNumberOfKeywords(); i++) {
            keywordTfList.push_back(0);
        }
        Logger::error(
                "position index buffer has bad encoding..last byte is not a terminating one");
        return;
    }

    getKeywordTfListFromVLBArray(piPtr, keywordTfList);
}

void ForwardList::getKeyWordPostionsInRecordField(unsigned keyOffset, unsigned attributeId,
		 vector<unsigned>& pl) const{

	if (positionIndexSize == 0){
		Logger::warn("Position Index not found in forward index!!");
		return;
	}

	const uint8_t * piPtr = getPositionIndexPointer();  // pointer to position index for the record

	if (*(piPtr + positionIndexSize - 1) & 0x80)
	{
		 Logger::error("position index buffer has bad encoding..last byte is not a terminating one");
		 return;
	}

	fetchDataFromVLBArray(keyOffset, attributeId, pl, piPtr);
}

void ForwardList::getKeyWordOffsetInRecordField(unsigned keyOffset, unsigned attributeId,
		vector<unsigned>& pl) const{
	if (offsetIndexSize == 0){
		Logger::warn("Offset Index not found in forward index!!");
		return;
	}

	const uint8_t * piPtr = getOffsetIndexPointer();  // pointer to position index for the record

	if (*(piPtr + offsetIndexSize - 1) & 0x80)
	{
		Logger::error("Offset index buffer has bad encoding..last byte is not a terminating one");
		return;
	}

	fetchDataFromVLBArray(keyOffset, attributeId, pl, piPtr);

}

void ForwardList::getSynonymCharLenInRecordField(unsigned keyOffset, unsigned attributeId,
		vector<unsigned>& pl) const{
	if (offsetIndexSize == 0 || charLenIndexSize == 0){
		Logger::warn("Char Len Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getCharLenIndexPointer();  // pointer to Char Len index for the record
	if (*(piPtr + charLenIndexSize - 1) & 0x80)
	{
		Logger::error("Char Len index buffer has bad encoding..last byte is not a terminating one");
		return;
	}

	fetchDataFromVLBArray(keyOffset, attributeId, pl, piPtr);
}

void ForwardList::getSynonymBitMapInRecordField(unsigned keyOffset, unsigned attributeId,
		vector<uint8_t>& synonymBitMap) const {

	if (offsetIndexSize == 0 || synonymBitMapSize == 0){
		Logger::warn("Synonym bitMap Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getSynonymBitMapPointer();  // pointer to Synonym BitMap for the record

	unsigned offset = 0;
	vector<unsigned> currKeywordAttributeIdsList;
	for (unsigned j = 0; j < keyOffset ; ++j) {
		getKeywordAttributeIdsList(j, currKeywordAttributeIdsList);

		unsigned count = currKeywordAttributeIdsList.size();
		for (unsigned k = 0; k < count; ++k){
			unsigned value;
			short byteRead;
			ULEB128::varLengthBytesToUInt32(piPtr + offset , &value, &byteRead);
			offset += byteRead + value;
		}
		currKeywordAttributeIdsList.clear();
	}

	getKeywordAttributeIdsList(keyOffset, currKeywordAttributeIdsList);

	// If keyword's attribute bitmap is 0 ( highly unlikely) or Attribute is not in keyword's attribute
	// bitmap (programmer's error) ...then return because we cannot get any position/offset info.

	if (currKeywordAttributeIdsList.size() == 0 ||
			std::find(currKeywordAttributeIdsList.begin(), currKeywordAttributeIdsList.end(), attributeId)
					== currKeywordAttributeIdsList.end())
		return;

	unsigned totalAttributes = currKeywordAttributeIdsList.size();
	for (int i = 0; i < totalAttributes; ++i){
		unsigned value;
		short byteRead;
		ULEB128::varLengthBytesToUInt32(piPtr + offset , &value, &byteRead);
		if (attributeId == currKeywordAttributeIdsList[i]){
			uint8_t * start = (uint8_t *)(piPtr + offset + byteRead);
			for (unsigned i = 0; i < value; ++i) {
				synonymBitMap.push_back(*(start + i));
			}
			break;
		}else {
			offset += byteRead + value;
		}
	}
}

void ForwardList::getKeywordTfListFromVLBArray(const uint8_t * piPtr,
        vector<float> & keywordTfList) const {
    // get the correct byte array position for current keyword + attribute combination
    unsigned piOffset = 0;
    vector<vector<unsigned> > keywordAttributeIdsLists;
    getKeywordAttributeIdsLists(this->getNumberOfKeywords(),keywordAttributeIdsLists) ;

    for (vector<vector<unsigned> >::iterator it = keywordAttributeIdsLists.begin();it !=  keywordAttributeIdsLists.end(); ++it) {

        // If keyword's attribute bitmap is 0 ( highly unlikely)
        if ((*it).size() == 0){
            keywordTfList.push_back(0);
            continue;
        }

        unsigned totalKeywordOccurTime = 0;

        for (unsigned k = 0; k < (*it).size(); ++k){
            unsigned value;
            short byteRead;
            vector<unsigned> pl;
            ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
            ULEB128::varLenByteArrayToInt32Vector((uint8_t *)(piPtr + piOffset + byteRead), value, pl);
            totalKeywordOccurTime += pl.size();
            piOffset += byteRead + value;
        }
        keywordTfList.push_back(sqrtf(totalKeywordOccurTime));
    }
}

void ForwardList::fetchDataFromVLBArray(unsigned keyOffset, unsigned attributeId,
		 vector<unsigned>& pl, const uint8_t * piPtr) const{
	// get the correct byte array position for current keyword + attribute combination

	unsigned piOffset = 0;
	vector<unsigned> currKeywordAttributeIdsList;
	for (unsigned j = 0; j < keyOffset ; ++j) {
		getKeywordAttributeIdsList(j, currKeywordAttributeIdsList);
		unsigned count = currKeywordAttributeIdsList.size();
		for (unsigned k = 0; k < count; ++k){
			unsigned value;
			short byteRead;
			ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
			piOffset += byteRead + value;
		}
		currKeywordAttributeIdsList.clear();
	}
	getKeywordAttributeIdsList(keyOffset, currKeywordAttributeIdsList);

	// If keyword's attribute bitmap is 0 ( highly unlikely) or Attribute is not in keyword's attribute
	// bitmap (programmer's error) ...then return because we cannot get any position/offset info.
	if (currKeywordAttributeIdsList.size() == 0 ||
		std::find(currKeywordAttributeIdsList.begin(), currKeywordAttributeIdsList.end(), attributeId)
								== currKeywordAttributeIdsList.end())
		return;

	unsigned totalAttributes = currKeywordAttributeIdsList.size();

	for (int i = 0; i < totalAttributes; ++i){
		unsigned value;
		short byteRead;
		ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
		if (attributeId == currKeywordAttributeIdsList[i]){
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
		if ((attrBitMap & (1 << shift)) > 0)
			++count;
		++shift;
	}
	return count;
}


/**********************************************/
StoredRecordBuffer ForwardIndex::getInMemoryData(unsigned internalRecordId) const {
    bool valid = false;
    shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
    this->getForwardListDirectory_ReadView(forwardListDirectoryReadView);
    const ForwardList* forwardList = this->getForwardList(forwardListDirectoryReadView, internalRecordId,valid);
    if (valid == false)
        return StoredRecordBuffer();
    else
        return forwardList->getInMemoryData();
}

float ForwardIndex::getTermRecordStaticScore(unsigned forwardIndexId,
        unsigned keywordOffset) const {
    bool valid = false;
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->getForwardListDirectory_ReadView(readView);
    const ForwardList* forwardList = this->getForwardList(readView, forwardIndexId,valid);
    if (valid == false)
        return 0;
    else
        return forwardList->getKeywordRecordStaticScore(keywordOffset);
}

bool ForwardIndex::isValidRecordTermHit(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		unsigned forwardIndexId,
        unsigned keywordOffset, const vector<unsigned>& filterAttributesList, ATTRIBUTES_OP attrOp,
        vector<unsigned> &matchingKeywordAttributesList,
        float& matchingKeywordRecordStaticScore) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardListDirectoryReadView, forwardIndexId,valid);
    if (valid && forwardList != NULL)
        return forwardList->isValidRecordTermHit(this->schemaInternal,
                keywordOffset, filterAttributesList, attrOp,
                matchingKeywordAttributesList,
                matchingKeywordRecordStaticScore);
    else
        return false;
}

bool ForwardIndex::isValidRecordTermHitWithStemmer(unsigned forwardIndexId,
        unsigned keywordOffset, unsigned searchableAttributeId,
        unsigned &matchingKeywordAttributeBitmap,
        float &matchingKeywordRecordStaticScore, bool &isStemmed) const {
    bool valid = false;
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->getForwardListDirectory_ReadView(readView);
    const ForwardList* forwardList = this->getForwardList(readView, forwardIndexId,
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
        this->deletedRecordInternalIds.insert(internalRecordId); // remember this deleted record ID
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
        // we need to remove this ID from the set of deleted IDs
        this->deletedRecordInternalIds.erase(internalRecordId);
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
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->getForwardListDirectory_ReadView(readView);
    this->getForwardList(readView, internalRecordId, valid);

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

unsigned ForwardIndex::getKeywordOffset(shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView,
		unsigned forwardListId,
        unsigned keywordId) const {
    bool valid = false;
    const ForwardList* forwardList = this->getForwardList(forwardListDirectoryReadView, forwardListId, valid);
    // if the record is not valid (e.g., marked deleted), we return a special flag
    if (!valid)
       return FORWARDLIST_NOTVALID;
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
    Schema * storedSchema = Schema::create();
    RecordSerializerUtil::populateStoredSchema(storedSchema, forwardIndex.getSchema());
    for (unsigned counter = 0; counter < forwardListDirectoryReadView->size(); ++counter) {
        bool valid = false;
        const ForwardList* fl = forwardIndex.getForwardList(forwardListDirectoryReadView, counter, valid);
        // ignore the invalid record
        if (valid == false)
            continue;
        StoredRecordBuffer buff = fl->getInMemoryData();
        string jsonBuffer;
        RecordSerializerUtil::convertCompactToJSONString(storedSchema, buff, fl->getExternalRecordId(),jsonBuffer);
        out << jsonBuffer << endl;
    }
    out.close();
    delete storedSchema;
}

/*
 *   Utility Functions
 */

// This function takes two attribute-ids lists as inputs and returns a third list which contains
// attribute common in both the input lists.
void fetchCommonAttributes(const vector<unsigned>& list1, const vector<unsigned>& list2,
		vector<unsigned>& outList) {

	// if a input list is empty then we consider it equivalent to a list
	// containing all the attributes of a data record. In such a case,
	// return the non-empty input list because the intersection of empty (all attributes)
	// and non-emoty (some attributes) will return some attributes (non-empty list).
	// e.g list1 = [] (Universal Set) and list2 = [ A , B, C ]
	// list1 intersectOp list2 = list2

	if (list1.size() == 0) {
		outList = list2;
	} else if (list2.size() == 0) {
		outList = list1;
	} else {
		// The assumption is that both list1 and list2 are sorted. Caller should ensure this.
		std::set_intersection(list1.begin(), list1.end(), list2.begin(), list2.end(),
				std::back_inserter(outList));
	}
}

// This function takes two attribute-ids lists as inputs and returns true if both the lists are same
// (all elements match in same order) and false otherwise.
bool isAttributesListsMatching(const vector<unsigned>& list1, const vector<unsigned>& list2) {
	if (list1.size() != list2.size())
		return false;

	for (unsigned i = 0; i < list1.size(); ++i) {
		if (list1[i] != list2[i])
			return false;
	}
	return true;
}

/*
 *  Util functions for reorder forward list logic
 */
// API to fetch VLB array of all attributes for a keyword.
void ForwardList::getKeywordAttributeIdsByteArray(unsigned keywordOffset, vector<uint8_t>& attributesVLBarray){

	if (attributeIdsIndexSize == 0){
		Logger::warn("Attribute Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getKeywordAttributeIdsPointer();  // pointer to position index for the record

	if (piPtr == NULL) {
		return;
	}

	if (*(piPtr + attributeIdsIndexSize - 1) & 0x80)
	{
		Logger::error("Attribute Ids index buffer has bad encoding..last byte is not a terminating one");
		return;
	}
	attributesVLBarray.clear();
	unsigned piOffset = 0;
	unsigned value;
	short byteRead;
	// First jump over all the attribute ids of other keywords.
	for (unsigned j = 0; j < keywordOffset ; ++j) {
		ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
		piOffset += byteRead + value;
	}
	// Now read the length of current keyword's attribute.
	ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
	unsigned totalbytes = byteRead + value;
	attributesVLBarray.resize(totalbytes);
	for (unsigned i = 0; i < totalbytes; ++i) {
		attributesVLBarray[i] = *(piPtr + piOffset + i);
	}
}
// API to fetch VLB array of word positions in all attributes for a keyword.
void ForwardList::getKeyWordPostionsByteArray(unsigned keywordOffset, vector<uint8_t>& positionsVLBarray){
	if (positionIndexSize == 0){
		Logger::warn("Position Index not found in forward index!!");
		return;
	}

	const uint8_t * piPtr = getPositionIndexPointer();  // pointer to position index for the record

	if (*(piPtr + positionIndexSize - 1) & 0x80)
	{
		 Logger::error("position index buffer has bad encoding..last byte is not a terminating one");
		 return;
	}

	fetchVLBArrayForKeyword(keywordOffset, piPtr, positionsVLBarray);
}
// API to fetch VLB array of char offsets in all attributes for a keyword.
void ForwardList::getKeyWordOffsetsByteArray(unsigned keywordOffset, vector<uint8_t>& charOffsetsVLBarray){
	if (offsetIndexSize == 0){
		Logger::warn("Offset Index not found in forward index!!");
		return;
	}

	const uint8_t * piPtr = getOffsetIndexPointer();  // pointer to position index for the record

	if (*(piPtr + offsetIndexSize - 1) & 0x80)
	{
		Logger::error("Offset index buffer has bad encoding..last byte is not a terminating one");
		return;
	}

	fetchVLBArrayForKeyword(keywordOffset, piPtr, charOffsetsVLBarray);
}
// API to fetch VLB array of synonym bitmaps in all attributes for a keyword.
void ForwardList::getKeywordSynonymsBitMapByteArray(unsigned keywordOffset, vector<uint8_t>& synonymBitMapVLBarray){

	if (offsetIndexSize == 0 || synonymBitMapSize == 0){
		Logger::warn("Synonym bitMap Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getSynonymBitMapPointer();  // pointer to Synonym BitMap for the record

	fetchVLBArrayForKeyword(keywordOffset, piPtr, synonymBitMapVLBarray);
}
// API to fetch VLB array of synonym original char lens in all attributes for a keyword.
void ForwardList::getSynonymCharLensByteArray(unsigned keywordOffset, vector<uint8_t>& synonymCharLensVLBarray){
	if (offsetIndexSize == 0 || charLenIndexSize == 0){
		Logger::warn("Char Len Index not found in forward index!!");
		return;
	}
	const uint8_t * piPtr = getCharLenIndexPointer();  // pointer to Char Len index for the record
	if (*(piPtr + charLenIndexSize - 1) & 0x80)
	{
		Logger::error("Char Len index buffer has bad encoding..last byte is not a terminating one");
		return;
	}

	fetchVLBArrayForKeyword(keywordOffset, piPtr, synonymCharLensVLBarray);
}
// common API to fetch VLB array for a keyword.
void ForwardList::fetchVLBArrayForKeyword(unsigned keyOffset, const uint8_t * piPtr, vector<uint8_t>& vlbArray) {

	unsigned piOffset = 0;
	vector<unsigned> currKeywordAttributeIdsList;
	for (unsigned j = 0; j < keyOffset ; ++j) {
		getKeywordAttributeIdsList(j, currKeywordAttributeIdsList);
		unsigned count = currKeywordAttributeIdsList.size();
		for (unsigned k = 0; k < count; ++k){
			unsigned value;
			short byteRead;
			ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
			piOffset += byteRead + value;
		}
		currKeywordAttributeIdsList.clear();
	}

	getKeywordAttributeIdsList(keyOffset, currKeywordAttributeIdsList);
	unsigned totalAttributes = currKeywordAttributeIdsList.size();
	for (int i = 0; i < totalAttributes; ++i){
		unsigned value;
		short byteRead;
		ULEB128::varLengthBytesToUInt32(piPtr + piOffset , &value, &byteRead);
		unsigned totalbytes = byteRead + value;
		for (unsigned i = 0; i < totalbytes; ++i) {
			vlbArray.push_back(*(piPtr + piOffset + i));
		}
		piOffset += totalbytes;
	}
}

}
}
