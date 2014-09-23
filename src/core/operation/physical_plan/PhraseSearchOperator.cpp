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
		// We need to get the read view from this->indexReadToken
		// instead of calling this->getTrie()->getTrieRootNode_ReadView()
		// since the latter may give a read view that is different from
		// the one we got when the search started.
		trieRootNode_ReadView = queryEvaluatorInternal->indexReadToken.trieRootNodeSharedPtr;

		for (int j = 0; j < phraseSearchInfo.phraseKeyWords.size(); ++j) {
			const string& keywordString = phraseSearchInfo.phraseKeyWords[j];
			const TrieNode *trieNode = queryEvaluatorInternal->getTrie()->getTrieNodeFromUtf8String(
					trieRootNode_ReadView->root, keywordString);
			if (trieNode == NULL){
				Logger::warn("keyword = '%s' of a phrase query was not found!", keywordString.c_str());
				phraseErr = true;
				return false;
			}
			unsigned keywordId = trieNode->getId();
			phraseSearchInfo.keywordIds.push_back(keywordId);
		}

	}

	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluatorInternal, params);

	return true;
}
PhysicalPlanRecordItem * PhraseSearchOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if (phraseErr)
		return NULL;

	if (this->queryEvaluatorInternal == NULL) {
		return NULL;  // open should be called first
	}

	ForwardIndex * forwardIndex = this->queryEvaluatorInternal->getForwardIndex();
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->queryEvaluatorInternal->getForwardIndex_ReadView(readView);
    /*
     *  Loop over the input records  and apply the phrase filter
     */
	while(true){
		PhysicalPlanRecordItem * nextRecord =
				this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			return NULL;
		}
        bool isValid = false;
        const ForwardList* forwardListPtr = forwardIndex->getForwardList(readView, nextRecord->getRecordId(), isValid);
        if (false == isValid){ // ignore this record if it's already deleted
        	continue;
        }
        vector<unsigned> listOfSlopDistances;
        if (matchPhrase(forwardListPtr, this->phraseSearchInfo, listOfSlopDistances)){
        	vector<TermType>& recordMatchingTermTypes = nextRecord->getTermTypesRef();
        	for (unsigned i = 0; i < recordMatchingTermTypes.size(); ++i) {
        		recordMatchingTermTypes[i] = TERM_TYPE_PHRASE;
        	}
        	//We check length of listOfSlops for defensive programming.
        	//Record's runtime score is set.
        	if(listOfSlopDistances.size() > 0){
        	    float sloppyFrequency = params.ranker->computeSloppyFrequency(listOfSlopDistances);
        	    float positionalScore = params.ranker->computePositionalScore(nextRecord->getRecordRuntimeScore(), sloppyFrequency);
        	    nextRecord->setRecordRuntimeScore(positionalScore);
        	}
        	return nextRecord;
        }
	}
	ASSERT(false); // we never reach to this point
	return NULL;
}
bool PhraseSearchOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluatorInternal = NULL;
	// If there was a phrase error then the operator did not open its child. So no need to close it
	if (!phraseErr)
		this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

string PhraseSearchOperator::toString(){
	string result = "PhraseSearchOperator" + this->phraseSearchInfo.toString();
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool PhraseSearchOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {

	if (phraseErr)
		return false;

	if (this->queryEvaluatorInternal == NULL) {
		return false;  // open should be called first
	}

	bool verifiedForKeywordExistance = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);

	if(verifiedForKeywordExistance == false){
		return false;
	}

	ForwardIndex * forwardIndex = this->queryEvaluatorInternal->getForwardIndex();
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->queryEvaluatorInternal->getForwardIndex_ReadView(readView);

    bool isValid = false;
    const ForwardList* forwardListPtr = forwardIndex->getForwardList(readView, parameters.recordToVerify->getRecordId(), isValid);
    if (false == isValid){ // ignore this record if it's already deleted
    	false;
    }

    vector<unsigned> listOfSlops;
    if (matchPhrase(forwardListPtr, this->phraseSearchInfo, listOfSlops)){
    	vector<TermType> recordMatchingTermTypes = parameters.recordToVerify->getTermTypesRef();
    	for (unsigned i = 0; i < recordMatchingTermTypes.size(); ++i) {
    		recordMatchingTermTypes[i] = TERM_TYPE_PHRASE;
    	}
    	return true;
    }

	return false;
}
PhraseSearchOperator::~PhraseSearchOperator(){
	delete this->phraseSearcher;
}
PhraseSearchOperator::PhraseSearchOperator(const PhraseInfo& phraseSearchInfo) {
	this->phraseSearchInfo = phraseSearchInfo;
	this->queryEvaluatorInternal = NULL;
	this->phraseErr = false;
	this->phraseSearcher = new PhraseSearcher();
}

