//$Id: HTTPRequestHandler.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <set>

#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"
#include "util/CustomizableJsonWriter.h"


#include "HTTPRequestHandler.h"
#include "IndexWriteUtil.h"
#include "instantsearch/TypedValue.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "ParsedParameterContainer.h"
#include "QueryParser.h"
#include "QueryValidator.h"
#include "QueryRewriter.h"
#include "QueryPlanGen.h"
#include "QueryPlan.h"
#include "QueryExecutor.h"
#include "ParserUtility.h"
#include <event2/http.h>
#include "util/FileOps.h"

#define SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS 2

namespace srch2is = srch2::instantsearch;
using srch2is::Analyzer;
using srch2is::QueryResultsInternal;
using srch2is::QueryResults;

using namespace snappy;
using namespace std;

namespace srch2 {
namespace httpwrapper {


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

void bmhelper_check_add_callback(evbuffer *buf, const evkeyvalq &headers,
        const string &out_payload) {
    const char *jsonpCallBack = evhttp_find_header(&headers,
            URLParser::jsonpCallBackName);
    if (jsonpCallBack) {
        size_t sz;
        char *jsonpCallBack_cstar = evhttp_uridecode(jsonpCallBack, 0, &sz);
        //std::cout << "[" << jsonpCallBack_cstar << "]" << std::endl;

        evbuffer_add_printf(buf, "%s(%s)", jsonpCallBack_cstar,
                out_payload.c_str());

        delete jsonpCallBack_cstar;
    } else {
        evbuffer_add_printf(buf, "%s", out_payload.c_str());
    }
}

void bmhelper_add_content_length(evhttp_request *req, evbuffer *buf) {
    size_t length = EVBUFFER_LENGTH(buf);
    std::stringstream length_str;
    length_str << length;
    evhttp_add_header(req->output_headers, "Content-Length",
            length_str.str().c_str());
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) {
    evbuffer *returnbuffer = create_buffer(req);
    bmhelper_check_add_callback(returnbuffer, headers, out_payload);
    bmhelper_add_content_length(req, returnbuffer);
    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload) {
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
void HTTPRequestHandler::printResults(evhttp_request *req,
        const evkeyvalq &headers, const QueryPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults, const Query *query,
        const Indexer *indexer, const unsigned start, const unsigned end,
        const unsigned retrievedResults, const string & message,
        const unsigned ts1, struct timespec &tstart, struct timespec &tend , bool onlyFacets ) {

    Json::Value root;
    static pair<string, string> internalRecordTags("srch2_internal_record_123456789", "record");

    // In each pair, the first one is the internal json label for the unparsed text, and
    // the second one is the final json label used in the print() function
    vector<pair<string, string> > tags;
    tags.push_back(internalRecordTags);
    // We use CustomizableJsonWriter with the internal record tag so that we don't need to
    // parse the internalRecordTag string to add it to the JSON object.
    CustomizableJsonWriter writer(&tags);

    // For logging
    string logQueries;

    root["searcher_time"] = ts1;
    clock_gettime(CLOCK_REALTIME, &tstart);

    if(onlyFacets == false){ // We send the matching records only if "facet != only".
        root["results"].resize(end - start);
        unsigned counter = 0;
        if (queryPlan.getSearchType() == GeoSearchType
                && query->getQueryTerms()->empty()) //check if the query type is range query without keywords
                {
            for (unsigned i = start; i < end; ++i) {
                root["results"][counter]["record_id"] = queryResults->getRecordId(
                        i);
                root["results"][counter]["score"] = (0
                        - queryResults->getResultScore(i).getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_RECORD
                        || indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SPECIFIED_ATTRIBUTES) {
                    unsigned internalRecordId = queryResults->getInternalRecordId(
                            i);
                    std::string compressedInMemoryRecordString = indexer
                            ->getInMemoryData(internalRecordId);

                    std::string uncompressedInMemoryRecordString;

                    snappy::Uncompress(compressedInMemoryRecordString.c_str(),
                            compressedInMemoryRecordString.size(),
                            &uncompressedInMemoryRecordString);

                    root["results"][counter][internalRecordTags.first] = uncompressedInMemoryRecordString;
                }
                ++counter;
            }

        } else // the query is including keywords:(1)only keywords (2)keywords+geo
        {

            for (unsigned i = start; i < end; ++i) {
                root["results"][counter]["record_id"] = queryResults->getRecordId(
                        i);
                root["results"][counter]["score"] = queryResults->getResultScore(i)
                        .getFloatTypedValue();

                // print edit distance vector
                vector<unsigned> editDistances;
                queryResults->getEditDistances(i, editDistances);

                root["results"][counter]["edit_dist"].resize(editDistances.size());
                for (unsigned int j = 0; j < editDistances.size(); ++j) {
                    root["results"][counter]["edit_dist"][j] = editDistances[j];
                }

                // print matching keywords vector
                vector<std::string> matchingKeywords;
                queryResults->getMatchingKeywords(i, matchingKeywords);

                root["results"][counter]["matching_prefix"].resize(
                        matchingKeywords.size());
                for (unsigned int j = 0; j < matchingKeywords.size(); ++j) {
                    root["results"][counter]["matching_prefix"][j] =
                            matchingKeywords[j];
                }

                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_RECORD
                        || indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SPECIFIED_ATTRIBUTES) {
                	unsigned internalRecordId = queryResults->getInternalRecordId(
                            i);
                    std::string compressedInMemoryRecordString = indexer
                            ->getInMemoryData(internalRecordId);

                    std::string uncompressedInMemoryRecordString;

                    snappy::Uncompress(compressedInMemoryRecordString.c_str(),
                            compressedInMemoryRecordString.size(),
                            &uncompressedInMemoryRecordString);

                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = uncompressedInMemoryRecordString;
                }
                ++counter;
            }

            root["query_keywords"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                string &term = *(query->getQueryTerms()->at(i)->getKeyword());
                root["query_keywords"][i] = term;
                if (i)
                    logQueries += "";
                logQueries += term;
            }
            root["query_keywords_complete"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                bool isCompleteTermType = (query->getQueryTerms()->at(i)->getTermType() == srch2is::TERM_TYPE_COMPLETE );
                root["query_keywords_complete"][i] = isCompleteTermType;
            }


            root["fuzzy"] = (int) queryPlan.isFuzzy();
        }
    }else{ // facet only case: we only want query information
    	if (queryPlan.getSearchType() != GeoSearchType
    			|| query->getQueryTerms()->empty() == false) //check if the query type is range query without keywords
    	{
            root["query_keywords"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                string &term = *(query->getQueryTerms()->at(i)->getKeyword());
                root["query_keywords"][i] = term;
                if (i)
                    logQueries += "";
                logQueries += term;
            }
            root["query_keywords_complete"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                bool isCompleteTermType = (query->getQueryTerms()->at(i)->getTermType() == srch2is::TERM_TYPE_COMPLETE );
                root["query_keywords_complete"][i] = isCompleteTermType;
            }
            root["fuzzy"] = (int) queryPlan.isFuzzy();
    	}
    }


    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    root["payload_access_time"] = ts2;

    // return some meta data

    root["type"] = queryPlan.getSearchTypeCode();
    root["offset"] = start;
    root["limit"] = end - start;

//    if (queryPlan.getSearchType() == GetAllResultsSearchType
//            || queryPlan.getSearchType() == GeoSearchType) // facet output must be added here.
//                    {
    root["results_found"] = retrievedResults;

    long int estimatedNumberOfResults = queryResults->getEstimatedNumberOfResults();
    // Since estimation of number of results can return a wrong number, if this value is less
    // than the actual number of found results, we use the real number.
    if(estimatedNumberOfResults < (long int)retrievedResults){
    	estimatedNumberOfResults = (long int)retrievedResults;
    }
    if(estimatedNumberOfResults != -1){
        // at this point we know for sure that estimatedNumberOfResults is positive, so we can cast
        // it to unsigned (because the thirdparty library we use here does not accept long integers.)
        root["estimated_number_of_results"] = (unsigned)estimatedNumberOfResults;
    }
    if(queryResults->isResultsApproximated() == true){
        root["result_set_approximation"] = true;
    }

//    }

    const std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > > * facetResults =
            queryResults->getFacetResults();
    // Example:
    // ["facet" : {"facet_field_name":"model" ,
    //             "facet_info":
    //                         {["category_name":"JEEP", "category_value":"10"]
    //                          ["category_name":"BMW", "category_value":"20"]
    //                          ["category_name":"HONDA", "category_value":"12"]
    //                         }
    //            }"facet_field_name":"price" ,
    //             "facet_info":
    //                         {["category_name":"lessthanstart", "category_value":"10"]
    //                          ["category_name":"0", "category_value":"30"]
    //                          ["category_name":"10", "category_value":"2"]
    //                          ["category_name":"20", "category_value":"23"]
    //                         }
    //]
    if (!facetResults->empty()) { // we have facet results to print
        root["facets"].resize(facetResults->size());

        unsigned attributeCounter = 0;
        for (std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator attr =
                facetResults->begin(); attr != facetResults->end(); ++attr) {
            root["facets"][attributeCounter]["facet_field_name"] = attr->first;
            root["facets"][attributeCounter]["facet_info"].resize(
                    attr->second.second.size());
            for (std::vector<std::pair<std::string, float> >::const_iterator category =
                    attr->second.second.begin(); category != attr->second.second.end();
                    ++category) {

                if(category == attr->second.second.begin() && attr->second.first == srch2is::FacetTypeRange){
                    root["facets"][attributeCounter]["facet_info"][(category
                            - attr->second.second.begin())]["category_name"] = "lessThanStart";
                }else{
                    root["facets"][attributeCounter]["facet_info"][(category
                            - attr->second.second.begin())]["category_name"] =
                            category->first;
                }
                root["facets"][attributeCounter]["facet_info"][(category
                        - attr->second.second.begin())]["category_value"] =
                        category->second;
            }

            //
            attributeCounter++;
        }
    }

    root["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void HTTPRequestHandler::printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
        const QueryPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults,
        const srch2is::Indexer *indexer,
        const string & message,
        const unsigned ts1,
        struct timespec &tstart, struct timespec &tend){

    Json::FastWriter writer;
    Json::Value root;

    // For logging
    string logQueries;

    root["searcher_time"] = ts1;
    root["results"].resize(queryResults->getNumberOfResults());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    for (unsigned i = 0; i < queryResults->getNumberOfResults(); ++i) {

        root["results"][counter]["record_id"] = queryResults->getRecordId(
                i);

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_RECORD
                || indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SPECIFIED_ATTRIBUTES) {
            unsigned internalRecordId = queryResults->getInternalRecordId(
                    i);
            std::string compressedInMemoryRecordString = indexer
                    ->getInMemoryData(internalRecordId);

            std::string uncompressedInMemoryRecordString;

            snappy::Uncompress(compressedInMemoryRecordString.c_str(),
                    compressedInMemoryRecordString.size(),
                    &uncompressedInMemoryRecordString);

            Json::Value in_mem_String;
            Json::Reader reader;
            reader.parse(uncompressedInMemoryRecordString, in_mem_String,
                    false);
            root["results"][counter]["record"] = in_mem_String;
        }
        ++counter;
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    root["payload_access_time"] = ts2;

    // return some meta data

    root["type"] = queryPlan.getSearchTypeCode();
    root["results_found"] = queryResults->getNumberOfResults();

    root["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}


void HTTPRequestHandler::printSuggestions(evhttp_request *req, const evkeyvalq &headers,
        const vector<string> & suggestions,
        const srch2is::Indexer *indexer,
        const string & message,
        const unsigned ts1,
        struct timespec &tstart, struct timespec &tend){

    Json::FastWriter writer;
    Json::Value root;

    // For logging
    string logQueries;

    root["searcher_time"] = ts1;
    root["suggestions"].resize(suggestions.size());

    clock_gettime(CLOCK_REALTIME, &tstart);

    for (unsigned i = 0; i < suggestions.size(); ++i) {

        root["suggestions"][i] = suggestions.at(i);
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    root["payload_access_time"] = ts2;

    // return some meta data

    root["suggestions_found"] = (unsigned)suggestions.size();

    root["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}


void HTTPRequestHandler::writeCommand_v0(evhttp_request *req,
        Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        size_t length = EVBUFFER_LENGTH(req->input_buffer);

        if (length == 0) {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",
                    "{\"message\":\"http body is empty\"}");
            Logger::warn("http body is empty");
            break;
        }

        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

        //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

        std::stringstream log_str;

        // Parse example data
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(post_data, root, false);

        if (parseSuccess == false) {
            log_str << "JSON object parse error";
        } else {
            Record *record = new Record(server->indexer->getSchema());

            if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
                // Iterates over the sequence elements.
                for ( int index = 0; index < root.size(); ++index ) {
                    Json::Value defaultValueToReturn = Json::Value("");
                    const Json::Value doc = root.get(index,
                                                defaultValueToReturn);

                    IndexWriteUtil::_insertCommand(server->indexer,
                            server->indexDataConfig, doc, record, log_str);
                    record->clear();

                    if (index < root.size() - 1)
                        log_str << ",";
                }
            } else {  // only one json object needs to be inserted
                const Json::Value doc = root;
                IndexWriteUtil::_insertCommand(server->indexer,
                        server->indexDataConfig, doc, record, log_str);
                record->clear();
            }
            delete record;
        }
        //std::cout << log_str.str() << std::endl;
        Logger::info("%s", log_str.str().c_str());

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");
        break;
    }
    case EVHTTP_REQ_DELETE: {
        std::stringstream log_str;

        evkeyvalq headers;
        evhttp_parse_query(req->uri, &headers);

        IndexWriteUtil::_deleteCommand_QueryURI(server->indexer,
                server->indexDataConfig, headers, log_str);

        Logger::info("%s", log_str.str().c_str());
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");

        // Free the objects
        evhttp_clear_headers(&headers);
        break;
    }
    default: {
        Logger::error(
                "error: The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
    }
    };
}

void HTTPRequestHandler::updateCommand(evhttp_request *req,
        Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        size_t length = EVBUFFER_LENGTH(req->input_buffer);

        if (length == 0) {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",
                    "{\"message\":\"http body is empty\"}");
            Logger::warn("http body is empty");
            break;
        }

        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

        //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

        std::stringstream log_str;

        // Parse example data
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(post_data, root, false);

        if (parseSuccess == false) {
            log_str << "JSON object parse error";
        } else {
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            Record *record = new Record(server->indexer->getSchema());

            if (root.type() == Json::arrayValue) {
                //the record parameter is an array of json objects
                for(Json::UInt index = 0; index < root.size(); index++) {
                    Json::Value defaultValueToReturn = Json::Value("");
                    const Json::Value doc = root.get(index,
                                                defaultValueToReturn);

                    IndexWriteUtil::_updateCommand(server->indexer,
                            server->indexDataConfig, headers, doc, record,
                            log_str);

                    record->clear();

                    if (index < root.size() - 1)
                        log_str << ",";
                }
            } else {
                // the record parameter is a single json object
                const Json::Value doc = root;

                IndexWriteUtil::_updateCommand(server->indexer,
                        server->indexDataConfig, headers, doc, record,
                        log_str);

                record->clear();
            }

            delete record;
            evhttp_clear_headers(&headers);
        }
        //std::cout << log_str.str() << std::endl;
        Logger::info("%s", log_str.str().c_str());

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");

        break;
    }
    default: {
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
    }
    };
}

void HTTPRequestHandler::writeCommand_v1(evhttp_request *req,
        Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        size_t length = EVBUFFER_LENGTH(req->input_buffer);

        if (length == 0) {
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",
                    "{\"message\":\"http body is empty\"}");
            Logger::warn("http body is empty");
            break;
        }

        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

