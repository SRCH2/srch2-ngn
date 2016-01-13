
#ifndef __CATALOGMANAGER_H__
#define __CATALOGMANAGER_H__

#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "instantsearch/LogicalPlan.h"
#include "operation/ActiveNode.h"
#include "util/Assert.h"
#include "QueryEvaluatorInternal.h"

#include "map"


using namespace std;

namespace srch2
{
namespace instantsearch
{


/*
 * This class keeps the meta-data of each logical plan node. LogicalPlanNode has a
 * pointer to this class which is null until logicalPlan comes into the core. These
 * objects (all over the logical plan tree) are then initialized and filled by the
 *  HistogramManager as the first step of query optimization.
 */
class LogicalPlanNodeAnnotation{
public:
	unsigned estimatedNumberOfResults;
	double estimatedProbability;
	unsigned estimatedNumberOfLeafNodes;
	boost::shared_ptr<PrefixActiveNodeSet> activeNodeSetExact;
	boost::shared_ptr<PrefixActiveNodeSet> activeNodeSetFuzzy;

	boost::shared_ptr<GeoBusyNodeSet> quadTreeNodeSet;

	LogicalPlanNodeAnnotation(){
		estimatedNumberOfLeafNodes = 0;
		estimatedNumberOfResults = 0;
		estimatedProbability = 0;
	}
	~LogicalPlanNodeAnnotation(){
		estimatedNumberOfResults = 0;
		estimatedProbability = 0;
	}
	unsigned getEstimatedNumberOfResults() const{
		return this->estimatedNumberOfResults;
	}
	void setEstimatedNumberOfResults(unsigned estimate){
		estimatedNumberOfResults = estimate;
	}

	unsigned getEstimatedNumberOfLeafNodes() const {
		return estimatedNumberOfLeafNodes;
	}

	void setEstimatedNumberOfLeafNodes(unsigned estimatedNumberOfLeafNodes) {
		this->estimatedNumberOfLeafNodes = estimatedNumberOfLeafNodes;
	}

	double getEstimatedProbability() const{
		return estimatedProbability;
	}
	void setEstimatedProbability(double p){
		estimatedProbability = p;
	}
	boost::shared_ptr<PrefixActiveNodeSet> getActiveNodeSetForEstimation(bool isFuzzy){
		if(isFuzzy == true){
			return activeNodeSetFuzzy;
		}
		return activeNodeSetExact;
	}

	boost::shared_ptr<GeoBusyNodeSet> getQuadTreeNodeSetForEstimation(){
		return this->quadTreeNodeSet;
	}
};

class QueryEvaluatorInternal;

/*
 * this class is responsible of calculating and maintaining catalog information.
 * Specifically, CatalogManager calculates and keeps required information for
 * calculating PhysicalPlan costs, upon receiving the LogicalPlan. The CatalogManager
 * maintains multiple maps to keep these pieces of information and does not put this info
 * in the LogicalPlan. For this reason, each LogicalPlanNode must have an ID so that CatalogManager
 * uses this ID to keep the information about that node.
 */
class HistogramManager {
public:
	HistogramManager(QueryEvaluatorInternal * queryEvaluator);
	~HistogramManager();

	// calculates required information for building physical plan.
	// this information is accessible later by passing nodeId of LogicalPlanNodes.
	void annotate(LogicalPlan * logicalPlan);
	static void freeStatsOfLogicalPlanTree(LogicalPlanNode * node) {
	    if(node == NULL){
	        return;
	    }
	    delete node->stats;
	    node->stats = NULL;
	    for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
	        freeStatsOfLogicalPlanTree(*child);
	    }
	}
	// traverses the tree (by recursive calling) and allocates the annotation object for each node
	static void allocateLogicalPlanNodeAnnotations(LogicalPlanNode * node){
		if(node == NULL){
			return;
		}
		if(node->stats != NULL){
			ASSERT(false);
			delete node->stats;
			node->stats = NULL;
		}
		node->stats = new LogicalPlanNodeAnnotation();
		for(vector<LogicalPlanNode * >::iterator child = node->children.begin(); child != node->children.end() ; ++child){
			allocateLogicalPlanNodeAnnotations(*child);
		}
	}
private:
	QueryEvaluatorInternal * queryEvaluator;

	void markTermToForceSuggestionPhysicalOperator(LogicalPlanNode * node , bool isFuzzy);

	void annotateWithActiveNodeSets(LogicalPlanNode * node , bool isFuzzy);
	void annotateWithEstimatedProbabilitiesAndNumberOfResults(LogicalPlanNode * node , bool isFuzzy);
	unsigned countNumberOfKeywords(LogicalPlanNode * node , bool isFuzzy);

	boost::shared_ptr<PrefixActiveNodeSet> computeActiveNodeSet(Term *term) const;
	boost::shared_ptr<GeoBusyNodeSet> computeQuadTreeNodeSet(Shape *shape);
	void computeEstimatedProbabilityOfPrefixAndNumberOfLeafNodes(TermType termType, PrefixActiveNodeSet * activeNodes ,
			unsigned threshold, double & probability, unsigned & numberOfLeafNodes) const;

	// this function is only used when term is complete so we need to
	// go further down the Trie
	void depthAggregateProbabilityAndNumberOfLeafNodes(const TrieNode* trieNode,
			unsigned editDistance,
			unsigned panDistance,
			unsigned bound,
			double & aggregatedProbability ,
			unsigned & aggregatedNumberOfLeafNodes) const;

	unsigned computeEstimatedNumberOfResults(double probability);
};

}
}

#endif // __CATALOGMANAGER_H__