// match phrase on attributes. do OR or AND logic depending upon the 32 bit of attributeBitMap
bool PhraseSearchOperator::matchPhrase(const ForwardList* forwardListPtr, const PhraseInfo& phraseInfo, vector<unsigned>& listOfSlopDistances) {

    vector<unsigned> keywordsOffsetinForwardList;
    vector<vector<unsigned> > keywordsAttrIdsInForwardList;

    // Attribute bit map may not be initialized if position indexing is not
    // enabled in config file. Just return false if so.
    if (forwardListPtr->getKeywordAttributesListPtr() == 0){
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
    	keywordsAttrIdsInForwardList.push_back(vector<unsigned>());
    	forwardListPtr->getKeywordAttributeIdsList(keywordOffset, keywordsAttrIdsInForwardList[i]);
    }

    // check whether it is AND or OR boolean operator for multiple fields.
    ATTRIBUTES_OP attrOp = phraseInfo.attrOps;

    //special check for AND operation ...if the set bits of query bit map does not match with all
    // the keywords bitmap then we should return false because AND condition will not be satisfied.
    // The below code goes over all keyword in phrase and check whether it has all the attribute
    // mentioned in the query ( i.e specified by phraseInfo.attributeBitMap)
    // e.g for a given record, if "new" is present in "title" and "description" field
    // and "york" is present only in "title" field and
    // if query is title.description:"new york" then there should skip this record.
    //
    if (attrOp == ATTRIBUTES_OP_AND && phraseInfo.attributeIdsList.size() > 0) {
    	for (int i = 0; i < keywordsAttrIdsInForwardList.size(); ++i) {
    		vector<unsigned> resultAttributeList;
    		fetchCommonAttributes(phraseInfo.attributeIdsList, keywordsAttrIdsInForwardList[i],
    				resultAttributeList);
    		if (!isAttributesListsMatching(resultAttributeList, phraseInfo.attributeIdsList)) {
    			return false;
    		}
    	}
    }
    // pre-determine the attributes that we should search. It is intersection of keyword
    // attributes and attributes mentioned in query
    vector<unsigned> allowedBitMap = phraseInfo.attributeIdsList;
    for (int i = 0; i < keywordsAttrIdsInForwardList.size(); ++i) {
    	vector<unsigned> temp;
    	fetchCommonAttributes( allowedBitMap, keywordsAttrIdsInForwardList[i], temp);
    	allowedBitMap = temp;
    }

    unsigned mask = 1;
    bool result = false;
    unsigned totalAttributes = phraseInfo.attributeIdsList.size();

    // pre-allocate the vectors for position lists of query keywords
    vector<vector<unsigned> > positionListVector;
    for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
    	positionListVector.push_back(vector<unsigned>());
    }

    for (unsigned i = 0; i < allowedBitMap.size(); ++i) {
    	unsigned attributeId = allowedBitMap[i];
        for (int i = 0; i < phraseInfo.keywordIds.size(); ++i) {
            unsigned keyOffset = keywordsOffsetinForwardList[i];
            vector<unsigned>& keyAttrIdsList = keywordsAttrIdsInForwardList[i];
            vector<unsigned>& positionList = positionListVector[i];
            forwardListPtr->getKeyWordPostionsInRecordField(keyOffset, attributeId,
            		positionList);
            if (positionList.size() == 0){
                Logger::debug("Position Indexes for keyword = %s , attribute = %d not be found",
                		phraseInfo.phraseKeyWords[i].c_str(), attributeId);
            }
        }

    	const vector<unsigned> & phraseOffsetRef = phraseInfo.phraseKeywordPositionIndex;

    	// This vector stores all the valid matched positions for a given phrase in the record.
    	vector< vector<unsigned> > matchedPositions;
    	//vector<unsigned> matchedPosition;
        unsigned slop = phraseInfo.proximitySlop;

        if (slop > 0){
            result = this->phraseSearcher->proximityMatch(positionListVector, phraseOffsetRef, slop,
            		matchedPositions, listOfSlopDistances, false);  // true means we stop at first match, false means we continue
        } else {
            result = this->phraseSearcher->exactMatch(positionListVector, phraseOffsetRef,
            		matchedPositions, listOfSlopDistances, false);  // true means we stop at first match, false means we continue
        }
        // AND operation and we didn't find result so we should break
        if (attrOp == ATTRIBUTES_OP_AND && result == false)
            break;
        // OR operation and we found result so we should break
        if (attrOp == ATTRIBUTES_OP_OR && result == true)
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

	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params);
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfGetNext(params);
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {

	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost PhraseSearchOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void PhraseSearchOptimizationOperator::getOutputProperties(IteratorProperties & prop){

	return;
}
void PhraseSearchOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){

	return;
}
PhysicalPlanNodeType PhraseSearchOptimizationOperator::getType() {
	return PhysicalPlanNode_PhraseSearch;
}
bool PhraseSearchOptimizationOperator::validateChildren() {
	for (unsigned i = 0; i < this->getChildrenCount(); ++i){
		switch (this->getChildAt(i)->getType()) {
		case PhysicalPlanNode_MergeSortedById:
		case PhysicalPlanNode_MergeByShortestList:
			break; // keep looping
		default:
			return false;
		}
	}
	return true;
}

}
}
