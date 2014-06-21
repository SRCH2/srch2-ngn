#include "SearchResultsAggregatorAndPrint.h"
#include "core/util/RecordSerializerUtil.h"
#include "sharding/routing/PendingMessages.h"
namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

SearchResultsAggregator::SearchResultsAggregator(ConfigManager * configurationManager, evhttp_request *req,
		boost::shared_ptr<const Cluster> clusterReadview, unsigned coreId) :
		ResponseAggregator<SearchCommand , SearchCommandResults>(clusterReadview, coreId){
    this->configurationManager = configurationManager;
    this->req = req;
}
LogicalPlan & SearchResultsAggregator::getLogicalPlan(){
    return logicalPlan;
}
ParsedParameterContainer * SearchResultsAggregator::getParamContainer(){
    return &paramContainer;
}

void SearchResultsAggregator::setParsingValidatingRewritingTime(unsigned time){
    this->parsingValidatingRewritingTime = time;
}

unsigned SearchResultsAggregator::getParsingValidatingRewritingTime(){
    return this->parsingValidatingRewritingTime;
}

struct timespec & SearchResultsAggregator::getStartTimer(){
    return this->tstart;
}


/*
 * This function is called by RoutingManager if a timeout happens, The call to
 * this function must be between preProcessing(...) and callBack()
 */
void SearchResultsAggregator::processTimeout(PendingMessage<SearchCommand,
        SearchCommandResults> * message,
        ResponseAggregatorMetadata metadata){

    boost::unique_lock< boost::shared_mutex > lock(_access);
    messages << ", WARNING : Shard #"<< message->getNodeId()<<" timed out.";

}

/*
 * The main function responsible of aggregating search results
 * this function uses aggregateRecords and aggregateFacets for
 * aggregating result records and calculated records
 */
void SearchResultsAggregator::callBack(vector<PendingMessage<SearchCommand,
        SearchCommandResults> * > messages){

    // to protect messages
    boost::unique_lock< boost::shared_mutex > lock(_access);

    // move on all responses of all shards and use them
    for(int responseIndex = 0 ; responseIndex < messages.size() ; ++responseIndex ){
        if(messages.at(responseIndex) == NULL || messages.at(responseIndex)->getResponseObject() == NULL){
            continue;
        }
        QueryResults * resultsOfThisShard = messages.at(responseIndex)->getResponseObject()->getQueryResults();
        const map<string, std::pair<string, RecordSnippet> > & queryResultInMemoryRecordString =
                messages.at(responseIndex)->getResponseObject()->getInMemoryRecordStrings();
        resultsOfAllShards.push_back(
                make_pair(resultsOfThisShard,  &queryResultInMemoryRecordString ));
        if(results.aggregatedSearcherTime < messages.at(responseIndex)->getResponseObject()->getSearcherTime()){
            results.aggregatedSearcherTime = messages.at(responseIndex)->getResponseObject()->getSearcherTime();
        }
    }
    aggregateRecords();
    aggregateFacets();

}

