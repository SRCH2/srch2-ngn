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

    /*
     * Example:
     * for example a user can provide a query like : foo~0.5 AND bar
     * for this query:
     * rawQueryKeyword vector is <foo , bar>
     * fuzzyLevel vector is <0.5,-1> // should rewrite -1 to what's comming from configuration file
     * boostLevel vector is <> , no rewrite, planGen understands empty vector
     * prefixComplete vector <>
     */
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

        if (paramContainer->hasParameterInQuery(KeywordBoostLevel)
                && paramContainer->keywordBoostLevel.at(k) < 0) { // -1 is the place holder for numerical parameters ...
            paramContainer->keywordBoostLevel.at(k) =
                    indexDataContainerConf->getQueryTermBoost();
        }

        if (paramContainer->hasParameterInQuery(KeywordFuzzyLevel)
                && paramContainer->keywordFuzzyLevel.at(k) < 0) {
            paramContainer->keywordFuzzyLevel.at(k) =
                    indexDataContainerConf->getQueryTermSimilarityBoost();
        }

        if (paramContainer->hasParameterInQuery(QueryPrefixCompleteFlag)
                && paramContainer->keywordPrefixComplete.at(k)
                        == srch2is::TERM_TYPE_NOT_SPECIFIED) {
            paramContainer->keywordPrefixComplete.at(k) =
                    indexDataContainerConf->getQueryTermType() ?
                            srch2is::TERM_TYPE_COMPLETE :
                            srch2is::TERM_TYPE_PREFIX;
            // true means complete
        }

        if (paramContainer->hasParameterInQuery(FieldFilter)
                && paramContainer->fieldFilterOps.at(k)
                        == srch2is::OP_NOT_SPECIFIED) {
            paramContainer->fieldFilterOps.at(k) = srch2is::BooleanOperatorOR; // default field filter operation is OR
            // TODO : get it from configuration file
        }

        //
        k++;
    }
}

