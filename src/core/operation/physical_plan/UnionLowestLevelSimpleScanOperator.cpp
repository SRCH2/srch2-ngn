
#include "UnionLowestLevelSimpleScanOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

#ifdef ANDROID
   double inline log2(double x) { return log(x) / log (2);  }
#endif

UnionLowestLevelSimpleScanOperator::UnionLowestLevelSimpleScanOperator() {
    queryEvaluator = NULL;
    parentIsCacheEnabled = false;
}

UnionLowestLevelSimpleScanOperator::~UnionLowestLevelSimpleScanOperator(){
}
bool UnionLowestLevelSimpleScanOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
    // first save the pointer to QueryEvaluator
    this->queryEvaluator = queryEvaluator;
    this->queryEvaluator->getInvertedIndex()->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
    this->queryEvaluator->getInvertedIndex()->getInvertedIndexKeywordIds_ReadView(invertedIndexKeywordIdsReadView);
    this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);

    // 1. get the pointer to logical plan node
    LogicalPlanNode * logicalPlanNode = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode();
    // 2. Get the Term object
    Term * term = logicalPlanNode->getTerm(params.isFuzzy);
    // 3. Get the ActiveNodeSet from the logical plan
    boost::shared_ptr<PrefixActiveNodeSet> activeNodeSet = logicalPlanNode->stats->getActiveNodeSetForEstimation(params.isFuzzy);


    // 4. Create the iterator and save it as a member of the class for future calls to getNext
    if (term->getTermType() == TERM_TYPE_PREFIX) { // prefix term
        for (LeafNodeSetIteratorForPrefix iter (activeNodeSet.get(), term->getThreshold()); !iter.isDone(); iter.next()) {
            TrieNodePointer leafNode;
            TrieNodePointer prefixNode;
            unsigned distance;
            iter.getItem(prefixNode, leafNode, distance);
            // get inverted list pointer and save it
            shared_ptr<vectorview<unsigned> > invertedListReadView;
            this->queryEvaluator->getInvertedIndex()->
                    getInvertedListReadView(invertedListDirectoryReadView,
                            leafNode->getInvertedListOffset() , invertedListReadView);
            if(invertedListReadView->size() == 0){
            	continue;
            }
            this->invertedListsSharedPointers.push_back(invertedListReadView);
            this->invertedLists.push_back(invertedListReadView.get());
            this->invertedListPrefixes.push_back(prefixNode);
            this->invertedListLeafNodes.push_back(leafNode);
            this->invertedListDistances.push_back(distance);
            this->invertedListIDs.push_back(leafNode->getInvertedListOffset());
        }
    }else{ // complete term

		LeafNodeSetIteratorForComplete iter(activeNodeSet.get() , term->getThreshold());
		for(; !iter.isDone(); iter.next()){
			TrieNodePointer trieNode;
			unsigned editDistance;
			iter.getItem(trieNode, editDistance);
	        // get inverted list pointer and save it
	        shared_ptr<vectorview<unsigned> > invertedListReadView;
	        this->queryEvaluator->getInvertedIndex()->
	                getInvertedListReadView(invertedListDirectoryReadView,
	                        trieNode->getInvertedListOffset() , invertedListReadView);
            if(invertedListReadView->size() == 0){
            	continue;
            }
	        this->invertedListsSharedPointers.push_back(invertedListReadView);
	        this->invertedLists.push_back(invertedListReadView.get());
	        this->invertedListPrefixes.push_back(trieNode);
	        this->invertedListLeafNodes.push_back(trieNode);
	        this->invertedListDistances.push_back(editDistance);
	        this->invertedListIDs.push_back(trieNode->getInvertedListOffset() );
		}
    }

    // check cache
    if(params.parentIsCacheEnabled == true || params.cacheObject == NULL){
        // either parent is not passing cache hit info or
        // there was no cache hit
        this->invertedListOffset = 0;
        this->cursorOnInvertedList = 0;
    }else if(params.cacheObject != NULL){
        UnionLowestLevelSimpleScanCacheEntry * cacheEntry =
                (UnionLowestLevelSimpleScanCacheEntry *)params.cacheObject;
        this->invertedListOffset = cacheEntry->invertedListOffset;
        this->cursorOnInvertedList = cacheEntry->cursorOnInvertedList;
    }

    /*
     * Printing number of leaf nodes and total sum of inverted lists
     */
    //cout << "\tshortestinfo(" << invertedListLeafNodes.size() << "$";
    //unsigned totalSizeOfInvertedLists = 0;
    //for(unsigned ii = 0; ii < invertedLists.size() ; ++ii){
    //	totalSizeOfInvertedLists += invertedLists.at(ii)->size();
    //}
    //cout << totalSizeOfInvertedLists << ")\t";

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
    Term * term = logicalPlanNode->getTerm(params.isFuzzy);

    // find the next record and check the its validity
    unsigned recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);

    unsigned keywordOffset =
            this->queryEvaluator->getInvertedIndex()->getKeywordOffset(this->forwardIndexDirectoryReadView,
                    this->invertedIndexKeywordIdsReadView,
                    recordID, this->invertedListIDs.at(this->invertedListOffset));

    bool foundValidHit = 0;
    float termRecordStaticScore = 0;
    vector<unsigned> matchedAttributeIdsList;
    while (1) {
        // We check the record only if it's valid
        if (keywordOffset != FORWARDLIST_NOTVALID &&
            this->queryEvaluator->getInvertedIndex()->isValidTermPositionHit(forwardIndexDirectoryReadView,
                recordID,
                keywordOffset,
                term->getAttributesToFilter(),term->getFilterAttrOperation(), matchedAttributeIdsList,
                termRecordStaticScore) ) {
        	foundValidHit = 1;
        	break;
        }
        this->cursorOnInvertedList ++;
        if (this->cursorOnInvertedList < this->invertedLists.at(this->invertedListOffset)->size()) {
            recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);
            // calculate record offset online
            keywordOffset =
                        this->queryEvaluator->getInvertedIndex()->getKeywordOffset(this->forwardIndexDirectoryReadView,
                                this->invertedIndexKeywordIdsReadView,
                                recordID, this->invertedListIDs.at(this->invertedListOffset));
        } else {
            this->invertedListOffset ++;
            this->cursorOnInvertedList = 0;
            if(this->invertedListOffset < this->invertedLists.size()){
                recordID = this->invertedLists.at(this->invertedListOffset)->at(this->cursorOnInvertedList);
                // calculate record offset online
                keywordOffset =
                            this->queryEvaluator->getInvertedIndex()->getKeywordOffset(this->forwardIndexDirectoryReadView,
                                    this->invertedIndexKeywordIdsReadView,
                                    recordID, this->invertedListIDs.at(this->invertedListOffset));
            }else{
                return NULL;
            }
        }
    }

    if(foundValidHit == 0){
        return NULL;
    }

    // return the item.
    PhysicalPlanRecordItem * newItem = this->queryEvaluator->getPhysicalPlanRecordItemPool()->createRecordItem();
    // record id
    newItem->setRecordId(recordID);
    // edit distance
    vector<unsigned> editDistances;
    editDistances.push_back(this->invertedListDistances.at(this->invertedListOffset));
    newItem->setRecordMatchEditDistances(editDistances);
    // matching prefix
    vector<TrieNodePointer> matchingPrefixes;
    matchingPrefixes.push_back(this->invertedListPrefixes.at(this->invertedListOffset));
    newItem->setRecordMatchingPrefixes(matchingPrefixes);
    // runtime score
    bool isPrefixMatch = this->invertedListPrefixes.at(this->invertedListOffset) !=
            this->invertedListLeafNodes.at(this->invertedListOffset);
    newItem->setRecordRuntimeScore(    params.ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
            this->invertedListDistances.at(this->invertedListOffset),
            term->getKeyword()->size(),
            isPrefixMatch,
            params.prefixMatchPenalty , term->getSimilarityBoost())/*added by Jamshid*/*term->getBoost());
    // static score
    newItem->setRecordStaticScore(termRecordStaticScore);
    // attributeBitmap
    vector<vector<unsigned> > matchedAttribureIdsList;
    matchedAttribureIdsList.push_back(matchedAttributeIdsList);
    newItem->setRecordMatchAttributeBitmaps(matchedAttribureIdsList);

    newItem->addTermType(term->getTermType());


    // prepare for next call
    this->cursorOnInvertedList ++;
    if(this->cursorOnInvertedList >= this->invertedLists.at(this->invertedListOffset)->size()){
        this->invertedListOffset ++;
        this->cursorOnInvertedList = 0;
    }


    return newItem;
}
bool UnionLowestLevelSimpleScanOperator::close(PhysicalPlanExecutionParameters & params){

    // set cache object
    UnionLowestLevelSimpleScanCacheEntry * cacheEntry =
            new UnionLowestLevelSimpleScanCacheEntry(this->invertedListOffset , this->cursorOnInvertedList);
    params.cacheObject = cacheEntry;

    this->invertedListsSharedPointers.clear();
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

string UnionLowestLevelSimpleScanOperator::toString(){
    string result = "UnionLowestLevelSimpleScanOperator" ;
    if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
        result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
    }
    return result;
}

bool UnionLowestLevelSimpleScanOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
      //do the verification
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet =
            this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(parameters.isFuzzy);

    Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(parameters.isFuzzy);

    return verifyByRandomAccessHelper(this->queryEvaluator, prefixActiveNodeSet.get(), term, parameters);
}



/*
 * This function continues traversing the trie from the prefix trie node
 * to reach to all leaf nodes which are within the allowed edit-distance.
 * So if the term type is prefix, and the match is exact, still we can have multiple leaf nodes.
 */
void UnionLowestLevelSimpleScanOperator::depthInitializeSimpleScanOperator(
        const TrieNode* trieNode, const TrieNode* prefixNode, unsigned editDistance, unsigned panDistance,  unsigned bound){
    if (trieNode->isTerminalNode()){

    }
    if (panDistance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeSimpleScanOperator(child, trieNode, editDistance, panDistance + 1, bound);
        }
    }
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
    // cost of going over leaf nodes and making the heap
    unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
    PhysicalPlanCost resultCost;
    resultCost.cost = estimatedNumberOfTerminalNodes;
    return resultCost ; // cost of going over leaf nodes.
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {

    PhysicalPlanCost resultCost;
    resultCost.cost = 1;
    return resultCost; // cost of sequential access
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
    PhysicalPlanCost resultCost;
    resultCost.cost = 1;
    return resultCost;
}
PhysicalPlanCost UnionLowestLevelSimpleScanOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
    unsigned estimatedNumberOfActiveNodes =
            this->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy)->getNumberOfActiveNodes();
    PhysicalPlanCost resultCost;
    resultCost.cost = estimatedNumberOfActiveNodes * log2(85.0);
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
