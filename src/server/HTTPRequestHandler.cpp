//$Id: HTTPRequestHandler.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
//#include <sys/signal.h>
#include <signal.h>

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
#include "util/ParserUtility.h"
#include <event2/http.h>
#include "util/FileOps.h"
#include "util/RecordSerializer.h"
#include "util/RecordSerializerUtil.h"
#include "DataConnectorThread.h"

#define SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS 2

namespace srch2is = srch2::instantsearch;
using srch2is::Analyzer;
using srch2is::QueryResultsInternal;
using srch2is::QueryResults;

using namespace snappy;
using namespace std;

namespace srch2 {
namespace httpwrapper {

namespace {
    static const pair<string, string> global_internal_record("srch2_internal_record_123456789", "record");
    static const pair<string, string> global_internal_snippet("srch2_internal_snippet_123456789", "snippet");
    static const pair<string, string> internal_data[] = { global_internal_record, global_internal_snippet};

    // The below objects are used to format the global_customized_writer. 
    // This global_customized_writer is created once and then can be used multiple times.
    static const vector<pair<string, string> > global_internal_skip_tags(internal_data, internal_data+2);    
    static const CustomizableJsonWriter global_customized_writer (&global_internal_skip_tags);
    static const char * JSON_MESSAGE = "message";
    static const char * JSON_LOG= "log";
    static const char * HTTP_INVALID_REQUEST_MESSAGE = "The request has an invalid or missing argument. See Srch2 API documentation for details.";

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

    // The below functions are the helper functions to format the HTTP response
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

            // libevent uses malloc for memory allocation. Hence, use free
            free(jsonpCallBack_cstar);
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

    void response_to_invalid_request (evhttp_request *req, Json::Value &response){
        response["error"] = HTTP_INVALID_REQUEST_MESSAGE;
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST", global_customized_writer.write(response));
        Logger::error(HTTP_INVALID_REQUEST_MESSAGE);
    }

    // This helper function is to wrap a Json::Value into a Json::Array and then return the later object.
    Json::Value wrap_with_json_array(Json::Value value){
        Json::Value array(Json::arrayValue);
        array.append(value);
        return array;
    }
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
boost::shared_ptr<Json::Value> HTTPRequestHandler::printResults(evhttp_request *req,
        const evkeyvalq &headers, const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults, const Query *query,
        const Indexer *indexer, const unsigned start, const unsigned end,
        const unsigned retrievedResults, const string& aclRoleId, const string & message,
        const unsigned ts1, struct timespec &tstart, struct timespec &tend ,
        const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) {

    boost::shared_ptr<Json::Value> root (new Json::Value());
    // For logging
    string logQueries;
    unsigned resultFound = retrievedResults;
    (*root)["searcher_time"] = ts1;
    clock_gettime(CLOCK_REALTIME, &tstart);

    vector<string> attributesToReturnFromQuery = queryPlan.getAttrToReturn();
    vector<string> *attributesToReturnFromQueryPtr;

    if (attributesToReturnFromQuery.size() != 0)
      attributesToReturnFromQueryPtr = &attributesToReturnFromQuery;
    else
     attributesToReturnFromQueryPtr = NULL;


    if(onlyFacets == false){ // We send the matching records only if "facet != only".
        (*root)["results"].resize(end - start);
        unsigned counter = 0;
        if (query->getQueryTerms()->empty()) //check if the query type is range query without keywords
        {
            for (unsigned i = start; i < end; ++i) {
            	unsigned internalRecordId = queryResults->getInternalRecordId(i);
            	StoredRecordBuffer inMemoryData = indexer->getInMemoryData(internalRecordId);
            	if (inMemoryData.start.get() == NULL) {
            		--resultFound;
            		continue;
            	}
                (*root)["results"][counter]["record_id"] = queryResults->getRecordId(
                        i);
                (*root)["results"][counter]["score"] = (0
                        - queryResults->getResultScore(i).getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR){
                    string sbuffer;
                    //This case executes when all the attributes are to be returned. However we let the user
                    //override if field list parameter is given in query
                    genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                            sbuffer, attributesToReturnFromQueryPtr, aclRoleId);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    (*root)["results"][counter][global_internal_record.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	string sbuffer;
                	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();

                	//Return the attributes specified in the config file
                	//If query has field list parameter we override attrToReturn using the attributes from query
                	//otherwise we use attributes mentioned in config file
                	if(attributesToReturnFromQuery.size() > 0){
                	    attrToReturn = attributesToReturnFromQueryPtr;
                	}

                	genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                	                            sbuffer, attrToReturn, aclRoleId);

                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	(*root)["results"][counter][global_internal_record.first] = sbuffer;
                }else{
                    //Return the attributes specified explicitly in the query otherwise no attributes are returned
                    string stringBuffer;
                    if(attributesToReturnFromQuery.size() > 0){
                        genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                                stringBuffer, attributesToReturnFromQueryPtr, aclRoleId);
                        (*root)["results"][counter][global_internal_record.first] = stringBuffer;
                    }

                }
                ++counter;
            }

        } else // the query is including keywords:(1)only keywords (2)keywords+geo
        {
            for (unsigned i = start; i < end; ++i) {
            	unsigned internalRecordId = queryResults->getInternalRecordId(i);
            	StoredRecordBuffer inMemoryData = indexer->getInMemoryData(internalRecordId);
            	if (inMemoryData.start.get() == NULL) {
            		--resultFound;
            		continue;
            	}
                (*root)["results"][counter]["record_id"] = queryResults->getRecordId(i);
                (*root)["results"][counter]["score"] = queryResults->getResultScore(i)
                        .getFloatTypedValue();

                // print edit distance vector
                vector<unsigned> editDistances;
                queryResults->getEditDistances(i, editDistances);

                (*root)["results"][counter]["edit_dist"].resize(editDistances.size());
                for (unsigned int j = 0; j < editDistances.size(); ++j) {
                    (*root)["results"][counter]["edit_dist"][j] = editDistances[j];
                }

                // print matching keywords vector
                vector<std::string> matchingKeywords;
                queryResults->getMatchingKeywords(i, matchingKeywords);

                (*root)["results"][counter]["matching_prefix"].resize(
                        matchingKeywords.size());
                for (unsigned int j = 0; j < matchingKeywords.size(); ++j) {
                    (*root)["results"][counter]["matching_prefix"][j] =
                            matchingKeywords[j];
                }
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
                    unsigned internalRecordId = queryResults->getInternalRecordId(i);
                    string sbuffer;

                    //This case executes when all the attributes are to be returned. However we let the user
                    //override if field list parameter is given in query
                    genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                                                sbuffer, attributesToReturnFromQueryPtr, aclRoleId);

                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    (*root)["results"][counter][global_internal_record.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	unsigned internalRecordId = queryResults->getInternalRecordId(i);
                    string sbuffer;
                    const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();

                    //Return the attributes specified in the config file
                    //If query has field list parameter we override attrToReturn using the attributes from query
                    //otherwise we use attributes mentioned in config file
                    if(attributesToReturnFromQuery.size() > 0){
                        attrToReturn = attributesToReturnFromQueryPtr;
                    }

                    genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                                                sbuffer, attrToReturn, aclRoleId);

                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	(*root)["results"][counter][global_internal_record.first] = sbuffer;
                }else{
                    //Return the attributes specified explicitly in the query otherwise no attributes are returned
                    string stringBuffer;
                    if(attributesToReturnFromQuery.size() > 0){
                        genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                                stringBuffer, attributesToReturnFromQueryPtr, aclRoleId);
                        (*root)["results"][counter][global_internal_record.first] = stringBuffer;
                    }
                }
                string sbuffer = string();
                sbuffer.reserve(1024);  //<< TODO: set this to max allowed snippet len
                genSnippetJSONString(i, start, recordSnippets, sbuffer, queryResults);
                (*root)["results"][counter][global_internal_snippet.first] = sbuffer;
                ++counter;
            }

