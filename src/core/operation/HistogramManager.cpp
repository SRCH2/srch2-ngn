

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

	allocateLogicalPlanNodeAnnotations(logicalPlan->getTree());

	annotateWithActiveNodeSets(logicalPlan->getTree(), logicalPlan->isFuzzy());

	annotateWithEstimatedProbabilitiesAndNumberOfResults(logicalPlan->getTree(), logicalPlan->isFuzzy());

	if(countNumberOfKeywords(logicalPlan->getTree() , logicalPlan->isFuzzy()) == 1){
		if(logicalPlan->getTree()->stats->getEstimatedNumberOfResults() >
			queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->keywordPopularityThreshold){
			markTermToForceSuggestionPhysicalOperator(logicalPlan->getTree(), logicalPlan->isFuzzy());
		}
	}


}

void HistogramManager::markTermToForceSuggestionPhysicalOperator(LogicalPlanNode * node , bool isFuzzy){
	switch (node->nodeType) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeNot:
		{
			ASSERT(node->children.size() == 1);
			markTermToForceSuggestionPhysicalOperator(node->children.at(0) , isFuzzy);
			return;
		}
		case LogicalPlanNodeTypeTerm:
		{
			node->forcedPhysicalNode = PhysicalPlanNode_UnionLowestLevelSuggestion;
			return;
		}
    }
}

// traverses the tree (by recursive calling) and allocates the annotation object for each node
void HistogramManager::allocateLogicalPlanNodeAnnotations(LogicalPlanNode * node){
	if(node == NULL){
		return;
	}
	if(node->stats == NULL){
		node->stats = new LogicalPlanNodeAnnotation();
	}
	for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
		allocateLogicalPlanNodeAnnotations(*child);
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
			node->stats->activeNodeSetExact = computeActiveNodeSet(node->exactTerm);
			if(isFuzzy == true){
				node->stats->activeNodeSetFuzzy = computeActiveNodeSet(node->fuzzyTerm);
			}
			break;
		default:
			break;
	}
	// and now activenodes for children
	for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
		annotateWithActiveNodeSets(*child , isFuzzy);
	}

}


void HistogramManager::annotateWithEstimatedProbabilitiesAndNumberOfResults(LogicalPlanNode * node , bool isFuzzy){
	if(node == NULL){
		return;
	}
	// first estimate probability and number of results for the children, then based on this operator aggregate the probabilities
	// and compute the number of results.
	for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
		annotateWithEstimatedProbabilitiesAndNumberOfResults(*child , isFuzzy);
	}

	switch (node->nodeType) {
		case LogicalPlanNodeTypeAnd:
		{
			/*
			 * P(A AND B AND C) = P(A) * P(B) * P(C)
			 */
			float conjunctionAggregatedProbability = 1;
			for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
				conjunctionAggregatedProbability = conjunctionAggregatedProbability * (*child)->stats->getEstimatedProbability();
			}
			node->stats->setEstimatedProbability(conjunctionAggregatedProbability);
			node->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(node->stats->getEstimatedProbability()));
			break;
		}
		case LogicalPlanNodeTypeOr:
		{
			/*
			 * P(A1 OR A2 OR ... OR An) = P((A1 OR A2 OR ... OR An-1) OR An),
			 * P(X OR Y) = P(X) + P(Y) - P(X) * P(Y)
			 */
			float disjunctionAggregatedProbability = 0;
			for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
				disjunctionAggregatedProbability =
						disjunctionAggregatedProbability + (*child)->stats->getEstimatedProbability()
						- (disjunctionAggregatedProbability * (*child)->stats->getEstimatedProbability());
			}
			node->stats->setEstimatedProbability(disjunctionAggregatedProbability);
			node->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(node->stats->getEstimatedProbability()));
			break;
		}
		case LogicalPlanNodeTypeNot:
		{
			/*
			 * P(NOT A) = 1 - P(A)
			 */
			ASSERT(node->children.size() == 1); // NOT should have exactly one child
			float childProbability = node->children.at(0)->stats->getEstimatedProbability();
			float negationProbability = 1 - childProbability;
			node->stats->setEstimatedProbability(negationProbability);
			node->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(node->stats->getEstimatedProbability()));
			break;
		}
		case LogicalPlanNodeTypeTerm:
		{
			unsigned thresholdForEstimation = isFuzzy ? node->fuzzyTerm->getThreshold() : node->exactTerm->getThreshold();
			ts_shared_ptr<PrefixActiveNodeSet> activeNodeSetForEstimation =  node->stats->getActiveNodeSetForEstimation(isFuzzy);
			float termProbability;
			unsigned numberOfLeafNodes;
			computeEstimatedProbabilityOfPrefixAndNumberOfLeafNodes(activeNodeSetForEstimation.get(), thresholdForEstimation, termProbability, numberOfLeafNodes);
			node->stats->setEstimatedProbability(termProbability);
			node->stats->setEstimatedNumberOfResults(computeEstimatedNumberOfResults(node->stats->getEstimatedProbability()));
			node->stats->setEstimatedNumberOfLeafNodes(numberOfLeafNodes);
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
		default:
			ASSERT(false);
			return 0;
	}

}

