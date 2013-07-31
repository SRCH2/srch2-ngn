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

#include "QueryRewriter.h"
#include <vector>
#include <string>
#include <algorithm>

using std::string;
using std::vector;

namespace srch2 {

namespace httpwrapper {

QueryRewriter::QueryRewriter(const Srch2ServerConf *indexDataContainerConf,
        const Schema & schema, const Analyzer & analyzer,
        ParsedParameterContainer * paramContainer) :
        schema(schema), analyzer(analyzer) {
    this->paramContainer = paramContainer;
    this->indexDataContainerConf = indexDataContainerConf;
}

void QueryRewriter::rewrite() {
    // go through the summary and call the analyzer on the query if needed.

    // make sure keyword parallel vectors have enough information, if not write information from config file
    prepareKeywordInfo();

    // prepare field filter bits
    // Field filter should be changed from field names to field bit filter
    prepareFieldFilters();

    // rewrite facet query:
    // 1. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file
    //    if it's not even in configuration file remove that field from facets and save error message.
    // 2. If facet is disabled by configuration facet information should be removed
    prepareFacetFilterInfo();

    // apply the analyzer on the query, TODO: analyzer framework needs to expose enough API to apply filters.
    // The following code should e temporary.
    applyAnalyzer();

}

void QueryRewriter::prepareKeywordInfo() {

    unsigned k = 0;
    for (vector<string>::iterator keyword =
            paramContainer->rawQueryKeywords.begin();
            keyword != paramContainer->rawQueryKeywords.end(); ++keyword) {

        if (paramContainer->hasParameterInSummary(KeywordBoostLevel)
                && paramContainer->keywordBoostLevel.at(k) < 0) {
            paramContainer->keywordBoostLevel.at(k) =
                    indexDataContainerConf->getQueryTermBoost();
        }

        if (paramContainer->hasParameterInSummary(KeywordFuzzyLevel)
                && paramContainer->keywordFuzzyLevel.at(k) < 0) {
            paramContainer->keywordFuzzyLevel.at(k) =
                    indexDataContainerConf->getQueryTermSimilarityBoost();
        }

        if (paramContainer->hasParameterInSummary(QueryPrefixCompleteFlag)
                && paramContainer->keywordPrefixComplete.at(k)
                        == srch2is::NOT_SPECIFIED) {
            paramContainer->keywordPrefixComplete.at(k) =
                    indexDataContainerConf->getQueryTermType() ?
                            srch2is::TERM_TYPE_COMPLETE :
                            srch2is::TERM_TYPE_PREFIX;
            // true means complete
        }

        if (paramContainer->hasParameterInSummary(FieldFilter)
                && paramContainer->fieldFilterOps.at(k)
                        == srch2is::OP_NOT_SPECIFIED) {
            paramContainer->fieldFilterOps.at(k) = srch2is::OR; // default field filter operation is OR
            // TODO : get it from configuration file
        }

        //
        k++;
    }
}

void QueryRewriter::applyAnalyzer() {
    Analyzer & analyzer2 = const_cast<Analyzer &>(analyzer); // because of bad design on analyser
    unsigned index = 0;
    vector<unsigned> indexesToErase; // stop word indexes, to be removed later
    // first apply the analyzer
    for (vector<string>::iterator keyword =
            paramContainer->rawQueryKeywords.begin();
            keyword != paramContainer->rawQueryKeywords.end(); ++keyword) {
        string afterAnalyzer = analyzer2.applyFilters(*keyword);
        if (afterAnalyzer.compare("") == 0) { // analyzer removed this keyword, it's probably stopword
            indexesToErase.push_back(index);
        } else { // just apply the analyzer
            *keyword = afterAnalyzer;
        }
        //
        ++index;
    }
    // now erase the data of erased keywords
    std::vector<std::string> rawQueryKeywords;
    std::vector<float> keywordFuzzyLevel;
    std::vector<unsigned> keywordBoostLevel;
    std::vector<srch2is::TermType> keywordPrefixComplete;
    std::vector<std::vector<std::string> > fieldFilter;
    std::vector<srch2is::BooleanOperation> fieldFilterOps;
    // first keep the rest of keywords so that indexes stay valid (cannot remove and iterate in the same time)
    for (int i = 0; i < paramContainer->rawQueryKeywords.size(); i++) {
        if (std::find(indexesToErase.begin(), indexesToErase.end(), i)
                == indexesToErase.end()) { // don't erase
            rawQueryKeywords.push_back(paramContainer->rawQueryKeywords.at(i));
            keywordFuzzyLevel.push_back(
                    paramContainer->keywordFuzzyLevel.at(i));
            keywordBoostLevel.push_back(
                    paramContainer->keywordBoostLevel.at(i));
            keywordPrefixComplete.push_back(
                    paramContainer->keywordPrefixComplete.at(i));
            fieldFilter.push_back(paramContainer->fieldFilter.at(i));
            fieldFilterOps.push_back(paramContainer->fieldFilterOps.at(i));
        }
    }
    // then copy back
    paramContainer->rawQueryKeywords = rawQueryKeywords;
    paramContainer->keywordFuzzyLevel = keywordFuzzyLevel;
    paramContainer->keywordBoostLevel = keywordBoostLevel;
    paramContainer->keywordPrefixComplete = keywordPrefixComplete;
    paramContainer->fieldFilter = fieldFilter;
    paramContainer->fieldFilterOps = fieldFilterOps;

    // TODO : validation case : what if all keywords are stop words and not the vectors are empty ?

}

// this function creates the bit sequence needed for field filter based on the filter names
void QueryRewriter::prepareFieldFilters() {

    unsigned f = 0;
    for (std::vector<std::vector<std::string> >::iterator fields =
            paramContainer->fieldFilter.begin();
            fields != paramContainer->fieldFilter.end(); ++fields) {
        srch2is::BooleanOperation op = paramContainer->fieldFilterOps.at(f);

        unsigned filter = 0;
        if (fields->size() == 0) {
            // get it from configuration file
            filter = 1; // TODO : temporary
        } else {
            for (std::vector<std::string>::iterator field = fields->begin();
                    field != fields->end(); ++field) {

                if (field->compare("*")) { // all fields
                    filter = 1;
                    break;
                }
                unsigned id = schema.getSearchableAttributeId(*field);
                unsigned bit = 1;
                bit <<= id;
                filter |= bit;
            }
            if (op == srch2is::AND) {
                filter |= 0x80000000;
            }
        }
        paramContainer->fieldFilterNumbers.at(f) = filter;

        //
        f++;
    }
}

// rewrite facet query:
// 1. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file
//    if it's not even in configuration file remove that field from facets and save error message.
// 2. If facet is disabled by configuration facet information should be removed
void QueryRewriter::prepareFacetFilterInfo() {

    FacetQueryContainer * facetQueryContainer = NULL;
    if (paramContainer->hasParameterInSummary(GetAllResultsSearchType)) { // get all results search
        if (paramContainer->getAllResultsParameterContainer->hasParameterInSummary(
                FacetQueryHandler)) { // we have facet filter
            facetQueryContainer =
                    paramContainer->getAllResultsParameterContainer->facetQueryContainer;
        }

    }

    if (paramContainer->hasParameterInSummary(GeoSearchType)) { // geo search
        if (paramContainer->geoParameterContainer->hasParameterInSummary(
                FacetQueryHandler)) { // we have facet filter
            facetQueryContainer =
                    paramContainer->geoParameterContainer->facetQueryContainer;
        }
    }

    if (facetQueryContainer == NULL) { // there is no facet filter
        return;
    }

    // there is a facet filter, continue with rewriting ...

    // 1. Remove everything if facet is disabled in config file.
    if (!indexDataContainerConf->isFacetEnabled()) {

        // remove facet flag from summary

        if (paramContainer->hasParameterInSummary(GetAllResultsSearchType)) { // get all results search

            paramContainer->getAllResultsParameterContainer->summary.erase(
                    remove(
                            paramContainer->getAllResultsParameterContainer->summary.begin(),
                            paramContainer->getAllResultsParameterContainer->summary.end(),
                            FacetQueryHandler),
                    paramContainer->getAllResultsParameterContainer->summary.end());
        } else if (paramContainer->hasParameterInSummary(GeoSearchType)) {

            paramContainer->geoParameterContainer->summary.erase(
                    remove(
                            paramContainer->geoParameterContainer->summary.begin(),
                            paramContainer->geoParameterContainer->summary.end(),
                            FacetQueryHandler),
                    paramContainer->geoParameterContainer->summary.end());
        }

        // free facet container
        delete facetQueryContainer;

        return;
    }

    // 2. Fill out the empty places in facet info vectors
    unsigned t = 0;
    for (std::vector<srch2is::FacetType>::iterator type =
            facetQueryContainer->types.begin();
            type != facetQueryContainer->types.end(); ++type) {
        if (*type == srch2is::Simple) { // just makes sure facet container vactors are not used in later steps in the pipeline
            facetQueryContainer->rangeStarts.at(t) = "";
            facetQueryContainer->rangeEnds.at(t) = "";
            facetQueryContainer->rangeGaps.at(t) = "";
        } else if (*type == srch2is::Range) { /// fills out the empty places
            if (facetQueryContainer->rangeStarts.at(t).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator tmp = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(t));
                // TODO : using tmp - begin() might not be safe
                if (tmp
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string startFromConfig =
                            indexDataContainerConf->getFacetStarts()->at(
                                    tmp
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeStarts.at(t) = startFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }

            if (facetQueryContainer->rangeEnds.at(t).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator tmp = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(t));
                // TODO : using tmp - begin() might not be safe
                if (tmp
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string endFromConfig =
                            indexDataContainerConf->getFacetEnds()->at(
                                    tmp
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeEnds.at(t) = endFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }

            if (facetQueryContainer->rangeGaps.at(t).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator tmp = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(t));
                // TODO : using tmp - begin() might not be safe
                if (tmp
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string gapFromConfig =
                            indexDataContainerConf->getFacetGaps()->at(
                                    tmp
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeGaps.at(t) = gapFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }
        }

        //
        t++;
    }

}

}
}
