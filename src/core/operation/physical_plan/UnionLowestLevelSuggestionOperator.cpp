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

#include "UnionLowestLevelSuggestionOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelSuggestionOperator::UnionLowestLevelSuggestionOperator() {
}

UnionLowestLevelSuggestionOperator::~UnionLowestLevelSuggestionOperator(){
}
bool UnionLowestLevelSuggestionOperator::open(QueryEvaluatorInternal * queryEvaluatorIntrnal, PhysicalPlanExecutionParameters & params){

    this->queryEvaluatorIntrnal = queryEvaluatorIntrnal;
    this->queryEvaluatorIntrnal->getInvertedIndex()->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
    this->queryEvaluatorIntrnal->getInvertedIndex()->getInvertedIndexKeywordIds_ReadView(invertedIndexKeywordIdsReadView);
    this->queryEvaluatorIntrnal->getForwardIndex()->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);
    // 1. first iterate on active nodes and find best estimated leaf nodes.
    Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(params.isFuzzy);
    unsigned numberOfSuggestionsToFind = 350;
    /*
     * Maybe we can get this value from a constant later
     */
    boost::shared_ptr<PrefixActiveNodeSet> activeNodeSets =
            this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy);
    queryEvaluatorIntrnal->findKMostPopularSuggestionsSorted(term ,
            activeNodeSets.get() ,
            numberOfSuggestionsToFind , suggestionPairs);


    // save shared pointer to inverted list read views for performance improvement
    for(std::vector<SuggestionInfo >::iterator suggestionInfoItr = suggestionPairs.begin();
			suggestionInfoItr != suggestionPairs.end(); ++suggestionInfoItr){
        shared_ptr<vectorview<unsigned> > invertedListReadView;
        queryEvaluatorIntrnal->getInvertedIndex()->
                    getInvertedListReadView(invertedListDirectoryReadView,
                            suggestionInfoItr->suggestedCompleteTermNode->getInvertedListOffset(), invertedListReadView);
        //Empty inverted lists should not be included in the lists of lowest level operators.
        if(invertedListReadView->size() == 0){
        	continue;
        }
        suggestionPairsInvertedListReadViews.push_back(invertedListReadView);

    }

    initializeHeap(term, params.ranker, params.prefixMatchPenalty);

    return true;
}
PhysicalPlanRecordItem * UnionLowestLevelSuggestionOperator::getNext(const PhysicalPlanExecutionParameters & params) {

    Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(params.isFuzzy);
	// get the next item from heap
	UnionLowestLevelSuggestionOperator::SuggestionCursorHeapItem nextItem;
	if(getNextHeapItem(term, params.ranker, params.prefixMatchPenalty, nextItem) == false){
		// heap is empty so there is no more results to return
		return NULL;
	}
    // return the item.
    PhysicalPlanRecordItem * newItem = this->queryEvaluatorIntrnal->getPhysicalPlanRecordItemPool()->createRecordItem();
    // record id
    newItem->setRecordId(nextItem.recordId);
    // edit distance
    vector<unsigned> editDistances;
    editDistances.push_back(suggestionPairs[nextItem.suggestionIndex].distance);
    newItem->setRecordMatchEditDistances(editDistances);
    // matching prefix
    vector<TrieNodePointer> matchingPrefixes;
    matchingPrefixes.push_back(suggestionPairs[nextItem.suggestionIndex].queryTermNode);
    newItem->setRecordMatchingPrefixes(matchingPrefixes);
    // runtime score
    bool isPrefixMatch = true;
    // runtime score is calculated before and used in heap
    newItem->setRecordRuntimeScore(nextItem.score);
    // static score
    newItem->setRecordStaticScore(nextItem.termRecordStaticScore);
    // attributeBitmap
    vector<vector<unsigned> > matchedAttributeIdsList;
    matchedAttributeIdsList.push_back(nextItem.attributeIdsList);
    newItem->setRecordMatchAttributeBitmaps(matchedAttributeIdsList);
    newItem->addTermType(term->getTermType());
    return newItem;

}
bool UnionLowestLevelSuggestionOperator::close(PhysicalPlanExecutionParameters & params){
    suggestionPairs.clear();
    suggestionPairsInvertedListReadViews.clear();
    recordItemsHeap.clear();
    return true;
}

