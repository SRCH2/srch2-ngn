// $Id: QueryResults.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */
#include "QueryPlanGen.h"
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

namespace srch2is = srch2::instantsearch;

using srch2is::Query;

namespace srch2 {

namespace httpwrapper {

QueryPlanGen::QueryPlanGen(const ParsedParameterContainer & paramsContainer,
        const Srch2ServerConf *indexDataContainerConf) :
        paramsContainer(paramsContainer) {
    this->indexDataContainerConf = indexDataContainerConf;
}

/*
 * 1. creates exact and fuzzy queries
 * 2. Generates the post processing plan
 */
QueryPlan QueryPlanGen::generatePlan() {

    QueryPlan plan;
    // create query objects
    createExactAndFuzzyQueries(&plan);
    // generate post processing plan
    createPostProcessingPlan(&plan);

    return plan;

}

// creates a post processing plan based on information from Query
void QueryPlanGen::createPostProcessingPlan(QueryPlan * plan) {

    // NOTE: FacetedSearchFilter should be always the last filter.
    // this function goes through the summary and uses the members of parsedInfo to fill out the query objects and
    // also creates and sets the plan

    // 1. allocate the post processing plan, it will be freed when the QueryPlan object is destroyed.
    plan->setPostProcessingPlan(new ResultsPostProcessorPlan());
    // 2. If there is a filter query, allocate the filter and add it to the plan
    if (paramsContainer.hasParameterInSummary(FilterQueryEvaluatorFlag)) { // there is a filter query
        srch2is::NonSearchableAttributeExpressionFilter * filterQuery =
                new srch2is::NonSearchableAttributeExpressionFilter();
        filterQuery->evaluator =
                paramsContainer.filterQueryContainer->evaluator;
        plan->getPostProcessingPlan()->addFilterToPlan(filterQuery);
    }

    // 3. If the search type is GetAllResults or Geo, look for Sort and Facet
    if (plan->getSearchType() == GetAllResultsSearchType) {
        // look for SortFiler
        if (paramsContainer.getAllResultsParameterContainer->hasParameterInSummary(
                SortQueryHandler)) { // there is a sort filter
            srch2is::SortFilter * sortFilter = new srch2is::SortFilter();
            sortFilter->evaluator =
                    paramsContainer.getAllResultsParameterContainer->sortQueryContainer->evaluator;
            plan->getPostProcessingPlan()->addFilterToPlan(sortFilter);
        }

        // look for Facet filter
        if (paramsContainer.getAllResultsParameterContainer->hasParameterInSummary(
                FacetQueryHandler)) { // there is a sort filter
            srch2is::FacetedSearchFilter * facetFilter =
                    new srch2is::FacetedSearchFilter();
            FacetQueryContainer * container =
                    paramsContainer.getAllResultsParameterContainer->facetQueryContainer;
            facetFilter->initialize(container->types, container->fields,
                    container->rangeStarts, container->rangeEnds,
                    container->rangeGaps);

            plan->getPostProcessingPlan()->addFilterToPlan(facetFilter);
        }
    } else if (plan->getSearchType() == GeoSearchType) {
        // look for SortFiler
        if (paramsContainer.geoParameterContainer->hasParameterInSummary(
                SortQueryHandler)) { // there is a sort filter
            srch2is::SortFilter * sortFilter = new srch2is::SortFilter();
            sortFilter->evaluator =
                    paramsContainer.geoParameterContainer->sortQueryContainer->evaluator;
            plan->getPostProcessingPlan()->addFilterToPlan(sortFilter);
        }

        // look for Facet filter
        if (paramsContainer.geoParameterContainer->hasParameterInSummary(
                FacetQueryHandler)) { // there is a sort filter
            srch2is::FacetedSearchFilter * facetFilter =
                    new srch2is::FacetedSearchFilter();
            FacetQueryContainer * container =
                    paramsContainer.geoParameterContainer->facetQueryContainer;
            facetFilter->initialize(container->types, container->fields,
                    container->rangeStarts, container->rangeEnds,
                    container->rangeGaps);

            plan->getPostProcessingPlan()->addFilterToPlan(facetFilter);
        }
    }

}

void QueryPlanGen::createExactAndFuzzyQueries(QueryPlan * plan) {
    // move on summary of the container and build the query object

    //1. first find the search type
    if (paramsContainer.hasParameterInSummary(TopKSearchType)) { // search type is TopK
        plan->setSearchType(TopKSearchType);
    } else if (paramsContainer.hasParameterInSummary(GetAllResultsSearchType)) { // get all results
        plan->setSearchType(GetAllResultsSearchType);
    } else if (paramsContainer.hasParameterInSummary(GeoSearchType)) { // GEO
        plan->setSearchType(GeoSearchType);
    } // else : there is no else because validator makes sure type is set in parser

    // 2. see if it is a fuzzy search or exact search, if there is no keyword (which means GEO search) then fuzzy is always false
    if (paramsContainer.hasParameterInSummary(IsFuzzyFlag)) {
        plan->setIsFuzzy(
                paramsContainer.isFuzzy
                        && (paramsContainer.rawQueryKeywords.size() != 0));
    } else { // get it from configuration file
        plan->setIsFuzzy(
                indexDataContainerConf->getIsFuzzyTermsQuery()
                        && (paramsContainer.rawQueryKeywords.size() != 0));
    }

    // 3. set the offset of results to retrieve

    if (paramsContainer.hasParameterInSummary(ResultsStartOffset)) {
        plan->setOffset(paramsContainer.resultsStartOffset);
    } else { // get it from configuration file
        plan->setOffset(0); // default is zero
    }

    // 4. set the number of results to retrieve
    if (paramsContainer.hasParameterInSummary(NumberOfResults)) {
        plan->setResultsToRetrieve(paramsContainer.numberOfResults);
    } else { // get it from configuration file
        plan->setResultsToRetrieve(
                indexDataContainerConf->getDefaultResultsToRetrieve());
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
    default:
        // SOME SORT OF ASSERT(false) debug gaurd
        break;
    }

    fillExactAndFuzzyQueriesWithCommonInformation(plan);

}

void QueryPlanGen::fillExactAndFuzzyQueriesWithCommonInformation(
        QueryPlan * plan) {

    // 1. first check to see if there is any keywords in the query
    if (paramsContainer.hasParameterInSummary(RawQueryKeywords)) {
        return;
    }

    // 2. Extract the common information from the container

    // length boost
    if (paramsContainer.hasParameterInSummary(LengthBoostFlag)) {
        plan->getExactQuery()->setLengthBoost(paramsContainer.lengthBoost);
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setLengthBoost(paramsContainer.lengthBoost);
        }
    } else { // get it from configuration file
        plan->getExactQuery()->setLengthBoost(
                indexDataContainerConf->getQueryTermLengthBoost());
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setLengthBoost(
                    indexDataContainerConf->getQueryTermLengthBoost());
        }
    }

