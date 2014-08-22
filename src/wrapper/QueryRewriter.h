// $Id$ 07/11/13 Jamshid


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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */


#ifndef _WRAPPER_QUERYREQRITER_H_
#define _WRAPPER_QUERYREQRITER_H_

#include "ParsedParameterContainer.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Schema.h"
#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"
#include "ConfigManager.h"
#include "instantsearch/LogicalPlan.h"

using srch2::instantsearch::Analyzer;
using srch2::instantsearch::Schema;


namespace srch2{

namespace httpwrapper{


class QueryRewriter
{
public:
	QueryRewriter(const CoreInfo_t *indexDataConfig,const Schema & schema, const Analyzer & analyzer ,ParsedParameterContainer * paramContainer);

	bool rewrite(LogicalPlan & logicalPlan);

private:
	const Schema & schema;
	const Analyzer & analyzer ;
	ParsedParameterContainer * paramContainer;
	const CoreInfo_t *indexDataConfig;

	void prepareKeywordInfo();
	bool applyAnalyzer();
	// this function creates the bit sequence needed for field filter based on the filter names
	void prepareFieldFilters();
	void prepareFacetFilterInfo();

	void prepareLogicalPlan(LogicalPlan & logicalPlan);
	void rewriteParseTree();
	void buildLogicalPlan(LogicalPlan & logicalPlan);
	LogicalPlanNode * buildLogicalPlan(ParseTreeNode * root, LogicalPlan & logicalPlan);
	void createExactAndFuzzyQueries(LogicalPlan & plan);
	void fillExactAndFuzzyQueriesWithCommonInformation(LogicalPlan & plan);
	void createExactAndFuzzyQueriesForTopK(LogicalPlan & plan);
	void createExactAndFuzzyQueriesForGetAllTResults(LogicalPlan & plan) ;
	void createPostProcessingPlan(LogicalPlan & plan) ;
	void addGeoToParseTree();
};

}
}

#endif // _WRAPPER_QUERYREQRITER_H_