// print results on HTTP channel
void SearchResultsAggregator::printResults(){

    // CoreInfo_t is a view of configurationManager which contains all information for the
    // core that we want to search on, this object is accesses through configurationManager.
    const CoreInfo_t *indexDataContainerConf = getClusterReadview()->getCoreById(getCoreId());


    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);


    vector<RecordSnippet> highlightInfo;
    /*
     *  Do snippet generation only if
     *  1. There are attributes marked to be highlighted
     *  2. Query is not facet only
     *  3. Highlight is not turned off in the query ( default is on )
     */
    struct timespec hltstart;
    clock_gettime(CLOCK_REALTIME, &hltstart);

    //    if (server->indexDataConfig->getHighlightAttributeIdsVector().size() > 0 &&
    //            !paramContainer.onlyFacets &&
    //            paramContainer.isHighlightOn) {
    //
    //        ServerHighLighter highlighter =  ServerHighLighter(finalResults, server, paramContainer,
    //                logicalPlan.getOffset(), logicalPlan.getNumberOfResultsToRetrieve());
    //        highlightInfo.reserve(logicalPlan.getNumberOfResultsToRetrieve());
    //        highlighter.generateSnippets(highlightInfo);
    //        if (highlightInfo.size() == 0 && finalResults->getNumberOfResults() > 0) {
    //            Logger::warn("Highligting is on but snippets were not generated!!");
    //        }
    //    }

    struct timespec hltend;
    clock_gettime(CLOCK_REALTIME, &hltend);
    unsigned hlTime = (hltend.tv_sec - hltstart.tv_sec) * 1000
            + (hltend.tv_nsec - hltstart.tv_nsec) / 1000000;

    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned parseAndSearchTime = (tend.tv_sec - getStartTimer().tv_sec) * 1000
            + (tend.tv_nsec - getStartTimer().tv_nsec) / 1000000;

    //5. call the print function to print out the results
    switch (logicalPlan.getQueryType()) {
    case srch2is::SearchTypeTopKQuery:
    {
        //            finalResults->printStats();
    	unsigned start = logicalPlan.getOffset();
    	unsigned end ;
    	if(results.allResults.size() < logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve()){
    		end = results.allResults.size();
    	}else{
    		end = logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve();
    	}
        printResults(req, headers, logicalPlan,
                indexDataContainerConf, results.allResults,
                logicalPlan.getExactQuery(),
                start,
                end,
                results.allResults.size(),
                paramContainer.getMessageString() + messages.str(), parseAndSearchTime , highlightInfo, hlTime,
                paramContainer.onlyFacets);

        break;
    }
    case srch2is::SearchTypeGetAllResultsQuery:
    case srch2is::SearchTypeMapQuery:
        //        finalResults->printStats();
        if(results.aggregatedEstimatedNumberOfResults < results.allResults.size()){
            results.aggregatedEstimatedNumberOfResults = results.allResults.size();
        }
        if (logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve()
                > results.allResults.size()) {
            // Case where you have return 10,20, but we got only 0,15 results.
            printResults(req, headers, logicalPlan,
                    indexDataContainerConf, results.allResults,
                    logicalPlan.getExactQuery(),
                    logicalPlan.getOffset(), results.allResults.size(),
                    results.allResults.size(),
                    paramContainer.getMessageString()+ messages.str(), parseAndSearchTime, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        } else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
            printResults(req, headers, logicalPlan,
                    indexDataContainerConf, results.allResults,
                    logicalPlan.getExactQuery(),
                    logicalPlan.getOffset(),
                    logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve(),
                    results.allResults.size(),
                    paramContainer.getMessageString()+ messages.str(), parseAndSearchTime, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        }
        break;
    case srch2is::SearchTypeRetrieveById:
        //        finalResults->printStats();
        printOneResultRetrievedById(req,
                headers,
                logicalPlan ,
                indexDataContainerConf,
                results.allResults ,
                paramContainer.getMessageString()+ messages.str() ,
                parseAndSearchTime);
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
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void SearchResultsAggregator::printResults(evhttp_request *req,
        const evkeyvalq &headers, const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<pair< QueryResult *, MapStringPtr> > allResults,
        const Query *query,
        const unsigned start, const unsigned end,
        const unsigned retrievedResults, const string & message,
        const unsigned ts1 , const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) {


    // start the timer for printing
    struct timespec tstart;
    // end the timer for printing
    struct timespec tend;

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
    unsigned resultFound = end - start;
    root["searcher_time"] = ts1;
    clock_gettime(CLOCK_REALTIME, &tstart);

    if(onlyFacets == false){ // We send the matching records only if "facet != only".
        root["results"].resize(end - start);
        unsigned counter = 0;
        if (queryPlan.getQueryType() == srch2is::SearchTypeMapQuery
                && query->getQueryTerms()->empty()) //check if the query type is range query without keywords
        {
            for (unsigned i = start; i < end; ++i) {
                char * inMemoryCharPtr = new char[allResults.at(i).second->size()];
                memcpy(inMemoryCharPtr, allResults.at(i).second->c_str(), allResults.at(i).second->size());
                StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i).second->size());
                if (inMemoryData.start.get() == NULL) {
                    --resultFound;
                    continue;
                }
                root["results"][counter]["record_id"] = allResults.at(i).first->internalRecordId;
                root["results"][counter]["score"] = (0
                        - allResults.at(i).first->_score.getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR){
                    string sbuffer;
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId, sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                    string sbuffer;
                    const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId,
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
                char * inMemoryCharPtr = new char[allResults.at(i).second->size()];
                memcpy(inMemoryCharPtr, allResults.at(i).second->c_str(), allResults.at(i).second->size());
                StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i).second->size());
                if (inMemoryData.start.get() == NULL) {
                    --resultFound;
                    continue;
                }
                root["results"][counter]["record_id"] = allResults.at(i).first->internalRecordId;
                root["results"][counter]["score"] = allResults.at(i).first->_score.getFloatTypedValue();

                // print edit distance vector
                vector<unsigned> editDistances;
                editDistances.assign(allResults.at(i).first->editDistances.begin(), allResults.at(i).first->editDistances.end() );

                root["results"][counter]["edit_dist"].resize(editDistances.size());
                for (unsigned int j = 0; j < editDistances.size(); ++j) {
                    root["results"][counter]["edit_dist"][j] = editDistances[j];
                }

                // print matching keywords vector
                vector<std::string> matchingKeywords;
                matchingKeywords.assign(allResults.at(i).first->matchingKeywords.begin(), allResults.at(i).first->matchingKeywords.end() );

                root["results"][counter]["matching_prefix"].resize(
                        matchingKeywords.size());
                for (unsigned int j = 0; j < matchingKeywords.size(); ++j) {
                    root["results"][counter]["matching_prefix"][j] =
                            matchingKeywords[j];
                }
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
                    unsigned internalRecordId = allResults.at(i).first->internalRecordId;
                    string sbuffer;
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId,sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                    unsigned internalRecordId = allResults.at(i).first->internalRecordId;
                    string sbuffer;
                    const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId,
                            sbuffer, attrToReturn);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                }

                string sbuffer = string();
                sbuffer.reserve(1024);
                genSnippetJSONString(allResults.at(i).second.getRecordSnippet(), sbuffer);
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
    root["results_found"] = resultFound;

    long int estimatedNumberOfResults = results.aggregatedEstimatedNumberOfResults;
    // Since estimation of number of results can return a wrong number, if this value is less
    // than the actual number of found results, we use the real number.
    if(estimatedNumberOfResults < (long int)resultFound){
        estimatedNumberOfResults = (long int)resultFound;
    }
    if(estimatedNumberOfResults != -1){
        // at this point we know for sure that estimatedNumberOfResults is positive, so we can cast
        // it to unsigned (because the thirdparty library we use here does not accept long integers.)
        root["estimated_number_of_results"] = (unsigned)estimatedNumberOfResults;
    }
    if(results.isResultsApproximated == true){
        root["result_set_approximation"] = true;
    }

    //    }

    const std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > > * facetResults =
            &this->results.aggregatedFacetResults;
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
    bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK", writer.write(root), headers);
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void SearchResultsAggregator::printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
        const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<pair< QueryResult *, MapStringPtr> > allResults,
        const string & message,
        const unsigned ts1){

    struct timespec tstart;
    struct timespec tend;

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
    root["results"].resize(allResults.size());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    unsigned resultFound = allResults.size();
    for (unsigned i = 0; i < allResults.size(); ++i) {
        char * inMemoryCharPtr = new char[allResults.at(i).second->size()];
        memcpy(inMemoryCharPtr, allResults.at(i).second->c_str(), allResults.at(i).second->size());
        StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i).second->size());
        if (inMemoryData.start.get() == NULL) {
            --resultFound;
            continue;
        }
        root["results"][counter]["record_id"] = allResults.at(i).first->internalRecordId;

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            unsigned internalRecordId = allResults.at(i).first->internalRecordId;
            string sbuffer;
            genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId, sbuffer);
            root["results"][counter][internalRecordTags.first] = sbuffer;
        } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
            unsigned internalRecordId = allResults.at(i).first->internalRecordId;
            string sbuffer;
            const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
            genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i).first->externalRecordId,
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
    root["results_found"] = resultFound;

    root["message"] = message;
    Logger::info(
            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
            req->remote_host, req->remote_port, req->uri + 1, ts1, ts2);
    bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK", writer.write(root), headers);
}



