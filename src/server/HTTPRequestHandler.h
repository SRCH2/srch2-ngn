//$Id: HTTPResponse.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#ifndef __WRAPPER_HTTPREQUESTHANDLER_H__
#define __WRAPPER_HTTPREQUESTHANDLER_H__

#include "ConfigManager.h"
#include "json/json.h"
#include "Srch2Server.h"

#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/GlobalCache.h>

#include "query/QueryResultsInternal.h"
#include <instantsearch/QueryEvaluator.h>
#include "operation/QueryEvaluatorInternal.h"
#include "QueryPlan.h"

#include "ParsedParameterContainer.h" // this include is to use enum ParameterName, after fixing the constant problem it must change
#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include "highlighter/Highlighter.h"

namespace srch2
{
namespace httpwrapper
{
// named access to multiple "cores" (ala Solr)
typedef std::map<const std::string, srch2http::Srch2Server *> CoreNameServerMap_t;

class HTTPRequestHandler
{
    public:

        static void searchCommand(evhttp_request *req, Srch2Server *server);
        static void searchAllCommand(evhttp_request *req, CoreNameServerMap_t * coreNameServerMap);
        static void suggestCommand(evhttp_request *req, Srch2Server *server);
        static void infoCommand(evhttp_request *req, Srch2Server *server, const string &versioninfo);
        static void writeCommand(evhttp_request *req, Srch2Server *server);
        static void updateCommand(evhttp_request *req, Srch2Server *server);
        static void saveCommand(evhttp_request *req, Srch2Server *server);
        static void shutdownCommand(evhttp_request *req, CoreNameServerMap_t * coreNameServerMap);
        static void exportCommand(evhttp_request *req, Srch2Server *server);
        static void resetLoggerCommand(evhttp_request *req, Srch2Server *server);
        static void lookupCommand(evhttp_request *req, Srch2Server *server);
		static void handleException(evhttp_request *req);

	private:

        static boost::shared_ptr<Json::Value> doSearchOneCore(evhttp_request *req,
            Srch2Server *server, evkeyvalq * headers,ParsedParameterContainer *paramContainer ) ;
		static boost::shared_ptr<Json::Value> printResults(evhttp_request *req, const evkeyvalq &headers,
				const LogicalPlan &queryPlan,
				const CoreInfo_t *indexDataConfig,
				const QueryResults *queryResults,
				const Query *query,
				const srch2is::Indexer *indexer,
				const unsigned offset,
				const unsigned nextK,
				const unsigned retrievedResults,
				const string & message,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend,
				const vector<RecordSnippet>& recordSnippets, unsigned hltime,
				bool onlyFacets = false
				);

		static boost::shared_ptr<Json::Value> printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
				const LogicalPlan &queryPlan,
				const CoreInfo_t *indexDataConfig,
				const QueryResults *queryResults,
				const srch2is::Indexer *indexer,
				const string & message,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend);

		static void printSuggestions(evhttp_request *req, const evkeyvalq &headers,
				const vector<string> & suggestions,
				const srch2is::Indexer *indexer,
				const string & message,
				const unsigned ts1,
				struct timespec &tstart, struct timespec &tend);
		static void cleanAndAppendToBuffer(const string& in, string& out);
		static void genRecordJsonString(const srch2is::Indexer *indexer, StoredRecordBuffer buffer,
				const string& externalId, string& sbuffer);
		static void genRecordJsonString(const srch2is::Indexer *indexer, StoredRecordBuffer buffer,
				const string& externalId, string& sbuffer,const vector<string>* attrToReturn);
		static void genSnippetJSONString(unsigned recIdx, unsigned start,
				const vector<RecordSnippet>& recordSnippets, string& sbuffer,
				const QueryResults *queryResults);
};

}
}

#endif // _WRAPPER_HTTPREQUESTHANDLER_H_