            (*root)["query_keywords"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                string &term = *(query->getQueryTerms()->at(i)->getKeyword());
                (*root)["query_keywords"][i] = term;
                if (i)
                    logQueries += "";
                logQueries += term;
            }
            (*root)["query_keywords_complete"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                bool isCompleteTermType = (query->getQueryTerms()->at(i)->getTermType() == srch2is::TERM_TYPE_COMPLETE );
                (*root)["query_keywords_complete"][i] = isCompleteTermType;
            }


            (*root)["fuzzy"] = (int) queryPlan.isFuzzy();
        }
    }else{ // facet only case: we only want query information
    	if ( query->getQueryTerms()->empty() == false) //check if the query type is range query without keywords
    	{
            (*root)["query_keywords"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                string &term = *(query->getQueryTerms()->at(i)->getKeyword());
                (*root)["query_keywords"][i] = term;
                if (i)
                    logQueries += "";
                logQueries += term;
            }
            (*root)["query_keywords_complete"].resize(query->getQueryTerms()->size());
            for (unsigned i = 0; i < query->getQueryTerms()->size(); i++) {
                bool isCompleteTermType = (query->getQueryTerms()->at(i)->getTermType() == srch2is::TERM_TYPE_COMPLETE );
                (*root)["query_keywords_complete"][i] = isCompleteTermType;
            }
            (*root)["fuzzy"] = (int) queryPlan.isFuzzy();
    	}
    }


    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    (*root)["payload_access_time"] = ts2;

    // return some meta data

    (*root)["type"] = queryPlan.getQueryType();
    (*root)["offset"] = start;
    (*root)["limit"] = end - start;

//    if (queryPlan.getSearchType() == GetAllResultsSearchType
//            || queryPlan.getSearchType() == GeoSearchType) // facet output must be added here.
//                    {
    (*root)["results_found"] = resultFound;

    long int estimatedNumberOfResults = queryResults->getEstimatedNumberOfResults();
    // Since estimation of number of results can return a wrong number, if this value is less
    // than the actual number of found results, we use the real number.
    if(estimatedNumberOfResults < (long int)resultFound){
    	estimatedNumberOfResults = (long int)resultFound;
    }
    if(estimatedNumberOfResults != -1){
        // at this point we know for sure that estimatedNumberOfResults is positive, so we can cast
        // it to unsigned (because the thirdparty library we use here does not accept long integers.)
        (*root)["estimated_number_of_results"] = (unsigned)estimatedNumberOfResults;
    }
    if(queryResults->isResultsApproximated() == true){
        (*root)["result_set_approximation"] = true;
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
        (*root)["facets"].resize(facetResults->size());

        unsigned attributeCounter = 0;
        for (std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator attr =
                facetResults->begin(); attr != facetResults->end(); ++attr) {
            (*root)["facets"][attributeCounter]["facet_field_name"] = attr->first;
            (*root)["facets"][attributeCounter]["facet_info"].resize(
                    attr->second.second.size());
            for (std::vector<std::pair<std::string, float> >::const_iterator category =
                    attr->second.second.begin(); category != attr->second.second.end();
                    ++category) {

                /*
                 * Offset is the relative position based on
                 * the start position of the iterator. e.g.
                 *
                 * {<a,1>,<b,2>,<c,3>}
                 *
                 * The offset of <a,1> will be 0, <b,2> will be 1, and
                 * <c,3> will be 2
                 */
                int offset = (category - attr->second.second.begin());

                if(category == attr->second.second.begin() && attr->second.first == srch2is::FacetTypeRange){
                    (*root)["facets"][attributeCounter]["facet_info"][offset]["category_name"] = "lessThanStart";
                }else{
                    (*root)["facets"][attributeCounter]["facet_info"][offset]["category_name"] =
                            category->first;
                }
                (*root)["facets"][attributeCounter]["facet_info"][offset]["category_value"] =
                        category->second;
            }

            attributeCounter++;
        }
    }

    (*root)["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, highlighter_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, hlTime, ts2);
    return root;
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
boost::shared_ptr<Json::Value> HTTPRequestHandler::printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
        const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const QueryResults *queryResults,
        const srch2is::Indexer *indexer,
        const string & aclRoleId,
        const string & message,
        const unsigned ts1,
        struct timespec &tstart, struct timespec &tend){

    boost::shared_ptr<Json::Value> root(new Json::Value());
    // For logging
    string logQueries;

    vector<string> attributesToReturnFromQuery = queryPlan.getAttrToReturn();
    vector<string> *attributesToReturnFromQueryPtr;

    if (attributesToReturnFromQuery.size() != 0)
      attributesToReturnFromQueryPtr = &attributesToReturnFromQuery;
    else
      attributesToReturnFromQueryPtr = NULL;

    (*root)["searcher_time"] = ts1;
    (*root)["results"].resize(queryResults->getNumberOfResults());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    unsigned resultFound = queryResults->getNumberOfResults();
    for (unsigned i = 0; i < queryResults->getNumberOfResults(); ++i) {
    	unsigned internalRecordId = queryResults->getInternalRecordId(i);
    	StoredRecordBuffer inMemoryData = indexer->getInMemoryData(internalRecordId);
    	if (inMemoryData.start.get() == NULL) {
    		--resultFound;
    		continue;
    	}
        (*root)["results"][counter]["record_id"] = queryResults->getRecordId(i);

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            unsigned internalRecordId = queryResults->getInternalRecordId(i);
            string sbuffer;

            //This case executes when all the attributes are to be returned. However we let the user
            //override if field list parameter is given in query
            genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                    sbuffer, attributesToReturnFromQueryPtr, aclRoleId);

            (*root)["results"][counter][global_internal_record.first] = sbuffer;
        } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
            unsigned internalRecordId = queryResults->getInternalRecordId(i);
            string sbuffer;
            const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();

            //Return the attributes specified in the config file
            //If query has field list parameter we override attrToReturn using the attributes from query
            //otherwise we use attributes mentioned in config file
            if(attributesToReturnFromQuery.size() > 0){
                attrToReturn = attributesToReturnFromQueryPtr;
            }
            genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                                        sbuffer, attrToReturn, aclRoleId);

            // The class CustomizableJsonWriter allows us to
            // attach the data string to the JSON tree without parsing it.
            (*root)["results"][counter][global_internal_record.first] = sbuffer;
        }else{
            //Return the attributes specified explicitly in the query otherwise no attributes are returned
            string stringBuffer;
            if(attributesToReturnFromQuery.size() > 0){
                genRecordJsonString(indexer, inMemoryData, queryResults->getRecordId(i),
                        stringBuffer, attributesToReturnFromQueryPtr, aclRoleId);
                (*root)["results"][counter][global_internal_record.first] = stringBuffer;
            }
        }
        ++counter;
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts2 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    (*root)["payload_access_time"] = ts2;

    // return some meta data

    (*root)["type"] = queryPlan.getQueryType();
    (*root)["results_found"] = resultFound;

    (*root)["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    return root;
}