void QueryRewriter::applyAnalyzer() {
    Analyzer & analyzerNotConst = const_cast<Analyzer &>(analyzer); // because of bad design on analyzer
    unsigned keywordIndex = 0;
    vector<unsigned> keywordIndexesToErase; // stop word indexes, to be removed later
    // first apply the analyzer
    for (vector<string>::iterator keyword =
            paramContainer->rawQueryKeywords.begin();
            keyword != paramContainer->rawQueryKeywords.end(); ++keyword) {
        string keywordAfterAnalyzer = analyzerNotConst.applyFilters(*keyword);
        if (keywordAfterAnalyzer.compare("") == 0) { // analyzer removed this keyword, it's probably stopword
            keywordIndexesToErase.push_back(keywordIndex);
        } else { // just apply the analyzer
            *keyword = keywordAfterAnalyzer;
        }
        //
        ++keywordIndex;
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
        if (std::find(keywordIndexesToErase.begin(), keywordIndexesToErase.end(), i)
                == keywordIndexesToErase.end()) { // don't erase
            rawQueryKeywords.push_back(paramContainer->rawQueryKeywords.at(i));
            keywordFuzzyLevel.push_back(paramContainer->keywordFuzzyLevel.at(i));
            keywordBoostLevel.push_back(paramContainer->keywordBoostLevel.at(i));
            keywordPrefixComplete.push_back(paramContainer->keywordPrefixComplete.at(i));
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

    unsigned keywordIndex = 0;
    for (std::vector<std::vector<std::string> >::iterator fields =
            paramContainer->fieldFilter.begin();
            fields != paramContainer->fieldFilter.end(); ++fields) {
        srch2is::BooleanOperation op = paramContainer->fieldFilterOps.at(keywordIndex);

        unsigned filter = 0;
        if (fields->size() == 0) {
            filter = 0x7fffffff;  // it can appear in all fields
            // TODO : get it from configuration file
        } else {
            bool shouldApplyAnd = true;
            for (std::vector<std::string>::iterator field = fields->begin();
                    field != fields->end(); ++field) {

                if (field->compare("*")) { // all fields
                    filter = 0x7fffffff;
                    shouldApplyAnd = false;
                    break;
                }
                unsigned id = schema.getSearchableAttributeId(*field);
                unsigned bit = 1;
                bit <<= id;
                filter |= bit;
            }
            if (op == srch2is::BooleanOperatorAND && shouldApplyAnd) {
                filter |= 0x80000000;
            }
        }
        paramContainer->fieldFilterNumbers.at(keywordIndex) = filter;

        //
        keywordIndex++;
    }
}

// rewrite facet query:
// 1. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file.
//    if it's not even in configuration file remove that field from facets and save error message.
// 2. If facet is disabled by configuration, facet information should be removed.
void QueryRewriter::prepareFacetFilterInfo() {

    FacetQueryContainer * facetQueryContainer = NULL;
    if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search
        if (paramContainer->getAllResultsParameterContainer->hasParameterInQuery(
                FacetQueryHandler)) { // we have facet filter
            facetQueryContainer =
                    paramContainer->getAllResultsParameterContainer->facetQueryContainer;
        }

    }

    if (paramContainer->hasParameterInQuery(GeoSearchType)) { // geo search
        if (paramContainer->geoParameterContainer->hasParameterInQuery(
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

        if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search

            paramContainer->getAllResultsParameterContainer->parametersInQuery.erase(
                    remove(
                            paramContainer->getAllResultsParameterContainer->parametersInQuery.begin(),
                            paramContainer->getAllResultsParameterContainer->parametersInQuery.end(),
                            FacetQueryHandler),
                    paramContainer->getAllResultsParameterContainer->parametersInQuery.end());
        } else if (paramContainer->hasParameterInQuery(GeoSearchType)) {

            paramContainer->geoParameterContainer->parametersInQuery.erase(
                    remove(
                            paramContainer->geoParameterContainer->parametersInQuery.begin(),
                            paramContainer->geoParameterContainer->parametersInQuery.end(),
                            FacetQueryHandler),
                    paramContainer->geoParameterContainer->parametersInQuery.end());
        }

        // free facet container
        delete facetQueryContainer;
        facetQueryContainer = NULL;

        return;
    }
    // type should also be rewritten in case it's not given by the user.
    if(facetQueryContainer->types.empty()){
        for (std::vector<std::string>::iterator fieldName =
                facetQueryContainer->fields.begin();
                fieldName != facetQueryContainer->fields.end() ; ++fieldName){

            vector<string>::const_iterator facetIteratorInConfVector = find(
                    indexDataContainerConf->getFacetAttributes()->begin(),
                    indexDataContainerConf->getFacetAttributes()->end(),
                    *fieldName);
            int fieldFacetTypeInt =
                    indexDataContainerConf->getFacetTypes()->at(
                            std::distance(indexDataContainerConf->getFacetAttributes()->begin() , facetIteratorInConfVector));
            // TODO : validation case : if there is any missing information and this field is not in config file either
            // this field must be removed from facet fields.
            if(fieldFacetTypeInt == 0){
                facetQueryContainer->types.push_back(srch2is::FacetTypeCategorical);
            }else if (fieldFacetTypeInt == 1){
                facetQueryContainer->types.push_back(srch2is::FacetTypeRange);
            }
        }
    }else{ // if types vector is not empty it's assumed to be equal size with fields (from parser)
        for (std::vector<srch2is::FacetType>::iterator facetType =
                facetQueryContainer->types.begin();
                facetType != facetQueryContainer->types.end(); ++facetType) {
            if(*facetType == srch2is::FacetTypeNonSpecified){
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(std::distance(facetType ,facetQueryContainer->types.begin() )));
                int fieldFacetTypeInt =
                        indexDataContainerConf->getFacetTypes()->at(
                                std::distance(indexDataContainerConf->getFacetAttributes()->begin() , facetIteratorInConfVector));
                // TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
                if(fieldFacetTypeInt == 0){
                    *facetType = srch2is::FacetTypeCategorical;
                }else if (fieldFacetTypeInt == 1){
                    *facetType = srch2is::FacetTypeRange;
                }
            }
        }
    }
    // 2. Fill out the empty places in facet info vectors
    unsigned facetFieldIndex = 0;
    for (std::vector<srch2is::FacetType>::iterator type =
            facetQueryContainer->types.begin();
            type != facetQueryContainer->types.end(); ++type) {
        if (*type == srch2is::FacetTypeCategorical) { // just makes sure facet container vactors are not used in later steps in the pipeline
            facetQueryContainer->rangeStarts.at(facetFieldIndex) = "";
            facetQueryContainer->rangeEnds.at(facetFieldIndex) = "";
            facetQueryContainer->rangeGaps.at(facetFieldIndex) = "";
        } else if (*type == srch2is::FacetTypeRange) { /// fills out the empty places
            if (facetQueryContainer->rangeStarts.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                // TODO : using tmp - begin() might not be safe
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string startFromConfig =
                            indexDataContainerConf->getFacetStarts()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeStarts.at(facetFieldIndex) = startFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }

            if (facetQueryContainer->rangeEnds.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                // TODO : using tmp - begin() might not be safe
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string endFromConfig =
                            indexDataContainerConf->getFacetEnds()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeEnds.at(facetFieldIndex) = endFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }

            if (facetQueryContainer->rangeGaps.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                // TODO : using tmp - begin() might not be safe
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string gapFromConfig =
                            indexDataContainerConf->getFacetGaps()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
                    facetQueryContainer->rangeGaps.at(facetFieldIndex) = gapFromConfig;
                }
                // else : TODO : validation case : if there is any missing information and this field is not in config file either
                // this field must be removed from facet fields.
            }
        }

        //
        facetFieldIndex++;
    }

}

}
}