        //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

        std::stringstream log_str;
        // Parse example data
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(post_data, root, false);

        if (parseSuccess == false) {
            log_str << "JSON object parse error";
        } else {
            if (root.type() == Json::arrayValue) {
                Record *record = new Record(server->indexer->getSchema());
                for (int index = 0; index < root.size(); ++index) // Iterates over the sequence elements.
                        {
                    Json::Value defaultValueToReturn = Json::Value("");
                    const Json::Value doc = root.get(index,
                            defaultValueToReturn);

                    /*Json::FastWriter writer;
                     std::cout << "[" << writer.write(doc)  << "]" << std::endl;*/

                    IndexWriteUtil::_insertCommand(server->indexer,
                            server->indexDataConfig, doc, record,
                            log_str);
                    record->clear();

                    if (index < root.size() - 1)
                        log_str << ",";
                }
                delete record;
            }
        }
        //std::cout << log_str.str() << std::endl;
        Logger::info("%s", log_str.str().c_str());
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");
        break;
    }
    case EVHTTP_REQ_DELETE: {
        std::stringstream log_str;

        evkeyvalq headers;
        evhttp_parse_query(req->uri, &headers);

        IndexWriteUtil::_deleteCommand_QueryURI(server->indexer,
                server->indexDataConfig, headers, log_str);

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The batch was processed successfully\",\"log\":["
                        + log_str.str() + "]}\n");

