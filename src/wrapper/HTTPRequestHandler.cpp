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
#include "QueryPlan.h"
#include "QueryExecutor.h"
#include "ParserUtility.h"
#include <event2/http.h>
#include "util/FileOps.h"
#include "ServerHighLighter.h"
#include "util/RecordSerializer.h"
#include "util/RecordSerializerUtil.h"

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
void HTTPRequestHandler::cleanAndAppendToBuffer(const string& in, string& out) {
	unsigned inLen = in.length();
	unsigned inIdx = 0;
	while (inIdx < inLen) {
		// remove non printable characters
		if (in[inIdx] < 32) {
			++inIdx; continue;
		}
		switch(in[inIdx]) {
		case '"':
		{
			// because we have reached here, there was no '\' before this '"'
			out +='\\'; out +='"';
			break;
		}
		case '\\':
		{
			if (inIdx != inLen - 1 and in[inIdx + 1] == '"') {  // looking for '\"'
				out += in[inIdx++];      // push them in one go...
				out += in[inIdx];
			} else {
				out +='\\'; out +='\\';  // escape the lonesome '\'
			}
			break;
		}
		default:
			out += in[inIdx];
		}
		++inIdx;
	}
}
/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void HTTPRequestHandler::printResults(evhttp_request *req,
        const evkeyvalq &headers, const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults, const Query *query,
        const Indexer *indexer, const unsigned start, const unsigned end,
        const unsigned retrievedResults, const string & message,
        const unsigned ts1, struct timespec &tstart, struct timespec &tend ,
        const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) {

    Json::Value root;
    static pair<string, string> internalRecordTags("srch2_internal_record_123456789", "record");
    static pair<string, string> internalSnippetTags("srch2_internal_snippet_123456789", "snippet");

    // In each pair, the first one is the internal json label for the unparsed text, and
    // the second one is the final json label used in the print() function
    vector<pair<string, string> > tags;
    tags.push_back(internalRecordTags);tags.push_back(internalSnippetTags);
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
        if (queryPlan.getQueryType() == srch2is::SearchTypeMapQuery
                && query->getQueryTerms()->empty()) //check if the query type is range query without keywords
        {
            for (unsigned i = start; i < end; ++i) {
                root["results"][counter]["record_id"] = queryResults->getRecordId(
                        i);
                root["results"][counter]["score"] = (0
                        - queryResults->getResultScore(i).getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR){
                    unsigned internalRecordId = queryResults->getInternalRecordId(i);
                    string sbuffer;
                    genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i), sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	unsigned internalRecordId = queryResults->getInternalRecordId(i);
                	string sbuffer;
                	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                	genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i),
                			sbuffer, attrToReturn);
                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	root["results"][counter][internalRecordTags.first] = sbuffer;
                }
                ++counter;
            }

        } else // the query is including keywords:(1)only keywords (2)keywords+geo
        {

            for (unsigned i = start; i < end; ++i) {
                root["results"][counter]["record_id"] = queryResults->getRecordId(i);
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
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
                    unsigned internalRecordId = queryResults->getInternalRecordId(i);
                    string sbuffer;
                    genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i),
                    		 sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	unsigned internalRecordId = queryResults->getInternalRecordId(i);
                	string sbuffer;
                	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                	genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i),
                			sbuffer, attrToReturn);
                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	root["results"][counter][internalRecordTags.first] = sbuffer;
                }

                string sbuffer = string();
                sbuffer.reserve(1024);  //<< TODO: set this to max allowed snippet len
                genSnippetJSONString(i, start, recordSnippets, sbuffer, queryResults);
                root["results"][counter][internalSnippetTags.first] = sbuffer;
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
    	if (queryPlan.getQueryType() != srch2is::SearchTypeMapQuery
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

    root["type"] = queryPlan.getQueryType();
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
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, highlighter_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, hlTime, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void HTTPRequestHandler::printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
        const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults,
        const srch2is::Indexer *indexer,
        const string & message,
        const unsigned ts1,
        struct timespec &tstart, struct timespec &tend){

    Json::Value root;
    pair<string, string> internalRecordTags("srch2_internal_record_123456789", "record");
    pair<string, string> internalSnippetTags("srch2_internal_snippet_123456789", "snippet");

    // In each pair, the first one is the internal json label for the unparsed text, and
    // the second one is the final json label used in the print() function
    vector<pair<string, string> > tags;
    tags.push_back(internalRecordTags);tags.push_back(internalSnippetTags);
    // We use CustomizableJsonWriter with the internal record tag so that we don't need to
    // parse the internalRecordTag string to add it to the JSON object.
    CustomizableJsonWriter writer(&tags);


    // For logging
    string logQueries;

    root["searcher_time"] = ts1;
    root["results"].resize(queryResults->getNumberOfResults());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    for (unsigned i = 0; i < queryResults->getNumberOfResults(); ++i) {

        root["results"][counter]["record_id"] = queryResults->getRecordId(i);

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            unsigned internalRecordId = queryResults->getInternalRecordId(i);
            string sbuffer;
            genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i), sbuffer);
            root["results"][counter][internalRecordTags.first] = sbuffer;
        } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
        	unsigned internalRecordId = queryResults->getInternalRecordId(i);
        	string sbuffer;
        	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
        	genRecordJsonString(indexer, internalRecordId, queryResults->getRecordId(i),
        			sbuffer, attrToReturn);
        	// The class CustomizableJsonWriter allows us to
        	// attach the data string to the JSON tree without parsing it.
        	root["results"][counter][internalRecordTags.first] = sbuffer;
        }
        ++counter;
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    root["payload_access_time"] = ts2;

    // return some meta data

    root["type"] = queryPlan.getQueryType();
    root["results_found"] = queryResults->getNumberOfResults();

    root["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}

void HTTPRequestHandler::genRecordJsonString(const srch2is::Indexer *indexer, unsigned internalRecordId,
		const string& extrnalRecordId, string& sbuffer){
	genRecordJsonString(indexer, internalRecordId, extrnalRecordId,
	                    		 sbuffer, NULL);
}
void HTTPRequestHandler::genRecordJsonString(const srch2is::Indexer *indexer, unsigned internalRecordId,
		const string& externalRecordId, string& sbuffer, const vector<string>* attrToReturn){

	StoredRecordBuffer buffer =  indexer->getInMemoryData(internalRecordId);
	Schema * storedSchema = Schema::create();
	RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
	RecordSerializerUtil::convertCompactToJSONString(storedSchema, buffer, externalRecordId, sbuffer, attrToReturn);

}

void HTTPRequestHandler::genSnippetJSONString(unsigned recIdx, unsigned start,
		const vector<RecordSnippet>& recordSnippets, string& sbuffer,const QueryResults *queryResults) {
	unsigned _idx = recIdx - start;
	if (_idx < recordSnippets.size()
			&& recordSnippets[_idx].recordId == queryResults->getInternalRecordId(recIdx)) {
		sbuffer.append("{");
		for (unsigned j = 0 ; j <  recordSnippets[_idx].fieldSnippets.size(); ++j) {
			sbuffer+='"'; sbuffer+=recordSnippets[_idx].fieldSnippets[j].FieldId; sbuffer+='"';
			sbuffer+=":[";
			for (unsigned k = 0 ; k <  recordSnippets[_idx].fieldSnippets[j].snippet.size(); ++k) {
				sbuffer+='"';
				cleanAndAppendToBuffer(recordSnippets[_idx].fieldSnippets[j].snippet[k], sbuffer);
				sbuffer+='"';
				sbuffer+=',';
			}
			if (recordSnippets[_idx].fieldSnippets[j].snippet.size())
				sbuffer.erase(sbuffer.length()-1);
			sbuffer+="],";
		}
		if (recordSnippets[_idx].fieldSnippets.size())
			sbuffer.erase(sbuffer.length()-1);
		sbuffer.append("}");
	} else {
		sbuffer += "[ ]";
		if (recordSnippets.size() != 0) {
			Logger::warn("snippet not found for record id = %s !!",
					queryResults->getRecordId(recIdx).c_str());
		}
	}
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


void HTTPRequestHandler::writeCommand(evhttp_request *req,
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
                "{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                         + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}\n");
        } else {
            FILE * oldLogger = Logger::swapLoggerFile(logFile);
            fclose(oldLogger);
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                         + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}\n");
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
        if (server->indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
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

    string combinedInfo = "{" + server->indexer->getIndexHealth() + ", "
        + "\"version\":\"" + versioninfo + "\"}";

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

// This code is not used anywhere yet. The function converts %26 to '&' character
// in the query so that the libevent can treat it as a header delimiter.
void decodeAmpersand(const char *uri, unsigned len, string& decodeUri) {
	char c;
	decodeUri.reserve(len);
	for (unsigned i = 0; i < len - 2; ++i) {
		if (uri[i] == '%' && uri[i+1] == '2' && uri[i+2] == '6') {
			decodeUri.push_back('&');
			i += 2;
		} else {
			decodeUri.push_back(uri[i]);
		}
	}
}

void HTTPRequestHandler::searchCommand(evhttp_request *req,
        Srch2Server *server) {

    // start the timer for search
    struct timespec tstart;
//    struct timespec tstart2;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
//    clock_gettime(CLOCK_REALTIME, &tstart2);

    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;

    ParsedParameterContainer paramContainer;

//    string decodedUri;
//    decodeAmpersand(req->uri, strlen(req->uri), decodedUri);
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);
//    cout << "Query: " << req->uri << endl;
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

//    clock_gettime(CLOCK_REALTIME, &tend);
//    unsigned parserTime = (tend.tv_sec - tstart2.tv_sec) * 1000
//            + (tend.tv_nsec - tstart2.tv_nsec) / 1000000;

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
    LogicalPlan logicalPlan;
    if(qr.rewrite(logicalPlan) == false){
        // if the query is not valid, print the error message to the response
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
                paramContainer.getMessageString(), headers);
        return;
    }

//    clock_gettime(CLOCK_REALTIME, &tend);
//    unsigned rewriterTime = (tend.tv_sec - tstart2.tv_sec) * 1000
//            + (tend.tv_nsec - tstart2.tv_nsec) / 1000000;
//    rewriterTime -= (validatorTime + parserTime);

    //4. now execute the plan
    srch2is::QueryResultFactory * resultsFactory =
            new srch2is::QueryResultFactory();
    // TODO : is it possible to make executor and planGen singleton ?
    QueryExecutor qe(logicalPlan, resultsFactory, server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    QueryResults * finalResults = new QueryResults();
    qe.execute(finalResults);

    // compute elapsed time in ms , end the timer
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    vector<RecordSnippet> highlightInfo;
    /*
     *  Do snippet generation only if
     *  1. There are attributes marked to be highlighted
     *  2. Query is not facet only
     *  3. Highlight is not turned off in the query ( default is on ) << TODO
     */
    struct timespec hltstart;
    clock_gettime(CLOCK_REALTIME, &hltstart);

    if (server->indexDataConfig->getHighlightAttributeIdsVector().size() > 0 &&
    		!paramContainer.onlyFacets &&
    		paramContainer.isHighlightOn) {

    	ServerHighLighter highlighter =  ServerHighLighter(finalResults, server, paramContainer,
    			logicalPlan.getOffset(), logicalPlan.getNumberOfResultsToRetrieve());
    	highlightInfo.reserve(logicalPlan.getNumberOfResultsToRetrieve());
    	highlighter.generateSnippets(highlightInfo);
    	if (highlightInfo.size() == 0 && finalResults->getNumberOfResults() > 0) {
    		Logger::warn("Highligting is on but snippets were not generated!!");
    	}
    }

    struct timespec hltend;
    clock_gettime(CLOCK_REALTIME, &hltend);
    unsigned hlTime = (hltend.tv_sec - hltstart.tv_sec) * 1000
            + (hltend.tv_nsec - hltstart.tv_nsec) / 1000000;

    //5. call the print function to print out the results
    switch (logicalPlan.getQueryType()) {
    case srch2is::SearchTypeTopKQuery:
        finalResults->printStats();
        HTTPRequestHandler::printResults(req, headers, logicalPlan,
                indexDataContainerConf, finalResults, logicalPlan.getExactQuery(),
                server->indexer, logicalPlan.getOffset(),
                finalResults->getNumberOfResults(),
                finalResults->getNumberOfResults(),
                paramContainer.getMessageString(), ts1, tstart, tend, highlightInfo, hlTime,
                paramContainer.onlyFacets);

        break;

    case srch2is::SearchTypeGetAllResultsQuery:
    case srch2is::SearchTypeMapQuery:
        finalResults->printStats();
        finalResults->impl->estimatedNumberOfResults = finalResults->impl->sortedFinalResults.size();
        if (logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve()
                > finalResults->getNumberOfResults()) {
            // Case where you have return 10,20, but we got only 0,15 results.
            HTTPRequestHandler::printResults(req, headers, logicalPlan,
                    indexDataContainerConf, finalResults,
                    logicalPlan.getExactQuery(), server->indexer,
                    logicalPlan.getOffset(), finalResults->getNumberOfResults(),
                    finalResults->getNumberOfResults(),
                    paramContainer.getMessageString(), ts1, tstart, tend , highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        } else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
            HTTPRequestHandler::printResults(req, headers, logicalPlan,
                    indexDataContainerConf, finalResults,
                    logicalPlan.getExactQuery(), server->indexer,
                    logicalPlan.getOffset(),
                    logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve(),
                    finalResults->getNumberOfResults(),
                    paramContainer.getMessageString(), ts1, tstart, tend, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        }
        break;
    case srch2is::SearchTypeRetrieveById:
        finalResults->printStats();
        HTTPRequestHandler::printOneResultRetrievedById(req,
                headers,
                logicalPlan ,
                indexDataContainerConf,
                finalResults ,
                server->indexer ,
                paramContainer.getMessageString() ,
                ts1, tstart , tend);
        break;
    default:
        break;
    }

//    clock_gettime(CLOCK_REALTIME, &tend);
//    unsigned printTime = (tend.tv_sec - tstart2.tv_sec) * 1000
//            + (tend.tv_nsec - tstart2.tv_nsec) / 1000000;
//    printTime -= (validatorTime + rewriterTime + executionTime + parserTime);
//    cout << "Times : " << parserTime << "\t" << validatorTime << "\t" << rewriterTime << "\t" << executionTime << "\t" << printTime << endl;
    // 6. delete allocated structures
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
        case MessageNotice:
            messagesString += "NOTICE : " + m->second + "\n";
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
    QueryEvaluatorRuntimeParametersContainer runTimeParameters(indexDataContainerConf->getKeywordPopularityThreshold());
    QueryEvaluator * queryEvaluator = new QueryEvaluator(server->indexer , &runTimeParameters);
    vector<string> suggestions ;
    int numberOfSuggestionsFound = queryEvaluator->suggest(keyword , fuzzyMatchPenalty , numberOfSuggestionsToReturn , suggestions);
    delete queryEvaluator;

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
