/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
			const AttributeAccessControl & attrAcl);

	bool rewrite(LogicalPlan & logicalPlan);

private:
	const Schema & schema;
	const Analyzer & analyzer ;
	ParsedParameterContainer * paramContainer;
	const CoreInfo_t *indexDataConfig;
	const AttributeAccessControl&  attributeAcl;

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
			ATTRIBUTES_OP& attributeOperation, vector<unsigned> *allowedAttributesForRole);
	void addGeoToParseTree();
};

}
}

#endif // _WRAPPER_QUERYREQRITER_H_

