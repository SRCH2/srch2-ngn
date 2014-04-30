#include "ResultsAggregatorAndPrint.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

//###########################################################################
//                       Result Aggregator
//###########################################################################
/*
 * The main function responsible of aggregating search results
 * this function uses aggregateRecords and aggregateFacets for
 * aggregating result records and calculated records
 */
void SearchResultAggregatorAndPrint::callBack(vector<SerializableSearchResults *> responseObjects){


	// move on all responses of all shards and use them
	for(int responseIndex = 0 ; responseIndex < responseObjects.size() ; ++responseIndex ){
		QueryResults * resultsOfThisShard = responseObjects.at(responseIndex)->getQueryResults();
		resultsOfAllShards.push_back(resultsOfThisShard);
		if(results.aggregatedSearcherTime < responseObjects.at(responseIndex)->getSearcherTime()){
			results.aggregatedSearcherTime = responseObjects.at(responseIndex)->getSearcherTime();
		}
	}
	aggregateRecords();
	aggregateFacets();

}

// print results on HTTP channel
void SearchResultAggregatorAndPrint::printResults(){

    // CoreInfo_t is a view of configurationManager which contains all information for the
    // core that we want to search on, this object is accesses through configurationManager.
    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreShardInfo->coreName);


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
//    		!paramContainer.onlyFacets &&
//    		paramContainer.isHighlightOn) {
//
//    	ServerHighLighter highlighter =  ServerHighLighter(finalResults, server, paramContainer,
//    			logicalPlan.getOffset(), logicalPlan.getNumberOfResultsToRetrieve());
//    	highlightInfo.reserve(logicalPlan.getNumberOfResultsToRetrieve());
//    	highlighter.generateSnippets(highlightInfo);
//    	if (highlightInfo.size() == 0 && finalResults->getNumberOfResults() > 0) {
//    		Logger::warn("Highligting is on but snippets were not generated!!");
//    	}
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
		//	        finalResults->printStats();
		printResults(req, headers, logicalPlan,
				indexDataContainerConf, results.allResults, logicalPlan.getExactQuery(),
				logicalPlan.getOffset(),
				results.allResults.size(),
				results.allResults.size(),
				paramContainer.getMessageString(), parseAndSearchTime , highlightInfo, hlTime,
				paramContainer.onlyFacets);

		break;

	case srch2is::SearchTypeGetAllResultsQuery:
	case srch2is::SearchTypeMapQuery:
//		finalResults->printStats();
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
					paramContainer.getMessageString(), parseAndSearchTime, highlightInfo, hlTime,
					paramContainer.onlyFacets);
		} else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
			printResults(req, headers, logicalPlan,
					indexDataContainerConf, results.allResults,
					logicalPlan.getExactQuery(),
					logicalPlan.getOffset(),
					logicalPlan.getOffset() + logicalPlan.getNumberOfResultsToRetrieve(),
					results.allResults.size(),
					paramContainer.getMessageString(), parseAndSearchTime, highlightInfo, hlTime,
					paramContainer.onlyFacets);
		}
		break;
	case srch2is::SearchTypeRetrieveById:
