// $Id$ 07/11/13 Jamshid


/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */


#ifndef _WRAPPER_QUERYPLEANGENERATOR_H_
#define _WRAPPER_QUERYPLEANGENERATOR_H_

#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/FacetedSearchFilter.h>
#include <instantsearch/NonSearchableAttributeExpressionFilter.h>
#include <instantsearch/SortFilter.h>
#include "QueryPlan.h"
#include <algorithm>
#include <sstream>

using namespace std;

namespace srch2{

namespace httpwrapper{

class QueryPlanGen
{
public:

	QueryPlanGen(const ParsedParameterContainer & paramsContainer , const Srch2ServerConf *indexDataContainerConf ){
		this->paramsContainer = paramsContainer;
		this->indexDataContainerConf = indexDataContainerConf;
	}


	/*
	 * 1. creates exact and fuzzy queries
	 * 2. Generates the post processing plan
	 */
	QueryPlan generatePlan(){

		QueryPlan plan;
		// create query objects
		createExactAndFuzzyQueries(&plan);
		// generate post processing plan
		createPostProcessingPlan(&plan);

		return plan;

	}

private:
	const ParsedParameterContainer & paramsContainer;
	const Srch2ServerConf *indexDataContainerConf ;

	// creates a post processing plan based on information from Query
	void createPostProcessingPlan(QueryPlan * plan){

		// NOTE: FacetedSearchFilter should be always the last filter.
		// this function goes through the summary and uses the members of parsedInfo to fill out the query objects and
		// also create and set the plan

		// 1. create the plan
		plan->setPostProcessingPlan(new ResultsPostProcessorPlan());
		// 2. If there is a filter query, allocate the filter and add it to the plan
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , FilterQueryEvaluatorFlag)
				!= paramsContainer.summary.end()  ){ // there is a filter query
			srch2is::NonSearchableAttributeExpressionFilter * filterQuery = new srch2is::NonSearchableAttributeExpressionFilter();
			filterQuery->evaluator = paramsContainer.filterQueryContainer->evaluator;
			plan->getPostProcessingPlan()->addFilterToPlan(filterQuery);
		}

		// 3. If the search type is GetAllResults or Geo, look for Sort and Facet
		if(plan->getSearchType() == GetAllResultsSearchType ){
			// look for SortFiler
			if ( find(paramsContainer.getAllResultsParameterContainer->summary.begin(),
					paramsContainer.getAllResultsParameterContainer->summary.end()
					, SortQueryHandler)
					!= paramsContainer.getAllResultsParameterContainer->summary.end()  ){ // there is a sort filter
				srch2is::SortFilter * sortFilter = new srch2is::SortFilter();
				sortFilter->evaluator = paramsContainer.getAllResultsParameterContainer->sortQueryContainer->evaluator;
				plan->getPostProcessingPlan()->addFilterToPlan(sortFilter);
			}

			// look for Facet filter
			if ( find(paramsContainer.getAllResultsParameterContainer->summary.begin(),
					paramsContainer.getAllResultsParameterContainer->summary.end()
					, FacetQueryHandler)
					!= paramsContainer.getAllResultsParameterContainer->summary.end()  ){ // there is a sort filter
				srch2is::FacetedSearchFilter * facetFilter = new FacetedSearchFilter();
				FacetQueryContainer * container = paramsContainer.getAllResultsParameterContainer->facetQueryContainer;
				facetFilter->initialize(container->types, // FIXME : this compile error is because of CONSTANT problem
										container->fields,
										container->rangeStarts,
										container->rangeEnds,
										container->rangeGaps);

				plan->getPostProcessingPlan()->addFilterToPlan(facetFilter);
			}
		} else if (plan->getSearchType() == GeoSearchType ){
			// look for SortFiler
			if ( find(paramsContainer.geoParameterContainer->summary.begin(),
					paramsContainer.geoParameterContainer->summary.end()
					, SortQueryHandler)
					!= paramsContainer.geoParameterContainer->summary.end()  ){ // there is a sort filter
				srch2is::SortFilter * sortFilter = new srch2is::SortFilter();
				sortFilter->evaluator = paramsContainer.geoParameterContainer->sortQueryContainer->evaluator;
				plan->getPostProcessingPlan()->addFilterToPlan(sortFilter);
			}

			// look for Facet filter
			if ( find(paramsContainer.geoParameterContainer->summary.begin(),
					paramsContainer.geoParameterContainer->summary.end()
					, FacetQueryHandler)
					!= paramsContainer.geoParameterContainer->summary.end()  ){ // there is a sort filter
				srch2is::FacetedSearchFilter * facetFilter = new FacetedSearchFilter();
				FacetQueryContainer * container = paramsContainer.geoParameterContainer->facetQueryContainer;
				facetFilter->initialize(container->types, // FIXME : this compile error is because of CONSTANT problem
										container->fields,
										container->rangeStarts,
										container->rangeEnds,
										container->rangeGaps);

				plan->getPostProcessingPlan()->addFilterToPlan(facetFilter);
			}
		}





	}

