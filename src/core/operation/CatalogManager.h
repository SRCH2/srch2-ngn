
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
#include "util/Assert.h"

#include "map"


using namespace std;

namespace srch2
{
namespace instantsearch
{
/*
 * this class is responsible of calculating and maintaining catalog information.
 * Specifically, CatalogManager calculates and keeps required information for
 * calculating PhysicalPlan costs, upon receiving the LogicalPlan. The CatalogManager
 * maintains multiple maps to keep these pieces of information and does not put this info
 * in the LogicalPlan. For this reason, each LogicalPlanNode must have an ID so that CatalogManager
 * uses this ID to keep the information about that node.
 */
class CatalogManager {
public:
	CatalogManager(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie);
	~CatalogManager();

	// calculates required information for building physical plan.
	// this information is accessible later by passing nodeId of LogicalPlanNodes.
	void calculateInfoForLogicalPlan(const LogicalPlan * logicalPlan);

	// returns the estimated number of results for a
	// LogicalPlanNode that its id is nodeId.
	// returns false if this id is not found.
	bool getEstimatedNumberOfResults(unsigned nodeId, unsigned & estimate);
private:
	ForwardIndex * forwardIndex;
	InvertedIndex * invertedIndex;
	Trie * trie;

	// this map maintains the estimated number of results for each LogicalPlanNode
	// It's a map from nodeId to the estimated number
	map<unsigned, unsigned> estimatedNumberOfResults;
};

}
}

#endif // __CATALOGMANAGER_H__