void HTTPRequestHandler::genRecordJsonString(const srch2is::Indexer *indexer, StoredRecordBuffer buffer,
		const string& extrnalRecordId, string& sbuffer, const string& aclRoleId){
	genRecordJsonString(indexer, buffer, extrnalRecordId,
	                    		 sbuffer, NULL, aclRoleId);
}
void HTTPRequestHandler::genRecordJsonString(const srch2is::Indexer *indexer, StoredRecordBuffer buffer,
		const string& externalRecordId, string& sbuffer, const vector<string>* attrToReturn,
		const string& aclRoleId){

	vector<string>  accessibleAttrsList;
	// perform access control check on fields to be returned to a user.
	if (attrToReturn == NULL) {
		// attributes to return are not specified. Hence, Go over the fields in the schema and check
		// whether they are accessible for a given role id.

		const Schema *schema = indexer->getSchema();
		std::map<std::string, unsigned>::const_iterator iter =
				schema->getSearchableAttribute().begin();
		// 1. Searchable fields in schema
		for ( ; iter != schema->getSearchableAttribute().end(); iter++) {
			if (indexer->getAttributeAcl().isSearchableFieldAccessibleForRole(aclRoleId, iter->first)) {
				accessibleAttrsList.push_back(iter->first);
			}
		}
		// 2. Refining fields in schema
		iter = schema->getRefiningAttributes()->begin();
		for ( ; iter != schema->getRefiningAttributes()->end(); iter++) {
			if (indexer->getAttributeAcl().isRefiningFieldAccessibleForRole(aclRoleId, iter->first)) {
				accessibleAttrsList.push_back(iter->first);
			}
		}

	} else {
		// if attributes to returned are specified then verify whether these attributes are accessible
		for (unsigned i = 0; i < attrToReturn->size(); ++i) {
			const string & fieldName = attrToReturn->operator[](i);
			if (indexer->getAttributeAcl().isRefiningFieldAccessibleForRole(aclRoleId, fieldName) ||
			    indexer->getAttributeAcl().isSearchableFieldAccessibleForRole(aclRoleId, fieldName)) {
				accessibleAttrsList.push_back(fieldName);
			}
		}
	}
	Schema * storedSchema = Schema::create();
	RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
	RecordSerializerUtil::convertCompactToJSONString(storedSchema, buffer, externalRecordId, sbuffer,
			&accessibleAttrsList);
	delete storedSchema;
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

    Json::Value response(Json::objectValue);
    bool isSuccess = true;
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        size_t length = EVBUFFER_LENGTH(req->input_buffer);

        if (length == 0) {
            isSuccess = false;
            response[JSON_MESSAGE] = "http body is empty";
            Logger::warn("http body is empty");
            break;
        }

        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

        //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;


        // Parse example data
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(post_data, root, false);

        if (parseSuccess == false) {
            isSuccess = false;
            response[JSON_MESSAGE] = "JSON object parsing error";
            Logger::warn("JSON object parse error");
            break;
        } else {
            Record *record = new Record(server->getIndexer()->getSchema());

            Json::Value insert_responses(Json::arrayValue);
            // append to each response
            if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
                // Iterates over the sequence elements.
                insert_responses.resize(root.size());
                for ( int index = 0; index < root.size(); ++index ) {
                    Json::Value defaultValueToReturn = Json::Value("");
                    const Json::Value doc = root.get(index,
                                                defaultValueToReturn);

                    vector<string> roleIds;
                    // check if there is roleId in the query or not
					std::stringstream log_str;
                    if( JSONRecordParser::_extractRoleIds(roleIds, doc, server->getCoreInfo(), log_str) ){
                    	if(server->getCoreInfo()->getHasRecordAcl()){
                    		// add role ids to the record object
                    		record->setRoleIds(roleIds);
                    	}else{
                    		Logger::error(
                    				"error: %s does not have record-based access control.", server->getCoreInfo()->getName().c_str());
                    		response[JSON_MESSAGE] = "error:" + server->getCoreInfo()->getName() + " does not have record-based access control.";
                    		response[JSON_LOG] = log_str.str();
                    		bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                    				global_customized_writer.write(response));
                    		return;
                    	}
                    }
                    Json::Value each_response =
                        IndexWriteUtil::_insertCommand(server->getIndexer(),
                            server->getCoreInfo(), doc, record );


                    each_response["acl_log"] = log_str.str();
                    insert_responses[index] = each_response;

                    record->clear();
                }
            } else {  // only one json object needs to be inserted
                const Json::Value doc = root;
                vector<string> roleIds;
				std::stringstream log_str;
                // check if there is roleId in the query or not
                if( JSONRecordParser::_extractRoleIds(roleIds, doc, server->getCoreInfo(), log_str) ){
                	if(server->getCoreInfo()->getHasRecordAcl()){
                		// add role ids to the record object
                		record->setRoleIds(roleIds);

                	}else{
                		Logger::error(
                				"error: %s does not have record-based access control.",server->getCoreInfo()->getName().c_str());
                		response[JSON_MESSAGE] = "error:" + server->getCoreInfo()->getName() + " does not have record-based access control.";
                		response[JSON_LOG] = log_str.str();
                		bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
                				global_customized_writer.write(response));
                		return;
                	}
                }

                Json::Value each_response = IndexWriteUtil::_insertCommand(server->getIndexer(),
                		server->getCoreInfo(), doc, record);
                each_response["acl_log"] = log_str.str();
                insert_responses.append(each_response);
                record->clear();
            }
            delete record;
            response[JSON_LOG] = insert_responses;
            response[JSON_MESSAGE] = "The insert was processed successfully";
        }
        Logger::info("%s", global_customized_writer.write(response).c_str());
        break;
    }
    case EVHTTP_REQ_DELETE: {

        evkeyvalq headers;
        evhttp_parse_query(req->uri, &headers);

        Json::Value deleteResponse = IndexWriteUtil::_deleteCommand_QueryURI(server->getIndexer(),
        		server->getCoreInfo(), headers);
        response[JSON_MESSAGE] = "The delete was processed successfully";
        response[JSON_LOG] = wrap_with_json_array(deleteResponse);

        Logger::info("%s", global_customized_writer.write(response).c_str());
        // Free the objects
        evhttp_clear_headers(&headers);
        break;
    }
    default: {
        response_to_invalid_request(req, response);
        return;
    }
    };

    if (isSuccess){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
    } else {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST", global_customized_writer.write(response));
    }

}

