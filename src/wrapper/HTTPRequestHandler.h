//$Id: HTTPResponse.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#ifndef __WRAPPER_HTTPREQUESTHANDLER_H__
#define __WRAPPER_HTTPREQUESTHANDLER_H__

#include "ConfigManager.h"
#include "URLParser.h"
#include "json/json.h"
#include "Srch2Server.h"

#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/IndexSearcher.h>

#include "query/QueryResultsInternal.h"
#include "operation/IndexSearcherInternal.h"

#include "QueryPlan.h"

#include "ParsedParameterContainer.h" // this include is to use enum ParameterName, after fixing the constant problem it must change

#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>

namespace srch2
{
namespace httpwrapper
{

class HTTPRequestHandler
{
    public:
        static void searchCommand(evhttp_request *req, Srch2Server *server);
        static void infoCommand(evhttp_request *req, Srch2Server *server, const string &versioninfo);
        static void writeCommand_v0(evhttp_request *req, Srch2Server *server);
        static void updateCommand(evhttp_request *req, Srch2Server *server);
        static void saveCommand(evhttp_request *req, Srch2Server *server);
        static void exportCommand(evhttp_request *req, Srch2Server *server);
        static void lookupCommand(evhttp_request *req, Srch2Server *server);
		static void writeCommand_v1(evhttp_request *req, Srch2Server *server);
		static void activateCommand(evhttp_request *req, Srch2Server *server);

	private:

		static void printResults(evhttp_request *req, const evkeyvalq &headers,
				const QueryPlan &queryPlan,
				const ConfigManager *indexDataContainerConf,
				const QueryResults *queryResults,
				const Query *query,
				const srch2is::Indexer *indexer,
				const unsigned offset,
				const unsigned nextK,
				const unsigned retrievedResults,
				const string & message,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend);
};

}}

#endif // _WRAPPER_HTTPREQUESTHANDLER_H_