        // Free the objects
        evhttp_clear_headers(&headers);

        Logger::info("%s", log_str.str().c_str());

        break;
    }
    default: {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    }
    };
}

void HTTPRequestHandler::activateCommand(evhttp_request *req,
        Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        //size_t length = EVBUFFER_LENGTH(req->input_buffer);
        //const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

        std::stringstream log_str;
        IndexWriteUtil::_commitCommand(server->indexer,
                server->indexDataConfig, 0, log_str);

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The initialization phase has started successfully\", \"log\":["
                        + log_str.str() + "]}\n");
        Logger::info("%s", log_str.str().c_str());
        break;
    }
    default: {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    }
    };
}

void HTTPRequestHandler::saveCommand(evhttp_request *req, Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        std::stringstream log_str;
        IndexWriteUtil::_saveCommand(server->indexer, log_str);

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The indexes have been saved to disk successfully\", \"log\":["
                        + log_str.str() + "]}\n");
        Logger::info("%s", log_str.str().c_str());
        break;
    }
    default: {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    }
    };
}

// The purpose of this function is to help rotate logger files by repointing logger file.
// When rotating log file, "logrotate(a 3rd-party program)" will rename the old "logger.txt" file to "logger.txt.1"
// and create a new file called "logger.txt"
// But srch2 engine currently still point to and write into the old file "logger.txt.1"
// The purpose of this function is to let srch2 engine point to the new-created logger file "logger.txt"
void HTTPRequestHandler::resetLoggerCommand(evhttp_request *req, Srch2Server *server) {
    //  TODO: we will need to consider concurrency control next.
    switch(req->type) {
    case EVHTTP_REQ_PUT: {
        // create a FILE* pointer to point to the new logger file "logger.txt"
        FILE *logFile = fopen(server->indexDataConfig->getHTTPServerAccessLogFile().c_str(),
                    "a");

        if (logFile == NULL) {
            Logger::error("Reopen Log file %s failed.",
                    server->indexDataConfig->getHTTPServerAccessLogFile().c_str());
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "REQUEST FAILED",
                "{\"message\":\"The logger file repointing is failed. Could not create new logger file\", \"log\":["
                         + server->indexDataConfig->getHTTPServerAccessLogFile() + "]}\n");
        } else {
            FILE * oldLogger = Logger::swapLoggerFile(logFile);
            fclose(oldLogger);
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The logger file repointing is done successfully\", \"log\":["
                         + server->indexDataConfig->getHTTPServerAccessLogFile() + "]}\n");
        }
        break;
    }
    default: {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    }
    };
}


