

#include "HistogramManager.h"

namespace srch2
{
namespace instantsearch
{

HistogramManager::HistogramManager(QueryEvaluatorInternal * queryEvaluator){
	this->queryEvaluator = queryEvaluator;
}

HistogramManager::~HistogramManager(){

}

// calculates required information for building physical plan.
// this information is accessible later by passing nodeId of LogicalPlanNodes.
void HistogramManager::annotate(LogicalPlan * logicalPlan){
	/*
	 * this function calculates this information:
	 * 0. allocate annotation object in logical plan
	 * 1. Active node sets for terminal nodes of LogicalPlan
	 * 2. Estimated number of results for each internal node of LogicalPlan.
	 */

    HistogramManager::allocateLogicalPlanNodeAnnotations(logicalPlan->getTree());

	// TODO : Jamshid change the name of it ...
	annotateWithActiveNodeSets(logicalPlan->getTree(), logicalPlan->isFuzzy());

	annotateWithEstimatedProbabilitiesAndNumberOfResults(logicalPlan->getTree(), logicalPlan->isFuzzy());

	// if we only have one terminal node and the number of results is very large we can use suggestion operator for this node
	// for this case the root of the tree should have one child or two child that one of them is a geo node
	if(countNumberOfKeywords(logicalPlan->getTree() , logicalPlan->isFuzzy()) == 1){
		if(logicalPlan->getTree()->children.size() == 1){
			if(logicalPlan->getTree()->stats->getEstimatedNumberOfResults() >
			queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->keywordPopularityThreshold){
				markTermToForceSuggestionPhysicalOperator(logicalPlan->getTree(), logicalPlan->isFuzzy());
			}
		}else if(logicalPlan->getTree()->children.size() == 2){
			if(logicalPlan->getTree()->children[0]->stats->getEstimatedNumberOfResults() >
						queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->keywordPopularityThreshold
			   && logicalPlan->getTree()->children[1]->stats->getEstimatedNumberOfResults() >
						queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->keywordPopularityThreshold){
				markTermToForceSuggestionPhysicalOperator(logicalPlan->getTree(), logicalPlan->isFuzzy());
			}
		}
	}


}

void HistogramManager::markTermToForceSuggestionPhysicalOperator(LogicalPlanNode * node , bool isFuzzy){
	switch (node->nodeType) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeNot:
		{
			// In this case each node can have at most 2 children. (Because of geo node)
			ASSERT(node->children.size() <= 2);
			for( unsigned i = 0 ; i < node->children.size() ; i++){
				markTermToForceSuggestionPhysicalOperator(node->children.at(i) , isFuzzy);
			}
			return;
		}
		case LogicalPlanNodeTypePhrase:
		{
			markTermToForceSuggestionPhysicalOperator(node->children.at(0) , isFuzzy);
			return;
		}
		case LogicalPlanNodeTypeTerm:
		{
			node->forcedPhysicalNode = PhysicalPlanNode_UnionLowestLevelSuggestion;
			return;
		}
		case LogicalPlanNodeTypeGeo:
		{
			// if the type of the term node is UnionLowestLevelSuggestion the type of the geo node should be RandomAccessGeo
			node->forcedPhysicalNode = PhysicalPlanNode_RandomAccessGeo;
			return;
		}
    }
}

// traverses the tree (by recursive calling) and computes active nodes when it sees a TERM node
void HistogramManager::annotateWithActiveNodeSets(LogicalPlanNode * node , bool isFuzzy){
	if(node == NULL){
		return;
	}
	switch (node->nodeType) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeNot:
			break;
		case LogicalPlanNodeTypeTerm:
			if(isFuzzy == false){
				node->stats->activeNodeSetExact = computeActiveNodeSet(node->exactTerm);
			}
			else {
				node->stats->activeNodeSetFuzzy = computeActiveNodeSet(node->fuzzyTerm);
			}
			break;
		case LogicalPlanNodeTypeGeo:
				node->stats->quadTreeNodeSet = computeQuadTreeNodeSet(node->regionShape);
			break;
		default:
			break;
	}
	// and now activenodes for children
	for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
		annotateWithActiveNodeSets(*child , isFuzzy);
	}

}


