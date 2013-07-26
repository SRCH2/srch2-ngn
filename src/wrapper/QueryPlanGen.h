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


#ifndef _WRAPPER_QUERYPLEANGENERATOR_H_
#define _WRAPPER_QUERYPLEANGENERATOR_H_

#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/FacetedSearchFilter.h>
#include <instantsearch/NonSearchableAttributeExpressionFilter.h>
#include <instantsearch/SortFilter.h>
#include "QueryPlan.h"
#include <algorithm>
#include <sstream>

#include "Srch2ServerConf.h"
using namespace std;



namespace srch2is = srch2::instantsearch;

using srch2is::Query;

namespace srch2{

namespace httpwrapper{

class QueryPlanGen
{
public:

	QueryPlanGen(const ParsedParameterContainer & paramsContainer , const Srch2ServerConf *indexDataContainerConf );


	/*
	 * 1. creates exact and fuzzy queries
	 * 2. Generates the post processing plan
	 */
	QueryPlan generatePlan();

private:
	const ParsedParameterContainer & paramsContainer;
	const Srch2ServerConf *indexDataContainerConf ;

	// creates a post processing plan based on information from Query
	void createPostProcessingPlan(QueryPlan * plan);

	void createExactAndFuzzyQueries(QueryPlan * plan);

	void fillExactAndFuzzyQueriesWithCommonInformation(QueryPlan * plan );
	void createExactAndFuzzyQueriesForTopK(QueryPlan * plan);
	void createExactAndFuzzyQueriesForGetAllTResults(QueryPlan * plan);

	void createExactAndFuzzyQueriesForGeo(QueryPlan * plan);


};
}
}


#endif // _WRAPPER_QUERYPLEANGENERATOR_H_