// exportCommand: if search-response-format is 0 or 2, we keep the compressed Json data in Forward Index, we can uncompress the data and export to a file
void HTTPRequestHandler::exportCommand(evhttp_request *req, Srch2Server *server) {
    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        // if search-response-format is 0 or 2
        if (server->indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_RECORD
                            || server->indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SPECIFIED_ATTRIBUTES) {
            std::stringstream log_str;
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            const char *exportedDataFileName = evhttp_find_header(&headers, URLParser::nameParamName);
            if(exportedDataFileName){
                if(checkDirExistence(exportedDataFileName)){
                    exportedDataFileName = "export_data.json";
                }
                IndexWriteUtil::_exportCommand(server->indexer, exportedDataFileName, log_str);

                bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                        "{\"message\":\"The indexed data has been exported to the file "+ string(exportedDataFileName) +" successfully.\", \"log\":["
                                + log_str.str() + "]}\n");
                Logger::info("%s", log_str.str().c_str());
            }else {
                bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                        "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
                Logger::error(
                        "The request has an invalid or missing argument. See Srch2 API documentation for details");
            }
        } else{
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                    "{\"message\":\"The indexed data failed to export to disk, The request need to set search-response-format to be 0 or 2\"}\n");
        }
        break;
    }
    default: {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    }
    };
}

