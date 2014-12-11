#include "ReadCommand.h"
#include "AclAttributeReadCommand.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../state_machine/StateMachine.h"
#include "../../notifications/SearchCommandNotification.h"
#include "../../notifications/SearchCommandResultsNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../../metadata_manager/Partition.h"
#include "processor/Partitioner.h"
#include "core/util/RecordSerializerUtil.h"
#include "core/analyzer/AnalyzerFactory.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

ReadCommand::ReadCommand(const CoreInfo_t * coreInfo,
		const ParsedParameterContainer & paramContainer,
		ConsumerInterface * consumer):ProducerInterface(consumer){
	this->coreInfo = coreInfo;
	this->paramContainer = paramContainer;
	this->clusterReadview = ((ReadviewTransaction *)(this->getTransaction().get()))->getReadview();
	this->root = SP(Json::Value)(new Json::Value(Json::objectValue));
}

SP(Transaction) ReadCommand::getTransaction(){
	if(this->getConsumer() == NULL){
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}


void ReadCommand::produce(){
	//
	if(coreInfo == NULL){
		finalize();
		return;
	}

	if(prepareAttributeAclInfo()){
		// we will continue search from the "consume" function when
		// the result of attribute acl comes.
		return;
	}

	// If all attributes are non-acl then we just continue with the search.
	search();
}


void ReadCommand::consume(bool status, const vector<unsigned> & searchableAttributeIds,
			const vector<unsigned> & refiningAttributeIds,
			const vector<JsonMessageCode> & messages){

	for(vector<JsonMessageCode>::const_iterator msgItr = messages.begin(); msgItr != messages.end(); ++msgItr){
		if(std::find(this->messageCodes.begin(), this->messageCodes.end(), *msgItr) == this->messageCodes.end()){
			this->messageCodes.push_back(*msgItr);
		}
	}
	if(! status){
		finalize();
		return;
	}
	this->aclApprovedSearchAttributes = searchableAttributeIds;
	this->aclApprovedRefiningAttributes = refiningAttributeIds;
	search();
}


void ReadCommand::search(){

	this->logicalPlan = prepareLogicalPlan();


	if(this->logicalPlan == NULL){
		finalize();
		return;
	}

	logicalPlan->accessibleRefiningAttributes = aclApprovedRefiningAttributes;
	logicalPlan->accessibleSearchableAttributes = aclApprovedSearchAttributes;

	// find targets of this search query
    CorePartitioner partitioner(clusterReadview->getPartitioner(coreInfo->getCoreId()));
    partitioner.getAllReadTargets(targets);
    // prepare search notifications
    vector<pair<SP(ShardingNotification), NodeId> > participants;
    for(vector<NodeTargetShardInfo>::iterator targetItr = targets.begin(); targetItr != targets.end(); ++targetItr){
    	SP(SearchCommand) notif = SP(SearchCommand)( new SearchCommand(coreInfo->getCoreId(), this->logicalPlan, *targetItr, clusterReadview));
    	notif->setDest(NodeOperationId(targetItr->getNodeId()));
    	participants.push_back(std::make_pair(notif, targetItr->getNodeId()));
    }

    if(participants.empty()){
    	finalize(false);
    }
    ConcurrentNotifOperation * searchRequester =
    		new ConcurrentNotifOperation(ShardingSearchResultsMessageType, participants, this);

    ShardManager::getStateMachine()->registerOperation(searchRequester);
}

void ReadCommand::end(map<NodeId, SP(ShardingNotification) > & replies){
	processSearchResults(replies);
}


void ReadCommand::processSearchResults(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.size() != targets.size()){
		// now find which nodes are dead
		for(vector<NodeTargetShardInfo>::iterator targetItr = targets.begin(); targetItr != targets.end(); ++targetItr){
			if(replies.find(targetItr->getNodeId()) == replies.end()){
				continue;
			}
			customMessageStrings.push_back(
					JsonResponseHandler::getCustomMessageStr(HTTP_Json_Node_Failure,
							clusterReadview->getNode(targetItr->getNodeId()).getName()));
		}
	}
	// now, for every reply, make a network operator and pass it to
	// make the cluster physical plan
	vector<QueryResults *> queryResultsList;
	for(map<NodeId, SP(ShardingNotification) >::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		SearchCommandResults * nodeResults = (SearchCommandResults *)(replyItr->second.get());
		for(unsigned shardIndex = 0; shardIndex < nodeResults->getShardResults().size(); ++shardIndex){
			SearchCommandResults::ShardResults * shardResult = nodeResults->getShardResults().at(shardIndex);
			queryResultsList.push_back(&(shardResult->queryResults));

			// Do shard related aggregations
			if(aggregatedSearcherTime < shardResult->searcherTime){
				aggregatedSearcherTime = shardResult->searcherTime;
			}
		}
	}
	this->results = replies;
	// pass network operators to physical plan of cluster.
	preparePhysicalPlan(queryResultsList);

	// execute the plan and prepare Json response
	executeAndPrint();

	// give final results  to the consumer
	finalize(true);
}


