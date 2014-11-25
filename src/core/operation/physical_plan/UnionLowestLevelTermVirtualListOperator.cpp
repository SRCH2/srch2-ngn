
#include "UnionLowestLevelTermVirtualListOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "PhysicalOperatorsHelper.h"
#include "cmath"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////
#ifdef ANDROID
   double inline log2(double x) { return log(x) / log (2);  }
#endif

UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperator() {
    this->parentIsCacheEnabled = false;
}

UnionLowestLevelTermVirtualListOperator::~UnionLowestLevelTermVirtualListOperator(){
	/*
	 *   heapItem vector is deleted in current operator's close function.
	 */
}
bool UnionLowestLevelTermVirtualListOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

	// first save the pointer to QueryEvaluator
	this->queryEvaluator = queryEvaluator;
	// 1. get the pointer to logical plan node
	LogicalPlanNode * logicalPlanNode = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode();
	// 2. Get the Term object
	Term * term = logicalPlanNode->getTerm(params.isFuzzy);

	this->invertedIndex = queryEvaluator->getInvertedIndex();
    this->invertedIndex->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
    this->invertedIndex->getInvertedIndexKeywordIds_ReadView(invertedIndexKeywordIdsReadView);
    this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);
    this->prefixActiveNodeSet = logicalPlanNode->stats->getActiveNodeSetForEstimation(params.isFuzzy);
    this->term = term;
    this->prefixMatchPenalty = params.prefixMatchPenalty;
    this->numberOfItemsInPartialHeap = 0;
    this->currentMaxEditDistanceOnHeap = 0;
    this->currentRecordID = -1;
	if (this->getTermType() == TERM_TYPE_PREFIX) { //case 1: Term is prefix
		LeafNodeSetIteratorForPrefix iter(prefixActiveNodeSet.get(), term->getThreshold());
		cursorVector.reserve(iter.size());
		invertedListReadViewVector.reserve(iter.size());
		for (; !iter.isDone(); iter.next()) {
			TrieNodePointer leafNode;
			TrieNodePointer prefixNode;
			unsigned distance;
			iter.getItem(prefixNode, leafNode, distance);
			initialiseTermVirtualListElement(prefixNode, leafNode, distance);
		}
	} else { // case 2: Term is complete
		// For a pivotal active node (PAN) (e.g., "game") and a query term (e.g., "garden"),
		// there are two distances between them. One is their real edit distance between "game" and "garden",
		// which is 3.  The other one is the PAN's internal distance as a prefix, which is 2,
		// corresponding to the distance between "game" and "garde" where the last two matched characters
		// We need both distances in the function call depthInitializeSimpleScanOperator() in order to compute the real edit
		// distance of a descendant of the current trie node. More details of the approach
		// http://www.ics.uci.edu/~chenli/pub/2011-vldbj-fuzzy-search.pdf Section 4.3.1.

		// For example search python36 in data {python16, pythoni}, we first got python16 with distance 1, and then pythoni with distance 2
		// We will keep the smaller one according to  https://bitbucket.org/srch2inc/srch2-ngn/src/2b4293ccaccaaecd9c16526bd5c6bbfd02dded52/src/core/operation/ActiveNode.h?at=master#cl-457
		LeafNodeSetIteratorForComplete iter(prefixActiveNodeSet.get() , term->getThreshold());
		cursorVector.reserve(iter.size());
		invertedListReadViewVector.reserve(iter.size());
		for(; !iter.isDone(); iter.next()){
			TrieNodePointer trieNode;
			unsigned editDistance;
			iter.getItem(trieNode, editDistance);
			initialiseTermVirtualListElement(NULL, trieNode, editDistance);
		}
	}
    // check cache
    parentIsCacheEnabled = params.parentIsCacheEnabled;
    if(params.parentIsCacheEnabled == false || params.cacheObject == NULL){
    	// parent is not feeding us with cache info and does not expect cache entry
    	// or there was no cache hit
		// Make partial heap by calling make_heap from begin() to begin()+"number of items within edit distance threshold"
        make_heap(itemsHeap.begin(), itemsHeap.begin()+numberOfItemsInPartialHeap, UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperatorHeapItemCmp());
    }else if(params.cacheObject != NULL){
    	// parent is feeding us with cache hit info and does expect newer cache entry.
    	UnionLowestLevelTermVirtualListCacheEntry * cacheEntry =
    			(UnionLowestLevelTermVirtualListCacheEntry *)params.cacheObject;
    	/*
    	 *   Free the memory allocated before because we will get heapItems from the cache.
    	 */
    	for (unsigned i = 0; i < this->itemsHeap.size(); ++i) {
    			delete this->itemsHeap[i];
    	}
    	itemsHeap.clear();
    	cursorVector.clear();
    	for(unsigned i = 0; i < cacheEntry->itemsHeap.size() ; ++i){
    		this->itemsHeap.push_back(
    				new UnionLowestLevelTermVirtualListOperatorHeapItem(*(cacheEntry->itemsHeap.at(i))));
    	}
    	this->cursorVector = cacheEntry->cursorVector;
    	this->currentMaxEditDistanceOnHeap = cacheEntry->currentMaxEditDistanceOnHeap;
    	this->numberOfItemsInPartialHeap = cacheEntry->numberOfItemsInPartialHeap;
    }

    return true;
}
PhysicalPlanRecordItem * UnionLowestLevelTermVirtualListOperator::getNext(const PhysicalPlanExecutionParameters & params) {

    // If partialHeap is empty
    if (this->numberOfItemsInPartialHeap == 0) {
        if (this->_addItemsToPartialHeap() == false)
            return NULL;
    }

    if (this->numberOfItemsInPartialHeap != 0) {
        // Elements are there in PartialHeap and pop them out to calling function
        UnionLowestLevelTermVirtualListOperatorHeapItem *currentHeapMax = *(itemsHeap.begin());
        pop_heap(itemsHeap.begin(), itemsHeap.begin() + this->numberOfItemsInPartialHeap,
                 UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperatorHeapItemCmp());

        // allocate new item and fill it out
        PhysicalPlanRecordItem * newItem = this->queryEvaluator->getPhysicalPlanRecordItemPool()->createRecordItem();
        newItem->setRecordId(currentHeapMax->recordId);
        newItem->setRecordRuntimeScore(currentHeapMax->termRecordRuntimeScore);
        vector<TrieNodePointer> prefixes;
        prefixes.push_back(currentHeapMax->trieNode);
        newItem->setRecordMatchingPrefixes(prefixes);
        vector<vector<unsigned> > matchedAttributeIdsList;
        matchedAttributeIdsList.push_back(currentHeapMax->attributeIdsList);
        newItem->setRecordMatchAttributeBitmaps(matchedAttributeIdsList);
        vector<unsigned> editDistances;
        editDistances.push_back(currentHeapMax->ed);
        newItem->setRecordMatchEditDistances(editDistances);
        vector<unsigned> positionIndexOffsets;
        positionIndexOffsets.push_back(currentHeapMax->positionIndexOffset);
        newItem->setPositionIndexOffsets(positionIndexOffsets);

        newItem->addTermType(term->getTermType());

        unsigned currentHeapMaxCursor = this->cursorVector[currentHeapMax->cursorVectorPosition];
        unsigned currentHeapMaxInvertetedListId = currentHeapMax->invertedListId;
        const shared_ptr<vectorview<unsigned> > &currentHeapMaxInvertedList = this->invertedListReadViewVector[currentHeapMax->cursorVectorPosition];
        unsigned currentHeapMaxInvertedListSize = currentHeapMaxInvertedList->size();

        bool foundValidHit = 0;

        // Check cursor is less than invertedList Size.
        while (currentHeapMaxCursor < currentHeapMaxInvertedListSize) {
            // InvertedList has more elements. Push invertedListElement at cursor into virtualList.

            unsigned recordId = currentHeapMaxInvertedList->getElement(currentHeapMaxCursor);
            // calculate record offset online
            unsigned keywordOffset = this->invertedIndex->getKeywordOffset(
            		this->forwardIndexDirectoryReadView,
            		this->invertedIndexKeywordIdsReadView,
            		recordId, currentHeapMaxInvertetedListId);
            vector<unsigned> matchedAttributeIdsList;
            currentHeapMaxCursor++;

            // check isValidTermPositionHit
            float termRecordStaticScore = 0;

            // We check the record only if it's valid
            if (keywordOffset != FORWARDLIST_NOTVALID &&
                this->invertedIndex->isValidTermPositionHit(forwardIndexDirectoryReadView,
                    recordId,
                    keywordOffset,
                    term->getAttributesToFilter(), term->getFilterAttrOperation(), matchedAttributeIdsList,
                    termRecordStaticScore)) {
                foundValidHit = 1;
                this->cursorVector[currentHeapMax->cursorVectorPosition] = currentHeapMaxCursor;
                // Update cursor of popped virtualList in invertedListCursorVector.
                // Cursor always points to next element in invertedList
                currentHeapMax->recordId = recordId;
                currentHeapMax->termRecordRuntimeScore =
                    params.ranker->computeTermRecordRuntimeScore(termRecordStaticScore,
                            currentHeapMax->ed,
                            term->getKeyword()->size(),
                            currentHeapMax->isPrefixMatch,
                            this->prefixMatchPenalty , term->getSimilarityBoost())/*added by Jamshid*/*term->getBoost();
                currentHeapMax->termRecordStaticScore = termRecordStaticScore;
                currentHeapMax->attributeIdsList = matchedAttributeIdsList;
                currentHeapMax->positionIndexOffset = keywordOffset;
                push_heap(itemsHeap.begin(), itemsHeap.begin()+this->numberOfItemsInPartialHeap,
                          UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperatorHeapItemCmp());
                break;
            }
        }

        if (!foundValidHit) {
            //InvertedList cursor end reached and so decrement number of elements in partialHeap
            this->numberOfItemsInPartialHeap--;
            //Delete the head of heap that represents empty converted list
            delete currentHeapMax;
            //TODO OPT Don't erase, accumulate and delete at the end.
            this->itemsHeap.erase(itemsHeap.begin()+this->numberOfItemsInPartialHeap);
        }

        return newItem;
    } else {
        return NULL;
    }

}
bool UnionLowestLevelTermVirtualListOperator::close(PhysicalPlanExecutionParameters & params){

	// if parent excepts cache must prepare a newer cache entry
	if(parentIsCacheEnabled){
		// set cache object
		UnionLowestLevelTermVirtualListCacheEntry * cacheEntry =
				new UnionLowestLevelTermVirtualListCacheEntry(this->itemsHeap,
						this->numberOfItemsInPartialHeap , this->currentMaxEditDistanceOnHeap , this->cursorVector);
		params.cacheObject = cacheEntry;
	}

    queryEvaluator = NULL;
    for (vector<UnionLowestLevelTermVirtualListOperatorHeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        UnionLowestLevelTermVirtualListOperatorHeapItem *currentItem = *iter;
        if (currentItem != NULL)
            delete currentItem;
    }
    this->itemsHeap.clear();
    this->cursorVector.clear();
    this->invertedListReadViewVector.clear();
    this->term = NULL;
    this->invertedIndex = NULL;
    // We don't delete activenodesets here. Be careful to delete them by PhysicalPlanNode
    return true;
}

