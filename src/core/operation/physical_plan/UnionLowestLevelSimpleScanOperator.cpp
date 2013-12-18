
#include "UnionLowestLevelSimpleScanOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelSimpleScanOperator::UnionLowestLevelSimpleScanOperator() {
	queryEvaluator = NULL;
}

UnionLowestLevelSimpleScanOperator::~UnionLowestLevelSimpleScanOperator(){
}
bool UnionLowestLevelSimpleScanOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// first save the pointer to QueryEvaluator
	this->queryEvaluator = queryEvaluator;

	// 1. get the pointer to logical plan node
	LogicalPlanNode * logicalPlanNode = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode();
	// 2. Get the Term object
	Term * term = NULL;
	if(params.isFuzzy){
		term = logicalPlanNode->fuzzyTerm;
	}else{
		term = logicalPlanNode->exactTerm;
	}
	// 3. Get the ActiveNodeSet from the logical plan
	PrefixActiveNodeSet * activeNodeSet = logicalPlanNode->stats->getActiveNodeSetForEstimation(params.isFuzzy);

	// 4. Create the iterator and save it as a member of the class for future calls to getNext
	if (term->getTermType() == TERM_TYPE_PREFIX) { // prefix term
        for (LeafNodeSetIterator iter (activeNodeSet, term->getThreshold()); !iter.isDone(); iter.next()) {
            TrieNodePointer leafNode;
            TrieNodePointer prefixNode;
            unsigned distance;
            iter.getItem(prefixNode, leafNode, distance);
            // get inverted list pointer and save it
            shared_ptr<vectorview<unsigned> > invertedListReadView;
            this->queryEvaluator->getInvertedIndex()->
            		getInvertedListReadView(leafNode->getInvertedListOffset() , invertedListReadView);
            this->invertedLists.push_back(invertedListReadView);
            this->invertedListPrefixes.push_back(prefixNode);
            this->invertedListLeafNodes.push_back(leafNode);
            this->invertedListDistances.push_back(distance);
            this->invertedListIDs.push_back(leafNode->getInvertedListOffset());
        }
	}else{ // complete term
        for (ActiveNodeSetIterator iter(activeNodeSet, term->getThreshold()); !iter.isDone(); iter.next()) {
            TrieNodePointer trieNode;
            unsigned distance;
            iter.getItem(trieNode, distance);
            distance = activeNodeSet->getEditdistanceofPrefix(trieNode);
            depthInitializeSimpleScanOperator(trieNode, trieNode, distance, term->getThreshold());
        }
	}
	this->invertedListOffset = 0;
	this->cursorOnInvertedList = 0;

	return true;

}
PhysicalPlanRecordItem * UnionLowestLevelSimpleScanOperator::getNext(const PhysicalPlanExecutionParameters & params) {

	if(this->invertedListOffset >= this->invertedLists.size()){
		return NULL;
	}
	// we dont have any list with size zero
	ASSERT(this->cursorOnInvertedList < this->invertedLists.at(this->invertedListOffset)->size());

	// 1. get the pointer to logical plan node
	LogicalPlanNode * logicalPlanNode = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode();
	// 2. Get the Term object
	Term * term = NULL;
	if(params.isFuzzy){
		term = logicalPlanNode->fuzzyTerm;
	}else{
		term = logicalPlanNode->exactTerm;
	}

	// find the next record and check the its validity
	unsigned recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);

	unsigned recordOffset =
			this->queryEvaluator->getInvertedIndex()->getKeywordOffset(recordID, this->invertedListIDs.at(this->invertedListOffset));

    bool foundValidHit = 0;
    float termRecordStaticScore = 0;
    unsigned termAttributeBitmap = 0;
    while (1) {
        if (this->queryEvaluator->getInvertedIndex()->isValidTermPositionHit(recordID, recordOffset,
                term->getAttributeToFilterTermHits(), termAttributeBitmap,
                termRecordStaticScore) ) {
            foundValidHit = 1;
            break;
        }
        this->cursorOnInvertedList ++;
        if (this->cursorOnInvertedList < this->invertedLists.at(this->invertedListOffset)->size()) {
        	recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);
            // calculate record offset online
        	recordOffset =
        				this->queryEvaluator->getInvertedIndex()->getKeywordOffset(recordID, this->invertedListIDs.at(this->invertedListOffset));
        } else {
        	this->invertedListOffset ++;
			this->cursorOnInvertedList = 0;
        	if(this->invertedListOffset < this->invertedLists.size()){
        		recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);
				// calculate record offset online
				recordOffset =
							this->queryEvaluator->getInvertedIndex()->getKeywordOffset(recordID, this->invertedListIDs.at(this->invertedListOffset));
        	}else{
        		return NULL;
        	}
        }
    }

    if(foundValidHit == 0){
    	return NULL;
    }

	// return the item.
	PhysicalPlanRecordItem * newItem = this->queryEvaluator->getPhysicalPlanRecordItemFactory()->createRecordItem();
	// record id
	newItem->setRecordId(recordID);
	// edit distance
	vector<unsigned> editDistances;
	editDistances.push_back(this->invertedListDistances.at(this->invertedListOffset));
	newItem->setRecordMatchEditDistances(editDistances);
	// matching prefix
	vector<TrieNodePointer> matchingPrefixes;
	matchingPrefixes.push_back(this->invertedListLeafNodes.at(this->invertedListOffset)); // TODO this might be wrong
	newItem->setRecordMatchingPrefixes(matchingPrefixes);
	// runtime score
	bool isPrefixMatch = this->invertedListPrefixes.at(this->invertedListOffset) !=
			this->invertedListLeafNodes.at(this->invertedListOffset);
	newItem->setRecordRuntimeScore(	params.ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
            this->invertedListDistances.at(this->invertedListOffset),
            term->getKeyword()->size(),
            isPrefixMatch,
            params.prefixMatchPenalty , term->getSimilarityBoost())/*added by Jamshid*/*term->getBoost());
	// static score
	newItem->setRecordStaticScore(termRecordStaticScore);
	// attributeBitmap
	vector<unsigned> attributeBitmaps;
	attributeBitmaps.push_back(termAttributeBitmap);
	newItem->setRecordMatchAttributeBitmaps(attributeBitmaps);




	// prepare for next call
	this->cursorOnInvertedList ++;
	if(this->cursorOnInvertedList >= this->invertedLists.at(this->invertedListOffset)->size()){
		this->invertedListOffset ++;
		this->cursorOnInvertedList = 0;
	}


	return newItem;
}
bool UnionLowestLevelSimpleScanOperator::close(PhysicalPlanExecutionParameters & params){

	this->invertedLists.clear();
	this->invertedListDistances.clear();
	this->invertedListLeafNodes.clear();
	this->invertedListPrefixes.clear();
	this->invertedListIDs.clear();
	this->cursorOnInvertedList = 0;
	this->invertedListOffset = 0;
	queryEvaluator = NULL;

	return true;
}
bool UnionLowestLevelSimpleScanOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	  //do the verification
	PrefixActiveNodeSet *prefixActiveNodeSet =
			this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(parameters.isFuzzy);

	Term * term = NULL;
	if(parameters.isFuzzy){
		term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->fuzzyTerm;
	}else{
		term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->exactTerm;
	}

	return verifyByRandomAccessHelper(this->queryEvaluator, prefixActiveNodeSet, term, parameters);
}