void HistogramManager::annotateWithEstimatedProbabilitiesAndNumberOfResults(LogicalPlanNode * logicalPlanNode , bool isFuzzy){
	if(logicalPlanNode == NULL){
		return;
	}
	// first estimate probability and number of results for the children, then based on this operator aggregate the probabilities
	// and compute the number of results.
	for(vector<LogicalPlanNode * >::iterator child = logicalPlanNode->children.begin(); child != logicalPlanNode->children.end() ; ++child){
		annotateWithEstimatedProbabilitiesAndNumberOfResults(*child , isFuzzy);
	}

	switch (logicalPlanNode->nodeType) {
		case LogicalPlanNodeTypeAnd:
		{
			/*
			 * P(A AND B AND C) = P(A) * P(B) * P(C)
			 */
			double conjunctionAggregatedProbability = 1;
			for(vector<LogicalPlanNode * >::iterator child = logicalPlanNode->children.begin(); child != logicalPlanNode->children.end() ; ++child){
				conjunctionAggregatedProbability = conjunctionAggregatedProbability * (*child)->stats->getEstimatedProbability();
			}
			logicalPlanNode->stats->setEstimatedProbability(conjunctionAggregatedProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(logicalPlanNode->stats->getEstimatedProbability()));
			/*
			 * if probability is not zero but estimated number of results is zero, it means probability has
			 * become very small. But since this zero will be multiplied to many other parts of costing
			 * it shouldn't be zero or it disables them ...
			 */
			if(conjunctionAggregatedProbability != 0 && logicalPlanNode->stats->getEstimatedNumberOfResults() == 0){
				logicalPlanNode->stats->setEstimatedNumberOfResults(1);
			}
			break;
		}
		case LogicalPlanNodeTypeOr:
		{
			/*
			 * P(A1 OR A2 OR ... OR An) = P((A1 OR A2 OR ... OR An-1) OR An),
			 * P(X OR Y) = P(X) + P(Y) - P(X) * P(Y)
			 */
			double disjunctionAggregatedProbability = 0;
			for(vector<LogicalPlanNode * >::iterator child = logicalPlanNode->children.begin(); child != logicalPlanNode->children.end() ; ++child){
				disjunctionAggregatedProbability =
						disjunctionAggregatedProbability + (*child)->stats->getEstimatedProbability()
						- (disjunctionAggregatedProbability * (*child)->stats->getEstimatedProbability());
			}
			logicalPlanNode->stats->setEstimatedProbability(disjunctionAggregatedProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(logicalPlanNode->stats->getEstimatedProbability()));
			/*
			 * if probability is not zero but estimated number of results is zero, it means probability has
			 * become very small. But since this zero will be multiplied to many other parts of costing
			 * it shouldn't be zero or it disables them ...
			 */
			if(disjunctionAggregatedProbability != 0 && logicalPlanNode->stats->getEstimatedNumberOfResults() == 0){
				logicalPlanNode->stats->setEstimatedNumberOfResults(1);
			}
			break;
		}
		case LogicalPlanNodeTypeNot:
		{
			/*
			 * P(NOT A) = 1 - P(A)
			 */
			ASSERT(logicalPlanNode->children.size() == 1); // NOT should have exactly one child
			double childProbability = logicalPlanNode->children.at(0)->stats->getEstimatedProbability();
			double negationProbability = 1 - childProbability;
			logicalPlanNode->stats->setEstimatedProbability(negationProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(logicalPlanNode->stats->getEstimatedProbability()));
			/*
			 * if probability is not zero but estimated number of results is zero, it means probability has
			 * become very small. But since this zero will be multiplied to many other parts of costing
			 * it shouldn't be zero or it disables them ...
			 */
			if(negationProbability != 0 && logicalPlanNode->stats->getEstimatedNumberOfResults() == 0){
				logicalPlanNode->stats->setEstimatedNumberOfResults(1);
			}
			break;
		}
		case LogicalPlanNodeTypePhrase:
		{
			double childProbability = logicalPlanNode->children.at(0)->stats->getEstimatedProbability();
			logicalPlanNode->stats->setEstimatedProbability(childProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(logicalPlanNode->stats->getEstimatedProbability()));
			break;
		}
		case LogicalPlanNodeTypeTerm:
		{
			unsigned thresholdForEstimation = isFuzzy ? logicalPlanNode->fuzzyTerm->getThreshold() : logicalPlanNode->exactTerm->getThreshold();
			TermType termType = isFuzzy ? logicalPlanNode->fuzzyTerm->getTermType() : logicalPlanNode->exactTerm->getTermType();
			boost::shared_ptr<PrefixActiveNodeSet> activeNodeSetForEstimation =  logicalPlanNode->stats->getActiveNodeSetForEstimation(isFuzzy);
			double termProbability;
			unsigned numberOfLeafNodes;
			computeEstimatedProbabilityOfPrefixAndNumberOfLeafNodes(termType, activeNodeSetForEstimation.get(), thresholdForEstimation, termProbability, numberOfLeafNodes);
			logicalPlanNode->stats->setEstimatedProbability(termProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(logicalPlanNode->stats->getEstimatedProbability()));
			logicalPlanNode->stats->setEstimatedNumberOfLeafNodes(numberOfLeafNodes);
			/*
			 * if probability is not zero but estimated number of results is zero, it means probability has
			 * become very small. But since this zero will be multiplied to many other parts of costing
			 * it shouldn't be zero or it disables them ...
			 */
			if(termProbability != 0 && logicalPlanNode->stats->getEstimatedNumberOfResults() == 0){
				logicalPlanNode->stats->setEstimatedNumberOfResults(1);
			}
			break;
		}
		case LogicalPlanNodeTypeGeo:
		{
			double geoElementsProbability = 0;
			double quadTreeNodeProbability;
			unsigned geoNumOfLeafNodes = 0;
			QuadTreeNode *geoNode;
			boost::shared_ptr<GeoBusyNodeSet> quadTreeNodeSetSharedPtr = logicalPlanNode->stats->getQuadTreeNodeSetForEstimation();
			vector<QuadTreeNode*>* quadTreeNodeSet = quadTreeNodeSetSharedPtr->getQuadTreeNodeSet();
			for( unsigned i = 0 ; i < quadTreeNodeSet->size() ; i++){
				geoNode = quadTreeNodeSet->at(i);
				quadTreeNodeProbability = ( double(geoNode->getNumOfElementsInSubtree()) /
														this->queryEvaluator->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView());
				geoElementsProbability = geoNode->aggregateValueByJointProbabilityDouble(geoElementsProbability, quadTreeNodeProbability);
				geoNumOfLeafNodes += geoNode->getNumOfLeafNodesInSubtree();
			}
			logicalPlanNode->stats->setEstimatedProbability(geoElementsProbability);
			logicalPlanNode->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(geoElementsProbability));
			logicalPlanNode->stats->setEstimatedNumberOfLeafNodes(geoNumOfLeafNodes);
			/*
			 * if probability is not zero but estimated number of results is zero, it means probability has
			 * become very small. But since this zero will be multiplied to many other parts of costing
			 * it shouldn't be zero or it disables them ...
			 */
			if(geoElementsProbability != 0 && logicalPlanNode->stats->getEstimatedNumberOfResults() == 0){
				logicalPlanNode->stats->setEstimatedNumberOfResults(1);
			}
			break;
		}
	}


}

