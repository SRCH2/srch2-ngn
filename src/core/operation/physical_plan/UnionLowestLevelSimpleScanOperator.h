
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

#ifndef __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__
#define __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

/*
 * This operator is a simple scan operator. It starts with a set of activenodes,
 * finds all the leaf nodes based on the term type (prefix or complete),
 * saves all the connected inverted lists, and then in each getNext call returns
 * one record from a list until all lists are iterated completely.
 * The difference between this operator and TVL is that this operator doesn't give
 * any guarantee of order for its output (the output is NOT sorted by score) and
 * it doesn't have the overhead of HEAP that TVL has.
 */
class UnionLowestLevelSimpleScanOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~UnionLowestLevelSimpleScanOperator();
private:
	UnionLowestLevelSimpleScanOperator() ;

	void depthInitializeSimpleScanOperator(
			const TrieNode* trieNode,const TrieNode* prefixNode, unsigned distance, unsigned bound);

	QueryEvaluatorInternal * queryEvaluator;

	vector< shared_ptr<vectorview<unsigned> > > invertedLists;
	vector< unsigned > invertedListDistances;
	vector< TrieNodePointer > invertedListPrefixes;
	vector< TrieNodePointer > invertedListLeafNodes;
	vector<unsigned> invertedListIDs;
	unsigned invertedListOffset;
	unsigned cursorOnInvertedList;
};

class UnionLowestLevelSimpleScanOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
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

#endif // __WRAPPER_UNIONLOWESTLEVELSUMPLESCANOPERATOR_H__
