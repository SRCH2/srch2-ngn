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
                unsigned keyId = indexSearcherInternal->getTrie()->getTrieNodeFromUtf8String(
                        trieRootNode_ReadView->root, keywordString)->getId();
                pi.keywordIds.push_back(keyId);
            }
        }
    }

    /*
     *  Loop over the input records  and apply all the phrase filter stored
     *  in phraseInfoVector
     */

    unsigned resultsCount =  input->getNumberOfResults();
    for (unsigned i = 0; i < resultsCount; ++i) {
        QueryResult * qr = input->impl->sortedFinalResults[i];
        bool isValid = false;
        const ForwardList* forwardListPtr = forwardIndex->getForwardList(qr->internalRecordId, isValid);
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

    PhraseSearcher *phraseSearcher = new PhraseSearcher();
    const vector<string>& keywords = phraseInfo.phraseKeyWords;
    vector<unsigned> matchedPosition;
    // check whether it is AND or OR boolean operator for multiple fields.
    bool ANDOperation = phraseInfo.attributeBitMap & 0x80000000;
    unsigned mask = 1;
    bool result = false;
    unsigned totalAttributes = sizeof(phraseInfo.attributeBitMap) - 1;
    unsigned slop = phraseInfo.proximitySlop;
    for (unsigned attributeId = 0; attributeId < totalAttributes; ++attributeId) {
        mask <<= attributeId;
        vector<vector<unsigned> > positionListVector;
        if ((phraseInfo.attributeBitMap & mask) == 0)
            continue;
        for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
            unsigned keywordId = phraseInfo.keywordIds[i];
            vector<unsigned> positionList;
            forwardListPtr->getKeyWordPostionsInRecordField(keywordId, attributeId, positionList);
            if (positionList.size() == 0){
                Logger::debug("Position Indexes for keywordId = %d , attribute = %d not be found",
                        keywordId, attributeId);
            }
            positionListVector.push_back(positionList);
        }
        if (slop > 0){
            result = phraseSearcher->proximityMatch(positionListVector, keywords, slop, matchedPosition);
        } else {
        	const vector<unsigned> & pKeyPosindexRef = phraseInfo.phraseKeywordPositionIndex;
            result = phraseSearcher->exactMatch(positionListVector, pKeyPosindexRef, matchedPosition);
        }
        // AND operation and we didn't find result so we should break
        if (result == false && ANDOperation)
            break;
        // OR operation and we found result so we should break
        if (result == true && !ANDOperation)
            break;
        positionListVector.clear();
    }
    return result;
}
}
}
