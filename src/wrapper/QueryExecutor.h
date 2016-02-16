#ifndef _WRAPPER_QUERYEXECUTOR_H_
#define _WRAPPER_QUERYEXECUTOR_H_

#include "instantsearch/LogicalPlan.h"
#include "instantsearch/QueryResults.h"
#include "Srch2Server.h"
#include "highlighter/Highlighter.h"

using srch2::instantsearch::QueryResults;
using srch2::instantsearch::QueryResultFactory;

namespace srch2{

namespace httpwrapper{


class QueryExecutor
{

public:

	QueryExecutor(LogicalPlan & queryPlan , QueryResultFactory * resultsFactory ,Srch2Server *server, const CoreInfo_t * configuration );

	void execute(QueryResults * finalResults);
	void executeKeywordSearch(QueryResults * finalResults);
	void executeRetrieveById(QueryResults * finalResults);

	void executePostProcessingPlan(Query * query,QueryResults * input, QueryResults *  output);


private:
	LogicalPlan & queryPlan;
	QueryResultFactory * queryResultFactory;
	Srch2Server * server;
	QueryEvaluator * queryEvaluator;
	const CoreInfo_t * configuration;

};

}
}

#endif // _WRAPPER_QUERYEXECUTOR_H_
