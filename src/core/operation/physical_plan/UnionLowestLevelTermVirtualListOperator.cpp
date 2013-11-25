
#include "UnionLowestLevelTermVirtualListOperator.h"

namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// merge when lists are sorted by ID Only top K////////////////////////////

UnionLowestLevelTermVirtualListOperator::UnionLowestLevelTermVirtualListOperator() {
    //TODO
}

UnionLowestLevelTermVirtualListOperator::~UnionLowestLevelTermVirtualListOperator(){
    //TODO
}
bool UnionLowestLevelTermVirtualListOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){

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

	this->invertedIndex = queryEvaluator->getInvertedIndex();
    this->prefixActiveNodeSet = logicalPlanNode->stats->getActiveNodeSetForEstimation(params.isFuzzy);
    this->term = term;
    this->prefixMatchPenalty = params.prefixMatchPenalty;
    this->numberOfItemsInPartialHeap = 0;
    this->currentMaxEditDistanceOnHeap = 0;
    this->currentRecordID = -1;
    // this flag indicates whether this TVL is for a tooPopular term or not.
    // If it is a TVL of a too popular term, this TVL is disabled, meaning it should not be used for iteration over
    // heapItems. In this case shouldIterateToLeafNodesAndScoreOfTopRecord is not equal to -1
    // check the TermType
    if (this->getTermType() == TERM_TYPE_PREFIX) { //case 1: Term is prefix
        LeafNodeSetIterator iter(prefixActiveNodeSet, term->getThreshold());
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
        ActiveNodeSetIterator iter(prefixActiveNodeSet, term->getThreshold());
        cursorVector.reserve(iter.size());
        invertedListReadViewVector.reserve(iter.size());
        for (; !iter.isDone(); iter.next()) {
            TrieNodePointer trieNode;
            unsigned distance;
            iter.getItem(trieNode, distance);
            distance = prefixActiveNodeSet->getEditdistanceofPrefix(trieNode);
            depthInitializeTermVirtualListElement(trieNode, distance, term->getThreshold());
        }
    }

    // Make partial heap by calling make_heap from begin() to begin()+"number of items within edit distance threshold"
    make_heap(itemsHeap.begin(), itemsHeap.begin()+numberOfItemsInPartialHeap, UnionLowestLevelTermVirtualListOperator::HeapItemCmp());

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
        HeapItem *currentHeapMax = *(itemsHeap.begin());
        pop_heap(itemsHeap.begin(), itemsHeap.begin() + this->numberOfItemsInPartialHeap,
                 TermVirtualList::HeapItemCmp());

        // allocate new item and fill it out
        PhysicalPlanRecordItem * newItem = this->queryEvaluator->getPhysicalPlanRecordItemFactory();

        newItem->setRecordId(currentHeapMax->recordId);
        newItem->setRecordRuntimeScore(currentHeapMax->termRecordRuntimeScore);
        vector<TrieNodePointer> prefixes;
        prefixes.push_back(currentHeapMax->trieNode);
        newItem->setRecordMatchingPrefixes(prefixes);
        vector<unsigned> attributeBitmaps;
        attributeBitmaps.push_back(currentHeapMax->attributeBitMap);
        newItem->setRecordMatchAttributeBitmaps(attributeBitmaps);
        vector<unsigned> editDistances;
        editDistances.push_back(currentHeapMax->ed);
        newItem->setRecordMatchEditDistances(editDistances);
        vector<unsigned> positionIndexOffsets;
        positionIndexOffsets.push_back(currentHeapMax->positionIndexOffset);
        newItem->setPositionIndexOffsets(positionIndexOffsets);


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
            unsigned recordOffset = this->invertedIndex->getKeywordOffset(recordId, currentHeapMaxInvertetedListId);
            unsigned termAttributeBitmap = 0;
            currentHeapMaxCursor++;

            // check isValidTermPositionHit
            float termRecordStaticScore = 0;
            if (this->invertedIndex->isValidTermPositionHit(recordId, recordOffset,
                    term->getAttributeToFilterTermHits(), termAttributeBitmap,
                    termRecordStaticScore)) {
                foundValidHit = 1;
                this->cursorVector[currentHeapMax->cursorVectorPosition] = currentHeapMaxCursor;
                // Update cursor of popped virtualList in invertedListCursorVector.
                // Cursor always points to next element in invertedList
                currentHeapMax->recordId = recordId;
                currentHeapMax->termRecordRuntimeScore =
                    DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                            currentHeapMax->ed,
                            term->getKeyword()->size(),
                            currentHeapMax->isPrefixMatch,
                            this->prefixMatchPenalty , term->getSimilarityBoost());
                currentHeapMax->termRecordStaticScore = termRecordStaticScore;
                currentHeapMax->attributeBitMap = termAttributeBitmap;
                currentHeapMax->positionIndexOffset = recordOffset;
                push_heap(itemsHeap.begin(), itemsHeap.begin()+this->numberOfItemsInPartialHeap,
                          TermVirtualList::HeapItemCmp());
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
    queryEvaluator = NULL;
    for (vector<HeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        HeapItem *currentItem = *iter;
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
bool UnionLowestLevelTermVirtualListOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
    //TODO
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
    //TODO
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
    //TODO
}
// the cost of close of a child is only considered once since each node's close function is only called once.
unsigned UnionLowestLevelTermVirtualListOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
    //TODO
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
    this->invertedIndex->getInvertedListReadView(invertedListId, invertedListReadView);
    unsigned recordId = invertedListReadView->getElement(invertedListCounter);
    // calculate record offset online
    unsigned recordOffset = this->invertedIndex->getKeywordOffset(recordId, invertedListId);
    ++ invertedListCounter;

    bool foundValidHit = 0;
    float termRecordStaticScore = 0;
    unsigned termAttributeBitmap = 0;
    while (1) {
        if (this->invertedIndex->isValidTermPositionHit(recordId, recordOffset,
                term->getAttributeToFilterTermHits(), termAttributeBitmap,
                termRecordStaticScore) ) {
            foundValidHit = 1;
            break;
        }

        if (invertedListCounter < invertedListReadView->size()) {
            recordId = invertedListReadView->getElement(invertedListCounter);
            // calculate record offset online
            recordOffset = this->invertedIndex->getKeywordOffset(recordId, invertedListId);
            ++invertedListCounter;
        } else {
            break;
        }
    }

    if (foundValidHit == 1) {
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
                        this->prefixMatchPenalty , term->getSimilarityBoost());
            this->itemsHeap.push_back(new HeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   termRecordStaticScore,
                                                   recordOffset, prefixNode,
                                                   distance, isPrefixMatch));
        } else { // complete term
            float termRecordRuntimeScore =
                DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore,
                        distance,
                        term->getKeyword()->size(),
                        false,
                        this->prefixMatchPenalty , term->getSimilarityBoost());// prefix match == false
            this->itemsHeap.push_back(new HeapItem(invertedListId, this->cursorVector.size(),
                                                   recordId, termAttributeBitmap, termRecordRuntimeScore,
                                                   termRecordStaticScore,
                                                   recordOffset, leafNode, distance, false));
        }

        // Cursor points to the next element on InvertedList
        this->cursorVector.push_back(invertedListCounter);
        // keep the inverted list readviews in invertedListVector such that we can safely access them
        this->invertedListReadViewVector.push_back(invertedListReadView);
    }
}


void UnionLowestLevelTermVirtualListOperator::depthInitializeTermVirtualListElement(const TrieNode* trieNode, unsigned distance, unsigned bound)
{
    if (trieNode->isTerminalNode())
        initialiseTermVirtualListElement(NULL, trieNode, distance);
    if (distance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthInitializeTermVirtualListElement(child, distance+1, bound);
        }
    }
}

//Called when this->numberOfItemsInPartialHeap = 0
bool UnionLowestLevelTermVirtualListOperator::_addItemsToPartialHeap()
{
    bool returnValue = false;
    // If partial heap is empty, increase editDistanceThreshold and add more elements
    for ( vector<HeapItem* >::iterator iter = this->itemsHeap.begin(); iter != this->itemsHeap.end(); iter++) {
        HeapItem *currentItem = *iter;
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
                  TermVirtualList::HeapItemCmp());
    }
    return returnValue;
}

}
}