//		finalResults->printStats();
		printOneResultRetrievedById(req,
				headers,
				logicalPlan ,
				indexDataContainerConf,
				results.allResults ,
				paramContainer.getMessageString() ,
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
void SearchResultAggregatorAndPrint::printResults(evhttp_request *req,
        const evkeyvalq &headers, const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<QueryResult *> queryResultsVector, const Query *query,
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
    unsigned resultFound = retrievedResults;
    root["searcher_time"] = ts1;
    clock_gettime(CLOCK_REALTIME, &tstart);

    if(onlyFacets == false){ // We send the matching records only if "facet != only".
        root["results"].resize(end - start);
        unsigned counter = 0;
        if (queryPlan.getQueryType() == srch2is::SearchTypeMapQuery
                && query->getQueryTerms()->empty()) //check if the query type is range query without keywords
        {
            for (unsigned i = start; i < end; ++i) {
            	unsigned internalRecordId = queryResultsVector[i]->internalRecordId;
            	StoredRecordBuffer inMemoryData ; // TODO = indexer->getInMemoryData(internalRecordId);
            	if (inMemoryData.start.get() == NULL) {
            		--resultFound;
            		continue;
            	}
                root["results"][counter]["record_id"] = queryResultsVector[i]->internalRecordId;
                root["results"][counter]["score"] = (0
                        - queryResultsVector[i]->_score.getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR){
                    string sbuffer;
                    //TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector[i]->internalRecordId, sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	string sbuffer;
                	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                	//TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector[i]->internalRecordId,
                	//TODO		sbuffer, attrToReturn);
                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	root["results"][counter][internalRecordTags.first] = sbuffer;
                }
                ++counter;
            }

        } else // the query is including keywords:(1)only keywords (2)keywords+geo
        {

            for (unsigned i = start; i < end; ++i) {
            	unsigned internalRecordId = queryResultsVector[i]->internalRecordId;
            	StoredRecordBuffer inMemoryData ; //TODO = indexer->getInMemoryData(internalRecordId);
            	if (inMemoryData.start.get() == NULL) {
            		--resultFound;
            		continue;
            	}
                root["results"][counter]["record_id"] = queryResultsVector[i]->internalRecordId;
                root["results"][counter]["score"] = queryResultsVector[i]->_score.getFloatTypedValue();

                // print edit distance vector
                vector<unsigned> editDistances;
                editDistances.assign(queryResultsVector[i]->editDistances.begin(), queryResultsVector[i]->editDistances.end() );

                root["results"][counter]["edit_dist"].resize(editDistances.size());
                for (unsigned int j = 0; j < editDistances.size(); ++j) {
                    root["results"][counter]["edit_dist"][j] = editDistances[j];
                }

                // print matching keywords vector
                vector<std::string> matchingKeywords;
                matchingKeywords.assign(queryResultsVector[i]->matchingKeywords.begin(), queryResultsVector[i]->matchingKeywords.end() );

                root["results"][counter]["matching_prefix"].resize(
                        matchingKeywords.size());
                for (unsigned int j = 0; j < matchingKeywords.size(); ++j) {
                    root["results"][counter]["matching_prefix"][j] =
                            matchingKeywords[j];
                }
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
                    unsigned internalRecordId = queryResultsVector[i]->internalRecordId;
                    string sbuffer;
                    //TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector[i]->internalRecordId,
                    //		 sbuffer);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    root["results"][counter][internalRecordTags.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                	unsigned internalRecordId = queryResultsVector[i]->internalRecordId;
                	string sbuffer;
                	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                	//TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector[i]->internalRecordId,
                	//		sbuffer, attrToReturn);
                	// The class CustomizableJsonWriter allows us to
                	// attach the data string to the JSON tree without parsing it.
                	root["results"][counter][internalRecordTags.first] = sbuffer;
                }

                string sbuffer = string();
                sbuffer.reserve(1024);  //<< TODO: set this to max allowed snippet len
                //TODO genSnippetJSONString(i, start, recordSnippets, sbuffer, queryResults);
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
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
void SearchResultAggregatorAndPrint::printOneResultRetrievedById(evhttp_request *req, const evkeyvalq &headers,
        const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<QueryResult *> queryResultsVector,
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
    root["results"].resize(queryResultsVector.size());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    unsigned resultFound = queryResultsVector.size();
    for (unsigned i = 0; i < queryResultsVector.size(); ++i) {
    	unsigned internalRecordId = queryResultsVector.at(i)->internalRecordId;
    	StoredRecordBuffer inMemoryData ; // TODO= indexer->getInMemoryData(internalRecordId);
    	if (inMemoryData.start.get() == NULL) {
    		--resultFound;
    		continue;
    	}
        root["results"][counter]["record_id"] = queryResultsVector.at(i)->internalRecordId;

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            unsigned internalRecordId = queryResultsVector.at(i)->internalRecordId;
            string sbuffer;
            //TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector.at(i)->internalRecordId, sbuffer);
            root["results"][counter][internalRecordTags.first] = sbuffer;
        } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
        	unsigned internalRecordId = queryResultsVector.at(i)->internalRecordId;
        	string sbuffer;
        	const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
        	//TODO genRecordJsonString(indexer, inMemoryData, queryResultsVector.at(i)->internalRecordId,
        	//TODO		sbuffer, attrToReturn);
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
    bmhelper_evhttp_send_reply(req, HTTP_OK, "OK", writer.write(root), headers);
}



/*
 * Combines the results coming from all shards and
 * resorts them based on their scores
 */
void SearchResultAggregatorAndPrint::aggregateRecords(){
	// aggregate results
	for(vector<QueryResults *>::iterator queryResultsItr = resultsOfAllShards.begin() ;
			queryResultsItr != resultsOfAllShards.end() ; ++queryResultsItr){
		results.allResults.insert(results.allResults.end() , (*queryResultsItr)->impl->sortedFinalResults.begin(),
				(*queryResultsItr)->impl->sortedFinalResults.end());
		for(unsigned queryResultIndex = 0; queryResultIndex < (*queryResultsItr)->impl->sortedFinalResults.size();
				queryResultIndex ++){
			results.recordData.push_back((*queryResultsItr)->getInMemoryRecordString(queryResultIndex));
		}
	}

	// sort final results
	std::sort(results.allResults.begin(), results.allResults.end(), SearchResultAggregatorAndPrint::QueryResultsComparatorOnlyScore());
}

/*
 * Combines the facet results coming from all shards and
 * re-calculates facet values
 */
void SearchResultAggregatorAndPrint::aggregateFacets(){

	for(vector<QueryResults *>::iterator queryResultsItr = resultsOfAllShards.begin() ;
			queryResultsItr != resultsOfAllShards.end() ; ++queryResultsItr){
		const std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > > *
		facetResults = (*queryResultsItr)->getFacetResults();
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
void SearchResultAggregatorAndPrint::mergeFacetVectors(std::vector<std::pair<std::string, float> > & source,
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


void SearchResultAggregatorAndPrint::aggregateEstimations(){
	results.isResultsApproximated = false;
	for(vector<QueryResults *>::iterator queryResultsItr = resultsOfAllShards.begin() ;
			queryResultsItr != resultsOfAllShards.end() ; ++queryResultsItr){
		results.isResultsApproximated  = results.isResultsApproximated || (*queryResultsItr)->impl->resultsApproximated;
		if((*queryResultsItr)->impl->estimatedNumberOfResults != -1){
			results.aggregatedEstimatedNumberOfResults += (*queryResultsItr)->impl->estimatedNumberOfResults;
		}
	}
}



}
}
