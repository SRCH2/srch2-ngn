//$Id: HTTPResponse.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "Srch2ServerConf.h"
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

#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>

namespace srch2
{
namespace httpwrapper
{

class HTTPResponse
{
	public:
		static void searchCommand(evhttp_request *req, Srch2Server *server);
		static void infoCommand(evhttp_request *req, Srch2Server *server, const string &versioninfo);
		static void writeCommand_v0(evhttp_request *req, Srch2Server *server);
		static void updateCommand(evhttp_request *req, Srch2Server *server);
		static void saveCommand(evhttp_request *req, Srch2Server *server);
        static void lookupCommand(evhttp_request *req, Srch2Server *server);
		static void writeCommand_v1(evhttp_request *req, Srch2Server *server);
		static void activateCommand(evhttp_request *req, Srch2Server *server);
		
	private:
		static void printResults(evhttp_request *req, const evkeyvalq &headers,
				const URLParserHelper &urlParserHelper,
				const Srch2ServerConf *indexDataContainerConf,
				const QueryResults *queryResults,
				const Query *query,
				const srch2is::Indexer *indexer,
				const unsigned offset,
				const unsigned nextK,
				const unsigned retrievedResults,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend);
};

}}

#endif // _HTTPRESPONSE_H_