void ReadCommand::finalize(bool status){
	this->getConsumer()->consume(coreInfo->getCoreId(), status, root, messageCodes, customMessageStrings);
}

unsigned ReadCommand::getTotalSearchTime() const{
	return totalSearchTime;
}
unsigned ReadCommand::getPayloadAccessTime() const{
	return payloadAccessTime;
}

bool ReadCommand::prepareAttributeAclInfo(){

    if (paramContainer.roleId == "" || (
    		coreInfo->getSchema()->getAclRefiningAttrIdsList().size() == 0
    		&& coreInfo->getSchema()->getAclSearchableAttrIdsList().size() == 0)) {
    	// do not do attribute acl query if
    	// 1. role Id is not present.  OR
    	// 2. non of the attributes are acl enabled. So attributes acl is implicitly disabled.
    	return false;
    }
    const CoreInfo_t * aclCore =
    		clusterReadview->getCore(coreInfo->getAttributeAclCoreId());
    aclAttributeReadCommnad = new AclAttributeReadCommand(this, paramContainer.roleId, aclCore);
    aclAttributeReadCommnad->produce();
    return true;
}

LogicalPlan * ReadCommand::prepareLogicalPlan(){

    struct timespec tstart;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
    clock_gettime(CLOCK_REALTIME, &(this->tstart));

    QueryValidator qv(*(coreInfo->getSchema()),
            *(coreInfo), &paramContainer,
            aclApprovedRefiningAttributes, aclApprovedSearchAttributes);

    bool valid = qv.validate();
    if (!valid) {
        return NULL;
    }

    LogicalPlan * logicalPlan = new LogicalPlan();
    logicalPlan->roleId = paramContainer.roleId;
    logicalPlan->facetOnlyFlag = paramContainer.onlyFacets;
    logicalPlan->highLightingOnFlag = paramContainer.isHighlightOn;

    //3. rewrite the query and apply analyzer and other stuff ...
    QueryRewriter qr(coreInfo,
            *(coreInfo->getSchema()),
            *(AnalyzerFactory::getCurrentThreadAnalyzer(coreInfo)),
            &paramContainer,
            aclApprovedRefiningAttributes, aclApprovedSearchAttributes);

    if(qr.rewrite(*logicalPlan) == false){
        // if the query is not valid, print the error message to the response
    	delete logicalPlan;
        return NULL;
    }
    // compute elapsed time in ms , end the timer
    clock_gettime(CLOCK_REALTIME, &tend);
    validationRewriteTime = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

	return logicalPlan;
}


void ReadCommand::preparePhysicalPlan(vector<QueryResults * > & networkOperators){

	// make the tree which outputs the records
	sortOperator = new ClusterSortOperator();

	if(logicalPlan->getPostProcessingInfo() != NULL &&
			logicalPlan->getPostProcessingInfo()->getSortEvaluator() != NULL){
		sortOperator->setSortEvaluator(logicalPlan->getPostProcessingInfo()->getSortEvaluator());
	}

    isResultsApproximated = false;
	for(vector<QueryResults * >::iterator queryResultsItr = networkOperators.begin();
			queryResultsItr != networkOperators.end(); ++queryResultsItr){
		NetworkOperator * networkOp = new NetworkOperator();
		networkOp->load(*queryResultsItr);
		sortOperator->addChild(networkOp);

		// do other aggregations
        isResultsApproximated  = isResultsApproximated || (*queryResultsItr)->impl->resultsApproximated;
        if((*queryResultsItr)->impl->estimatedNumberOfResults != -1){
        	aggregatedEstimatedNumberOfResults += (*queryResultsItr)->impl->estimatedNumberOfResults;
        }
	}

	// make the tree which outputs the facet results
	if(logicalPlan->getPostProcessingInfo() != NULL){
		if(logicalPlan->getPostProcessingInfo()->getfacetInfo() != NULL){
			for(vector<QueryResults * >::iterator queryResultsItr = networkOperators.begin();
					queryResultsItr != networkOperators.end(); ++queryResultsItr){
				facetAggregator.addShardFacetResults(&((*queryResultsItr)->impl->facetResults));
			}
		}
	}
	// Result : aggregateFacetResults can be called in print
}

