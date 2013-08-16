//$Id: HTTPResponse.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <set>

#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"

#include "HTTPResponse.h"
#include "IndexWriteUtil.h"

#include <event2/http.h>
#define SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS 2

namespace srch2is = srch2::instantsearch;
using srch2is::Analyzer;
using srch2is::QueryResultsInternal;
using srch2is::QueryResults;

using namespace snappy;

namespace srch2
{
namespace httpwrapper
{


/**
 * Create evbuffer. If failed, send 503 response.
 * @param req request
 * @return buffer
 */
evbuffer *create_buffer(evhttp_request *req) {
  evbuffer *buf = evbuffer_new();
  if (!buf) {
    fprintf(stderr, "Failed to create response buffer\n");
    evhttp_send_reply(req, HTTP_SERVUNAVAIL,
                      "Failed to create response buffer", NULL);
    return NULL;
  }
  return buf;
}

void bmhelper_check_add_callback(evbuffer *buf, const evkeyvalq &headers, const string &out_payload)
{
    const char *jsonpCallBack = evhttp_find_header(&headers, URLParser::jsonpCallBackName);
    if (jsonpCallBack)
    {
        size_t sz;
        char *jsonpCallBack_cstar = evhttp_uridecode(jsonpCallBack, 0, &sz);
        //std::cout << "[" << jsonpCallBack_cstar << "]" << std::endl;
        
        evbuffer_add_printf(buf, "%s(%s)", jsonpCallBack_cstar, out_payload.c_str());
        
        delete jsonpCallBack_cstar;    
    }
    else
    {
        evbuffer_add_printf(buf, "%s", out_payload.c_str());    
    }
}

void bmhelper_add_content_length(evhttp_request *req, evbuffer *buf)
{
    size_t length = EVBUFFER_LENGTH(buf);
    std::stringstream length_str;
    length_str << length;
    evhttp_add_header(req->output_headers, "Content-Length", length_str.str().c_str());
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code, const char *reason, const string &out_payload, const evkeyvalq &headers)
{
    evbuffer *returnbuffer = create_buffer(req);    
    bmhelper_check_add_callback(returnbuffer, headers, out_payload);
    bmhelper_add_content_length(req, returnbuffer);
    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code, const char *reason, const string &out_payload)
{
    evbuffer *returnbuffer = create_buffer(req);

    evbuffer_add_printf(returnbuffer, "%s", out_payload.c_str());
    bmhelper_add_content_length(req, returnbuffer);

    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void HTTPResponse::printResults( evhttp_request *req, const evkeyvalq &headers,
        const URLParserHelper &urlParserHelper,
        const ConfigManager *indexDataContainerConf,
        const QueryResults *queryResults,
        const Query *query,
        const Indexer *indexer,
        const unsigned start,
        const unsigned end,
        const unsigned retrievedResults,
        const unsigned ts1,
        struct timespec &tstart, struct timespec &tend)
{
    Json::FastWriter writer;
    Json::Value root;

    // For logging
    string logQueries;

    root["searcher_time"] = ts1;
    root["results"].resize(end-start);

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;
    if(urlParserHelper.searchType==SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS&&query->getQueryTerms()->empty()) //check if the query type is range query without keywords
    {
        for(unsigned i = start; i < end; ++i)
        {
            root["results"][counter]["record_id"] = queryResults->getRecordId(i);
            root["results"][counter]["score"] = (0-queryResults->getResultScore(i));//the actual distance between the point of record and the center point of the range
            if (indexDataContainerConf->getSearchResponseFormat() == 0
                    || indexDataContainerConf->getSearchResponseFormat() == 2)
            {
                unsigned internalRecordId = queryResults->getInternalRecordId(i);
                std::string compressedInMemoryRecordString = indexer->getInMemoryData(internalRecordId);

                std::string uncompressedInMemoryRecordString;

                snappy::Uncompress(compressedInMemoryRecordString.c_str(), compressedInMemoryRecordString.size(), &uncompressedInMemoryRecordString);

                Json::Value in_mem_String;
                Json::Reader reader;
                reader.parse(uncompressedInMemoryRecordString, in_mem_String, false);
                root["results"][counter]["record"] = in_mem_String;
            }
            ++counter;
        }

    }
    else // the query is including keywords:(1)only keywords (2)keywords+geo
    {
        for(unsigned i = start; i < end; ++i)
        {

            root["results"][counter]["record_id"] = queryResults->getRecordId(i);
            root["results"][counter]["score"] = queryResults->getResultScore(i);

            // print edit distance vector
            vector<unsigned> editDistances;
            queryResults->getEditDistances(i, editDistances);

            root["results"][counter]["edit_dist"].resize(editDistances.size());
            for(unsigned int j = 0; j < editDistances.size(); ++j)
            {
                root["results"][counter]["edit_dist"][j] = editDistances[j];
            }

            // print matching keywords vector
            vector<std::string> matchingKeywords;
            queryResults->getMatchingKeywords(i, matchingKeywords);

            root["results"][counter]["matching_prefix"].resize(matchingKeywords.size());
            for(unsigned int j = 0; j < matchingKeywords.size(); ++j)
            {
                root["results"][counter]["matching_prefix"][j] = matchingKeywords[j];
            }

            if (indexDataContainerConf->getSearchResponseFormat() == 0
                    || indexDataContainerConf->getSearchResponseFormat() == 2)
            {
                unsigned internalRecordId = queryResults->getInternalRecordId(i);
                std::string compressedInMemoryRecordString = indexer->getInMemoryData(internalRecordId);

                std::string uncompressedInMemoryRecordString;

                snappy::Uncompress(compressedInMemoryRecordString.c_str(), compressedInMemoryRecordString.size(), &uncompressedInMemoryRecordString);

                Json::Value in_mem_String;
                Json::Reader reader;
                reader.parse(uncompressedInMemoryRecordString, in_mem_String, false);
                root["results"][counter]["record"] = in_mem_String;
            }
            ++counter;
        }
        root["query_keywords"].resize(query->getQueryTerms()->size());
        for(unsigned i = 0; i < query->getQueryTerms()->size(); i++)
        {
            string &term = *(query->getQueryTerms()->at(i)->getKeyword());
            root["query_keywords"][i] = term;
            if(i)
                logQueries += "";
            logQueries += term;
        }

        root["fuzzy"] = (int)urlParserHelper.isFuzzy;
    }
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    root["payload_access_time"] = ts2;

    // return some meta data

    root["type"] = urlParserHelper.searchType;
    root["offset"] = start;
    root["limit"] = end - start;

    root["results_found"] = retrievedResults;

    if (urlParserHelper.searchType==1)
    {
        root["sortby"] = urlParserHelper.sortby;
        root["order"] = (int)urlParserHelper.order;
    }

    Logger::info("ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms", req->remote_host, req->remote_port, req->uri+1, ts1, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root) , headers);
    
}

void HTTPResponse::writeCommand_v0(evhttp_request *req, Srch2Server *server)
{
    /* Yes, we are expecting a post request */
    switch (req->type)
    {
        case EVHTTP_REQ_PUT:
        {
            size_t length = EVBUFFER_LENGTH(req->input_buffer);

            if (length == 0)
            {
                bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",  "{\"message\":\"http body is empty\"}" );
                Logger::warn("http body is empty");
                break;
            }

            const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

            //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

            std::stringstream log_str;

            // Parse example data
            Json::Value root;
            Json::Reader reader;
            bool parseSuccess = reader.parse(post_data, root, false);

            if(parseSuccess == false)
            {
                log_str << "JSON object parse error";
            }
            else
            {
                Record *record = new Record(server->indexer->getSchema());

                //for ( int index = 0; index < root.size(); ++index )  // Iterates over the sequence elements.
                //{
                    const Json::Value doc = root;
                    IndexWriteUtil::_insertCommand(server->indexer, server->indexDataContainerConf, doc, 0, record, log_str);
                    record->clear();
                //}
                delete record;
            }
            //std::cout << log_str.str() << std::endl;
            Logger::info("%s", log_str.str().c_str());

            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The batch was processed successfully\",\"log\":["+log_str.str()+"]}\n" );
            break;
        }
        case EVHTTP_REQ_DELETE:
        {
            std::stringstream log_str;

            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);

            IndexWriteUtil::_deleteCommand_QueryURI(server->indexer, server->indexDataContainerConf, headers, 0, log_str);

            Logger::info("%s", log_str.str().c_str());
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The batch was processed successfully\",\"log\":["+log_str.str()+"]}\n" );

