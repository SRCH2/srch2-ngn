
/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

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

	boost::shared_ptr<GeoActiveNodeSet> quadTreeNodeSet;

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

	boost::shared_ptr<GeoActiveNodeSet> getQuadTreeNodeSetForEstimation(){
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

private:
	QueryEvaluatorInternal * queryEvaluator;

	void markTermToForceSuggestionPhysicalOperator(LogicalPlanNode * node , bool isFuzzy);

	void allocateLogicalPlanNodeAnnotations(LogicalPlanNode * node);

	void annotateWithActiveNodeSets(LogicalPlanNode * node , bool isFuzzy);
	void annotateWithEstimatedProbabilitiesAndNumberOfResults(LogicalPlanNode * node , bool isFuzzy);
	unsigned countNumberOfKeywords(LogicalPlanNode * node , bool isFuzzy);

	boost::shared_ptr<PrefixActiveNodeSet> computeActiveNodeSet(Term *term) const;
	boost::shared_ptr<GeoActiveNodeSet> computeQuadTreeNodeSet(Shape *range);
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
