
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
	unsigned numberOfSuggestionsToFind = 10;
	/*
	 * Maybe we can get this value from a constant later
	 */
	boost::shared_ptr<PrefixActiveNodeSet> activeNodeSets =
			this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy);
    queryEvaluatorIntrnal->findKMostPopularSuggestionsSorted(term ,
    		activeNodeSets.get() ,
    		numberOfSuggestionsToFind , suggestionPairs);

    suggestionPairCursor = invertedListCursor = 0;

    return true;
}
PhysicalPlanRecordItem * UnionLowestLevelSuggestionOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(params.isFuzzy);
	shared_ptr<vectorview<unsigned> > invertedListReadView;
	queryEvaluatorIntrnal->getInvertedIndex()->
				getInvertedListReadView(invertedListDirectoryReadView,
						suggestionPairs[suggestionPairCursor].suggestedCompleteTermNode->getInvertedListOffset(), invertedListReadView);
	unsigned termAttributeBitmap = 0;
	float termRecordStaticScore = 0;
	// move on inverted list and add the records which are valid
	while(true){
		if(invertedListCursor < invertedListReadView->size()){
			unsigned recordId = invertedListReadView->getElement(invertedListCursor++);
			unsigned recordOffset = queryEvaluatorIntrnal->getInvertedIndex()->getKeywordOffset(
					this->forwardIndexDirectoryReadView,
					this->invertedIndexKeywordIdsReadView,
					recordId, suggestionPairs[suggestionPairCursor].suggestedCompleteTermNode->getInvertedListOffset());
			if (queryEvaluatorIntrnal->getInvertedIndex()->isValidTermPositionHit(forwardIndexDirectoryReadView,
					recordId,
					recordOffset,
					0x7fffffff,  termAttributeBitmap, termRecordStaticScore)) { // 0x7fffffff means OR on all attributes
				// return the item.
				PhysicalPlanRecordItem * newItem = this->queryEvaluatorIntrnal->getPhysicalPlanRecordItemPool()->createRecordItem();
				// record id
				newItem->setRecordId(recordId);
				// edit distance
				vector<unsigned> editDistances;
				editDistances.push_back(suggestionPairs[suggestionPairCursor].distance);
				newItem->setRecordMatchEditDistances(editDistances);
				// matching prefix
				vector<TrieNodePointer> matchingPrefixes;
				matchingPrefixes.push_back(suggestionPairs[suggestionPairCursor].queryTermNode);
				newItem->setRecordMatchingPrefixes(matchingPrefixes);
				// runtime score
				bool isPrefixMatch = true;
				newItem->setRecordRuntimeScore(	params.ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
						suggestionPairs[suggestionPairCursor].distance,
						term->getKeyword()->size(),
						isPrefixMatch,
						params.prefixMatchPenalty , term->getSimilarityBoost())/*added by Jamshid*/*term->getBoost());
				// static score
				newItem->setRecordStaticScore(termRecordStaticScore);
				// attributeBitmap
				vector<unsigned> attributeBitmaps;
				attributeBitmaps.push_back(termAttributeBitmap);
				newItem->setRecordMatchAttributeBitmaps(attributeBitmaps);


				return newItem;
			}
		}else{
			suggestionPairCursor ++;
			if(suggestionPairCursor >= suggestionPairs.size()){
				return NULL;
			}
			queryEvaluatorIntrnal->getInvertedIndex()->
						getInvertedListReadView(invertedListDirectoryReadView,
								suggestionPairs[suggestionPairCursor].suggestedCompleteTermNode->getInvertedListOffset(), invertedListReadView);
			invertedListCursor = 0;
		}
	}

	return NULL;// we never reach here
}
bool UnionLowestLevelSuggestionOperator::close(PhysicalPlanExecutionParameters & params){
	suggestionPairs.clear();
	suggestionPairCursor = invertedListCursor = 0;
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
	// no output property
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