#if 0
// this function gets the acl command and does the appropriate operations
// the acl command could be add, append or delete
void HTTPRequestHandler::aclModifyRolesForRecord(evhttp_request *req, Srch2Server *server, srch2::instantsearch::RecordAclCommandType commandType){

	Json::Value response(Json::objectValue);
	bool isSuccess = true;
	Json::Value edit_responses(Json::arrayValue);

	if(server->getCoreInfo()->getHasRecordAcl()){ // this core has record Acl

		size_t length = EVBUFFER_LENGTH(req->input_buffer);

		if (length == 0) {
			isSuccess = false;
			response[JSON_MESSAGE] = "http body is empty";
			Logger::warn("http body is empty");
		}

		const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

		// Parse example data
		Json::Value root;
		Json::Reader reader;
		bool parseSuccess = reader.parse(post_data, root, false);

		if (parseSuccess == false) {
			isSuccess = false;
			response[JSON_MESSAGE] = "JSON object parsing error";
			Logger::warn("JSON object parse error");
		}else{
			if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
				vector<string> roleIds;
				for ( int index = 0; index < root.size(); ++index ) {
					Json::Value defaultValueToReturn = Json::Value("");
					const Json::Value doc = root.get(index,
							defaultValueToReturn);
					string primaryKeyID;
					std::stringstream log_str;
					// extract all the role ids from the query
					if( JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID, doc, server->getCoreInfo(), log_str) ){
						if(roleIds.size() != 0){
							log_str << global_customized_writer.write(IndexWriteUtil::_aclRecordModifyRoles(server->getIndexer(), primaryKeyID, roleIds, commandType));
						}
					}

					roleIds.clear();
					edit_responses[index] = log_str.str();
				}
			}else{ // The input is only one JSON object.
				const Json::Value doc = root;
				vector<string> roleIds;
				string primaryKeyID;
				std::stringstream log_str;
				// extract all the role ids from the query
				if( JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID, doc, server->getCoreInfo(), log_str) ){
					if(roleIds.size() != 0){
						log_str << global_customized_writer.write(IndexWriteUtil::_aclRecordModifyRoles(server->getIndexer(), primaryKeyID, roleIds, commandType));
					}
				}
				edit_responses.append(log_str.str());
			}
		}

	}else{
		Logger::error(
				"error: %s does not have record-based access control.",server->getCoreInfo()->getName().c_str());
		response[JSON_MESSAGE] = "error:" + server->getCoreInfo()->getName() + " does not have record-based access control.";
		bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
				global_customized_writer.write(response));
		return;
	}

	response[JSON_LOG] = edit_responses;
	Logger::info("%s", global_customized_writer.write(edit_responses).c_str());
    if (isSuccess){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
    } else {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST", global_customized_writer.write(response));
    }
}


