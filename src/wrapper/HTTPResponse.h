//$Id: HTTPResponse.h 3149 2013-01-29 00:28:02Z oliverax $

#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "BimapleServerConf.h"
#include "URLParser.h"
#include "json/json.h"
#include "BimapleServer.h"

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

namespace bimaple
{
namespace httpwrapper
{

class HTTPResponse
{
	public:
		static void searchCommand(evhttp_request *req, BimapleServer *server);
		static void infoCommand(evhttp_request *req, BimapleServer *server, const string &versioninfo);
		static void writeCommand_v0(evhttp_request *req, BimapleServer *server);
		static void updateCommand(evhttp_request *req, BimapleServer *server);
		static void saveCommand(evhttp_request *req, BimapleServer *server);
        static void lookupCommand(evhttp_request *req, BimapleServer *server);
		static void writeCommand_v1(evhttp_request *req, BimapleServer *server);
		static void activateCommand(evhttp_request *req, BimapleServer *server);
		
	private:
		static void printResults(evhttp_request *req, const evkeyvalq &headers,
				const URLParserHelper &urlParserHelper,
				const BimapleServerConf *indexDataContainerConf,
				const QueryResults *queryResults,
				const Query *query,
				const bmis::Indexer *indexer,
				const unsigned offset,
				const unsigned nextK,
				const unsigned retrievedResults,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend);
};

}}

#endif // _HTTPRESPONSE_H_
