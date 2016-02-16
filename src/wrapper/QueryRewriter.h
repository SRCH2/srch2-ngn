#ifndef _WRAPPER_QUERYREQRITER_H_
#define _WRAPPER_QUERYREQRITER_H_

#include "ParsedParameterContainer.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Schema.h"
#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"
#include "src/sharding/configuration/ConfigManager.h"
#include "instantsearch/LogicalPlan.h"
#include "operation/AttributeAccessControl.h"
using srch2::instantsearch::Analyzer;
using srch2::instantsearch::Schema;


namespace srch2{

namespace httpwrapper{


class QueryRewriter
{
public:
	QueryRewriter(const CoreInfo_t *indexDataConfig,const Schema & schema,
			const Analyzer & analyzer ,ParsedParameterContainer * paramContainer,
			const vector<unsigned> & accessibleSearchAttrs,
			const vector<unsigned> & accessibleRefiningAttrs);

	bool rewrite(LogicalPlan & logicalPlan);

private:
	const Schema & schema;
	const Analyzer & analyzer ;
	ParsedParameterContainer * paramContainer;
	const CoreInfo_t *indexDataConfig;
	const vector<unsigned> & accessibleSearchAttrs;
	const vector<unsigned> & accessibleRefiningAttrs;

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
	void getFieldFiltersBasedOnAcl(vector<unsigned>& fieldFiltersList,
			ATTRIBUTES_OP& attributeOperation, vector<unsigned> allowedAttributesForRole);
	void addGeoToParseTree();
};

}
}

#endif // _WRAPPER_QUERYREQRITER_H_