// gets the acl command from the role view and modifies the access list.
// curl "http://localhost:8081/product/AclAddRecordsForRoles" -i -X PUT -d '{"roleId": “1234", “resourceId”: ["33", "45"]}'
void HTTPRequestHandler::aclModifyRecordsForRole(evhttp_request *req, Srch2Server *server, srch2::instantsearch::RecordAclCommandType commandType){

	Json::Value response(Json::objectValue);
	bool isSuccess = true;
	Json::Value responseOfAction(Json::arrayValue);

	if(server->getCoreInfo()->getHasRecordAcl()){ // this resource core has a role core

		size_t length = EVBUFFER_LENGTH(req->input_buffer);

		if (length == 0) {
			isSuccess = false;
			response[JSON_MESSAGE] = "http body is empty";
			Logger::warn("http body is empty");
		}

		const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

		// Parse example data
		Json::Value root;
		Json::Reader reader;
		bool parseSuccess = reader.parse(post_data, root, false);

		if (parseSuccess == false) {
			isSuccess = false;
			response[JSON_MESSAGE] = "JSON object parsing error";
			Logger::warn("JSON object parse error");
		}else{
			if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
				vector<string> resourceIds;
				vector<string> removedIds;
				string removedRoleIds = "";
				for ( int index = 0; index < root.size(); ++index ) {
					Json::Value defaultValueToReturn = Json::Value("");
					const Json::Value doc = root.get(index,
							defaultValueToReturn);
					string roleID;
					std::stringstream log_str;
					// 1- extract roleId and resourceIds from the request
					// 2- check that records with these resourceIds exist in the resourceCore or not
					if( JSONRecordParser::_extractRoleAndResourceIds(resourceIds, roleID, doc, server->getCoreInfo(), log_str) ){

						for(vector<string>::iterator i = resourceIds.begin() ; i != resourceIds.end() ; ){
							INDEXLOOKUP_RETVAL returnValue = server->indexer->lookupRecord(*i);

							if(returnValue == LU_ABSENT_OR_TO_BE_DELETED){
								// there is no record in role core with this id
								// we should remove this id from roleIds
								removedIds.push_back(*i);
								i = resourceIds.erase(i);
							}else{
								++i;
							}
						}

						for(unsigned i = 0 ; i < removedIds.size() ; ++i){
							removedRoleIds = removedRoleIds + ", " + removedIds[i];
						}

						if(removedIds.size() != 0){
							log_str << "Warning: No record in " + server->getCoreInfo()->getName() + " with these primary keys: [" << removedRoleIds << "]";
						}

						if(resourceIds.size() != 0){
							log_str << global_customized_writer.write(IndexWriteUtil::_aclModifyRecordsOfRole(server->indexer, roleID, resourceIds, commandType));
						}
					}

					resourceIds.clear();
					removedIds.clear();
					removedRoleIds = "";
					responseOfAction[index] = log_str.str();
				}
			}else{ // The input is only one JSON object.
				const Json::Value doc = root;
				vector<string> resourceIds;
				vector<string> removedIds;
				string removedRoleIds = "";
				string roleID;
				std::stringstream log_str;
				// extract all the role ids from the query
				if( JSONRecordParser::_extractRoleAndResourceIds(resourceIds, roleID, doc, server->indexDataConfig, log_str) ){
					for(vector<string>::iterator i = resourceIds.begin() ; i != resourceIds.end() ; ){
						INDEXLOOKUP_RETVAL returnValue = server->getIndexer()->lookupRecord(*i);
						if(returnValue == LU_ABSENT_OR_TO_BE_DELETED){
							// there is no record in role core with this id
							// we should remove this id from roleIds
							removedIds.push_back(*i);
							i = resourceIds.erase(i);
						}else{
							++i;
						}
					}

					for(unsigned i = 0 ; i < removedIds.size() ; ++i){
						removedRoleIds = removedRoleIds + ", " + removedIds[i];
					}

					if(removedIds.size() != 0){

						log_str << "Warning: No record in " + server->getCoreInfo()->getName() + " with these primary keys: [" << removedRoleIds << "]";
					}

					if(resourceIds.size() != 0){
						log_str << global_customized_writer.write(IndexWriteUtil::_aclModifyRecordsOfRole(server->indexer, roleID, resourceIds, commandType));
					}
				}
				responseOfAction.append(log_str.str());
			}
		}

	}else{
		Logger::error(
				"error: %s does not record-based access control.",server->getCoreInfo()->getName().c_str());
		response[JSON_MESSAGE] = "error:" + server->getCoreInfo()->getName() + " does not have record-based access control.";
		bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID REQUEST",
				global_customized_writer.write(response));
		return;
	}

	response[JSON_LOG] = responseOfAction;
    if (isSuccess){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
    } else {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST", global_customized_writer.write(response));
    }
}
// overwrites role ids in a record's access list
// example : Suppose we have a resource core called "product" with a primary key attribute called "pid then the query is like:
// curl "http://localhost:8081/product/aclRecordRoleAdd" -i -X PUT -d '{"pid": "1234", "roleId": ["33", "45"]}'
//
void HTTPRequestHandler::aclRecordRoleReplace(evhttp_request *req, Srch2Server *server){
	aclModifyRolesForRecord(req, server, srch2::instantsearch::Acl_Record_Add);
}

// add role ids to a record
// example : Suppose we have a resource core called "product" with a primary key attribute called "pid then the query is like:
// curl "http://localhost:8081/product/aclRecordRoleAppend" -i -X PUT -d '{"pid": "1234", "roleId": ["33", "45"]}'
//
void HTTPRequestHandler::aclRecordRoleAppend(evhttp_request *req, Srch2Server *server){
	aclModifyRolesForRecord(req, server, srch2::instantsearch::Acl_Record_Append);
}

// delete role ids from a records access list
// example : Suppose we have a resource core called "product" with a primary key attribute called "pid then the query is like:
// curl "http://localhost:8081/product/aclRecordRoleDelete" -i -X PUT -d '{"pid": "1234", "roleId": ["33", "45"]}'
//
void HTTPRequestHandler::aclRecordRoleDelete(evhttp_request *req, Srch2Server *server){
	aclModifyRolesForRecord(req, server, srch2::instantsearch::Acl_Record_Delete);
}



void HTTPRequestHandler::aclAddRecordsForRole(evhttp_request *req, Srch2Server *server){
	aclModifyRecordsForRole(req, server, srch2::instantsearch::Acl_Record_Add);
}

void HTTPRequestHandler::aclAppendRecordsForRole(evhttp_request *req, Srch2Server *server){
	aclModifyRecordsForRole(req, server, srch2::instantsearch::Acl_Record_Append);
}

void HTTPRequestHandler::aclDeleteRecordsForRole(evhttp_request *req, Srch2Server *server){
	aclModifyRecordsForRole(req, server, srch2::instantsearch::Acl_Record_Delete);
}
#endif
void HTTPRequestHandler::updateCommand(evhttp_request *req,
        Srch2Server *server) {
    /* Yes, we are expecting a post request */

    Json::Value response(Json::objectValue);
    bool isSuccess = true;
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        size_t length = EVBUFFER_LENGTH(req->input_buffer);

        if (length == 0) {
            isSuccess = false;
            response[JSON_MESSAGE] = "http body is empty";
            Logger::warn("http body is empty");
            break;
        }

        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

        //std::cout << "length:[" << length << "][" << string(post_data) << "]" << std::endl;

        // Parse example data
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(post_data, root, false);

        if (parseSuccess == false) {
            isSuccess = false;
            response[JSON_MESSAGE] = "JSON object parse error";
            Logger::warn( response[JSON_MESSAGE].asCString());
        } else {
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            Record *record = new Record(server->getIndexer()->getSchema());

            Json::Value update_responses(Json::arrayValue);
            if (root.type() == Json::arrayValue) {
                //the record parameter is an array of json objects
                update_responses.resize(root.size());
                for(Json::UInt index = 0; index < root.size(); index++) {
                    Json::Value defaultValueToReturn = Json::Value("");
                    const Json::Value doc = root.get(index,
                                                defaultValueToReturn);

                    update_responses[index] = 
                        IndexWriteUtil::_updateCommand(server->getIndexer(),
                            server->getCoreInfo(), headers, doc, record);

                    record->clear();
                }
            } else {
                // the record parameter is a single json object
                const Json::Value doc = root;
                update_responses.append(IndexWriteUtil::_updateCommand(server->getIndexer(),
                        server->getCoreInfo(), headers, doc, record));
                record->clear();
            }

            delete record;
            evhttp_clear_headers(&headers);
            response[JSON_LOG] = update_responses;
            response[JSON_MESSAGE] = "The update was processed successfully";
        }
        Logger::info("%s", global_customized_writer.write(response).c_str());
        break;
    }
    default: {
        response_to_invalid_request(req, response);
        return;
    }
    };

    if (isSuccess){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
    } else {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST", global_customized_writer.write(response));
    }
}

