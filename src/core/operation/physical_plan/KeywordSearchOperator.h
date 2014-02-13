
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

#ifndef __PHYSICALPLAN_KEYWORDSEARCHOPERATOR_H__
#define __PHYSICALPLAN_KEYWORDSEARCHOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "../QueryOptimizer.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


/*
 * KeywordSearchOperator is a special operator which does the main search.
 * It can be viewed as a controller and executor of other search modules.
 * These tasks are done in this operator:
 * 1. Exact/Fuzzy policy is implemented in this operator. So tasks 2 to ??? are repeated for fuzzy query if
 * ---- we don't get enough results from exact.
 * 2. The number of results that we need is calculated based on searchType and postProcessing request (like having Facet)
 * 3. It uses HistogramManager to annotate the LogicalPlan for further steps.
 * 4. It uses QueryOptimizer to build a good physical plan (which is the core plan)
 * 5. It opens the plan (by calling open(...)) and calls getNext as many times as needed .
 */
class KeywordSearchOperator : public PhysicalPlanNode {
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~KeywordSearchOperator();
	KeywordSearchOperator(LogicalPlan * logicalPlan);
private:
	LogicalPlan * logicalPlan;
	vector<PhysicalPlanRecordItem *> results;
	unsigned cursorOnResults;
};

class KeywordSearchOptimizationOperator : public PhysicalPlanOptimizationNode {
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};


}
}


#endif // __PHYSICALPLAN_KEYWORDSEARCHOPERATOR_H__