void HTTPRequestHandler::infoCommand(evhttp_request *req, Srch2Server *server,
        const string &versioninfo) {
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    string combinedInfo = "[" + server->indexer->getIndexHealth() + ", "
            + versioninfo + "]";

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", combinedInfo, headers);
    evhttp_clear_headers(&headers);
}

void HTTPRequestHandler::lookupCommand(evhttp_request *req,
        Srch2Server *server) {
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;
    string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    const char *pKeyParamName = evhttp_find_header(&headers,
            primaryKeyName.c_str());

    std::stringstream response_msg;

    if (pKeyParamName) {
        size_t sz;
        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
        delete pKeyParamName_cstar;

        response_msg << "{\"rid\":\"" << primaryKeyStringValue
                << "\",\"lookup\":\"";

        //lookup the record on the index
        switch (server->indexer->lookupRecord(primaryKeyStringValue)) {
        case srch2is::LU_ABSENT_OR_TO_BE_DELETED: {
            response_msg << "absent or to be deleted\"}";
            break;
        }
        case srch2is::LU_TO_BE_INSERTED: {
            response_msg << "to be inserted\"}";
            break;
        }
        default: // LU_PRESENT_IN_READVIEW_AND_WRITEVIEW
        {
            response_msg << "present in readview and writeview\"}";
        }
        };
    } else {
        response_msg
                << "{\"rid\":\"NULL\",\"lookup\":\"failed\",\"reason\":\"no record with given primary key\"}";
    }

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", response_msg.str(), headers);
    evhttp_clear_headers(&headers);
}