void HTTPRequestHandler::saveCommand(evhttp_request *req, Srch2Server *server) {
    /* Yes, we are expecting a post request */
    Json::Value response(Json::objectValue);
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        response[JSON_LOG] = wrap_with_json_array( IndexWriteUtil::_saveCommand(server->getIndexer()));
        response[JSON_MESSAGE] = "The indexes have been saved to disk successfully";

        //TODO : commented out in merge, must be taken care of when we bring in adaptors
//        //Call the save function implemented by each database connector.
//        DataConnectorThread::saveConnectorTimestamps();

        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
        Logger::info("%s", response[JSON_MESSAGE].asString().c_str());
        break;
    }
    default: {
        response_to_invalid_request(req, response);
        return;
    }
    };
}

void HTTPRequestHandler::shutdownCommand(evhttp_request *req, const CoreNameServerMap_t *coreNameServerMap){
    /* Yes, we are expecting a post request */
    Json::Value response(Json::objectValue);
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        // graceful shutdown
        // since the main process is catching the kill signal, we can simply send the kill to itself
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
                "{\"message\":\"Bye\"}\n");
        Logger::info("Server is shuting down");
        kill(getpid(),SIGTERM);
        break;
    }
    default: {
        response_to_invalid_request(req, response);
        return;
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
    Json::Value response(Json::objectValue);
    switch(req->type) {
    case EVHTTP_REQ_PUT: {
        // create a FILE* pointer to point to the new logger file "logger.txt"
        FILE *logFile = fopen(server->getCoreInfo()->getHTTPServerAccessLogFile().c_str(),
                    "a");

        if (logFile == NULL) {
            response[JSON_MESSAGE] = "The logger file repointing failed. Could not create a new logger file";
            response[JSON_LOG] = wrap_with_json_array( server->getCoreInfo()->getHTTPServerAccessLogFile());

            Logger::error("Reopen Log file %s failed.",
                    server->getCoreInfo()->getHTTPServerAccessLogFile().c_str());
            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "REQUEST FAILED", global_customized_writer.write(response));
        } else {
            FILE * oldLogger = Logger::swapLoggerFile(logFile);
            fclose(oldLogger);
            response[JSON_MESSAGE] = "The logger file repointing succeeded";
            response[JSON_LOG] = wrap_with_json_array(server->getCoreInfo()->getHTTPServerAccessLogFile());
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
        }
        break;
    }
    default: {
        response_to_invalid_request(req, response);
    }
    };
}


// exportCommand: if search-response-format is 0 or 2, we keep the compressed Json data in Forward Index, we can uncompress the data and export to a file
void HTTPRequestHandler::exportCommand(evhttp_request *req, Srch2Server *server) {
    /* Yes, we are expecting a post request */
    Json::Value response(Json::objectValue);
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
        // if search-response-format is 0 or 2
        if (server->getCoreInfo()->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            std::stringstream log_str;
            evkeyvalq headers;
            evhttp_parse_query(req->uri, &headers);
            const char *exportedDataFileName = evhttp_find_header(&headers, URLParser::nameParamName);
            if(exportedDataFileName){
                if(checkDirExistence(exportedDataFileName)){
                    exportedDataFileName = "export_data.json";
                }
                Json::Value export_response = IndexWriteUtil::_exportCommand(server->getIndexer(), exportedDataFileName);

                response[JSON_MESSAGE] ="The indexed data has been exported to the file "+ string(exportedDataFileName) +" successfully."; 
                response[JSON_LOG] = wrap_with_json_array(export_response);
                std::string str_response = global_customized_writer.write( response);
                bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", str_response);
                Logger::info("%s", str_response.c_str());
            }else {
                response_to_invalid_request(req, response);
            }
        } else{
            response[JSON_MESSAGE] = "The indexed data failed to export to disk, The request need to set search-response-format to be 0 or 2";
            bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response));
        }
        break;
    }
    default: {
        response_to_invalid_request(req, response);
    }
    };
}

void HTTPRequestHandler::infoCommand(evhttp_request *req, Srch2Server *server,
        const string &versioninfo) {
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    const char* c_key = "engine_status";
    Json::Value response(Json::objectValue);
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(server->getIndexer()->getIndexHealth(), root);
    if (parseSuccess){
        response[c_key] = root[c_key];
    } else {
        response[c_key] = server->getIndexer()->getIndexHealth();
    }
    response["version"] = versioninfo;

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response) , headers);
    evhttp_clear_headers(&headers);
}

void HTTPRequestHandler::lookupCommand(evhttp_request *req,
        Srch2Server *server) {
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    Json::Value response(Json::objectValue);
    const CoreInfo_t *indexDataContainerConf = server->getCoreInfo();
    string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    const char *pKeyParamName = evhttp_find_header(&headers,
            primaryKeyName.c_str());

    if (pKeyParamName) {
        size_t sz;
        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
        delete pKeyParamName_cstar;

        response["rid"] = primaryKeyStringValue;

        //lookup the record on the index
        const string LOG = "lookup";
        switch (server->getIndexer()->lookupRecord(primaryKeyStringValue)) {
        case srch2is::LU_ABSENT_OR_TO_BE_DELETED: {
            response[LOG] = "absent or to be deleted";
            break;
        }
        case srch2is::LU_TO_BE_INSERTED: {
            response[LOG] = "to be inserted";
            break;
        }
        default: // LU_PRESENT_IN_READVIEW_AND_WRITEVIEW
        {
            response[LOG] = "present in readview and writeview";
        }
        };
    } else {
        response["rid"] = "NULL";
        response["lookup"] = "failed";
        response["reason"] = "no record with given primary key";
    }

    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(response), headers);
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

#if 0
/*
 *   Wrapper layer API to handle ACL operations such as insert, delete, and append.
 *   example url :
 *   http://<ip>:<port>/aclAttributeRoleAppend -X PUT -d { "attributes": "f1,f2", "roleId": "r1"}
 *   http://<ip>:<port>/aclAttributeRoleReplace -X PUT -d { "attributes": "f1,f2", "roleId": "r2"}
 *   http://<ip>:<port>/aclAttributeRoleDelete -X PUT -d { "attributes": "f2", "roleId": "r2"}
 */
