





#include "PhraseSearchOperator.h"
#include "PhysicalOperatorsHelper.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "util/Assert.h"
#include "instantsearch/TypedValue.h"
#include "util/DateAndTimeHandler.h"
#include "operation/QueryEvaluatorInternal.h"
#include "operation/PhraseSearcher.h"

namespace srch2 {
namespace instantsearch {




bool PhraseSearchOperator::open(QueryEvaluatorInternal * queryEvaluatorInternal, PhysicalPlanExecutionParameters & params){

	this->queryEvaluatorInternal = queryEvaluatorInternal;

	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);

    /*
     * fetch internal keyword ids from keyword string
     */

	{
		boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
		queryEvaluatorInternal->getTrie()->getTrieRootNode_ReadView(trieRootNode_ReadView);

		for ( unsigned i = 0 ; i < phraseSearchInfoContainer->phraseInfoVector.size(); ++i) {
			PhraseInfo & pi = phraseSearchInfoContainer->phraseInfoVector[i];
			for (int j = 0; j < pi.phraseKeyWords.size(); ++j) {
				const string& keywordString = pi.phraseKeyWords[j];
				const TrieNode *trieNode = queryEvaluatorInternal->getTrie()->getTrieNodeFromUtf8String(
						trieRootNode_ReadView->root, keywordString);
				if (trieNode == NULL){
					Logger::warn("TrieNode is null for phrase keyword = %s", keywordString.c_str());
					return false;
				}
				unsigned keywordId = trieNode->getId();
				pi.keywordIds.push_back(keywordId);
			}
		}
	}


	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluatorInternal, params);

	return true;
}
PhysicalPlanRecordItem * PhraseSearchOperator::getNext(const PhysicalPlanExecutionParameters & params) {
    /*
     *  Loop over the input records  and apply all the phrase filter stored
     *  in phraseInfoVector
     */

	ForwardIndex * forwardIndex = this->queryEvaluatorInternal->getForwardIndex();

	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			return NULL;
		}
        bool isValid = false;
        const ForwardList* forwardListPtr = forwardIndex->getForwardList(nextRecord->getRecordId(), isValid);
        if (false == isValid){
        	continue;
        }
        bool pass = true;
        for ( unsigned j = 0 ; j < this->phraseSearchInfoContainer->phraseInfoVector.size(); ++j) {
            pass &= matchPhrase(forwardListPtr, this->phraseSearchInfoContainer->phraseInfoVector[j]);
        }
        if (pass){
        	return nextRecord;
        }
	}
	ASSERT(false); // we never reach to this point
	return NULL;
}
bool PhraseSearchOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluatorInternal = NULL;
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

void PhraseSearchOperator::getUniqueStringForCache(bool ignoreLastLeafNode, string & uniqueString){

}

bool PhraseSearchOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	ASSERT(false);
	return false;
}
PhraseSearchOperator::~PhraseSearchOperator(){

}
PhraseSearchOperator::PhraseSearchOperator(PhraseSearchInfoContainer * phraseSearchInfoContainer) {
	this->phraseSearchInfoContainer = phraseSearchInfoContainer;
}

// match phrase on attributes. do OR or AND logic depending upon the 32 bit of attributeBitMap
bool PhraseSearchOperator::matchPhrase(const ForwardList* forwardListPtr, const PhraseInfo& phraseInfo) {

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

    // check whether it is AND or OR boolean operator for multiple fields.
    bool ANDOperation = phraseInfo.attributeBitMap & 0x80000000;

    //special check for AND operation ...if the set bits of query bit map does not match with all
    // the keywords bitmap then we should return false because AND condition will not be satisfied.
    // The below code goes over all keyword in phrase and check whether it has all the attribute
    // mentioned in the query ( i.e specified by phraseInfo.attributeBitMap)
    // e.g for a given record, if "new" is present in "title" and "description" field
    // and "york" is present only in "title" field and
    // if query is title.description:"new york" then there should skip this record.
    //
    if (ANDOperation && phraseInfo.attributeBitMap != 0xFFFFFFFF) {
    	for (int i = 0; i < keywordsAttrBitMapInForwardList.size(); ++i) {
    		unsigned resultBitMap = phraseInfo.attributeBitMap & keywordsAttrBitMapInForwardList[i];
    		if (resultBitMap != phraseInfo.attributeBitMap) {
    			return false;
    		}
    	}
    }
    // pre-determine the attributes that we should search. It is intersection of keyword
    // attributes and attributes mentioned in query
    unsigned allowedBitMap = phraseInfo.attributeBitMap;
    for (int i = 0; i < keywordsAttrBitMapInForwardList.size(); ++i) {
    	allowedBitMap &= keywordsAttrBitMapInForwardList[i];
    }

    unsigned mask = 1;
    bool result = false;
    unsigned totalAttributes = sizeof(phraseInfo.attributeBitMap) * 8 - 1;

    // pre-allocate the vectors for position lists of query keywords
    vector<vector<unsigned> > positionListVector;
    for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
    	positionListVector.push_back(vector<unsigned>());
    }

    for (unsigned attributeId = 0; attributeId < totalAttributes; ++attributeId) {
        mask = 1 << attributeId;
        if ((allowedBitMap & mask) == 0) {
        	continue;
        }
        for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
            unsigned keyOffset = keywordsOffsetinForwardList[i];
            unsigned keyAttrBitMap = keywordsAttrBitMapInForwardList[i];
            vector<unsigned>& positionList = positionListVector[i];
            forwardListPtr->getKeyWordPostionsInRecordField(keyOffset, attributeId, keyAttrBitMap,
            		positionList);
            if (positionList.size() == 0){
                Logger::debug("Position Indexes for keyword = %s , attribute = %d not be found",
                		phraseInfo.phraseKeyWords[i].c_str(), attributeId);
            }
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

        // clear the position list for next iteration.
        for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
        	// clear makes size = 0 but does not reduce the allocated internal buffer
        	positionListVector[i].clear();
        }
    }

    return result;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	ASSERT(false);
	return PhysicalPlanCost(0);
}
void PhraseSearchOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	ASSERT(false);
	return;
}
void PhraseSearchOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	ASSERT(false);
	return;
}
PhysicalPlanNodeType PhraseSearchOptimizationOperator::getType() {
	return PhysicalPlanNode_PhraseSearch;
}
bool PhraseSearchOptimizationOperator::validateChildren(){
	ASSERT(false);
	return false;
}

}
}