void HTTPRequestHandler::searchCommand(evhttp_request *req,
        Srch2Server *server) {

    // start the timer for search
    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);

    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;

    ParsedParameterContainer paramContainer;

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
    //1. first create query parser to parse the url
    QueryParser qp(headers, &paramContainer);
    bool isSyntaxValid = qp.parse();
    if (!isSyntaxValid) {
        // if the query is not valid print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
                paramContainer.getMessageString(), headers);
        return;
    }

    //2. validate the query
    QueryValidator qv(*(server->indexer->getSchema()),
            *(server->indexDataConfig), &paramContainer);

    bool valid = qv.validate();

    if (!valid) {
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
                paramContainer.getMessageString(), headers);
        return;
    }

    //3. rewrite the query and apply analyzer and other stuff ...
    QueryRewriter qr(server->indexDataConfig,
            *(server->indexer->getSchema()),
            *(AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf)),
            &paramContainer);
    qr.rewrite();

    //4. generate the queries and the plan
    QueryPlanGen qpg(paramContainer, indexDataContainerConf);
    QueryPlan queryPlan;
    qpg.generatePlan(&queryPlan);

    //5. now execute the plan
    srch2is::QueryResultFactory * resultsFactory =
            new srch2is::QueryResultFactory();
    // TODO : is it possible to make executor and planGen singleton ?
    QueryExecutor qe(queryPlan, resultsFactory, server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    QueryResults * finalResults = new QueryResults();
    qe.execute(finalResults);

    // compute elapsed time in ms , end the timer
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    //6. call the print function to print out the results
    // TODO : re-implement a print function which print the results in JSON format.
    switch (queryPlan.getSearchType()) {
    case TopKSearchType:
        finalResults->printStats();
        HTTPRequestHandler::printResults(req, headers, queryPlan,
                indexDataContainerConf, finalResults, queryPlan.getExactQuery(),
                server->indexer, queryPlan.getOffset(),
                finalResults->getNumberOfResults(),
                finalResults->getNumberOfResults(),
                paramContainer.getMessageString(), ts1, tstart, tend);
        break;

    case GetAllResultsSearchType:
    case GeoSearchType:
        finalResults->printStats();
        if (queryPlan.getOffset() + queryPlan.getResultsToRetrieve()
                > finalResults->getNumberOfResults()) {
            // Case where you have return 10,20, but we got only 0,15 results.
            HTTPRequestHandler::printResults(req, headers, queryPlan,
                    indexDataContainerConf, finalResults,
                    queryPlan.getExactQuery(), server->indexer,
                    queryPlan.getOffset(), finalResults->getNumberOfResults(),
                    finalResults->getNumberOfResults(),
                    paramContainer.getMessageString(), ts1, tstart, tend , paramContainer.onlyFacets);
        } else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
            HTTPRequestHandler::printResults(req, headers, queryPlan,
                    indexDataContainerConf, finalResults,
                    queryPlan.getExactQuery(), server->indexer,
                    queryPlan.getOffset(),
                    queryPlan.getOffset() + queryPlan.getResultsToRetrieve(),
                    finalResults->getNumberOfResults(),
                    paramContainer.getMessageString(), ts1, tstart, tend, paramContainer.onlyFacets);
        }
        break;
    case RetrieveByIdSearchType:
        finalResults->printStats();
        HTTPRequestHandler::printOneResultRetrievedById(req,
                headers,
                queryPlan ,
                indexDataContainerConf,
                finalResults ,
                server->indexer ,
                paramContainer.getMessageString() ,
                ts1, tstart , tend);
        break;
    default:
        break;
    }

    // 7. delete allocated structures
    // Free the objects
    evhttp_clear_headers(&headers);
    delete finalResults;
    delete resultsFactory;
}

