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

namespace srch2{

namespace httpwrapper{


QueryValidator::QueryValidator(const Schema & schema ,
		const Srch2ServerConf *indexDataContainerConf,
		ParsedParameterContainer * paramContainer) : schema(schema){
	this->paramContainer = paramContainer;
	this->indexDataContainerConf = indexDataContainerConf;
}


// this function goes through the sumary and based on that validates the query.
bool QueryValidator::validate(){


	// validate filter query
	if(! validateExistanceOfAttributesInFieldList()){
		return false;
	}

	// validate sort filter
	if(! validateExistanceOfAttributesInSortFiler()){
		return false;
	}


	// validate facet filter
	if(! validateExistanceOfAttributesInFacetFiler()){
		return false;
	}

	// validation case : Only one of the search types should exist in summary
	int tmp = 0;
	if(paramContainer->hasParameterInSummary(TopKSearchType) ){
		tmp ++;
	}
	if(paramContainer->hasParameterInSummary(GetAllResultsSearchType) ){
		tmp ++;
	}
	if(paramContainer->hasParameterInSummary(GeoSearchType)){
		tmp++;
	}
	if(tmp > 1){ // search type is not clear , fatal error
		paramContainer->messages.push_back(std::make_pair(MessageError,
				"Search type is not clear. Fatal error." ));
		return false;
	}

	// validation case: if search type is TopK or GetAllResults query keywords should not be empty
	if(paramContainer->hasParameterInSummary(TopKSearchType)  ||
			paramContainer->hasParameterInSummary(GetAllResultsSearchType)){ // search type is either TopK or GetAllResults
		if (paramContainer->rawQueryKeywords.size() == 0){
			paramContainer->messages.push_back(std::make_pair(MessageError,
					"No keywords provided for search." ));
			return false;
		}
	}


	// validation case: If search type is Geo, some latitude longitude must be provided.
	if(paramContainer->hasParameterInSummary(GeoSearchType)){
		if(! (paramContainer->geoParameterContainer->hasParameterInSummary(GeoTypeRectangular)) // no rectangular values
				&&
				! (paramContainer->geoParameterContainer->hasParameterInSummary(GeoTypeCircular))){ // no circular values
			if (paramContainer->rawQueryKeywords.size() == 0){
				paramContainer->messages.push_back(std::make_pair(MessageError,
						"No latitude longitude provided for geo search." ));
				return false;
			}
		}
	}

	// validation case: if search type is TopK/GetAllResults, index type must be TopK/GetAllResults too
	if(paramContainer->hasParameterInSummary(TopKSearchType)
			|| paramContainer->hasParameterInSummary(GetAllResultsSearchType)){
		if(indexDataContainerConf->getIndexType() != 0){
			paramContainer->messages.push_back(std::make_pair(MessageError,
					"Query type is wrong" ));
			return false;
		}
	}

	// validation case: if search type is Geo, index type must be Geo too
	if(paramContainer->hasParameterInSummary(GeoSearchType)){
		if(indexDataContainerConf->getIndexType() != 1){
			paramContainer->messages.push_back(std::make_pair(MessageError,
					"Query type is wrong" ));
			return false;
		}
	}

	return true;
}

bool QueryValidator::validateExistanceOfAttributesInFieldList(){
	if(paramContainer->hasParameterInSummary(FieldFilter) ){ // field filter list is not empty

		const std::map<std::string, unsigned>& searchableAttributes = schema.getSearchableAttribute();
		for(vector<vector<string> >::iterator fields = paramContainer->fieldFilter.begin() ;
				fields != paramContainer->fieldFilter.end() ; ++ fields){
			for(vector<string>::iterator field = fields->begin(); field != fields->end() ; ++field){
				if(searchableAttributes.find(*field) == searchableAttributes.end()){ // field does not exist in searchable attributes
					// write a warning and change field value to *
					paramContainer->messages.push_back(std::make_pair(MessageWarning,
							"Field " + *field + " is not a searchable field. We changed it to * (all fields). " ));
					*field = "*";
				}
			}
		}
	}
	return true;
}

bool QueryValidator::validateExistanceOfAttributesInSortFiler(){

	// first find if we have any sort filter in query
	SortQueryContainer * sortQueryContainer = NULL;
	if(paramContainer->hasParameterInSummary(GetAllResultsSearchType)){ // get all results search
		if(paramContainer->getAllResultsParameterContainer->hasParameterInSummary(SortQueryHandler)){ // we have sort filter
			sortQueryContainer	= paramContainer->getAllResultsParameterContainer->sortQueryContainer;
		}

	}

	if(paramContainer->hasParameterInSummary(GeoSearchType) ){ // geo search
		if(paramContainer->geoParameterContainer->hasParameterInSummary(SortQueryHandler)){ // we have sort filter
			sortQueryContainer = paramContainer->geoParameterContainer->sortQueryContainer;
		}
	}

	// now if we have sort filter validate the fields
	if(sortQueryContainer != NULL){ // we have sort filter
		bool sortFilterShouldBeRemoved = false;
		for(std::vector<std::string>::iterator field = sortQueryContainer->evaluator->field.begin() ;
					field != sortQueryContainer->evaluator->field.end() ; ++field){
			if(schema.getNonSearchableAttributeId(*field) == -1){ // field does not exist
				// Warning : Sort will be canceled.
				paramContainer->messages.push_back(std::make_pair(MessageWarning,
						"Field " + *field + " is not a searchable field. No sort will be performed. " ));

				sortFilterShouldBeRemoved = true;
				break;
			}
		}
		if(sortFilterShouldBeRemoved){
			if(paramContainer->hasParameterInSummary(GetAllResultsSearchType)){ // get all results search

				paramContainer->getAllResultsParameterContainer->summary.erase(
						remove(paramContainer->getAllResultsParameterContainer->summary.begin(),
								paramContainer->getAllResultsParameterContainer->summary.end(), SortQueryHandler) ,
								paramContainer->getAllResultsParameterContainer->summary.end());
				// FIXME : should we delete sort container here ?
			}else if(paramContainer->hasParameterInSummary(GeoSearchType) ){

				paramContainer->geoParameterContainer->summary.erase(
						remove(paramContainer->geoParameterContainer->summary.begin(),
								paramContainer->geoParameterContainer->summary.end(), SortQueryHandler) ,
								paramContainer->geoParameterContainer->summary.end());
			}

		}
	}

	return true;

}

bool QueryValidator::validateExistanceOfAttributesInFacetFiler(){

	FacetQueryContainer * facetQueryContainer = NULL;
	if(paramContainer->hasParameterInSummary(GetAllResultsSearchType) ){ // get all results search
		if(paramContainer->getAllResultsParameterContainer->hasParameterInSummary(FacetQueryHandler)){ // we have facet filter
			facetQueryContainer	= paramContainer->getAllResultsParameterContainer->facetQueryContainer;
		}

	}

	if(paramContainer->hasParameterInSummary(GeoSearchType) ){ // geo search
		if(paramContainer->geoParameterContainer->hasParameterInSummary(FacetQueryHandler)){ // we have facet filter
			facetQueryContainer = paramContainer->geoParameterContainer->facetQueryContainer;
		}
	}

	if(facetQueryContainer != NULL){ // there is a facet filter request
		int index = 0;
		vector<int> indexesToErase;
		for(std::vector<std::string>::iterator field = facetQueryContainer->fields.begin();
				field != facetQueryContainer->fields.end() ; ++field){

			//1. Validate the existence of attribites
			if(schema.getNonSearchableAttributeId(*field) == -1){ // field does not exist
				// Warning : Facet will be canceled for this field.
				paramContainer->messages.push_back(std::make_pair(MessageWarning,
						"Field " + *field + " is not a proper field. No facet will be calculated on this field. " ));

				indexesToErase.push_back(index);
				continue;// no need to do anymore validation for this field because it'll be removed from facets.
			}

			//2. Range facets should be of type unsigned or float or date
			FilterType type =  schema.getTypeOfNonSearchableAttribute(schema.getNonSearchableAttributeId(*field));

			if( !(type == srch2is::UNSIGNED || type == srch2is::FLOAT || type == srch2is::TIME) &&
					facetQueryContainer->types.at(index) == srch2is::Range){
				paramContainer->messages.push_back(std::make_pair(MessageWarning,
						"Field " + *field + " is not a proper field for range facet. No facet will be calculated on this field. " ));

				indexesToErase.push_back(index);
				continue;
			}


			//3. validate the start,end, gap values
			bool valid = true;
			if(facetQueryContainer->rangeStarts.at(index).compare("") != 0){
				valid = validateValueWithType(type, facetQueryContainer->rangeStarts.at(index));
			}
			if(valid && facetQueryContainer->rangeEnds.at(index).compare("") != 0){
				valid = validateValueWithType(type, facetQueryContainer->rangeEnds.at(index));
			}
			if(valid && facetQueryContainer->rangeGaps.at(index).compare("") != 0){
				valid = validateValueWithType(type, facetQueryContainer->rangeGaps.at(index));
			}
			if(! valid){ // start,end or gap value is not compatible with attribute type
				paramContainer->messages.push_back(std::make_pair(MessageWarning,
						"Start,End or Gap value for field " + *field + " is not compatible with its type. No facet will be calculated on this field. " ));

				indexesToErase.push_back(index);
				continue;
			}

			//
			index++;
		}
		if(indexesToErase.size() == facetQueryContainer->fields.size()){ // all facet fields are removed.
			;//FIXME : should we delete facet container here ?
		}
		// remove all facet fields which are not among nonSearchable attributes
		std::vector<srch2is::FacetType> types;
		std::vector<std::string> fields;
		std::vector<std::string> rangeStarts;
		std::vector<std::string> rangeEnds;
		std::vector<std::string> rangeGaps;
		for(int i=0;i<facetQueryContainer->types.size() ; i++){
			if(find(indexesToErase.begin(),indexesToErase.end(),i) == indexesToErase.end()){ // don't erase
				types.push_back(facetQueryContainer->types.at(i));
				fields.push_back(facetQueryContainer->fields.at(i));
				rangeStarts.push_back(facetQueryContainer->rangeStarts.at(i));
				rangeEnds.push_back(facetQueryContainer->rangeEnds.at(i));
				rangeGaps.push_back(facetQueryContainer->rangeGaps.at(i));
			}
		}
		facetQueryContainer->types = types;
		facetQueryContainer->fields = fields;
		facetQueryContainer->rangeStarts = rangeStarts;
		facetQueryContainer->rangeEnds = rangeEnds;
		facetQueryContainer->rangeGaps = rangeGaps;
	}

	return true;

}

bool QueryValidator::validateValueWithType(srch2is::FilterType type, string value){
	switch (type) {
		case srch2is::UNSIGNED:
			return isInteger(value);
			break;
		case srch2is::FLOAT:
			return isFloat(value);
			break;
		case srch2is::TEXT:
			return true; // TEXT does not have any criteria ?????
			break;
		case srch2is::TIME:
			return isTime(value);
			break;
	}
	// flow never reaches here
	return false;
}


}
}
