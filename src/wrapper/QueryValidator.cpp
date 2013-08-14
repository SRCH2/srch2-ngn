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

#include "QueryValidator.h"

#include "ParserUtility.h"
#include <algorithm>
#include <string>
#include <vector>
#include <map>

using namespace std;

namespace srch2 {

namespace httpwrapper {

QueryValidator::QueryValidator(const Schema & schema,
        const ConfigManager &indexDataContainerConf,
        ParsedParameterContainer * paramContainer) :
        schema(schema), indexDataContainerConf(indexDataContainerConf){
    this->paramContainer = paramContainer;
}

// this function goes through the sumary and based on that validates the query.
bool QueryValidator::validate() {

    // validation case : Only one of the search types should exist in summary
    int numberOfProvidedSearchTypes = 0;
    if (paramContainer->hasParameterInQuery(TopKSearchType)) {
        numberOfProvidedSearchTypes++;
    }
    if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) {
        numberOfProvidedSearchTypes++;
    }
    if (paramContainer->hasParameterInQuery(GeoSearchType)) {
        numberOfProvidedSearchTypes++;
    }
    if (numberOfProvidedSearchTypes != 1) { // search type is not clear , fatal error
        paramContainer->messages.push_back(
                std::make_pair(MessageError,
                        "Search type is not clear. Fatal error."));
        return false;
    }

    // validation case: if search type is TopK or GetAllResults, query keywords should not be empty
    if (paramContainer->hasParameterInQuery(TopKSearchType)
            || paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // search type is either TopK or GetAllResults
        if (paramContainer->rawQueryKeywords.size() == 0) {
            paramContainer->messages.push_back(
                    std::make_pair(MessageError,
                            "No keywords provided for search."));
            return false;
        }
    }

    // validation case: If search type is Geo, some latitude longitude must be provided.
    if (paramContainer->hasParameterInQuery(GeoSearchType)) {
        if (!(paramContainer->geoParameterContainer->hasParameterInQuery(GeoTypeRectangular) // no rectangular values
                || paramContainer->geoParameterContainer->hasParameterInQuery(GeoTypeCircular))) { // no circular values
            paramContainer->messages.push_back(
                    std::make_pair(MessageError,
                            "No latitude longitude provided for geo search."));
            return false;
        }
    }

    // validation case: if search type is TopK/GetAllResults, index type must be TopK/GetAllResults too
    if (paramContainer->hasParameterInQuery(TopKSearchType)
            || paramContainer->hasParameterInQuery(GetAllResultsSearchType)) {
        if (indexDataContainerConf.getIndexType() != 0) { // zero means normal index type
            paramContainer->messages.push_back(
                    std::make_pair(MessageError,
                            "Geo index type is not compatible with this query."));
            return false;
        }
    }

    // validation case: if search type is Geo, index type must be Geo too
    if (paramContainer->hasParameterInQuery(GeoSearchType)) {
        if (indexDataContainerConf.getIndexType() != 1) { // One means geo index type
            paramContainer->messages.push_back(
                    std::make_pair(MessageError,
                            "Geo index type is needed for this query."));
            return false;
        }
    }

    // validate filter query
    if (!validateExistenceOfAttributesInFieldList()) {
        return false;
    }

    // validate sort filter
    if (!validateExistenceOfAttributesInSortFiler()) {
        return false;
    }

    // validate facet filter
    if (!validateExistenceOfAttributesInFacetFiler()) {
        return false;
    }

    return true;
}

bool QueryValidator::validateExistenceOfAttributesInFieldList() {
    if (paramContainer->hasParameterInQuery(FieldFilter)) { // field filter list is not empty

        const std::map<std::string, unsigned>& searchableAttributes =
                schema.getSearchableAttribute();
        for (vector<vector<string> >::iterator fields =
                paramContainer->fieldFilter.begin();
                fields != paramContainer->fieldFilter.end(); ++fields) {
            for (vector<string>::iterator field = fields->begin();
                    field != fields->end(); ++field) {
                if (searchableAttributes.find(*field)
                        == searchableAttributes.end()) { // field does not exist in searchable attributes
                        // write a warning and change field value to *
                    paramContainer->messages.push_back(
                            std::make_pair(MessageWarning,
                                    "Field " + *field
                                            + " is not a searchable field. We changed it to * (all fields). "));
                    *field = "*"; // this * means all fields , query rewriter will interpret this one.
                }
            }
        }
    }
    return true;
}

bool QueryValidator::validateExistenceOfAttributesInSortFiler() {

    // first find if we have any sort filter in query
    SortQueryContainer * sortQueryContainer = NULL;
    if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search
        if (paramContainer->getAllResultsParameterContainer->hasParameterInQuery(
                SortQueryHandler)) { // we have sort filter
            sortQueryContainer =
                    paramContainer->getAllResultsParameterContainer->sortQueryContainer;
        }

    }

