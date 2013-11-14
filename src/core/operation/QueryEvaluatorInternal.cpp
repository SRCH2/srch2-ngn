
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

#include "operation/QueryEvaluatorInternal.h"
#include <instantsearch/Query.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/TypedValue.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "operation/IndexerInternal.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/ActiveNode.h"
#include "operation/TermVirtualList.h"
#include "query/QueryResultsInternal.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "util/Assert.h"
#include "index/ForwardIndex.h"
#include "geo/QuadTree.h"
#include "CatalogManager.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

namespace srch2
{
namespace instantsearch
{

/**
 * Creates an QueryEvaluatorInternal object.
 * @param indexer - An object holding the index structures and cache.
 */
QueryEvaluatorInternal::QueryEvaluatorInternal(IndexReaderWriter *indexer , QueryEvaluatorRuntimeParametersContainer * parameters = NULL){
	// if parameters is NULL, the default constructor of the object is used which has the default values for everything.
	if(parameters != NULL){
		this->parameters = *parameters;
	}
    this->indexData = dynamic_cast<const IndexData*>(indexer->getReadView(this->indexReadToken));
    this->cacheManager = dynamic_cast<Cache*>(indexer->getCache());
    this->indexer = indexer;
}

/*
 * Finds the suggestions for a keyword based on fuzzyMatchPenalty.
 * Returns the number of suggestions found.
 */
// TODO : FIXME: This function is not compatible with the new api
int QueryEvaluatorInternal::suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions ){

}

/*
 * Returns the estimated number of results
 */
unsigned QueryEvaluatorInternal::estimateNumberOfResults(const LogicalPlan * logicalPlan){

}

/**
 * If the search type is set to TopK in LogicalPlan, this function
 * finds the next topK answers starting from
 * offset. This function can be used to support pagination of
 * search results. queryResults is a QueryResults
 * object, which must be created using the same query object
 * as the first argument in this function.
 *
 * returns the number of records found (at most topK).
 *
 * if search type is getAllResults, this function finds all the results.
 */
int QueryEvaluatorInternal::search(const LogicalPlan * logicalPlan , QueryResults *queryResults){

	/*
	 * 1. Use CatalogManager to collect statistics and meta data about the logical plan
	 * ---- 1.1. computes and attaches active node sets for each term
	 * ---- 1.2. estimates and saves the number of results of each internal logical operator
	 * ---- 1.3. ...
	 */
	CatalogManager catalogManager(this->indexData->forwardIndex , this->indexData->invertedIndex , this->indexData->trie);
	catalogManager.calculateInfoForLogicalPlan(logicalPlan);
	/*
	 * 2. Use QueryOptimizer to build PhysicalPlan and optimize it
	 * ---- 2.1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
	 *           and makes sure inputs and outputs of operators are consistent.
	 * ---- 2.2. Applies optimization rules on the physical plan
	 * ---- 2.3. ...
	 */
	QueryOptimizer queryOptimizer(this->indexData->forwardIndex,
			this->indexData->invertedIndex,
			this->indexData->trie,
			catalogManager,
			logicalPlan);
	PhysicalPlan physicalPlan(this->indexData->forwardIndex,
			this->indexData->invertedIndex,
			this->indexData->trie,
			catalogManager);
	queryOptimizer.buildAndOptimizePhysicalPlan(physicalPlan);
	 /*
	  * 3. Execute physical plan
	 */
	/*
	 * PhysicalPlanExecutor is responsible of executing the PhysicalPlan.
	 * Also it takes care of the following tasks :
	 * 1. Pagination: In the case of topK, it handles pagination by setting K to the right value
	 * ---- which is a function of K and offset.
	 * 2. Post processing: Post processing physical operators are added to the tree by QueryOptimizer.
	 * ---- So there is nothing much to be done here, maybe just passing up the recorded data by some filters
	 * ---- like facet.
	 * 3. Passing the results to QueryResults : This module is also responsible of populating QueryResults from
	 * ---- the outputs of the root operator and make everything inside core transparent to outside layers.
	 * 4. Exact and fuzzy policy : If exactOnly is passed as true to getNext(...) of any operator, that operator
	 * ---- only returns exact results (if not, all the results are returned.) Therefore, the physical plan is executed first
	 * ---- by exactOnly=true, and then if we don't have enough results we re-execute the plan by passing exactOnly=false.
	 * ---- please note that since topK, or GetAll (the search type) is embedded in the structure of PhysicalPlan,
	 * ---- this policy is applied on all search types.
	 */


}

/**
 * Does Map Search
 */
int QueryEvaluatorInternal::search(const Query *query, QueryResults *queryResults){

}

// for doing a geo range query with a circle
void QueryEvaluatorInternal::search(const Circle &queryCircle, QueryResults *queryResults){

}

// for doing a geo range query with a rectangle
void QueryEvaluatorInternal::search(const Rectangle &queryRectangle, QueryResults *queryResults){

}

// for retrieving only one result by having the primary key
void QueryEvaluatorInternal::search(const std::string & primaryKey, QueryResults *queryResults){

}

// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
std::string QueryEvaluatorInternal::getInMemoryData(unsigned internalRecordId) const {

}

void QueryEvaluatorInternal::cacheClear() {

}

}}