    // prefix match penalty flag

    if (paramsContainer.hasParameterInSummary(PrefixMatchPenaltyFlag)) {
        plan->getExactQuery()->setPrefixMatchPenalty(
                paramsContainer.prefixMatchPenalty);
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setPrefixMatchPenalty(
                    paramsContainer.prefixMatchPenalty);
        }
    } else { // get it from configuration file
        plan->getExactQuery()->setPrefixMatchPenalty(
                indexDataContainerConf->getPrefixMatchPenalty());
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setPrefixMatchPenalty(
                    indexDataContainerConf->getPrefixMatchPenalty());
        }
    }

    // get query keywords
    const std::vector<std::string> & rawQueryKeywords =
            paramsContainer.rawQueryKeywords;

    vector<float> keywordFuzzyLevel;

    if (paramsContainer.hasParameterInSummary(KeywordFuzzyLevel)) {
        keywordFuzzyLevel = paramsContainer.keywordFuzzyLevel;
    } else { // get it from configuration file
        for (unsigned i = 0; i < rawQueryKeywords.size(); i++) {
            keywordFuzzyLevel.push_back(
                    indexDataContainerConf->getQueryTermSimilarityBoost());
        }
    }

    vector<int> keywordBoostLevel;

    if (paramsContainer.hasParameterInSummary(KeywordBoostLevel)) {
        keywordBoostLevel = paramsContainer.keywordBoostLevel;
    } else { // get it from configuration file
        for (unsigned i = 0; i < rawQueryKeywords.size(); i++) {
            keywordFuzzyLevel.push_back(
                    indexDataContainerConf->getQueryTermBoost());
        }
    }

    vector<srch2is::TermType> keywordPrefixComplete;

    if (paramsContainer.hasParameterInSummary(QueryPrefixCompleteFlag)) {
        keywordPrefixComplete = paramsContainer.keywordPrefixComplete;
    } else { // get it from configuration file
        for (unsigned i = 0; i < rawQueryKeywords.size(); i++) {
            keywordPrefixComplete.push_back(
                    indexDataContainerConf->getQueryTermType() ?
                            srch2is::TERM_TYPE_COMPLETE :
                            srch2is::TERM_TYPE_PREFIX);
            // true means COMPLETE
        }
    }

    vector<unsigned> fieldFilter;

    if (paramsContainer.hasParameterInSummary(FieldFilter)) {
        fieldFilter = paramsContainer.fieldFilterNumbers;
    } else { // get it from configuration file
        for (unsigned i = 0; i < rawQueryKeywords.size(); i++) {
            fieldFilter.push_back(1); // 1 means all attributes with OR operator
        }
    }

    // 3. Fill up query objects
    // exact query
    for (int i = 0; i < rawQueryKeywords.size(); i++) {
        srch2is::Term *exactTerm;
        exactTerm = new srch2is::Term(rawQueryKeywords[i],
                keywordPrefixComplete[i], keywordBoostLevel[i],
                keywordFuzzyLevel[i], 0);
        exactTerm->addAttributeToFilterTermHits(fieldFilter[i]);

        plan->getExactQuery()->add(exactTerm);
    }
    // fuzzy query
    if (plan->isIsFuzzy()) {
        for (int i = 0; i < rawQueryKeywords.size(); i++) {
            srch2is::Term *fuzzyTerm;
            fuzzyTerm = new srch2is::Term(rawQueryKeywords[i],
                    keywordPrefixComplete[i], keywordBoostLevel[i],
                    keywordFuzzyLevel[i],
                    srch2is::Term::getNormalizedThreshold(
                            getUtf8StringCharacterNumber(rawQueryKeywords[i])));
            fuzzyTerm->addAttributeToFilterTermHits(fieldFilter[i]);

            plan->getExactQuery()->add(fuzzyTerm);
        }
    }

}