ts_shared_ptr<PrefixActiveNodeSet> HistogramManager::computeActiveNodeSet(Term *term) const
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
    ts_shared_ptr<PrefixActiveNodeSet> initialPrefixActiveNodeSet ;
    int cacheResponse = this->queryEvaluator->cacheManager->getActiveNodesCache()->findLongestPrefixActiveNodes(term, initialPrefixActiveNodeSet); //initialPrefixActiveNodeSet is Busy

    if ( cacheResponse == 0) { // NO CacheHit,  response = 0
        //std::cout << "|NO Cache|" << std::endl;;
        // No prefix has a cached TermActiveNode Set. Create one for the empty std::string "".
    	initialPrefixActiveNodeSet.reset(new PrefixActiveNodeSet(this->queryEvaluator->indexReadToken.trieRootNodeSharedPtr,
    			term->getThreshold(), this->queryEvaluator->indexData->getSchema()->getSupportSwapInEditDistance()));
    }
    cachedPrefixLength = initialPrefixActiveNodeSet->getPrefixLength();

    /// 2. do the incremental computation. BusyBit of prefixActiveNodeSet is busy.
    ts_shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet = initialPrefixActiveNodeSet;

    for (unsigned iter = cachedPrefixLength; iter < keywordLength; iter++) {
        CharType additionalCharacter = charTypeKeyword[iter]; // get the appended character

        ts_shared_ptr<PrefixActiveNodeSet> newPrefixActiveNodeSet = prefixActiveNodeSet->computeActiveNodeSetIncrementally(additionalCharacter);

        prefixActiveNodeSet = newPrefixActiveNodeSet;

        //std::cout << "Cache Set:" << *(prefixActiveNodeSet->getPrefix()) << std::endl;

        if (iter >= 2 && (cacheResponse != -1)) { // Cache not busy and keywordLength is at least 2.
            cacheResponse = this->queryEvaluator->cacheManager->getActiveNodesCache()->setPrefixActiveNodeSet(prefixActiveNodeSet);
        }
    }
    // Possible memory leak due to last prefixActiveNodeSet not being cached. This is checked for
    // and deleted by the caller "QueryResultsInternal()"
    return prefixActiveNodeSet;
}

void HistogramManager::computeEstimatedProbabilityOfPrefixAndNumberOfLeafNodes(PrefixActiveNodeSet * activeNodes ,
		unsigned threshold, float & probability, unsigned & numberOfLeafNodes) const{

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
    float aggregatedProbability = 0;
    unsigned aggregatedNumberOfLeafNodes = 0;
    for(std::vector<TrieNodePointer>::iterator trieNodeIter = topTrieNodes.begin() ; trieNodeIter != topTrieNodes.end() ; ++trieNodeIter){
    	TrieNodePointer topTrieNode = *trieNodeIter;
    	aggregatedProbability = topTrieNode->aggregateValueByJointProbability(aggregatedProbability , topTrieNode->getNodeProbabilityValue());
    	aggregatedNumberOfLeafNodes += topTrieNode->getNumberOfTerminalNodes();
    }

    probability = aggregatedProbability;
    numberOfLeafNodes = aggregatedNumberOfLeafNodes;

}

unsigned HistogramManager::computeEstimatedNumberOfResults(float probability){
	return (unsigned)(probability * this->queryEvaluator->indexData->forwardIndex->getTotalNumberOfForwardLists_ReadView());
}

}
}