void HTTPRequestHandler::suggestCommand(evhttp_request *req, Srch2Server *server){
    // start the timer for search
    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);


    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;

    // 1. first parse the headers
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    QueryParser qp(headers);

    string keyword;
    float fuzzyMatchPenalty;
    int numberOfSuggestionsToReturn;
    std::vector<std::pair<MessageType, std::string> > messages;
    bool isSyntaxValid = qp.parseForSuggestions(keyword, fuzzyMatchPenalty, numberOfSuggestionsToReturn, messages);

    // prepare the messages.
    std::string messagesString = "";
    for (std::vector<std::pair<MessageType, std::string> >::iterator m =
            messages.begin(); m != messages.end(); ++m) {
        switch (m->first) {
        case MessageError:
            messagesString += "ERROR : " + m->second + "\n";
            break;
        case MessageWarning:
            messagesString += "WARNING : " + m->second + "\n";
            break;
	default:
	    break;
        }
    }

    if(! isSyntaxValid){
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
                messagesString, headers);
        return;
    }


    // 2. second, use configuration file if some information is missing
    if(numberOfSuggestionsToReturn == -1){
        numberOfSuggestionsToReturn = indexDataContainerConf->getDefaultNumberOfSuggestionsToReturn();
    }
    if(fuzzyMatchPenalty == -1){
        fuzzyMatchPenalty = indexDataContainerConf->getFuzzyMatchPenalty();
    }


    // 3. now search for suggestions
    // "IndexSearcherRuntimeParametersContainer" is the class which contains the parameters that we want to send to the core.
    // Each time IndexSearcher is created, we container must be made and passed to it as an argument.
    IndexSearcherRuntimeParametersContainer runTimeParameters(indexDataContainerConf->getKeywordPopularityThreshold());
    IndexSearcher * indexSearcher = srch2is::IndexSearcher::create(server->indexer , &runTimeParameters);
    vector<string> suggestions ;
    int numberOfSuggestionsFound = indexSearcher->suggest(keyword , fuzzyMatchPenalty , numberOfSuggestionsToReturn , suggestions);
    delete indexSearcher;

    // compute elapsed time in ms , end the timer
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    // 4. Print the results
    printSuggestions(req , headers , suggestions , server->indexer , messagesString , ts1 , tstart , tend);

}

void HTTPRequestHandler::handleException(evhttp_request *req) {
    const string INTERNAL_SERVER_ERROR_MSG =
            "{\"error:\" Ooops!! The engine failed to process this request. Please check srch2 server logs for more details. If the problem persists please contact srch2 inc.}";
    bmhelper_evhttp_send_reply(req, HTTP_INTERNAL, "INTERNAL SERVER ERROR",
            INTERNAL_SERVER_ERROR_MSG);
}

}
}