	void createExactAndFuzzyQueries(QueryPlan * plan){
		// move on summary of the container and build the query object


		//1. first find the search type
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , TopKSearchType) != paramsContainer.summary.end()  ){ // search type is TopK
			plan->setSearchType(TopKSearchType);
		} else if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , GetAllResultsSearchType) != paramsContainer.summary.end() ){ // get all results
			plan->setSearchType(GetAllResultsSearchType);
		} else if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , GeoSearchType ) != paramsContainer.summary.end() ) { // GEO
			plan->setSearchType(GeoSearchType);
		} else { // get it from config file
			switch (indexDataContainerConf->getSearchType()) {
				case 0:
					plan->setSearchType(TopKSearchType);
					break;
				case 1:
					plan->setSearchType(GetAllResultsSearchType);
					break;
				case 2:
					plan->setSearchType(GeoSearchType);
					break;
			}
		}

		// 2. see if it is a fuzzy search or exact search, if there is no keyword then fuzzy is always false
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , IsFuzzyFlag) != paramsContainer.summary.end()  ){
			plan->setIsFuzzy(paramsContainer.isFuzzy && (paramsContainer.rawQueryKeywords.size() != 0));
		} else { // get it from configuration file
			plan->setIsFuzzy(indexDataContainerConf->getIsFuzzyTermsQuery() && (paramsContainer.rawQueryKeywords.size() != 0));
		}


		// 3. set the offset of results to retrieve

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , ResultsStartOffset) != paramsContainer.summary.end()  ){
			plan->setOffset(paramsContainer.resultsStartOffset);
		} else { // get it from configuration file
			plan->setOffset(0);
		}

		// 4. set the number of results to retrieve
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , NumberOfResults) != paramsContainer.summary.end()  ){
			plan->setResultsToRetrieve(paramsContainer.numberOfResults)
		} else { // get it from configuration file
			plan->setResultsToRetrieve(indexDataContainerConf->getDefaultResultsToRetrieve());
		}

		// 5. based on the search type get needed information and create the query objects
		switch (plan->getSearchType()) {
			case TopKSearchType:
				createExactAndFuzzyQueriesForTopK(plan);
				break;
			case GetAllResultsSearchType:
				createExactAndFuzzyQueriesForGetAllTResults(plan);
				break;
			case GeoSearchType:
				createExactAndFuzzyQueriesForGeo(plan);
				break;
		}

		fillExactAndFuzzyQueriesWithCommonInformation(plan);

	}

	void fillExactAndFuzzyQueriesWithCommonInformation(QueryPlan * plan ){

		// 1. first check to see if there is any keywords in the query
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , RawQueryKeywords) == paramsContainer.summary.end()  ){
			return;
		}



		// 2. Extract the common information from the container

		// length boost
		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , LengthBoostFlag) != paramsContainer.summary.end()  ){
			plan->getExactQuery()->setLengthBoost(paramsContainer.lengthBoost);
			plan->getFuzzyQuery()->setLengthBoost(paramsContainer.lengthBoost);
		} else { // get it from configuration file
			plan->getExactQuery()->setLengthBoost(indexDataContainerConf->getQueryTermLengthBoost());
			plan->getFuzzyQuery()->setLengthBoost(indexDataContainerConf->getQueryTermLengthBoost());
		}

		// prefix match penalty flag

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , PrefixMatchPenaltyFlag) != paramsContainer.summary.end()  ){
			plan->getExactQuery()->setPrefixMatchPenalty(paramsContainer.prefixMatchPenalty);
			plan->getFuzzyQuery()->setPrefixMatchPenalty(paramsContainer.prefixMatchPenalty);
		} else { // get it from configuration file
			plan->getExactQuery()->setPrefixMatchPenalty(indexDataContainerConf->getPrefixMatchPenalty());
			plan->getFuzzyQuery()->setPrefixMatchPenalty(indexDataContainerConf->getPrefixMatchPenalty());
		}

		// get query keywords
		std::vector<std::string> rawQueryKeywords = paramsContainer.rawQueryKeywords;


		vector<float> keywordFuzzyLevel;

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , KeywordFuzzyLevel) != paramsContainer.summary.end()  ){
			keywordFuzzyLevel = paramsContainer.keywordFuzzyLevel;
		} else { // get it from configuration file
			for(unsigned i=0;i<rawQueryKeywords.size() ; i++){
				keywordFuzzyLevel.push_back(indexDataContainerConf->getQueryTermSimilarityBoost());
			}
		}

		vector<float> keywordBoostLevel;

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , KeywordBoostLevel) != paramsContainer.summary.end()  ){
			keywordBoostLevel = paramsContainer.keywordBoostLevel;
		} else { // get it from configuration file
			for(unsigned i=0;i<rawQueryKeywords.size() ; i++){
				keywordFuzzyLevel.push_back(indexDataContainerConf->getQueryTermBoost());
			}
		}

		vector<QueryPrefixComplete> keywordPrefixComplete;

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , QueryPrefixCompleteFlag) != paramsContainer.summary.end()  ){
			keywordPrefixComplete = paramsContainer.keywordPrefixComplete;
		} else { // get it from configuration file
			for(unsigned i=0;i<rawQueryKeywords.size() ; i++){
				keywordPrefixComplete.push_back(indexDataContainerConf->getQueryTermType()? PREFIX : COMPLETE);
							// TODO: Make sure true means PREFIX and false means COMPLETE
			}
		}

		vector<unsigned> fieldFilter;

		if ( find(paramsContainer.summary.begin(), paramsContainer.summary.end() , FieldFilter) != paramsContainer.summary.end()  ){
			for(vector<string>::iterator filterIter = paramsContainer.fieldFilter.begin() ;
										filterIter!=paramsContainer.fieldFilter.end(); ++filterIter){
				stringstream strValue(*filterIter);
				unsigned int intFilter;
				strValue >> intFilter;
				fieldFilter.push_back(intFilter);
			}
		} else { // get it from configuration file
			for(unsigned i=0;i<rawQueryKeywords.size() ; i++){
				fieldFilter.push_back(1); // TODO : make sure setting it to 1 is correct
			}
		}


		// 3. Fill up query objects
        srch2is::Term *fuzzyTerm;
		// exact query
		for(int i=0;i<rawQueryKeywords.size();i++){
			srch2is::Term *exactTerm;
            exactTerm = new srch2is::Term(rawQueryKeywords[i],
                    (keywordPrefixComplete[i] == PREFIX)? srch2is::PREFIX:srch2is::COMPLETE,
                    keywordBoostLevel[i],
                    keywordFuzzyLevel[i],
                    0);
            exactTerm->addAttributeToFilterTermHits(fieldFilter[i]);

            plan->getExactQuery()->add(exactTerm);
		}
		// fuzzy query
		if(plan->isIsFuzzy()){
			for(int i=0;i<rawQueryKeywords.size();i++){
				srch2is::Term *fuzzyTerm;
	            fuzzyTerm = new srch2is::Term(rawQueryKeywords[i],
	                    (keywordPrefixComplete[i] == PREFIX)? srch2is::PREFIX:srch2is::COMPLETE,
	                    keywordBoostLevel[i],
	                    keywordFuzzyLevel[i],
	                    srch2is::Term::getNormalizedThreshold(getUtf8StringCharacterNumber(rawQueryKeywords[i])));
	            fuzzyTerm->addAttributeToFilterTermHits(fieldFilter[i]);

	            plan->getExactQuery()->add(fuzzyTerm);
			}
		}


	}

	void createExactAndFuzzyQueriesForTopK(QueryPlan * plan){
		// 2. allocate the objects
		plan->setExactQuery(new Query(srch2is::TopKQuery));
		if(plan->isIsFuzzy()){
			plan->setFuzzyQuery(new Query(srch2is::TopKQuery));
		}

	}
	void createExactAndFuzzyQueriesForGetAllTResults(QueryPlan * plan){
		plan->setExactQuery(new Query(srch2is::GetAllResultsQuery));
		if(plan->isIsFuzzy()){
			plan->setFuzzyQuery(new Query(srch2is::GetAllResultsQuery));
		}
	}

	void createExactAndFuzzyQueriesForGeo(QueryPlan * plan){
		plan->setExactQuery(new Query(srch2is::MapQuery));
		if(plan->isIsFuzzy()){
			plan->setFuzzyQuery(new Query(srch2is::MapQuery));
		}
		GeoParameterContainer * gpc = paramsContainer.geoParameterContainer;

		if ( find(gpc->summary.begin(), pgpc->summary.end() , GeoTypeRectangular) != gpc->summary.end()  ){
			plan->getExactQuery()->setRange(gpc->leftBottomLatitude,
											gpc->leftBottomLongitude,
											gpc->rightTopLatitude,
											gpc->rightTopLongitude);
			if(plan->isIsFuzzy()){
				plan->getFuzzyQuery()->setRange(gpc->leftBottomLatitude,
												gpc->leftBottomLongitude,
												gpc->rightTopLatitude,
												gpc->rightTopLongitude);
			}
		} else if ( find(gpc->summary.begin(), pgpc->summary.end() , GeoTypeCircular) != gpc->summary.end() ){
			plan->getExactQuery()->setRange(gpc->centerLatitude,
											gpc->centerLongitude,
											gpc->radius);
			if(plan->isIsFuzzy()){
				plan->getFuzzyQuery()->setRange(gpc->centerLatitude,
												gpc->centerLongitude,
												gpc->radius);
			}
		}

	}


};
}
}


#endif // _WRAPPER_QUERYPLEANGENERATOR_H_
