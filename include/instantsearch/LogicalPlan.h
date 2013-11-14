
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

#ifndef __WRAPPER_LOGICALPLAN_H__
#define __WRAPPER_LOGICALPLAN_H__

#include "Constants.h"
#include "instantsearch/Term.h"
#include <vector>
#include <string>
using namespace std;

namespace srch2 {
namespace instantsearch {

/*
 * LogicalPlanNode is the common class used for the logical plan tree operators and operands. And tree is
 * constructed by attaching the instances of this class and the pointer to the root is kept in LogicalPlan.
 */
class LogicalPlanNode{
	// Since constructor is private, only LogicalPlan can allocate space for LogicalPlanNodes.
	friend class LogicalPlan;
public:
	LogicalPlanNodeType nodeType;
	vector<LogicalPlanNode *> children;
	Term * term;

	~LogicalPlanNode();

    // changes a dummy logical plan node to a term node
    void changeToTermLogicalPlanNode(
    		LogicalPlanNode * logicalPlanNode,
    		const std::string &queryKeyword,
    		TermType type,
    		const float boost,
    		const float similarityBoost,
    		const uint8_t threshold,
    		unsigned fieldFilter);

    unsigned getNodeId();

private:
	unsigned nodeId;
	LogicalPlanNode(Term * term);
	LogicalPlanNode(LogicalPlanNodeType nodeType);
	LogicalPlanNode();


};

/*
 * LogicalPlan is the class which maintains the logical plan of query and its metadata. The logical plan is
 * a tree of operators (AND,OR and NOT) and query terms. For example, for the query:
 * q=(John AND hello*) OR (authors:Kundera AND freedom~0.5)
 * the logical plan is :
 *
 *   [OR]_____[AND]_____{John}
 *    |         |
 *    |         |_______{hello, TermType=PREFIX}
 *    |
 *    |_______[AND]_____{Kundera, fieldFilter=Authors}
 *              |
 *              |_______{freedom, similarityThreshold=0.5}
 *
 * Each node in this tree is a LogicalPlanNode.
 */
class LogicalPlan {
private:
	// SearchType
	// PostProcessingInfo
    LogicalPlanNode * tree;
    unsigned logicalPlanNodeId;

public:
    LogicalPlan();
    ~LogicalPlan();

    // constructs a term logical plan node
    LogicalPlanNode * createTermLogicalPlanNode(const std::string &queryKeyword,
    		TermType type,
    		const float boost,
    		const float similarityBoost,
    		const uint8_t threshold,
    		unsigned fieldFilter);
    // constructs an internal (operator) logical plan node
    LogicalPlanNode * createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType);
    // construct a dummy logical plan node which can be used as place_holder.
    LogicalPlanNode * createDummyLogicalPlanNode();

};

}
}

#endif // __WRAPPER_LOGICALPLAN_H__
