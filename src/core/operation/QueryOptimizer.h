
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

#ifndef __WRAPPER_QUERYOPTIMIZER_H__
#define __WRAPPER_QUERYOPTIMIZER_H__

#include "instantsearch/Constants.h"
#include "operation/CatalogManager.h"
#include "physical_plan/PhysicalPlan.h"
#include "instantsearch/LogicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

/*
 * This class is responsible of doing two tasks :
 * 1. Translating LogicalPlan and building PhysicalPlan from it
 * 2. Applying rules on PhysicalPlan and optimizing it
 *
 * The physical plan built by this class is then executed to procduce the result set.
 */
class QueryOptimizer {
public:
	QueryOptimizer(ForwardIndex * forwardIndex ,
			InvertedIndex * invertedIndex,
			Trie * trie,
			CatalogManager * catalogManager,
			const LogicalPlan * logicalPlan);

	/*
	 *
	 * this function builds PhysicalPlan and optimizes it
	 * ---- 2.1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
	 *           and makes sure inputs and outputs of operators are consistent.
	 * ---- 2.2. Applies optimization rules on the physical plan
	 */
	void buildAndOptimizePhysicalPlan(PhysicalPlan & physicalPlan);

private:

	/*
	 * This function maps LogicalPlan nodes to Physical nodes and builds a very first
	 * version of the PhysicalPlan. This plan will optimizer in next steps.
	 */
	void buildPhysicalPlanFirstVersion(PhysicalPlan & physicalPlan);

	/*
	 * this function applies optimization rules (funtions starting with Rule_) on the plan one by one
	 */
	void applyOptimizationRulesOnThePlan(PhysicalPlan & physicalPlan);

	/*
	 * An example of an optimization rule function.
	 */
	void Rule_1(PhysicalPlan & physicalPlan);

	ForwardIndex * forwardIndex;
	InvertedIndex * invertedIndex;
	Trie * trie;
	CatalogManager * catalogManager;
	const LogicalPlan * logicalPlan;
};

}
}

#endif //__WRAPPER_QUERYOPTIMIZER_H__