unsigned HistogramManager::countNumberOfKeywords(LogicalPlanNode * node , bool isFuzzy){
	switch (node->nodeType) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeNot:
		{
			unsigned numberOfTermsInChildren = 0;
			for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
				numberOfTermsInChildren += countNumberOfKeywords(*child, isFuzzy);
			}
			return numberOfTermsInChildren;
		}
		case LogicalPlanNodeTypeTerm:
		{
			return 1;
		}
		case LogicalPlanNodeTypePhrase:
		{
			return countNumberOfKeywords(node->children.at(0), isFuzzy);
		}
		case LogicalPlanNodeTypeGeo:
		{
			return 0; // we don't have any keyword in the Geo Node
		}
		default:
			ASSERT(false);
			return 0;
	}

}

boost::shared_ptr<PrefixActiveNodeSet> HistogramManager::computeActiveNodeSet(Term *term) const
{
    // it should not be an empty std::string
    string *keyword = term->getKeyword();
    vector<CharType> charTypeKeyword;
    utf8StringToCharTypeVector(*keyword, charTypeKeyword);
    unsigned keywordLength = charTypeKeyword.size();
    ASSERT(keywordLength > 0);

    // We group the active trie nodes based on their edit distance to the term prefix

    // 1. Get the longest prefix that has active nodes
    unsigned cachedPrefixLength = 0;
    boost::shared_ptr<PrefixActiveNodeSet> initialPrefixActiveNodeSet ;
    int cacheResponse = this->queryEvaluator->cacheManager->getActiveNodesCache()->findLongestPrefixActiveNodes(term, initialPrefixActiveNodeSet); //initialPrefixActiveNodeSet is Busy

    if ( cacheResponse == 0) { // NO CacheHit,  response = 0
        //std::cout << "|NO Cache|" << std::endl;;
        // No prefix has a cached TermActiveNode Set. Create one for the empty std::string "".
    	initialPrefixActiveNodeSet.reset(new PrefixActiveNodeSet(this->queryEvaluator->indexReadToken.trieRootNodeSharedPtr,
    			term->getThreshold(), this->queryEvaluator->indexData->getSchema()->getSupportSwapInEditDistance()));
    }
    cachedPrefixLength = initialPrefixActiveNodeSet->getPrefixLength();

    /// 2. do the incremental computation. BusyBit of prefixActiveNodeSet is busy.
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet = initialPrefixActiveNodeSet;

    for (unsigned iter = cachedPrefixLength; iter < keywordLength; iter++) {
        CharType additionalCharacter = charTypeKeyword[iter]; // get the appended character

        boost::shared_ptr<PrefixActiveNodeSet> newPrefixActiveNodeSet = prefixActiveNodeSet->computeActiveNodeSetIncrementally(additionalCharacter);

        prefixActiveNodeSet = newPrefixActiveNodeSet;

        //std::cout << "Cache Set:" << *(prefixActiveNodeSet->getPrefix()) << std::endl;

        if (iter >= 2 && (cacheResponse != -1)) { // Cache not busy and keywordLength is at least 2.
        	prefixActiveNodeSet->prepareForIteration(); // this is the last write operation on prefixActiveNodeSet
            cacheResponse = this->queryEvaluator->cacheManager->getActiveNodesCache()->setPrefixActiveNodeSet(prefixActiveNodeSet);
        }
    }
    // Possible memory leak due to last prefixActiveNodeSet not being cached. This is checked for
    // and deleted by the caller "QueryResultsInternal()"
    return prefixActiveNodeSet;
}