            // Free the objects
            evhttp_clear_headers(&headers);
            break;
        }
        default:
        {
            Logger::error("error: The request has an invalid or missing argument. See Srch2 API documentation for details");
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        }
    };
}

void HTTPResponse::updateCommand(evhttp_request *req, Srch2Server *server)
{
    /* Yes, we are expecting a post request */
    switch (req->type)
    {
        case EVHTTP_REQ_PUT:
        {
            size_t length = EVBUFFER_LENGTH(req->input_buffer);

            if (length == 0)
            {
                bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",  "{\"message\":\"http body is empty\"}" );
                Logger::warn("http body is empty");
                break;
            }

            const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

            //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

            std::stringstream log_str;

            // Parse example data
            Json::Value root;
            Json::Reader reader;
            bool parseSuccess = reader.parse(post_data, root, false);

            if(parseSuccess == false)
            {
                log_str << "JSON object parse error";
            }
            else
            {
                evkeyvalq headers;
                evhttp_parse_query(req->uri, &headers);

                Record *record = new Record(server->indexer->getSchema());
                const Json::Value doc = root;

                IndexWriteUtil::_updateCommand(server->indexer, server->indexDataContainerConf, headers, doc, 0, record, log_str);

                record->clear();
                delete record;

                evhttp_clear_headers(&headers);
            }
            //std::cout << log_str.str() << std::endl;
            Logger::info("%s", log_str.str().c_str());

            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The batch was processed successfully\",\"log\":["+log_str.str()+"]}\n" );
            
            break;
        }
        default:
        {
            Logger::error("The request has an invalid or missing argument. See Srch2 API documentation for details");
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        }
    };
}