void QueryPlanGen::createExactAndFuzzyQueriesForTopK(QueryPlan * plan) {
    // allocate the objects
    plan->setExactQuery(new Query(srch2is::TopKQuery));
    if (plan->isIsFuzzy()) {
        plan->setFuzzyQuery(new Query(srch2is::TopKQuery));
    }

}
void QueryPlanGen::createExactAndFuzzyQueriesForGetAllTResults(
        QueryPlan * plan) {
    plan->setExactQuery(new Query(srch2is::GetAllResultsQuery));
    srch2is::SortOrder order =
            (indexDataContainerConf->getOrdering() == 0) ?
                    srch2is::Ascending : srch2is::Descending;
    plan->getExactQuery()->setSortableAttribute(
            indexDataContainerConf->getAttributeToSort(), order);
    // TODO : sortableAttribute and order must be removed from here, all sort jobs must be transfered to
    //        to sort filter, now, when it's GetAllResults, it fisrt finds the results based on an the order given here
    //        and then also applies the sort filter. When this is removed, core also must be changed to not need this
    //        sortable attribute anymore.

    if (plan->isIsFuzzy()) {
        plan->setFuzzyQuery(new Query(srch2is::GetAllResultsQuery));
        plan->getFuzzyQuery()->setSortableAttribute(
                indexDataContainerConf->getAttributeToSort(), order);
    }
}

void QueryPlanGen::createExactAndFuzzyQueriesForGeo(QueryPlan * plan) {
    plan->setExactQuery(new Query(srch2is::MapQuery));
    if (plan->isIsFuzzy()) {
        plan->setFuzzyQuery(new Query(srch2is::MapQuery));
    }
    GeoParameterContainer * gpc = paramsContainer.geoParameterContainer;

    if (gpc->hasParameterInSummary(GeoTypeRectangular)) {
        plan->getExactQuery()->setRange(gpc->leftBottomLatitude,
                gpc->leftBottomLongitude, gpc->rightTopLatitude,
                gpc->rightTopLongitude);
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setRange(gpc->leftBottomLatitude,
                    gpc->leftBottomLongitude, gpc->rightTopLatitude,
                    gpc->rightTopLongitude);
        }
    } else if (gpc->hasParameterInSummary(GeoTypeCircular)) {
        plan->getExactQuery()->setRange(gpc->centerLatitude,
                gpc->centerLongitude, gpc->radius);
        if (plan->isIsFuzzy()) {
            plan->getFuzzyQuery()->setRange(gpc->centerLatitude,
                    gpc->centerLongitude, gpc->radius);
        }
    }

}

}
}
