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


#ifndef _WRAPPER_QUERYEXECUTOR_H_
#define _WRAPPER_QUERYEXECUTOR_H_

#include "instantsearch/LogicalPlan.h"
#include "instantsearch/QueryResults.h"
#include "Srch2Server.h"

using srch2::instantsearch::QueryResults;
using srch2::instantsearch::QueryResultFactory;

namespace srch2{

namespace httpwrapper{


class QueryExecutor
{

public:

	QueryExecutor(LogicalPlan & queryPlan , QueryResultFactory * resultsFactory ,Srch2Server *server, const CoreInfo_t * configuration );

	void execute(QueryResults * finalResults);
	void executeForDPInternal(QueryResults * finalResults, map<string,string> & inMemoryRecordStrings) ;
	void executeKeywordSearch(QueryResults * finalResults);
	void executeGeo(QueryResults * finalResults);
	void executeRetrieveById(QueryResults * finalResults);

	void executePostProcessingPlan(Query * query,QueryResults * input, QueryResults *  output);


private:
	LogicalPlan & queryPlan;
	QueryResultFactory * queryResultFactory;
	Srch2Server * server;
	QueryEvaluator * queryEvaluator;
	const CoreInfo_t * configuration;

	void fillInMemoryRecordStrings(QueryResults * queryResults, map<string,string> & inMemoryRecordStrings);
};

}
}

#endif // _WRAPPER_QUERYEXECUTOR_H_