void HTTPResponse::writeCommand_v1(evhttp_request *req, Srch2Server *server)
{
    /* Yes, we are expecting a post request */
    switch (req->type)
    {
        case EVHTTP_REQ_PUT:
        {
            size_t length = EVBUFFER_LENGTH(req->input_buffer);

            if (length == 0)
            {
                bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",  "{\"message\":\"http body is empty\"}" );
                Logger::warn("http body is empty");
                break;
            }

            const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

            //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

            std::stringstream log_str;
            // Parse example data
            Json::Value root;
            Json::Reader reader;
            bool parseSuccess = reader.parse(post_data, root, false);

            if(parseSuccess == false)
            {
                log_str << "JSON object parse error";
            }
            else
            {
                if (root.type() == Json::arrayValue)
                {
                    Record *record = new Record(server->indexer->getSchema());
                    for ( int index = 0; index < root.size(); ++index )  // Iterates over the sequence elements.
                    {
                        Json::Value defaultValueToReturn = Json::Value("");
                        const Json::Value doc = root.get(index, defaultValueToReturn);

                        /*Json::FastWriter writer;
                        std::cout << "[" << writer.write(doc)  << "]" << std::endl;*/

                        IndexWriteUtil::_insertCommand(server->indexer, server->indexDataContainerConf, doc, 0, record, log_str);
                        record->clear();

                        if (index < root.size() - 1)
                            log_str << ",";
                    }
                    delete record;
                }
            }
            //std::cout << log_str.str() << std::endl;
            Logger::info("%s", log_str.str().c_str());
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The batch was processed successfully\",\"log\":["+log_str.str()+"]}\n" );
            break;
        }
        case EVHTTP_REQ_DELETE:
        {
            std::stringstream log_str;

            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);

            IndexWriteUtil::_deleteCommand_QueryURI(server->indexer, server->indexDataContainerConf, headers, 0, log_str);

            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The batch was processed successfully\",\"log\":["+log_str.str()+"]}\n" );

            // Free the objects
            evhttp_clear_headers(&headers);

            Logger::info("%s", log_str.str().c_str());

            break;
        }
        default:
        {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
            Logger::error("The request has an invalid or missing argument. See Srch2 API documentation for details");
        }
    };
}

void HTTPResponse::activateCommand(evhttp_request *req, Srch2Server *server)
{
    /* Yes, we are expecting a post request */
    switch (req->type)
    {
        case EVHTTP_REQ_PUT:
        {
            //size_t length = EVBUFFER_LENGTH(req->input_buffer);
            //const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

            std::stringstream log_str;
            IndexWriteUtil::_commitCommand(server->indexer, server->indexDataContainerConf, 0, log_str);

            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The initialization phase has started successfully\", \"log\":["+log_str.str()+"]}\n" );
            Logger::info("%s", log_str.str().c_str());
            break;
        }
        default:
        {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
            Logger::error("The request has an invalid or missing argument. See Srch2 API documentation for details");
        }
    };
}