void HTTPRequestHandler::attributeAclModify(evhttp_request *req, Srch2Server *server) {
	Json::Value response(Json::objectValue);
	switch (req->type) {
	    case EVHTTP_REQ_PUT: {
	        size_t length = EVBUFFER_LENGTH(req->input_buffer);

	        if (length == 0) {
	            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "BAD REQUEST",
	                    "{\"message\":\"http body is empty\"}");
	            Logger::warn("http body is empty");
	            break;
	        }

	        // Identify the type of access control request.
	        // req->uri should be "/aclAttributeRoleDelete" or "/aclAttributeRoleAppend"
	        // or "/aclAttributeRoleReplace" for default core
	        // Otherwise it should be /corename/aclAttributeRoleDelete etc.
	        string uriString = req->uri;
	        string apiName;
	        AclActionType action;
	        string corename = server->getCoreInfo()->getName();
	        if (corename == ConfigManager::defaultCore) {
	        	corename.clear();
	        } else {
	        	corename = "/" + corename;
	        }
	        if (uriString == corename + "/aclAttributeRoleReplace") {
	        	action = ACL_REPLACE;
	        	apiName = "aclAttributeRoleReplace";
	        }
	        else if (uriString == corename + "/aclAttributeRoleDelete") {
	        	apiName = "aclAttributeRoleDelete";
	        	action = ACL_DELETE;
	        }
	        else if (uriString == corename + "/aclAttributeRoleAppend") {
	        	apiName = "aclAttributeRoleAppend";
	        	action = ACL_APPEND;
	        }
	        else {
	        	stringstream log_str;
	        	log_str << "Error: Invalid access control HTTP request ='" << uriString << "'";
	        	response[JSON_LOG] = log_str.str();
	        	response[JSON_MESSAGE] = "The request was NOT processed successfully";
	        	bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID DATA",
	        			global_customized_writer.write(response));
	        	Logger::info("%s", global_customized_writer.write(response).c_str());
	        	return;
	        }
	        // get input JSON
	        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

	        std::stringstream log_str;
	        Json::Value root;
	        Json::Reader reader;
	        bool parseSuccess = reader.parse(post_data, root, false);
	        bool error = false;
        	Json::Value aclAttributeResponses(Json::arrayValue);
	        if (parseSuccess == false) {
	            log_str << "API : "<< apiName << ", Error: JSON object parse error";
	            response[JSON_LOG] = log_str.str();
	            response[JSON_MESSAGE] = "The request was NOT processed successfully";
	            bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID DATA",
	            		global_customized_writer.write(response));
	            Logger::info("%s", global_customized_writer.write(response).c_str());
	            return;
	        } else {
	        	const AttributeAccessControl& attrAcl = server->getIndexer()->getAttributeAcl();
	        	if (root.type() == Json::arrayValue) {
	        		aclAttributeResponses.resize(root.size());
	        		//the record parameter is an array of json objects
	        		for(Json::UInt index = 0; index < root.size(); index++) {
	        			Json::Value defaultValueToReturn = Json::Value("");
	        			const Json::Value doc = root.get(index,
	        					defaultValueToReturn);

	        			bool  status = attrAcl.processSingleJSONAttributeAcl(doc, action, apiName,
	        					aclAttributeResponses[index]);
	        			if (status == false) {
	        				error = true;
	        				break;
	        			} else {
	        				// if the response is empty then add success message.
	        				if (aclAttributeResponses[index].asString().size() == 0){
	        					stringstream ss;
	        					ss << "API : " << apiName << ", Success";
	        					aclAttributeResponses[index] = ss.str();
	        				}
	        			}
	        		}
	        	} else {
	        		aclAttributeResponses.resize(1);
	        		// the record parameter is a single json object
	        		const Json::Value doc = root;
	        		bool  status = attrAcl.processSingleJSONAttributeAcl(doc, action, apiName,
	        				aclAttributeResponses[0]);
	        		if (status == false) {
	        			error = true;
	        		} else {
	        			// if the response is empty then add success message.
	        			if (aclAttributeResponses[0].asString().size() == 0){
	        				stringstream ss;
	        				ss << "API : " << apiName << ", Success";
	        				aclAttributeResponses[0] = ss.str();
	        			}
	        		}
	        	}
	        }

	        if (!error) {
	        	response[JSON_LOG] = aclAttributeResponses;
	        	response[JSON_MESSAGE] = "The batch was processed successfully";
	        	bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
	        			global_customized_writer.write(response));
	        } else {
	        	response[JSON_LOG] = aclAttributeResponses;
	        	response[JSON_MESSAGE] = "The request was NOT processed successfully";
	        	bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "INVALID DATA",
	        			global_customized_writer.write(response));
	        }
	        Logger::info("%s", global_customized_writer.write(response).c_str());
	        break;
	    }
	    default:
	    	response_to_invalid_request(req, response);

	}
}
#endif
#if 0
void HTTPRequestHandler::searchCommand(evhttp_request *req,
        Srch2Server *server) {
    evkeyvalq headers;

    std::stringstream errorStream;
    boost::shared_ptr<Json::Value> root = doSearchOneCore( req, server, &headers, errorStream );

    if (root ){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(*root), headers);
    } else{
        Json::Value errorResponse(Json::objectValue);
        errorResponse["error"] = errorStream.str();
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", global_customized_writer.write(errorResponse), headers);
    }
    evhttp_clear_headers(&headers);
}

//    const CoreInfo_t *indexDataContainerConf = server->getCoreInfo();

