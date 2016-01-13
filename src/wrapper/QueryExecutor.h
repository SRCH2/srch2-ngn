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
	void executeKeywordSearch(QueryResults * finalResults);
	void executeRetrieveById(QueryResults * finalResults);


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