void HTTPResponse::saveCommand(evhttp_request *req, Srch2Server *server)
{
    /* Yes, we are expecting a post request */
    switch (req->type)
    {
        case EVHTTP_REQ_PUT:
        {
            std::stringstream log_str;
            IndexWriteUtil::_saveCommand(server->indexer, log_str);

            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",  "{\"message\":\"The index has been saved to disk successfully\", \"log\":["+log_str.str()+"]}\n" );
            Logger::info("%s", log_str.str().c_str());
            break;
        }
        default:
        {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
            Logger::error("The request has an invalid or missing argument. See Srch2 API documentation for details");
        }
    };
}

void HTTPResponse::infoCommand(evhttp_request *req, Srch2Server *server, const string &versioninfo)
{
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    string combinedInfo = "[" + server->indexer->getIndexHealth() + ", " + versioninfo + "]";

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", combinedInfo, headers);
    evhttp_clear_headers(&headers);
}

void HTTPResponse::lookupCommand(evhttp_request *req, Srch2Server *server)
{
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    const ConfigManager *indexDataContainerConf = server->indexDataContainerConf;
    string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());

    std::stringstream response_msg;

    if (pKeyParamName)
    {
        size_t sz;
        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
        delete pKeyParamName_cstar;

        response_msg << "{\"rid\":\"" << primaryKeyStringValue << "\",\"lookup\":\"";

        //lookup the record on the index
        switch(server->indexer->lookupRecord(primaryKeyStringValue))
        {
            case LU_ABSENT_OR_TO_BE_DELETED:
            {
                response_msg << "absent or to be deleted\"}";
                break;
            }
            case LU_TO_BE_INSERTED:
            {
                response_msg << "to be inserted\"}";
                break;
            }
            default: // LU_PRESENT_IN_READVIEW_AND_WRITEVIEW
            {
                response_msg << "present in readview and writeview\"}";
            }
        };
    }
    else
    {
        response_msg << "{\"rid\":\"NULL\",\"lookup\":\"failed\",\"reason\":\"no record with given primary key\"}";
    }

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", response_msg.str(), headers);
    evhttp_clear_headers(&headers);
}