void SearchResultsAggregator::genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
        const string& extrnalRecordId, string& sbuffer){
    genRecordJsonString(schema, buffer, extrnalRecordId,
            sbuffer, NULL);
}
void SearchResultsAggregator::genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
        const string& externalRecordId, string& sbuffer, const vector<string>* attrToReturn){
    Schema * storedSchema = Schema::create();
    srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    srch2::util::RecordSerializerUtil::convertCompactToJSONString(storedSchema, buffer, externalRecordId, sbuffer, attrToReturn);
    delete storedSchema;
}

/*
 *   This functions removes new line and non-printable characters from the input string
 *   and returns a clean string.
 */
void SearchResultsAggregator::cleanAndAppendToBuffer(const string& in, string& out) {
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

void SearchResultsAggregator::genSnippetJSONString(const RecordSnippet& recordSnippet, string& sbuffer) {
	sbuffer.append("{");
	for (unsigned j = 0 ; j <  recordSnippet.fieldSnippets.size(); ++j) {
		sbuffer+='"'; sbuffer+=recordSnippet.fieldSnippets[j].FieldId; sbuffer+='"';
		sbuffer+=":[";
		for (unsigned k = 0 ; k <  recordSnippet.fieldSnippets[j].snippet.size(); ++k) {
			sbuffer+='"';
			cleanAndAppendToBuffer(recordSnippet.fieldSnippets[j].snippet[k], sbuffer);
			sbuffer+='"';
			sbuffer+=',';
		}
		if (recordSnippet.fieldSnippets[j].snippet.size())
			sbuffer.erase(sbuffer.length()-1);
		sbuffer+="],";
	}
	if (recordSnippet.fieldSnippets.size())
		sbuffer.erase(sbuffer.length()-1);
	sbuffer.append("}");
}

/*
 * Combines the results coming from all shards and
 * resorts them based on their scores
 */
void SearchResultsAggregator::aggregateRecords(){
    // aggregate results
    for(unsigned resultSetIndex = 0 ; resultSetIndex < resultsOfAllShards.size() ; ++resultSetIndex){
        QueryResults * queryResultsItr = resultsOfAllShards.at(resultSetIndex).first;
        const map<string, std::pair<string, RecordSnippet> > * queryResultsRecordData =
        		resultsOfAllShards.at(resultSetIndex).second;
        for(unsigned queryResultIndex = 0; queryResultIndex < queryResultsItr->impl->sortedFinalResults.size();
                queryResultIndex ++){
            string resultKey = queryResultsItr->getRecordId(queryResultIndex);
            MapStringPtr recordDataStringPtr(queryResultsRecordData->find(resultKey));
            results.allResults.push_back(
                    make_pair(queryResultsItr->impl->sortedFinalResults.at(queryResultIndex),
                            recordDataStringPtr ));
        }
    }

    // sort final results
    std::sort(results.allResults.begin(), results.allResults.end(), SearchResultsAggregator::QueryResultsComparatorOnlyScore());
    // NOTE, TODO :
    // this sort here does NOT guarantee that exact results are higher than fuzzy results. This problem must be addressed in future by
    // adding a flag to the QueryResult class to indicate whether this results is exact or fuzzy. And here we should sort exact results
    // together and fuzzy results together...
}

/*
 * Combines the facet results coming from all shards and
 * re-calculates facet values
 */
void SearchResultsAggregator::aggregateFacets(){

    for(vector<pair< QueryResults *, const map<string, std::pair<string, RecordSnippet> > * > >::iterator resultsItr = resultsOfAllShards.begin() ;
            resultsItr != resultsOfAllShards.end() ; ++resultsItr){
        QueryResults * queryResultsItr = resultsItr->first;
        const std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > > *
        facetResults = queryResultsItr->getFacetResults();
        for(std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator facetGroupItr = facetResults->begin();
                facetGroupItr != facetResults->end() ; ++facetGroupItr){

            // first check to see if this facet group exists
            std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::iterator existingFacetGroupItr =
                    results.aggregatedFacetResults.find(facetGroupItr->first);

            if( existingFacetGroupItr == results.aggregatedFacetResults.end()){ // group is new
                results.aggregatedFacetResults[facetGroupItr->first] = facetGroupItr->second;
            }else{ // new group must be merged with the existing group
                ASSERT(existingFacetGroupItr->second.first == facetGroupItr->second.first);
                mergeFacetVectors(existingFacetGroupItr->second.second, facetGroupItr->second.second);
            }
        }
    }
}



/*
 * Merges destination with source and adds new items to source
 */
void SearchResultsAggregator::mergeFacetVectors(std::vector<std::pair<std::string, float> > & source,
        const std::vector<std::pair<std::string, float> > & destination){
    for(std::vector<std::pair<std::string, float> >::const_iterator destinationItr = destination.begin();
            destinationItr != destination.end(); ++destinationItr){
        //try to find this facet in the source
        bool found = false;
        for(std::vector<std::pair<std::string, float> >::iterator sourceItr = source.begin();
                sourceItr != source.end(); ++sourceItr){
            if(destinationItr->first.compare(sourceItr->first) == 0){ // the same facet category
                sourceItr->second += destinationItr->second;
                found = true;
                break;
            }
        }
        if(found == false){
            source.push_back(*destinationItr);
        }
    }
}


void SearchResultsAggregator::aggregateEstimations(){
    results.isResultsApproximated = false;
    for(vector<pair< QueryResults *, const map<string, std::pair<string, RecordSnippet> > * > >::iterator resultsItr = resultsOfAllShards.begin() ;
            resultsItr != resultsOfAllShards.end() ; ++resultsItr){
        QueryResults * queryResultsItr = resultsItr->first;
        results.isResultsApproximated  = results.isResultsApproximated || queryResultsItr->impl->resultsApproximated;
        if(queryResultsItr->impl->estimatedNumberOfResults != -1){
            results.aggregatedEstimatedNumberOfResults += queryResultsItr->impl->estimatedNumberOfResults;
        }
    }
}


}
}