void UnionLowestLevelSuggestionOperator::initializeHeap(Term * term, Ranker * ranker, float prefixMatchPenalty){

	// move on suggestions and for each one find the first valid record and put it in heap
	for(unsigned suggestionIndex = 0 ; suggestionIndex < suggestionPairsInvertedListReadViews.size() ; ++suggestionIndex){

		unsigned firstInvertedListCursotToAdd = 0;
		vector<unsigned> matchedAttributeIdsList;
		float termRecordStaticScore = 0;
		while(true){
			// inverted list of this suggestion is completely invalid so we don't put anything from this
			// suggestion in heap
			if(suggestionPairsInvertedListReadViews.at(suggestionIndex)->size() <= firstInvertedListCursotToAdd){
				break;
			}
			unsigned recordId = suggestionPairsInvertedListReadViews.at(suggestionIndex)->getElement(firstInvertedListCursotToAdd);
			unsigned keywordOffset = queryEvaluatorIntrnal->getInvertedIndex()->getKeywordOffset(
					this->forwardIndexDirectoryReadView,
					this->invertedIndexKeywordIdsReadView,
					recordId, suggestionPairs[suggestionIndex].suggestedCompleteTermNode->getInvertedListOffset());
			// We check the record only if it's valid
			vector<unsigned> filterAttributes;
			if (keywordOffset != FORWARDLIST_NOTVALID &&
				queryEvaluatorIntrnal->getInvertedIndex()->isValidTermPositionHit(forwardIndexDirectoryReadView,
					recordId,
					keywordOffset,
					filterAttributes, ATTRIBUTES_OP_OR, matchedAttributeIdsList, termRecordStaticScore)) { // 0x7fffffff means OR on all attributes

				// calculate the runtime score of this record
				float score = ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
                        suggestionPairs[suggestionIndex].distance,
                        term->getKeyword()->size(),
                        true,
                        prefixMatchPenalty , term->getSimilarityBoost())*term->getBoost();
				// put the item in heap
				recordItemsHeap.push_back(SuggestionCursorHeapItem(suggestionIndex, firstInvertedListCursotToAdd,
						recordId, score, matchedAttributeIdsList, termRecordStaticScore ));
				break;
			}else{
				firstInvertedListCursotToAdd++;
			}
		}
	}

	// heapify
	std::make_heap (recordItemsHeap.begin(),recordItemsHeap.end(),SuggestionCursorHeapItem());
}

bool UnionLowestLevelSuggestionOperator::getNextHeapItem(Term * term, Ranker * ranker, float prefixMatchPenalty,
		UnionLowestLevelSuggestionOperator::SuggestionCursorHeapItem & item){
	// if heap is empty return false so that getNext returns NULL
	if(recordItemsHeap.size() == 0){
		return false;
	}

	// pop the top element of heap
	item = recordItemsHeap.front();
	std::pop_heap (recordItemsHeap.begin(),recordItemsHeap.end(),SuggestionCursorHeapItem()); recordItemsHeap.pop_back();


	// iterate on the same inverted list and push another record item to
	// heap (starting from the next position on inverted list)
	unsigned firstInvertedListCursotToAdd = item.invertedListCursor+1;
	vector<unsigned> matchedAttributeIdsList;
	float termRecordStaticScore = 0;
	while(true){
		if(suggestionPairsInvertedListReadViews.at(item.suggestionIndex)->size() <= firstInvertedListCursotToAdd){
			break;
		}
		unsigned recordId = suggestionPairsInvertedListReadViews.at(item.suggestionIndex)->getElement(firstInvertedListCursotToAdd);
		unsigned keywordOffset = queryEvaluatorIntrnal->getInvertedIndex()->getKeywordOffset(
				this->forwardIndexDirectoryReadView,
				this->invertedIndexKeywordIdsReadView,
				recordId, suggestionPairs[item.suggestionIndex].suggestedCompleteTermNode->getInvertedListOffset());
		// We check the record only if it's valid
		vector<unsigned> attributeFilter;
		if (keywordOffset != FORWARDLIST_NOTVALID &&
			queryEvaluatorIntrnal->getInvertedIndex()->isValidTermPositionHit(forwardIndexDirectoryReadView,
				recordId,
				keywordOffset,
				attributeFilter, ATTRIBUTES_OP_OR, matchedAttributeIdsList, termRecordStaticScore)) { // 0x7fffffff means OR on all attributes

			float score = ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
					suggestionPairs[item.suggestionIndex].distance,
					term->getKeyword()->size(),
					true,
					prefixMatchPenalty , term->getSimilarityBoost())*term->getBoost();
			recordItemsHeap.push_back(SuggestionCursorHeapItem(item.suggestionIndex, firstInvertedListCursotToAdd,
					recordId, score, matchedAttributeIdsList, termRecordStaticScore ));
			std::push_heap(recordItemsHeap.begin(), recordItemsHeap.end(),SuggestionCursorHeapItem());
			return true;
		}else{
			firstInvertedListCursotToAdd++;
		}
	}
	// if we reach here, it means that inverted list didn't have any more records
	return true;


}

string UnionLowestLevelSuggestionOperator::toString(){
    string result = "UnionLowestLevelSuggestionOperator" ;
    if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
        result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
    }
    return result;
}

bool UnionLowestLevelSuggestionOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
    ASSERT(false); // this operator should be used only when there is a single keyword in the query
    return true;
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost UnionLowestLevelSuggestionOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
    PhysicalPlanCost resultCost;
    // costing is not needed for this operator.
    return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost UnionLowestLevelSuggestionOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
    PhysicalPlanCost resultCost;
    // costing is not needed for this operator.
    return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost UnionLowestLevelSuggestionOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
    PhysicalPlanCost resultCost;
    // costing is not needed for this operator.
    return resultCost;
}
PhysicalPlanCost UnionLowestLevelSuggestionOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
    return PhysicalPlanCost();// costing is not needed for this operator.
}
void UnionLowestLevelSuggestionOptimizationOperator::getOutputProperties(IteratorProperties & prop){
    prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void UnionLowestLevelSuggestionOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
    // the only requirement for input is to be directly connected to inverted index,
    // so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
    prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelSuggestionOptimizationOperator::getType() {
    return PhysicalPlanNode_UnionLowestLevelSuggestion;
}
bool UnionLowestLevelSuggestionOptimizationOperator::validateChildren(){
    return true;
}

}
}
