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
//$Id: HTTPResponse.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#ifndef __WRAPPER_HTTPREQUESTHANDLER_H__
#define __WRAPPER_HTTPREQUESTHANDLER_H__

#include "src/sharding/configuration/ConfigManager.h"
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
        static void searchAllCommand(evhttp_request *req, const CoreNameServerMap_t * coreNameServerMap);
        static void suggestCommand(evhttp_request *req, Srch2Server *server);
        static void infoCommand(evhttp_request *req, Srch2Server *server, const string &versioninfo);
        static void writeCommand(evhttp_request *req, Srch2Server *server);
        static void updateCommand(evhttp_request *req, Srch2Server *server);
        static void saveCommand(evhttp_request *req, Srch2Server *server);
        static void shutdownCommand(evhttp_request *req, const CoreNameServerMap_t * coreNameServerMap);
        static void exportCommand(evhttp_request *req, Srch2Server *server);
        static void resetLoggerCommand(evhttp_request *req, Srch2Server *server);
        static void lookupCommand(evhttp_request *req, Srch2Server *server);
        static void handleException(evhttp_request *req);
        static void attributeAclModify(evhttp_request *req, Srch2Server *server);
        static void aclRecordRoleReplace(evhttp_request *req, Srch2Server *server);
        static void aclRecordRoleAppend(evhttp_request *req, Srch2Server *server);
        static void aclRecordRoleDelete(evhttp_request *req, Srch2Server *server);
        static void aclAddRecordsForRole(evhttp_request *req, Srch2Server *server);
        static void aclAppendRecordsForRole(evhttp_request *req, Srch2Server *server);
        static void aclDeleteRecordsForRole(evhttp_request *req, Srch2Server *server);
        static void processFeedback(evhttp_request *req, Srch2Server *server);

	private:

        static boost::shared_ptr<Json::Value> doSearchOneCore(evhttp_request *req,Srch2Server *server, 
                evkeyvalq* headers, std::stringstream &errorStream) ;

		static boost::shared_ptr<Json::Value> printResults(evhttp_request *req, const evkeyvalq &headers,
				const LogicalPlan &queryPlan,
				const CoreInfo_t *indexDataConfig,
				const QueryResults *queryResults,
				const Query *query,
				const srch2is::Indexer *indexer,
				const unsigned offset,
				const unsigned nextK,
				const unsigned retrievedResults,
				const string & aclRoleId,
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
				const string & aclRoleId,
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
				const string& externalId, string& sbuffer, const string& aclRoleId);
		static void genRecordJsonString(const srch2is::Indexer *indexer, StoredRecordBuffer buffer,
				const string& externalId, string& sbuffer,const vector<string>* attrToReturn, const string& aclRoleId);
		static void genSnippetJSONString(unsigned recIdx, unsigned start,
				const vector<RecordSnippet>& recordSnippets, string& sbuffer,
				const QueryResults *queryResults);
		static void aclModifyRecordsForRole(evhttp_request *req, Srch2Server *server, srch2::instantsearch::RecordAclCommandType commandType);
		static void aclModifyRolesForRecord(evhttp_request *req, Srch2Server *server, srch2::instantsearch::RecordAclCommandType commandType);
		static bool processSingleFeedback(const Json::Value& doc,
				Srch2Server *server, Json::Value& feedbackResponse);
};

}
}

#endif // _WRAPPER_HTTPREQUESTHANDLER_H_