void HTTPResponse::searchCommand(evhttp_request *req, Srch2Server *server)
{
    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);

    const ConfigManager *indexDataContainerConf = server->indexDataContainerConf;
    const Analyzer *analyzer = server->indexer->getAnalyzer();

    URLParserHelper urlParserHelper;

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);
    
    // CHENLI: DEBUG
    //std::cout << "[" << req->uri << "]" << std::endl;

    URLToDoubleQuery *urlToDoubleQuery = new URLToDoubleQuery(headers, analyzer, indexDataContainerConf, server->indexer->getSchema(), urlParserHelper);

    //urlParserHelper.print();
    //evhttp_clear_headers(&headers);

    if (urlParserHelper.parserSuccess)
    {
        int idsExactFound = 0;
        srch2is::IndexSearcher *indexSearcher = srch2is::IndexSearcher::create(server->indexer);

        //do the search
        switch(urlParserHelper.searchType)
        {
        case 0://TopK
        {
            if (indexDataContainerConf->getIndexType() != 0)
            {
                evhttp_send_reply(req, HTTP_BADREQUEST, "Query type is wrong", NULL);
            }
            else
            {

              srch2is::QueryResults *exactQueryResults = srch2is::QueryResults::create(indexSearcher, urlToDoubleQuery->exactQuery);
              idsExactFound = indexSearcher->search(urlToDoubleQuery->exactQuery, exactQueryResults, 0, urlParserHelper.offset + urlParserHelper.resultsToRetrieve);

                //fill visitedList
                std::set<std::string> exactVisitedList;
                for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
                {
                    exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
                }

                int idsFuzzyFound = 0;

                if ( urlParserHelper.isFuzzy && (idsExactFound < (int)(urlParserHelper.resultsToRetrieve + urlParserHelper.offset)))
                {
                    QueryResults *fuzzyQueryResults = QueryResults::create(indexSearcher, urlToDoubleQuery->fuzzyQuery);
                    idsFuzzyFound = indexSearcher->search(urlToDoubleQuery->fuzzyQuery, fuzzyQueryResults, 0, urlParserHelper.offset + urlParserHelper.resultsToRetrieve);
                    // create final queryResults to print.

                    QueryResultsInternal *exact_qs = dynamic_cast<QueryResultsInternal *>(exactQueryResults);
                    QueryResultsInternal *fuzzy_qs = dynamic_cast<QueryResultsInternal *>(fuzzyQueryResults);

                    unsigned fuzzyQueryResultsIter = 0;

                    while (exact_qs->sortedFinalResults.size() < (unsigned)(urlParserHelper.offset + urlParserHelper.resultsToRetrieve)
                            && fuzzyQueryResultsIter < fuzzyQueryResults->getNumberOfResults())
                    {
                        std::string recordId = fuzzy_qs->getRecordId(fuzzyQueryResultsIter);
                        if ( ! exactVisitedList.count(recordId) )// recordid not there
                        {
                            exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[fuzzyQueryResultsIter]);
                        }
                        fuzzyQueryResultsIter++;
                    }
                    delete fuzzyQueryResults;
                }

                // compute elapsed time in ms
                struct timespec tend;
                clock_gettime(CLOCK_REALTIME, &tend);
                unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

                //std::stringstream search_time;
                //search_time << req->uri << "|" << idsExactFound <<"[executed in " << ts1 << " ms]";

                //std::cout << search_time.str() << std::endl;
                //TODO: Logging
                //cry_wrapper(conn, search_time.str().c_str());

                unsigned idsFound = exactQueryResults->getNumberOfResults();

                exactQueryResults->printStats();

                HTTPResponse::printResults(req, headers, urlParserHelper, indexDataContainerConf, exactQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
                        urlParserHelper.offset, idsFound, idsFound, ts1, tstart, tend);

                delete exactQueryResults;
            }
        }
        break;

        case 1://GetAllResults
        {
            if (indexDataContainerConf->getIndexType() != 0)
            {
                evhttp_send_reply(req, HTTP_BADREQUEST, "Query type is wrong", NULL);
            }
            else
            {

              srch2is::QueryResults *queryResults = NULL;
              unsigned idsFound = 0;
              
              if ( !urlParserHelper.isFuzzy )
              {
                queryResults = srch2is::QueryResults::create(indexSearcher, urlToDoubleQuery->exactQuery);
                idsFound = indexSearcher->search(urlToDoubleQuery->exactQuery, queryResults, 0);
              }
              else
                {
                    queryResults = QueryResults::create(indexSearcher, urlToDoubleQuery->fuzzyQuery);
                    idsFound = indexSearcher->search(urlToDoubleQuery->fuzzyQuery, queryResults, 0);
                }

                // compute elapsed time in ms
                struct timespec tend;
                clock_gettime(CLOCK_REALTIME, &tend);
                unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

                //std::stringstream search_time;
                //search_time << req->uri << "|" << idsExactFound <<"[executed in " << ts1 << " ms]";

                //std::cout << search_time.str() << std::endl;
                //TODO: Logging
                //cry_wrapper(conn, search_time.str().c_str());

                queryResults->printStats();

                if (urlParserHelper.offset + urlParserHelper.resultsToRetrieve  > idsFound) // Case where you have return 10,20, but we got only 0,15 results.
                {
                    HTTPResponse::printResults(req, headers, urlParserHelper, indexDataContainerConf, queryResults, urlToDoubleQuery->exactQuery, server->indexer,
                            urlParserHelper.offset, idsFound, idsFound, ts1, tstart, tend);
                }
                else // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
                {
                    HTTPResponse::printResults(req, headers, urlParserHelper, indexDataContainerConf, queryResults, urlToDoubleQuery->exactQuery, server->indexer,
                            urlParserHelper.offset, urlParserHelper.offset + urlParserHelper.resultsToRetrieve, idsFound, ts1, tstart, tend);
                }

                delete queryResults;
            }
        }
        break;

        case 2://MapQuery
        {
            if (indexDataContainerConf->getIndexType() != 1)
            {
                bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", "{\"error\":\"query type is wrong\"}", headers);
            }
            else
            {
                // for the range query without keywords.
                srch2is::QueryResults *exactQueryResults = srch2is::QueryResults::create(indexSearcher, urlToDoubleQuery->exactQuery);
                if(urlToDoubleQuery->exactQuery->getQueryTerms()->empty())//check if query type is a range query without keywords
                {
                    vector<double> values;
                    urlToDoubleQuery->exactQuery->getRange(values);//get  query range: use the number of values to decide if it is rectangle range or circle range
                    //range query with a circle
                    if (values.size()==3)
                    {
                        Point p;
                        p.x = values[0];
                        p.y = values[1];
                        Circle *circleRange = new Circle(p, values[2]);
                        indexSearcher->search(*circleRange, exactQueryResults);
                        delete circleRange;
                    }
                    else
                    {
                        pair<pair<double, double>, pair<double, double> > rect;
                        rect.first.first = values[0];
                        rect.first.second = values[1];
                        rect.second.first = values[2];
                        rect.second.second = values[3];
                        Rectangle *rectangleRange = new Rectangle(rect);
                        indexSearcher->search(*rectangleRange, exactQueryResults);
                        delete rectangleRange;
                    }
                }
                else// keywords and geo search
                {
                    //cout << "reached map query" << endl;
                    //srch2is::QueryResults *exactQueryResults = srch2is::QueryResults::create(indexSearcher, urlToDoubleQuery->exactQuery);
                    indexSearcher->search(urlToDoubleQuery->exactQuery, exactQueryResults);
                    idsExactFound = exactQueryResults->getNumberOfResults();

                    //fill visitedList
                    std::set<std::string> exactVisitedList;
                    for(unsigned i = 0; i < exactQueryResults->getNumberOfResults(); ++i)
                    {
                        exactVisitedList.insert(exactQueryResults->getRecordId(i));// << queryResults->getRecordId(i);
                    }

                    int idsFuzzyFound = 0;

                    if ( urlParserHelper.isFuzzy && idsExactFound < (int)(urlParserHelper.resultsToRetrieve+urlParserHelper.offset))
                    {
                        QueryResults *fuzzyQueryResults = QueryResults::create(indexSearcher, urlToDoubleQuery->fuzzyQuery);
                        indexSearcher->search( urlToDoubleQuery->fuzzyQuery, fuzzyQueryResults);
                        idsFuzzyFound = fuzzyQueryResults->getNumberOfResults();

                        // create final queryResults to print.
                        QueryResultsInternal *exact_qs = dynamic_cast<QueryResultsInternal *>(exactQueryResults);
                        QueryResultsInternal *fuzzy_qs = dynamic_cast<QueryResultsInternal *>(fuzzyQueryResults);

                        unsigned fuzzyQueryResultsIter = 0;

                        while (exact_qs->sortedFinalResults.size() < (unsigned)(urlParserHelper.offset + urlParserHelper.resultsToRetrieve)
                                && fuzzyQueryResultsIter < fuzzyQueryResults->getNumberOfResults())
                        {
                            std::string recordId = fuzzy_qs->getRecordId(fuzzyQueryResultsIter);
                            if ( ! exactVisitedList.count(recordId) )// recordid not there
                            {
                                exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[fuzzyQueryResultsIter]);
                            }
                            fuzzyQueryResultsIter++;
                        }
                        delete fuzzyQueryResults;
                    }
                }
                // compute elapsed time in ms
                struct timespec tend;
                clock_gettime(CLOCK_REALTIME, &tend);
                unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

                //std::stringstream search_time;
                //search_time << req->uri << "[executed in " << ts1 << " ms]";

                //TODO: Logging
                //cry_wrapper(conn, search_time.str().c_str());

                int idsFound = exactQueryResults->getNumberOfResults();

                exactQueryResults->printStats();

                //cout << "reached map query executed" << endl;

                if (urlParserHelper.offset + urlParserHelper.resultsToRetrieve  > idsFound) // Case where you have return 10,20, but we got only 0,15 results.
                {
                    HTTPResponse::printResults(req, headers, urlParserHelper, indexDataContainerConf, exactQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
                            urlParserHelper.offset, idsFound, idsFound, ts1, tstart, tend);
                }
                else // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
                {
                    HTTPResponse::printResults(req, headers, urlParserHelper, indexDataContainerConf, exactQueryResults, urlToDoubleQuery->exactQuery, server->indexer,
                            urlParserHelper.offset, urlParserHelper.offset + urlParserHelper.resultsToRetrieve, idsFound, ts1, tstart, tend);
                }

                delete exactQueryResults;
            }
        }
        };
        delete indexSearcher;
    }
    else
    {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", urlParserHelper.parserErrorMessage.str(), headers);
    }

    // Free the objects
    evhttp_clear_headers(&headers);
    delete urlToDoubleQuery;
}

}}
