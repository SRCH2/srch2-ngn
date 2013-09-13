/*
 * PhraseSearchFilter.cpp
 *
 *  Created on: Sep 9, 2013
 *      Author: sbisht
 */

#include "PhraseSearchFilter.h"
#include "operation/PhraseSearcher.h"
#include "operation/IndexSearcherInternal.h"

namespace srch2 {
namespace instantsearch {

void PhraseQueryFilter::doFilter(
    IndexSearcher* indexSearcher, const Query* query, QueryResults* input,
    QueryResults* output) {

	unsigned resultsCount =  input->getNumberOfResults();

	if (resultsCount == 0)
	    	return;

	IndexSearcherInternal * indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(indexSearcher);

	ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

    /*
     * fetch internal keyword ids from keyword string
     */

	{
		boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
		indexSearcherInternal->getTrie()->getTrieRootNode_ReadView(trieRootNode_ReadView);

		for ( unsigned i = 0 ; i < phraseInfoVector.size(); ++i) {
			PhraseInfo & pi = phraseInfoVector[i];
			for (int j = 0; j < pi.phraseKeyWords.size(); ++j) {
				const string& keywordString = pi.phraseKeyWords[j];
				const TrieNode *trieNode = indexSearcherInternal->getTrie()->getTrieNodeFromUtf8String(
						trieRootNode_ReadView->root, keywordString);
				if (trieNode == NULL){
					Logger::warn("TrieNode is null for phrase keyword = %s", keywordString.c_str());
					return;
				}
				unsigned keywordId = trieNode->getId();
				pi.keywordIds.push_back(keywordId);
			}
		}
	}

    /*
     *  Loop over the input records  and apply all the phrase filter stored
     *  in phraseInfoVector
     */

    for (unsigned i = 0; i < resultsCount; ++i) {
        QueryResult * qr = input->impl->sortedFinalResults[i];
        bool isValid = false;
        const ForwardList* forwardListPtr = forwardIndex->getForwardList(qr->internalRecordId, isValid);
        if (false == isValid){
        	continue;
        }        	
        bool pass = true;
        for ( unsigned j = 0 ; j < phraseInfoVector.size(); ++j) {
            pass &= matchPhrase(forwardListPtr, phraseInfoVector[j]);
        }
        if (pass)
            output->impl->sortedFinalResults.push_back(qr);
    }
}

void PhraseQueryFilter::addPhrase(const vector<string>& phraseKeywords,
        const vector<unsigned>& phraseKeywordsPositionIndex,
        unsigned proximitySlop,
        unsigned attributeBitMap){

    PhraseInfo pi;
    pi.phraseKeywordPositionIndex = phraseKeywordsPositionIndex;
    pi.attributeBitMap = attributeBitMap;
    pi.phraseKeyWords = phraseKeywords;
    pi.proximitySlop = proximitySlop;
    phraseInfoVector.push_back(pi);
}
// match phrase on attributes. do OR or AND logic depending upon the 32 bit of attributeBitMap
bool PhraseQueryFilter::matchPhrase(const ForwardList* forwardListPtr, const PhraseInfo& phraseInfo) {
    
    vector<unsigned> keywordsOffsetinForwardList;
    vector<unsigned> keywordsAttrBitMapInForwardList;
    
    // Attribute bit map may not be initialized if position indexing is not
    // enabled in config file. Just return false if so.
    if (forwardListPtr->getKeywordAttributeBitmaps() == 0){
    	Logger::warn("Attribute info not found in forward List!!");
    	return false; 
    }  
    
    // store keyword offsets and their attribute bit map vector.
    for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
    	unsigned keywordOffset = forwardListPtr->getKeywordOffset(phraseInfo.keywordIds[i]);
    	if (keywordOffset > forwardListPtr->getNumberOfKeywords()){
    		Logger::warn("Keyword %s not found in forward List !!",
    				phraseInfo.phraseKeyWords[i].c_str());
    		return false;
    	}
    	keywordsOffsetinForwardList.push_back(keywordOffset);
    	keywordsAttrBitMapInForwardList.push_back(
    			forwardListPtr->getKeywordAttributeBitmap(keywordOffset));
    }

    // pre-determine the attributes that we should search. It is intersection of keyword
    // attributes and attributes mentioned in query
    unsigned allowedBitMap = phraseInfo.attributeBitMap;
    for (int i = 0; i < keywordsAttrBitMapInForwardList.size(); ++i) {
    	allowedBitMap &= keywordsAttrBitMapInForwardList[i];
    }

    // check whether it is AND or OR boolean operator for multiple fields.
    bool ANDOperation = phraseInfo.attributeBitMap & 0x80000000;

    unsigned mask = 1;
    bool result = false;
    unsigned totalAttributes = sizeof(phraseInfo.attributeBitMap) * 8 - 1;

    for (unsigned attributeId = 0; attributeId < totalAttributes; ++attributeId) {
        mask = 1 << attributeId;
        if ((allowedBitMap & mask) == 0) {
        	continue;
        }

        vector<vector<unsigned> > positionListVector;
        for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
            unsigned keyOffset = keywordsOffsetinForwardList[i];
            unsigned keyAttrBitMap = keywordsAttrBitMapInForwardList[i];
            vector<unsigned> positionList;
            forwardListPtr->getKeyWordPostionsInRecordField(keyOffset, attributeId, keyAttrBitMap,
            		positionList);
            if (positionList.size() == 0){
                Logger::debug("Position Indexes for keyword = %s , attribute = %d not be found",
                		phraseInfo.phraseKeyWords[i].c_str(), attributeId);
            }
            positionListVector.push_back(positionList);
        }
        
    	const vector<unsigned> & phraseOffsetRef = phraseInfo.phraseKeywordPositionIndex;
    	
    	PhraseSearcher *phraseSearcher = new PhraseSearcher();
    	vector<unsigned> matchedPosition;
        unsigned slop = phraseInfo.proximitySlop;

        if (slop > 0){
            result = phraseSearcher->proximityMatch(positionListVector, phraseOffsetRef, slop,
            		matchedPosition);
        } else {
            result = phraseSearcher->exactMatch(positionListVector, phraseOffsetRef,
            		matchedPosition);
        }
        // AND operation and we didn't find result so we should break
        if (ANDOperation && result == false)
            break;
        // OR operation and we found result so we should break
        if (!ANDOperation && result == true)
            break;
        positionListVector.clear();
    }
    return result;
}
}
}