// print results on HTTP channel
void ReadCommand::executeAndPrint(){
	ASSERT(sortOperator != NULL);
    // CoreInfo_t is a view of configurationManager which contains all information for the
    // core that we want to search on, this object is accesses through configurationManager.
    const CoreInfo_t *indexDataContainerConf = this->coreInfo;

    vector<QueryResult *> queryResults;
    vector<RecordSnippet> highlightInfo;
    ClusterPhysicalPlanExecutionParameter params ;
    sortOperator->open(&params);
    while(true){
    	QueryResult * queryResult = sortOperator->getNext(&params);
    	if(queryResult == NULL){
    		break;
    	}
    	queryResults.push_back(queryResult);
    	highlightInfo.push_back(queryResult->recordSnippet);
    }

    struct timespec hltstart;
    clock_gettime(CLOCK_REALTIME, &hltstart);

    struct timespec hltend;
    clock_gettime(CLOCK_REALTIME, &hltend);
    unsigned hlTime = (hltend.tv_sec - hltstart.tv_sec) * 1000
            + (hltend.tv_nsec - hltstart.tv_nsec) / 1000000;

    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    totalSearchTime = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    //5. call the print function to print out the results
    switch (logicalPlan->getQueryType()) {
    case srch2is::SearchTypeTopKQuery:
    {
        //            finalResults->printStats();
    	unsigned start = logicalPlan->getOffset();
    	unsigned end ;
    	if(queryResults.size() < logicalPlan->getOffset() + logicalPlan->getNumberOfResultsToRetrieve()){
    		end = queryResults.size();
    	}else{
    		end = logicalPlan->getOffset() + logicalPlan->getNumberOfResultsToRetrieve();
    	}
        root = printResults(*logicalPlan,
                indexDataContainerConf, queryResults,
                logicalPlan->getExactQuery(),
                start,
                end,
                queryResults.size(), totalSearchTime , highlightInfo, hlTime,
                paramContainer.onlyFacets);

        break;
    }
    case srch2is::SearchTypeGetAllResultsQuery:
//                finalResults->printStats();
        if(aggregatedEstimatedNumberOfResults < queryResults.size()){
            aggregatedEstimatedNumberOfResults = queryResults.size();
        }
        if (logicalPlan->getOffset() + logicalPlan->getNumberOfResultsToRetrieve()
                > queryResults.size()) {
            // Case where you have return 10,20, but we got only 0,15 results.
        	root = printResults(*logicalPlan,
                    indexDataContainerConf, queryResults,
                    logicalPlan->getExactQuery(),
                    logicalPlan->getOffset(), queryResults.size(),
                    queryResults.size(), totalSearchTime, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        } else { // Case where you have return 10,20, but we got only 0,25 results and so return 10,20
        	root = printResults(*logicalPlan,
                    indexDataContainerConf, queryResults,
                    logicalPlan->getExactQuery(),
                    logicalPlan->getOffset(),
                    logicalPlan->getOffset() + logicalPlan->getNumberOfResultsToRetrieve(),
                    queryResults.size(), totalSearchTime, highlightInfo, hlTime,
                    paramContainer.onlyFacets);
        }
        break;
    case srch2is::SearchTypeRetrieveById:
        //        finalResults->printStats();
    	root = printOneResultRetrievedById(*logicalPlan ,
                indexDataContainerConf,
                queryResults ,
                totalSearchTime);
        break;
    default:
        break;
    }

//	    //    clock_gettime(CLOCK_REALTIME, &tend);
//	    //    unsigned printTime = (tend.tv_sec - tstart2.tv_sec) * 1000
//	    //            + (tend.tv_nsec - tstart2.tv_nsec) / 1000000;
//	    //    printTime -= (validatorTime + rewriterTime + executionTime + parserTime);
//	    //    cout << "Times : " << parserTime << "\t" << validatorTime << "\t" << rewriterTime << "\t" << executionTime << "\t" << printTime << endl;
//	    // 6. delete allocated structures
//	    // Free the objects
//		if (root ){
//			CustomizableJsonWriter writer (&global_internal_skip_tags);
//			bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK", writer.write(*root), headers);
//	    } else{
//	        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "Bad Request", "" , headers);
//	    }
//		evhttp_clear_headers(&headers);
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
boost::shared_ptr<Json::Value> ReadCommand::printResults(const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<QueryResult * > allResults,
        const Query *query,
        const unsigned start, const unsigned end,
        const unsigned retrievedResults,
        const unsigned ts1 , const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) {


    // start the timer for printing
    struct timespec tstart;
    // end the timer for printing
    struct timespec tend;

    // For logging
    string logQueries;
    unsigned resultFound = end - start;
    (*root)["searcher_time"] = ts1;
    clock_gettime(CLOCK_REALTIME, &tstart);

    vector<string> attributesToReturnFromQuery = queryPlan.getAttrToReturn();
    vector<string> *attributesToReturnFromQueryPtr;
    if (attributesToReturnFromQuery.size() != 0){
		attributesToReturnFromQueryPtr = &attributesToReturnFromQuery;
    }else{
    	attributesToReturnFromQueryPtr = NULL;
    }

    if(onlyFacets == false){ // We send the matching records only if "facet != only".
        (*root)["results"].resize(end - start);
        unsigned counter = 0;
        if (query->getQueryTerms()->empty()) //check if the query type is range query without keywords
        {
            for (unsigned i = start; i < end; ++i) {
                char * inMemoryCharPtr = new char[allResults.at(i)->inMemoryRecordString.size()];
                memcpy(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.c_str(),
                		allResults.at(i)->inMemoryRecordString.size());
                StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.size());
                if (inMemoryData.start.get() == NULL) {
                    --resultFound;
                    continue;
                }
                (*root)["results"][counter]["record_id"] = allResults.at(i)->internalRecordId;
                (*root)["results"][counter]["score"] = (0
                        - allResults.at(i)->_score.getFloatTypedValue()); //the actual distance between the point of record and the center point of the range
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR){
                    string sbuffer;
                    //This case executes when all the attributes are to be returned. However we let the user
					//override if field list parameter is given in query
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                    		sbuffer, attributesToReturnFromQueryPtr);
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
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                            sbuffer, attrToReturn);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    (*root)["results"][counter][global_internal_record.first] = sbuffer;
                }else{
                    //Return the attributes specified explicitly in the query otherwise no attributes are returned
                    string stringBuffer;
                    if(attributesToReturnFromQuery.size() > 0){
                        genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                                stringBuffer, attributesToReturnFromQueryPtr);
                        (*root)["results"][counter][global_internal_record.first] = stringBuffer;
                    }

                }
                ++counter;
            }

        } else // the query is including keywords:(1)only keywords (2)keywords+geo
        {

            for (unsigned i = start; i < end; ++i) {
                char * inMemoryCharPtr = new char[allResults.at(i)->inMemoryRecordString.size()];
                memcpy(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.c_str(), allResults.at(i)->inMemoryRecordString.size());
                StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.size());
                if (inMemoryData.start.get() == NULL) {
                    --resultFound;
                    continue;
                }
                (*root)["results"][counter]["record_id"] = allResults.at(i)->internalRecordId;
                (*root)["results"][counter]["score"] = allResults.at(i)->_score.getFloatTypedValue();

                // print edit distance vector
                vector<unsigned> editDistances;
                editDistances.assign(allResults.at(i)->editDistances.begin(), allResults.at(i)->editDistances.end() );

                (*root)["results"][counter]["edit_dist"].resize(editDistances.size());
                for (unsigned int j = 0; j < editDistances.size(); ++j) {
                	(*root)["results"][counter]["edit_dist"][j] = editDistances[j];
                }

                // print matching keywords vector
                vector<std::string> matchingKeywords;
                matchingKeywords.assign(allResults.at(i)->matchingKeywords.begin(), allResults.at(i)->matchingKeywords.end() );

                (*root)["results"][counter]["matching_prefix"].resize(
                        matchingKeywords.size());
                for (unsigned int j = 0; j < matchingKeywords.size(); ++j) {
                	(*root)["results"][counter]["matching_prefix"][j] =
                            matchingKeywords[j];
                }
                if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
                    unsigned internalRecordId = allResults.at(i)->internalRecordId;
                    string sbuffer;
                    //This case executes when all the attributes are to be returned. However we let the user
					//override if field list parameter is given in query
                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                    		sbuffer, attributesToReturnFromQueryPtr);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    (*root)["results"][counter][global_internal_record.first] = sbuffer;
                } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
                    unsigned internalRecordId = allResults.at(i)->internalRecordId;
                    string sbuffer;
                    const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
                    //Return the attributes specified in the config file
                    //If query has field list parameter we override attrToReturn using the attributes from query
                    //otherwise we use attributes mentioned in config file
                    if(attributesToReturnFromQuery.size() > 0){
                        attrToReturn = attributesToReturnFromQueryPtr;
                    }

                    genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                            sbuffer, attrToReturn);
                    // The class CustomizableJsonWriter allows us to
                    // attach the data string to the JSON tree without parsing it.
                    (*root)["results"][counter][global_internal_record.first] = sbuffer;
                }else{
                    //Return the attributes specified explicitly in the query otherwise no attributes are returned
                    string stringBuffer;
                    if(attributesToReturnFromQuery.size() > 0){
                        genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                                stringBuffer, attributesToReturnFromQueryPtr);
                        (*root)["results"][counter][global_internal_record.first] = stringBuffer;
                    }
                }

                string sbuffer = string();
                sbuffer.reserve(1024);
                genSnippetJSONString(allResults.at(i)->recordSnippet, sbuffer);
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
    	if (query->getQueryTerms()->empty() == false) //check if the query type is range query without keywords

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



    // return some meta data

    (*root)["type"] = queryPlan.getQueryType();
    (*root)["offset"] = start;
    (*root)["limit"] = end - start;

    //    if (queryPlan.getSearchType() == GetAllResultsSearchType
    //            || queryPlan.getSearchType() == GeoSearchType) // facet output must be added here.
    //                    {
    (*root)["results_found"] = resultFound;

    long int estimatedNumberOfResults = aggregatedEstimatedNumberOfResults;
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
    if(isResultsApproximated == true){
    	(*root)["result_set_approximation"] = true;
    }

    //    }

    FacetResults * facetResults = new FacetResults();
    this->facetAggregator.aggregateFacetResults(facetResults);
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

            //
            attributeCounter++;
        }
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    payloadAccessTime = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    (*root)["payload_access_time"] = payloadAccessTime;
//	    (*root)["message"] = message;
//	    Logger::info(
//	            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, highlighter_time: %d ms, payload_access_time: %d ms",
//	            req->remote_host, req->remote_port, req->uri + 1, ts1, hlTime, ts2);
//	//    bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK", writer.write(root), headers);
    return root;
}