    if (paramContainer->hasParameterInQuery(GeoSearchType)) { // geo search
        if (paramContainer->geoParameterContainer->hasParameterInQuery(
                SortQueryHandler)) { // we have sort filter
            sortQueryContainer =
                    paramContainer->geoParameterContainer->sortQueryContainer;
        }
    }

    if (sortQueryContainer == NULL) { // no sort filter to validate
        return true;
    }

    // now validate the fields , if one field is not found in attributes sort will be canceled.

    bool sortFilterShouldBeRemoved = false;
    for (std::vector<std::string>::iterator field =
            sortQueryContainer->evaluator->field.begin();
            field != sortQueryContainer->evaluator->field.end(); ++field) {
        if (schema.getNonSearchableAttributeId(*field) < 0) { // field does not exist
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Field " + *field
                                    + " is not a non-searchable field. No sort will be performed. "));

            sortFilterShouldBeRemoved = true;
            break;
        }
    }
    if (sortFilterShouldBeRemoved) {
        // Warning : Sort will be canceled.
        // The following if-else statement removes the flag of sort filter from the corresponding summary
        if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search

            paramContainer->getAllResultsParameterContainer->parametersInQuery.erase(
                    remove(
                            paramContainer->getAllResultsParameterContainer->parametersInQuery.begin(),
                            paramContainer->getAllResultsParameterContainer->parametersInQuery.end(),
                            SortQueryHandler),
                    paramContainer->getAllResultsParameterContainer->parametersInQuery.end());
        } else if (paramContainer->hasParameterInQuery(GeoSearchType)) {

            paramContainer->geoParameterContainer->parametersInQuery.erase(
                    remove(
                            paramContainer->geoParameterContainer->parametersInQuery.begin(),
                            paramContainer->geoParameterContainer->parametersInQuery.end(),
                            SortQueryHandler),
                    paramContainer->geoParameterContainer->parametersInQuery.end());
        }
        // container and its validator must be freed here
        delete sortQueryContainer->evaluator;
        delete sortQueryContainer;
        if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search
            paramContainer->getAllResultsParameterContainer->sortQueryContainer = NULL;

        }

        if (paramContainer->hasParameterInQuery(GeoSearchType)) { // geo search
            paramContainer->geoParameterContainer->sortQueryContainer = NULL;
        }
    }

    return true;

}