string UnionLowestLevelTermVirtualListOperator::toString(){
	string result = "UnionLowestLevelTermVirtualListOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool UnionLowestLevelTermVirtualListOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	  //do the verification
	boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet =
			this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(parameters.isFuzzy);

	Term * term = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->getTerm(parameters.isFuzzy);

	return verifyByRandomAccessHelper(this->queryEvaluator, prefixActiveNodeSet.get(), term, parameters);

}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	// cost of going over leaf nodes and making the heap
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.cost = estimatedNumberOfTerminalNodes;
	return resultCost ; // cost of going over leaf nodes.

}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.cost = log2((double)estimatedNumberOfTerminalNodes + 1);
	return resultCost; // cost of sequential access
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	unsigned estimatedNumberOfTerminalNodes = this->getLogicalPlanNode()->stats->getEstimatedNumberOfLeafNodes();
	PhysicalPlanCost resultCost;
	resultCost.cost = estimatedNumberOfTerminalNodes;
	return resultCost; // cost of deleting heap items
}
PhysicalPlanCost UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	unsigned estimatedNumberOfActiveNodes = 0;

	Term * term = this->getLogicalPlanNode()->getTerm(params.isFuzzy);
	if(term->getTermType() == TERM_TYPE_COMPLETE){
		LeafNodeSetIteratorForComplete iter(this->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy).get()
						, term->getThreshold());
		estimatedNumberOfActiveNodes = iter.size();
	}else{ // prefix
		estimatedNumberOfActiveNodes = this->getLogicalPlanNode()->stats->getActiveNodeSetForEstimation(params.isFuzzy)->getNumberOfActiveNodes();
	}
	PhysicalPlanCost resultCost;
	resultCost.cost = estimatedNumberOfActiveNodes * log2(85.0);
	return resultCost;
}
void UnionLowestLevelTermVirtualListOptimizationOperator::getOutputProperties(IteratorProperties & prop){
    prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}
void UnionLowestLevelTermVirtualListOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
    // the only requirement for input is to be directly connected to inverted index,
    // so since no operator outputs PhysicalPlanIteratorProperty_LowestLevel TVL will be pushed down to lowest level
    prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}
PhysicalPlanNodeType UnionLowestLevelTermVirtualListOptimizationOperator::getType() {
    return PhysicalPlanNode_UnionLowestLevelTermVirtualList;
}
bool UnionLowestLevelTermVirtualListOptimizationOperator::validateChildren(){
    if(getChildrenCount() > 0){ // this operator cannot have any children
        return false;
    }
    return true;
}


void UnionLowestLevelTermVirtualListOperator::initialiseTermVirtualListElement(TrieNodePointer prefixNode,
        TrieNodePointer leafNode, unsigned distance)
{
    unsigned invertedListId = leafNode->getInvertedListOffset();
    unsigned invertedListCounter = 0;

    shared_ptr<vectorview<unsigned> > invertedListReadView;
    this->invertedIndex->getInvertedListReadView(invertedListDirectoryReadView,
    		invertedListId, invertedListReadView);
    //Empty inverted lists should not be included in the lists of lowest level operators.
    if(invertedListReadView->size() == 0){
    	return;
    }
    unsigned recordId = invertedListReadView->getElement(invertedListCounter);
    // calculate record offset online
    unsigned keywordOffset = this->invertedIndex->getKeywordOffset(this->forwardIndexDirectoryReadView,
    		this->invertedIndexKeywordIdsReadView, recordId, invertedListId);
    ++ invertedListCounter;

    bool foundValidHit = 0;
    float termRecordStaticScore = 0;
    vector<unsigned> termAttributeBitmap;
    while (1) {
        // We check the record only if it's valid
        if (keywordOffset != FORWARDLIST_NOTVALID &&
            this->invertedIndex->isValidTermPositionHit(forwardIndexDirectoryReadView,
                recordId,
                keywordOffset,
                term->getAttributesToFilter(), term->getFilterAttrOperation(), termAttributeBitmap,
                termRecordStaticScore) ) {
            foundValidHit = 1;
            break;
        }

        if (invertedListCounter < invertedListReadView->size()) {
            recordId = invertedListReadView->getElement(invertedListCounter);
            // calculate record offset online
            keywordOffset = this->invertedIndex->getKeywordOffset(this->forwardIndexDirectoryReadView,
            		this->invertedIndexKeywordIdsReadView, recordId, invertedListId);
            ++invertedListCounter;
        } else {
            break;
        }
    }

    if (keywordOffset != FORWARDLIST_NOTVALID && foundValidHit == 1) {
        this->numberOfItemsInPartialHeap ++; // increment partialHeap counter

        if (this->numberOfItemsInPartialHeap == 0)
            this->currentMaxEditDistanceOnHeap = distance;


        if (this->getTermType() == srch2::instantsearch::TERM_TYPE_PREFIX) { // prefix term
            bool isPrefixMatch = (prefixNode != leafNode);
            float termRecordRuntimeScore =
                DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                        distance,
                        term->getKeyword()->size(),
                        isPrefixMatch,
                        this->prefixMatchPenalty , term->getSimilarityBoost()) * term->getBoost();
            this->itemsHeap.push_back(new UnionLowestLevelTermVirtualListOperatorHeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   termRecordStaticScore,
                                                   keywordOffset, prefixNode,
                                                   distance, isPrefixMatch));
        } else { // complete term
            float termRecordRuntimeScore =
                DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                        distance,
                        term->getKeyword()->size(),
                        false,
                        this->prefixMatchPenalty , term->getSimilarityBoost()) * term->getBoost();// prefix match == false
            this->itemsHeap.push_back(new UnionLowestLevelTermVirtualListOperatorHeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   termRecordStaticScore,
                                                   keywordOffset, leafNode, distance, false));
        }

        // Cursor points to the next element on InvertedList
        this->cursorVector.push_back(invertedListCounter);
        // keep the inverted list readviews in invertedListVector such that we can safely access them
        this->invertedListReadViewVector.push_back(invertedListReadView);
    }
}