/**
 * Iterate over the recordIDs in queryResults and get the record.
 * Add the record information to the request.out string.
 */
boost::shared_ptr<Json::Value> ReadCommand::printOneResultRetrievedById(const LogicalPlan &queryPlan,
        const CoreInfo_t *indexDataConfig,
        const vector<QueryResult *> allResults,
        const unsigned ts1){

    struct timespec tstart;
    struct timespec tend;

    // For logging
    string logQueries;

    vector<string> attributesToReturnFromQuery = queryPlan.getAttrToReturn();
    vector<string> *attributesToReturnFromQueryPtr;
    if (attributesToReturnFromQuery.size() != 0){
    	attributesToReturnFromQueryPtr = &attributesToReturnFromQuery;
    }else{
    	attributesToReturnFromQueryPtr = NULL;
    }

    (*root)["searcher_time"] = ts1;
    (*root)["results"].resize(allResults.size());

    clock_gettime(CLOCK_REALTIME, &tstart);
    unsigned counter = 0;

    unsigned resultFound = allResults.size();
    for (unsigned i = 0; i < allResults.size(); ++i) {
        char * inMemoryCharPtr = new char[allResults.at(i)->inMemoryRecordString.size()];
        memcpy(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.c_str(), allResults.at(i)->inMemoryRecordString.size());
        StoredRecordBuffer inMemoryData(inMemoryCharPtr, allResults.at(i)->inMemoryRecordString.size());
        if (inMemoryData.start.get() == NULL) {
            --resultFound;
            continue;
        }
        (*root)["results"][counter]["record_id"] = allResults.at(i)->internalRecordId;

        if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
            unsigned internalRecordId = allResults.at(i)->internalRecordId;
            string sbuffer;
            //This case executes when all the attributes are to be returned. However we let the user
            //override if field list parameter is given in query
            genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
            		sbuffer, attributesToReturnFromQueryPtr);
            (*root)["results"][counter][global_internal_record.first] = sbuffer;
        } else if (indexDataConfig->getSearchResponseFormat() == RESPONSE_WITH_SELECTED_ATTR){
            unsigned internalRecordId = allResults.at(i)->internalRecordId;
            string sbuffer;
            const vector<string> *attrToReturn = indexDataConfig->getAttributesToReturn();
            //Return the attributes specified in the config file
            //If query has field list parameter we override attrToReturn using the attributes from query
            //otherwise we use attributes mentioned in config file
            if(attributesToReturnFromQuery.size() > 0){
                attrToReturn = attributesToReturnFromQueryPtr;
            }
            genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                    sbuffer, attrToReturn);
            // The class CustomizableJsonWriter allows us to
            // attach the data string to the JSON tree without parsing it.
            (*root)["results"][counter][global_internal_record.first] = sbuffer;
        }else{
            //Return the attributes specified explicitly in the query otherwise no attributes are returned
            string stringBuffer;
            if(attributesToReturnFromQuery.size() > 0){
                genRecordJsonString(indexDataConfig->getSchema(), inMemoryData, allResults.at(i)->externalRecordId,
                        stringBuffer, attributesToReturnFromQueryPtr);
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

//	    (*root)["message"] = message;
//	    Logger::info(
//	            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, highlighter_time: %d ms, payload_access_time: %d ms",
//	            req->remote_host, req->remote_port, req->uri + 1, ts1, hlTime, ts2);
//	//    bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK", writer.write(root), headers);
    return root;
}



void ReadCommand::genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
        const string& extrnalRecordId, string& sbuffer){
    genRecordJsonString(schema, buffer, extrnalRecordId,
            sbuffer, NULL);
}
void ReadCommand::genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
        const string& externalRecordId, string& sbuffer, const vector<string>* attrToReturn){

	vector<string>  accessibleAttrsList;
	// perform access control check on fields to be returned to a user.
	if (attrToReturn == NULL) {
		// attributes to return are not specified. Hence, Go over the fields in the schema and check
		// whether they are accessible .
		std::map<std::string, unsigned>::const_iterator iter =
				schema->getSearchableAttribute().begin();
		// 1. Searchable fields in schema
		for ( ; iter != schema->getSearchableAttribute().end(); iter++) {
			if ( AttributeAccessControl::isFieldAccessible(iter->second,
					aclApprovedSearchAttributes, schema->getNonAclSearchableAttrIdsList())) {
				accessibleAttrsList.push_back(iter->first);
			}
		}
		// 2. Refining fields in schema
		iter = schema->getRefiningAttributes()->begin();
		for ( ; iter != schema->getRefiningAttributes()->end(); iter++) {
			if (AttributeAccessControl::isFieldAccessible(iter->second,
					aclApprovedRefiningAttributes, schema->getNonAclRefiningAttrIdsList())) {
				accessibleAttrsList.push_back(iter->first);
			}
		}

	} else {
		// if attributes to returned are specified then verify whether these attributes are accessible
		for (unsigned i = 0; i < attrToReturn->size(); ++i) {
			const string & fieldName = attrToReturn->operator[](i);

			int id = schema->getSearchableAttributeId(fieldName);
			if (id != -1) {
				if (AttributeAccessControl::isFieldAccessible(id,
						aclApprovedSearchAttributes, schema->getNonAclSearchableAttrIdsList())) {
					accessibleAttrsList.push_back(fieldName);
				}
			} else {
				id = schema->getRefiningAttributeId(fieldName);
				if (id != -1) {
					if (AttributeAccessControl::isFieldAccessible(id,
							aclApprovedRefiningAttributes, schema->getNonAclRefiningAttrIdsList())) {
						accessibleAttrsList.push_back(fieldName);
					};
				}
			}
		}
	}

    Schema * storedSchema = Schema::create();
    srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    srch2::util::RecordSerializerUtil::convertCompactToJSONString(storedSchema, buffer,
    		externalRecordId, sbuffer, &accessibleAttrsList);
    delete storedSchema;
}

/*
 *   This functions removes new line and non-printable characters from the input string
 *   and returns a clean string.
 */
void ReadCommand::cleanAndAppendToBuffer(const string& in, string& out) {
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
			break;
		}
		++inIdx;
	}
}

void ReadCommand::genSnippetJSONString(const RecordSnippet& recordSnippet, string& sbuffer) {
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


}
}