boost::shared_ptr<Json::Value> HTTPRequestHandler::doSearchOneCore(evhttp_request *req,
        Srch2Server *server, evkeyvalq* headers, std::stringstream &errorStream) {

    boost::shared_ptr<Json::Value> root;
    ParsedParameterContainer paramContainer;

    if(server->getCoreInfo()->getHasRecordAcl()){
    	paramContainer.hasRoleCore = true;
    }

//    string decodedUri;
//    decodeAmpersand(req->uri, strlen(req->uri), decodedUri);
    evhttp_parse_query(req->uri, headers);
    //cout << "Query: " << req->uri << endl;
    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
    //1. first create query parser to parse the url
    QueryParser qp(*headers, &paramContainer);
    bool isSyntaxValid = qp.parse(NULL);
    if (!isSyntaxValid) {
        // if the query is not valid print the error message to the response
        errorStream << paramContainer.getMessageString();
        return root;
    }

//    clock_gettime(CLOCK_REALTIME, &tend);
//    unsigned parserTime = (tend.tv_sec - tstart2.tv_sec) * 1000
//            + (tend.tv_nsec - tstart2.tv_nsec) / 1000000;


    // start the timer for search
    struct timespec tstart;
//    struct timespec tstart2;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
//    clock_gettime(CLOCK_REALTIME, &tstart2);

    const CoreInfo_t *indexDataContainerConf = server->getCoreInfo();
    //2. validate the query
    QueryValidator qv(*(server->getIndexer()->getSchema()),
            *(server->getCoreInfo()), &paramContainer,
            server->getIndexer()->getAttributeAcl());

    bool valid = qv.validate();

    if (!valid) {
        // if the query is not valid, print the error message to the response
        errorStream << paramContainer.getMessageString();
        return root;
    }
    //3. rewrite the query and apply analyzer and other stuff ...
    QueryRewriter qr(server->getCoreInfo(),
            *(server->getIndexer()->getSchema()),
            *(AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf)),
            &paramContainer, server->getIndexer()->getAttributeAcl());
    LogicalPlan logicalPlan;
    if(qr.rewrite(logicalPlan) == false){
        // if the query is not valid, print the error message to the response
        errorStream << paramContainer.getMessageString();
        return root;
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
     *  3. Highlight is not turned off in the query ( default is on )
     */
    struct timespec hltstart;
    clock_gettime(CLOCK_REALTIME, &hltstart);

    if (server->getCoreInfo()->getHighlightAttributeIdsVector().size() > 0 &&
    		!paramContainer.onlyFacets &&
    		paramContainer.isHighlightOn) {

    	ServerHighLighter highlighter =  ServerHighLighter(finalResults, server, paramContainer,
    			logicalPlan.getOffset(), logicalPlan.getNumberOfResultsToRetrieve());
    	highlightInfo.reserve(logicalPlan.getNumberOfResultsToRetrieve());
    	//highlighter.generateSnippets(highlightInfo);
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
        root = HTTPRequestHandler::printResults(req, *headers, logicalPlan,
                indexDataContainerConf, finalResults, logicalPlan.getExactQuery(),
                server->getIndexer(), logicalPlan.getOffset(),
                finalResults->getNumberOfResults(),
                finalResults->getNumberOfResults(), paramContainer.roleId,
                paramContainer.getMessageString(), ts1, tstart, tend, highlightInfo, hlTime,
                paramContainer.onlyFacets);

        break;

    case srch2is::SearchTypeGetAllResultsQuery:
        finalResults->printStats();
        if(finalResults->impl->estimatedNumberOfResults < finalResults->impl->sortedFinalResults.size()){
			finalResults->impl->estimatedNumberOfResults = finalResults->impl->sortedFinalResults.size();
        }
        if (logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve()
                > finalResults->getNumberOfResults()) {
            // Case where you have return 10,20, but we got only 0,15 results.
            root = HTTPRequestHandler::printResults(req, *headers, logicalPlan,
                    indexDataContainerConf, finalResults,
                    logicalPlan.getExactQuery(), server->getIndexer(),
                    logicalPlan.getOffset(), finalResults->getNumberOfResults(),
                    finalResults->getNumberOfResults(), paramContainer.roleId,
                    paramContainer.getMessageString(), ts1, tstart, tend , highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        } else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
            root = HTTPRequestHandler::printResults(req, *headers, logicalPlan,
                    indexDataContainerConf, finalResults,
                    logicalPlan.getExactQuery(), server->getIndexer(),
                    logicalPlan.getOffset(),
                    logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve(),
                    finalResults->getNumberOfResults(), paramContainer.roleId,
                    paramContainer.getMessageString(), ts1, tstart, tend, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        }
        break;
    case srch2is::SearchTypeRetrieveById:
        finalResults->printStats();
        root = HTTPRequestHandler::printOneResultRetrievedById(req,
                *headers,
                logicalPlan ,
                indexDataContainerConf,
                finalResults ,
                server->getIndexer() ,
                paramContainer.roleId,
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
    delete finalResults;
    delete resultsFactory;
    return root;
}

void HTTPRequestHandler::searchAllCommand(evhttp_request *req, const CoreNameServerMap_t * coreNameServerMap){

    evkeyvalq headers;
    Json::Value root(Json::objectValue);
    int cSuccess = 0;
    for( CoreNameServerMap_t::const_iterator it = coreNameServerMap->begin(); 
            it != coreNameServerMap->end(); ++it){
        std::stringstream errorStream;
        boost::shared_ptr<Json::Value> subRoot = doSearchOneCore( req, it->second, &headers, errorStream );
        Json::Value errorResponse(Json::objectValue);
        errorResponse["error"] = errorStream.str();

        if (subRoot ){
            root[it->first] = *subRoot;
            cSuccess +=1;
        } else {
            root[it->first] = errorResponse;
        }
    }

    //We return SUCCESS as long as one of the cores succeeds.
    if (cSuccess > 0){
        bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", global_customized_writer.write(root), headers);
    } else {
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", global_customized_writer.write(root), headers);
    }
    evhttp_clear_headers(&headers);
}
#endif

void HTTPRequestHandler::suggestCommand(evhttp_request *req, Srch2Server *server){
    // start the timer for search
    struct timespec tstart;
    clock_gettime(CLOCK_REALTIME, &tstart);


    const CoreInfo_t *indexDataContainerConf = server->getCoreInfo();

    // 1. first parse the headers
    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    Json::Value response(Json::objectValue);
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
        response["error"] = messagesString;
        bmhelper_evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request",
                global_customized_writer.write(response), headers);
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
    QueryEvaluator * queryEvaluator = new QueryEvaluator(server->getIndexer() , &runTimeParameters);
    vector<string> suggestions ;
    int numberOfSuggestionsFound = queryEvaluator->suggest(keyword , fuzzyMatchPenalty , numberOfSuggestionsToReturn , suggestions);
    delete queryEvaluator;

    // compute elapsed time in ms , end the timer
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    // 4. Print the results
    printSuggestions(req , headers , suggestions , server->getIndexer() , messagesString , ts1 , tstart , tend);

}

void HTTPRequestHandler::handleException(evhttp_request *req) {
    const string INTERNAL_SERVER_ERROR_MSG =
            "{\"error:\" Ooops!! The engine failed to process this request. Please check srch2 server logs for more details. If the problem persists please contact srch2 inc.}";
    bmhelper_evhttp_send_reply(req, HTTP_INTERNAL, "INTERNAL SERVER ERROR",
            INTERNAL_SERVER_ERROR_MSG);
}

}
}