bool QueryValidator::validateExistenceOfAttributesInFacetFiler() {

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

    if (facetQueryContainer == NULL) { // there is no facet query to validate

        return true;
    }

    int facetParallelVectorsIndex = 0;
    vector<int> facetParallelVectorsIndexesToErase;
    for (std::vector<std::string>::iterator field =
            facetQueryContainer->fields.begin();
            field != facetQueryContainer->fields.end(); ++field) {

        //1. Validate the existence of attributes
        if (schema.getNonSearchableAttributeId(*field) < 0) { // field does not exist
            // Warning : Facet will be canceled for this field.
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Field " + *field
                                    + " is not a proper field. No facet will be calculated on this field. "));

            facetParallelVectorsIndexesToErase.push_back(
                    facetParallelVectorsIndex);
            continue; // no need to do anymore validation for this field because it'll be removed from facets.
        }



        //2. Range facets should be of type unsigned or float or date
        FilterType fieldType = schema.getTypeOfNonSearchableAttribute(
                schema.getNonSearchableAttributeId(*field));

        if (   !facetQueryContainer->types.empty()
            && facetQueryContainer->types.at(facetParallelVectorsIndex) == srch2is::FacetTypeRange
            && !(
                    fieldType == srch2is::ATTRIBUTE_TYPE_UNSIGNED ||
                    fieldType == srch2is::ATTRIBUTE_TYPE_FLOAT ||
                    fieldType == srch2is::ATTRIBUTE_TYPE_TIME
             )) {
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Field " + *field
                                    + " is not a proper field for range facet. No facet will be calculated on this field. "));

            facetParallelVectorsIndexesToErase.push_back(
                    facetParallelVectorsIndex);
            continue;
        }

        // empty string is the place holder for start,end and gap of facets.
        //3. validate the start,end, gap values
        bool valid = true;
        if (!facetQueryContainer->rangeStarts.empty() &&
                facetQueryContainer->rangeStarts.at(facetParallelVectorsIndex).compare(
                "") != 0) {
            valid = validateValueWithType(fieldType,
                    facetQueryContainer->rangeStarts.at(
                            facetParallelVectorsIndex));
        }
        // TODO : else : then this field must exist in config file
        if (valid && !facetQueryContainer->rangeEnds.empty()
                && facetQueryContainer->rangeEnds.at(facetParallelVectorsIndex).compare(
                        "") != 0) {
            valid = validateValueWithType(fieldType,
                    facetQueryContainer->rangeEnds.at(
                            facetParallelVectorsIndex));
        }
        // TODO : else : then this field must exist in config file
        if (valid && !facetQueryContainer->rangeGaps.empty()
                && facetQueryContainer->rangeGaps.at(facetParallelVectorsIndex).compare(
                        "") != 0) {
            valid = validateValueWithType(fieldType,
                    facetQueryContainer->rangeGaps.at(
                            facetParallelVectorsIndex));
        }
        // TODO :  else : then this field must exist in config file
        if (!valid) { // start,end or gap value is not compatible with attribute type
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Start,End or Gap value for field " + *field
                                    + " is not compatible with its type. No facet will be calculated on this field. "));

            facetParallelVectorsIndexesToErase.push_back(
                    facetParallelVectorsIndex);
            continue;
        }

        //
        facetParallelVectorsIndex++;
    }
    if (facetQueryContainer->fields.size() != 0 &&
            facetParallelVectorsIndexesToErase.size() == facetQueryContainer->fields.size()) {
        // all facet fields are removed, so there is no facet query anymore
        // facet must be removed from summary
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
        // facet container should be freed here
        delete facetQueryContainer;
        if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results search
            paramContainer->getAllResultsParameterContainer->facetQueryContainer = NULL;

        }

        if (paramContainer->hasParameterInQuery(GeoSearchType)) { // geo search
            paramContainer->geoParameterContainer->facetQueryContainer = NULL;
        }
        return true;
    }

    // remove all facet fields that must be removed
    // Because we collected indexes, first copy the rest of them in temporary vectors and then copy all back
    std::vector<srch2is::FacetType> types;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
    for (int i = 0; i < facetQueryContainer->fields.size(); i++) {
        if (find(facetParallelVectorsIndexesToErase.begin(),
                facetParallelVectorsIndexesToErase.end(), i)
                == facetParallelVectorsIndexesToErase.end()) { // index not in eraseVector so don't erase
            fields.push_back(facetQueryContainer->fields.at(i));
            if(!facetQueryContainer->types.empty()){
                types.push_back(facetQueryContainer->types.at(i));
            }
            if(!facetQueryContainer->rangeStarts.empty()){
                rangeStarts.push_back(facetQueryContainer->rangeStarts.at(i));
            }
            if(!facetQueryContainer->rangeEnds.empty()){
                rangeEnds.push_back(facetQueryContainer->rangeEnds.at(i));
            }
            if(!facetQueryContainer->rangeGaps.empty()){
                rangeGaps.push_back(facetQueryContainer->rangeGaps.at(i));
            }
        }
    }
    // now copy back
    facetQueryContainer->types = types;
    facetQueryContainer->fields = fields;
    facetQueryContainer->rangeStarts = rangeStarts;
    facetQueryContainer->rangeEnds = rangeEnds;
    facetQueryContainer->rangeGaps = rangeGaps;

    return true;

}

//bool QueryValidator::validateValueWithType(srch2is::FilterType type,
//        string  & value) {
//    switch (type) {
//    case srch2is::ATTRIBUTE_TYPE_UNSIGNED:
//        return isInteger(value);
//    case srch2is::ATTRIBUTE_TYPE_FLOAT:
//        return isFloat(value);
//    case srch2is::ATTRIBUTE_TYPE_TEXT:
//        return true; // TEXT does not have any criteria ?????
//    case srch2is::ATTRIBUTE_TYPE_TIME:
//        return isTime(value);
//    }
//    // flow never reaches here
//    return false;
//}

}
}