void UnionLowestLevelTermVirtualListOperator::depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned editDistance, unsigned panDistance, unsigned bound)
{
    if (trieNode->isTerminalNode())
        initialiseTermVirtualListElement(NULL, trieNode, editDistance > panDistance ? editDistance : panDistance);
    if (panDistance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeTermVirtualListElement(child, editDistance, panDistance + 1, bound);
        }
    }
}

//Called when this->numberOfItemsInPartialHeap = 0
bool UnionLowestLevelTermVirtualListOperator::_addItemsToPartialHeap()
{
    bool returnValue = false;
    // If partial heap is empty, increase editDistanceThreshold and add more elements
    for ( vector<UnionLowestLevelTermVirtualListOperatorHeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        UnionLowestLevelTermVirtualListOperatorHeapItem *currentItem = *iter;
        if (this->numberOfItemsInPartialHeap == 0) {
            // partialHeap is empty, assign new maxEditDistance and add items to partialHeap
            if (currentItem->ed > this->currentMaxEditDistanceOnHeap) {
                this->currentMaxEditDistanceOnHeap = currentItem->ed;
                this->numberOfItemsInPartialHeap++;
                returnValue =true;
            }
        } else {
            // Edit distance on itemHeap is less than maxEditDistance so far seen
            if (currentItem->ed <= this->currentMaxEditDistanceOnHeap) {
                this->numberOfItemsInPartialHeap++;
                returnValue = true;
            }        //stopping condition: partialheap is not empty and edit distance is greater than maxEditDistance
            else
                break;
        }
    }
    // PartialHeap changed;
    if (returnValue) {
        make_heap(this->itemsHeap.begin(),this->itemsHeap.begin()+numberOfItemsInPartialHeap,
                  UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperatorHeapItemCmp());
    }
    return returnValue;
}

}
}