boost::shared_ptr<GeoBusyNodeSet> HistogramManager::computeQuadTreeNodeSet(Shape* shape){
	// first create a shared pointer of geoActiveNodeSet
	boost::shared_ptr<GeoBusyNodeSet> geoActiveNodeSet;
	geoActiveNodeSet.reset(new GeoBusyNodeSet(this->queryEvaluator->indexReadToken.quadTreeRootNodeSharedPtr));
	// Then by calling computeQuadTreeNodeSet find all quadTreeNodes inside the query region
	geoActiveNodeSet->computeQuadTreeNodeSet(*shape);
	return geoActiveNodeSet;
}

void HistogramManager::computeEstimatedProbabilityOfPrefixAndNumberOfLeafNodes(TermType termType, PrefixActiveNodeSet * activeNodes ,
		unsigned threshold, double & probability, unsigned & numberOfLeafNodes) const{

	/*
	 * If termType is complete, we shouldn't estimate
	 * numberOfLeafNodes or probability value. we can use their exact values
	 *
	 * For example, for a complete keyword "mic", we can use the length of the inverted
	 * list of the terminal node "mic" to compute its exact probability, without considering other
	 * leaf nodes of this node, such as "michael" and "microsoft".
	 *
	 */
	if(termType == TERM_TYPE_COMPLETE){
	    unsigned aggregatedNumberOfLeafNodes = 0;
	    double aggregatedProbability = 0;

	    for(LeafNodeSetIteratorForComplete iter(activeNodes , threshold); !iter.isDone(); iter.next()){
	        TrieNodePointer trieNode;
	        unsigned distance;
	        iter.getItem(trieNode, distance);

			aggregatedNumberOfLeafNodes ++; // each terminal node is one leaf node when term is complete

			// fetch the inverted list to get its size
			shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
			queryEvaluator->getInvertedIndex()->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
			shared_ptr<vectorview<unsigned> > invertedListReadView;
			queryEvaluator->getInvertedIndex()->getInvertedListReadView(invertedListDirectoryReadView,
					trieNode->getInvertedListOffset(), invertedListReadView);
			// calculate the probability of this node by using invertedlist size
			double individualProbabilityOfCompleteTermTrieNode = (invertedListReadView->size() * 1.0) /
					this->queryEvaluator->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView();
			// use new probability in joint probability
			aggregatedProbability =
					trieNode->aggregateValueByJointProbabilityDouble(aggregatedProbability , individualProbabilityOfCompleteTermTrieNode);

	    }

	    numberOfLeafNodes = aggregatedNumberOfLeafNodes;
	    probability = aggregatedProbability;
	    return;

	}

	/*
	 * Example :
	 *                          --t(112,112,112)$              ---- t ----- ***e ----- e ----- n(96,96,96)$
	 *    root                  |                              |  (96,96)   (96,96)  (96,96)
	 *      |                   |                              |
	 *      |       (32,112) (32,112)        (32,32,96)$       |---- s(80,80,80)$
	 *      ----------- c ----- a------------ ***n ------------|
	 *      |                                                  |---- ***c ----- e ----- r(64,64,64)$
	 *      |                                                  |     (64,64)  (64,64)
	 *      ***a(16,16)                                        |
	 *      |                                                  ---- a ----- d ----- a(48,48,48)$
	 *      |                                                     (48,48) (48,48)
	 *      n(16,16)
	 *      |
	 *      |
	 *      ***d(16,16,16)$
	 *
	 * and suppose those trie nodes that have *** next to them are activeNodes. (n,a,c,d, and e)
	 *
	 * After sorting in pre-order, the order of these active nodes will be :
	 * a , d , n , e , c
	 *
	 * and if we always compare the currentNode with the last top node:
	 * 1. 'a' is added to topNodes
	 * 2. 'd' is ignored because it's a descendant of 'a'
	 * 3. 'n' has no relationship with 'a' so it's added to topNodes.
	 * 4. 'e' is ignored because it's a descendant of 'n'
	 * 5. 'c' is ignored because it's a descendant of 'n'
	 * so we will have <a,n>
	 *
	 * and then we use joint probability to aggregate 'a' and 'n' values.
	 *
	 */
	std::vector<TrieNodePointer> topTrieNodes;
	std::vector<TrieNodePointer> preOrderSortedTrieNodes;
	// iterate on active nodes and keep them in preOrderSortedTrieNodes to be sorted in next step
    for (ActiveNodeSetIterator iter(activeNodes, threshold); !iter.isDone(); iter.next()) {
        TrieNodePointer trieNode;
        unsigned distance;
        iter.getItem(trieNode, distance);
        preOrderSortedTrieNodes.push_back(trieNode);
    }
    // now sort them in preOrder
    std::sort(preOrderSortedTrieNodes.begin() , preOrderSortedTrieNodes.end() , TrieNodePreOrderComparator());

    // now move from left to right and always compare the current node with the last node in topTrieNodes
    for(unsigned trieNodeIter = 0 ; trieNodeIter < preOrderSortedTrieNodes.size() ; ++trieNodeIter){
    	TrieNodePointer currentTrieNode = preOrderSortedTrieNodes.at(trieNodeIter);
    	if(topTrieNodes.size() == 0){ // We always push the first node into topTrieNodes.
    		topTrieNodes.push_back(currentTrieNode);
    		continue;
    	}
    	/// since trie nodes are coming in preOrder, currentTrieNode is either a descendant of the last node in topTrieNodes,
    	// or it has no relationship with that node. In the former case, we ignore currentTrieNode. In the latter case, we append it to topTrieNodes.
    	if(currentTrieNode->isDescendantOf(topTrieNodes.at(topTrieNodes.size()-1)) == false){ // if it's not descendant
    		topTrieNodes.push_back(currentTrieNode);
    	}// else : if it's a descendant we don't have to do anything

    }

    // now we have the top level trieNodes
    // we move on all top trie nodes and aggregate their probability by using Joint Probability formula
    double aggregatedProbability = 0;
    unsigned aggregatedNumberOfLeafNodes = 0;
    for(std::vector<TrieNodePointer>::iterator trieNodeIter = topTrieNodes.begin() ; trieNodeIter != topTrieNodes.end() ; ++trieNodeIter){
    	TrieNodePointer topTrieNode = *trieNodeIter;
    	aggregatedProbability = topTrieNode->aggregateValueByJointProbabilityDouble(aggregatedProbability , topTrieNode->getNodeProbabilityValue());
    	aggregatedNumberOfLeafNodes += topTrieNode->getNumberOfTerminalNodes();
    }

    probability = aggregatedProbability;
    numberOfLeafNodes = aggregatedNumberOfLeafNodes;

}


void HistogramManager::depthAggregateProbabilityAndNumberOfLeafNodes(const TrieNode* trieNode,
		unsigned editDistance,
		unsigned panDistance,
		unsigned bound,
		double & aggregatedProbability ,
		unsigned & aggregatedNumberOfLeafNodes) const{
    if (trieNode->isTerminalNode()){

    }
    if (panDistance < bound) {
        for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
            const TrieNode *child = trieNode->getChild(childIterator);
            depthAggregateProbabilityAndNumberOfLeafNodes(child, editDistance,
            		panDistance + 1, bound ,
            		aggregatedProbability, aggregatedNumberOfLeafNodes);
        }
    }
}


unsigned HistogramManager::computeEstimatedNumberOfResults(double probability){
	return (unsigned)(probability * this->queryEvaluator->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView());
}


}
}