void UnionLowestLevelSimpleScanOperator::depthInitializeSimpleScanOperator(
		const TrieNode* trieNode, const TrieNode* prefixNode, unsigned distance, unsigned bound){
    if (trieNode->isTerminalNode()){
        // get inverted list pointer and save it
        shared_ptr<vectorview<unsigned> > invertedListReadView;
        this->queryEvaluator->getInvertedIndex()->
        		getInvertedListReadView(trieNode->getInvertedListOffset() , invertedListReadView);
        this->invertedLists.push_back(invertedListReadView);
        this->invertedListPrefixes.push_back(prefixNode);
        this->invertedListLeafNodes.push_back(trieNode);
        this->invertedListDistances.push_back(distance);
        this->invertedListIDs.push_back(trieNode->getInvertedListOffset() );
    }
    if (distance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeSimpleScanOperator(child, trieNode, distance+1, bound);
        }
    }
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(3);
	resultCost.addInstructionCost(3 + 3 * estimatedNumberOfTerminalNodes);
	resultCost.addSmallFunctionCost(estimatedNumberOfTerminalNodes);
	return resultCost ; // cost of going over leaf nodes.
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {

	PhysicalPlanCost resultCost;
	resultCost.addLargeFunctionCost();
	return resultCost; // cost of sequential access
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	resultCost.addSmallFunctionCost();
	return resultCost;
}
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.addFunctionCallCost(5);
	resultCost.addMediumFunctionCost(estimatedNumberOfTerminalNodes);
	return resultCost;
}
void UnionLowestLevelSimpleScanOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// no output property
}
void UnionLowestLevelSimpleScanOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// the only requirement for input is to be directly connected to inverted index,
	// so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelSimpleScanOptimizationOperator::getType() {
	return PhysicalPlanNode_UnionLowestLevelSimpleScanOperator;
}
bool UnionLowestLevelSimpleScanOptimizationOperator::validateChildren(){
	if(getChildrenCount() > 0){ // this operator cannot have any children
		return false;
	}
	return true;
}

}
}
