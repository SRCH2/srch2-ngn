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
        const CoreInfo_t &indexDataContainerConf,
        ParsedParameterContainer * paramContainer,
        const AttributeAccessControl & attrAcl) :
        schema(schema), indexDataContainerConf(indexDataContainerConf), attributeAcl(attrAcl){
    this->paramContainer = paramContainer;
}

// this function goes through the queryParameters and based on that validates the query.
bool QueryValidator::validate() {

	// validation case : If search type is RetrievByIdSearchType, then no need to continue any more validation.
	if (paramContainer->hasParameterInQuery(RetrieveByIdSearchType)) {
		return true;
	}

    // validation case : Only one of the search types should exist in queryParameters
    int numberOfProvidedSearchTypes = 0;
    if (paramContainer->hasParameterInQuery(TopKSearchType)) {
        numberOfProvidedSearchTypes++;
    }
    if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) {
        numberOfProvidedSearchTypes++;
    }
    if (numberOfProvidedSearchTypes != 1) { // search type is not clear , fatal error
        paramContainer->messages.push_back(
                std::make_pair(MessageError,
                        "Search type is not clear. Fatal error."));
        return false;
    }

    // validation case: if search type is TopK or GetAllResults, query keywords should not be empty.
    if(paramContainer->hasParameterInQuery(GeoSearchFlag) == false){
		if (paramContainer->hasParameterInQuery(TopKSearchType)
				|| paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // search type is either TopK or GetAllResults
			ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
			unsigned numberOfTerms = 0;
			while(termIterator.hasMore()){
				termIterator.getNext();
				numberOfTerms ++;
			}
			if (numberOfTerms == 0) {
				paramContainer->messages.push_back(
						std::make_pair(MessageError,
								"No keywords provided for search."));
				return false;
			}
		}
    }

    // TODO: in case of removing the geo index type from config file we should remove this part
    // validation case: if search type is Geo, index type must be Geo too
    if (paramContainer->hasParameterInQuery(GeoSearchFlag)) {
        if (indexDataContainerConf.getIndexType() != 1) { // One means geo index type
            paramContainer->messages.push_back(
                    std::make_pair(MessageError,
                            "Geo index type is needed for this query."));
            return false;
        }
    }

    // validate filter list
    // Example : q= title,name:foo AND body.abstract:bar
    // title, name, body and abstract should be declared as searchable attributes.
    if (!validateExistenceOfAttributesInFieldList()) {
        return false;
    }

    // validate boost list
    // Example : qf= title^19+name^123
    // title and name should be declared as searchable attributes.
    if (!validateExistenceOfAttributesInQueryFieldBoost()) {
        return false;
    }

    // validate sort filter
    // Example : sort=price,discount&orderby=asc
    // price and discount must be non-searchable attributes
    if (!validateExistenceOfAttributesInSortFiler()) {
        return false;
    }

    // validate facet filter
    // Example : facet=true&facet.field=model&facet.range=price&f.price.start=10
    // model and price must be non-searchable
    // f.price.start should be compatible with price type
    // price should not be of type text
    if (!validateExistenceOfAttributesInFacetFiler()) {
        return false;
    }

    // validate filter query
    // Example :
    // fq=price:[* TO 100] AND model:JEEP AND boolexp$price - discount < 100$
    // price, model and discount should be non-searchable attributes.
    if(!validateFilterQuery()){
        return false;
    }


    // validate the structure of the boolean tree
    // Example :
    // q= NOT john  ==> this query is not acceptable
    // q= john AND hello ==> this query is acceptable and correct
    if(! validateParseTreeBooleanStructure()){
    	return false;
    }

    return true;
}

bool QueryValidator::validateExistenceOfAttributesInQueryFieldBoost() {
    if (paramContainer->hasParameterInQuery(QueryFieldBoostFlag)) {
      // query field boost list is not empty

        const std::map<std::string, unsigned>& searchableAttributes =
                schema.getSearchableAttribute();
        std::vector<QueryFieldAttributeBoost> validatedBoostFields;

        for (std::vector<QueryFieldAttributeBoost>::iterator boostIter =
        		paramContainer->qfContainer->boosts.begin();
        		boostIter != paramContainer->qfContainer->boosts.end(); ++boostIter) {

        	bool validField = false;
        	if (paramContainer->attrAclOn) {
        		// if the attribute acl switch is ON then check attribute ACL for validity
        		// of this field.
        		validField = attributeAcl.isSearchableFieldAccessibleForRole(paramContainer->roleId,
        		        			boostIter->attribute);
        	} else {
        		// if the attribute acl switch is OFF then check whether the field is searchable
        		validField =  searchableAttributes.count(boostIter->attribute) > 0 ? true : false;
        	}

        	if (!validField){
        		// field does not exist in accessible searchable attributes
        		// write a warning and remove it
        		paramContainer->messages.push_back(
        				std::make_pair(MessageWarning,
        						"Field " + boostIter->attribute
        						+ " is not an accessible searchable field."
        						"It will be removed"));
        		continue;
        	}

        	validatedBoostFields.push_back(*boostIter);
        }
        paramContainer->qfContainer->boosts.swap(validatedBoostFields);
    }  
    return true;
}

bool QueryValidator::validateExistenceOfAttributesInFieldList() {
    if (paramContainer->hasParameterInQuery(FieldFilter)) { // field filter list is not empty

        const std::map<std::string, unsigned>& searchableAttributes =
                schema.getSearchableAttribute();
        ParseTreeNode * leafNode;
        ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
        while(termIterator.hasMore()){
        	leafNode = termIterator.getNext();
        	for (vector<string>::iterator field = leafNode->termIntermediateStructure->fieldFilter.begin();
        			field != leafNode->termIntermediateStructure->fieldFilter.end(); ++field) {
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

	if(paramContainer->hasParameterInQuery(SortQueryHandler) == false){
		return true; // no sort is available to validate
	}

    // first find if we have any sort filter in query
    SortQueryContainer * sortQueryContainer = paramContainer->sortQueryContainer;;

    if (sortQueryContainer == NULL) { // we should never go into this if since sortContainer shouldn't be NULL
    	ASSERT(false);
        return false;
    }

    // now validate the fields , if one field is not found in attributes sort will be canceled.

    bool sortFilterShouldBeRemoved = false;
    for (std::vector<std::string>::iterator field =
            sortQueryContainer->evaluator->field.begin();
            field != sortQueryContainer->evaluator->field.end(); ++field) {

    	bool validField = false;
    	if (paramContainer->attrAclOn) {
    		// if the attribute acl switch is ON then check attribute ACL for validity
    		// of this field.
    		validField = attributeAcl.isRefiningFieldAccessibleForRole(paramContainer->roleId,
    				*field);
    	} else {
    		// if the attribute acl switch is OFF then check whether the field is refining
    		validField =  schema.getRefiningAttributes()->count(*field) > 0 ? true : false;
    	}

        if (!validField) {
        	// field does not exist in accessible refining list.
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Field " + *field
                                    + " is not an accessible refining field. No sort will be performed. "));

            sortFilterShouldBeRemoved = true;
            break;
        }
    }
    if (sortFilterShouldBeRemoved) {
        // Warning : Sort will be canceled.
        // The following if-else statement removes the flag of sort filter from the corresponding queryParameters
        paramContainer->parametersInQuery.erase(
                remove(
                        paramContainer->parametersInQuery.begin(),
                        paramContainer->parametersInQuery.end(),
                        SortQueryHandler),
                paramContainer->parametersInQuery.end());
        // container and its validator must be freed here
        delete sortQueryContainer->evaluator;
        delete sortQueryContainer;
		paramContainer->sortQueryContainer = NULL;
    }

    return true;

}

bool QueryValidator::validateExistenceOfAttributesInFacetFiler() {

	if(paramContainer->hasParameterInQuery(FacetQueryHandler) == false){
		return true; // no facet available to validate
	}
    FacetQueryContainer * facetQueryContainer = paramContainer->facetQueryContainer;

    if (facetQueryContainer == NULL) { // it's just for double check, we should never go into this if
    	// there is no facet query to validate but FacetQueryHandler is in parameters vector
    	ASSERT(false);
        return false;
    }

    int facetParallelVectorsIndex = -1;
    vector<int> facetParallelVectorsIndexesToErase;
    for (std::vector<std::string>::iterator field =
            facetQueryContainer->fields.begin();
            field != facetQueryContainer->fields.end(); ++field) {
        facetParallelVectorsIndex++;

        //1. Validate the existence of attributes and also
        //   check whether the attribute is accessible for current role.
    	bool validField = false;
    	if (paramContainer->attrAclOn) {
    		// if the attribute acl switch is ON then check attribute ACL for validity
    		// of this field.
    		validField = attributeAcl.isRefiningFieldAccessibleForRole(paramContainer->roleId,
    				*field);
    	} else {
    		// if the attribute acl switch is OFF then check whether the field is refining
    		validField =  schema.getRefiningAttributes()->count(*field) > 0 ? true : false;
    	}

        if (!validField) {
            //Facet will be not be calculated for this field.
        	paramContainer->messages.push_back(
        			std::make_pair(MessageWarning,
        					"Field " + *field
        					+ " is unaccessible refining field. No facet will be calculated on this field. "));

        	facetParallelVectorsIndexesToErase.push_back(
        			facetParallelVectorsIndex);
        	continue; // no need to do anymore validation for this field because it'll be removed from facets.
        }

        //2. Range facets should be of type unsigned or float or date
        FilterType fieldType = schema.getTypeOfRefiningAttribute(
                schema.getRefiningAttributeId(*field));

        if (   !facetQueryContainer->types.empty()
            && facetQueryContainer->types.at(facetParallelVectorsIndex) == srch2is::FacetTypeRange
            && !(
                    fieldType == srch2is::ATTRIBUTE_TYPE_INT ||fieldType == srch2is::ATTRIBUTE_TYPE_LONG ||
                    fieldType == srch2is::ATTRIBUTE_TYPE_FLOAT ||fieldType == srch2is::ATTRIBUTE_TYPE_DOUBLE||
                    fieldType == srch2is::ATTRIBUTE_TYPE_TIME
             )) {
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Field " + *field
                                    + " is not a proper field for range facet. No facet will be calculated on this field. "));

            facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
            continue;
        }

        // should fill the UNSPECIFIED facet types
        if(!facetQueryContainer->types.empty() &&
                facetQueryContainer->types.at(facetParallelVectorsIndex) == srch2is::FacetTypeNonSpecified){
            if(std::find(this->indexDataContainerConf.getFacetAttributes()->begin() ,
                    this->indexDataContainerConf.getFacetAttributes()->end() ,
                    *field) == this->indexDataContainerConf.getFacetAttributes()->end()){
                paramContainer->messages.push_back(
                        std::make_pair(MessageWarning,
                                "Facet type for field " + *field
                                        + " is not given. No facet will be calculated on this field. "));
                facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
                continue;
            }
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
        }else{
            if(std::find(this->indexDataContainerConf.getFacetAttributes()->begin() ,
                    this->indexDataContainerConf.getFacetAttributes()->end() ,
                    *field) == this->indexDataContainerConf.getFacetAttributes()->end()){
                paramContainer->messages.push_back(
                        std::make_pair(MessageWarning,
                                "Start value for field " + *field
                                        + " is not given. No facet will be calculated on this field. "));
                facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
                continue;
            }
        }
        if (valid && !facetQueryContainer->rangeEnds.empty()
                && facetQueryContainer->rangeEnds.at(facetParallelVectorsIndex).compare(
                        "") != 0) {
            valid = validateValueWithType(fieldType,
                    facetQueryContainer->rangeEnds.at(
                            facetParallelVectorsIndex));
        }else{
            if(std::find(this->indexDataContainerConf.getFacetAttributes()->begin() ,
                    this->indexDataContainerConf.getFacetAttributes()->end() ,
                    *field) == this->indexDataContainerConf.getFacetAttributes()->end()){
                paramContainer->messages.push_back(
                        std::make_pair(MessageWarning,
                                "End value for field " + *field
                                        + " is not given. (or values are not compatible with types.) No facet will be calculated on this field. "));
                facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
                continue;
            }
        }

        if (valid && !facetQueryContainer->rangeGaps.empty()
                && facetQueryContainer->rangeGaps.at(facetParallelVectorsIndex).compare(
                        "") != 0) {
        	if(fieldType == srch2is::ATTRIBUTE_TYPE_TIME){
        		valid = srch2is::DateAndTimeHandler::verifyDateTimeString
        				(facetQueryContainer->rangeGaps.at(facetParallelVectorsIndex) , srch2is::DateTimeTypeDurationOfTime);
        	}else{
				valid = validateValueWithType(fieldType,
						facetQueryContainer->rangeGaps.at(facetParallelVectorsIndex));
        	}
        }else{
            if(std::find(this->indexDataContainerConf.getFacetAttributes()->begin() ,
                    this->indexDataContainerConf.getFacetAttributes()->end() ,
                    *field) == this->indexDataContainerConf.getFacetAttributes()->end()){
                paramContainer->messages.push_back(
                        std::make_pair(MessageWarning,
                                "Gap value for field " + *field
                                        + " is not given. (or values are not compatible with types.) No facet will be calculated on this field. "));
                facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
                continue;
            }
        }
        if (!valid) { // start,end or gap value is not compatible with attribute type
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Start,End or Gap value for field " + *field
                                    + " is not compatible with its type. No facet will be calculated on this field. "));

            facetParallelVectorsIndexesToErase.push_back(facetParallelVectorsIndex);
            continue;
        }

    }
    if (facetQueryContainer->fields.size() != 0 &&
            facetParallelVectorsIndexesToErase.size() == facetQueryContainer->fields.size()) {
        // all facet fields are removed, so there is no facet query anymore
        // facet must be removed from queryParameters
        paramContainer->parametersInQuery.erase(
                remove(
                        paramContainer->parametersInQuery.begin(),
                        paramContainer->parametersInQuery.end(),
                        FacetQueryHandler),
                paramContainer->parametersInQuery.end());
        // facet container should be freed here
        delete facetQueryContainer;
        paramContainer->facetQueryContainer = NULL;
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

bool QueryValidator::validateFilterQuery(){
    if(paramContainer->hasParameterInQuery(FilterQueryEvaluatorFlag)){
        FilterQueryContainer * filterQueryContainer = paramContainer->filterQueryContainer;
        if (! filterQueryContainer->evaluator->validate(schema, paramContainer->roleId,
        		attributeAcl, paramContainer->attrAclOn)){
            paramContainer->parametersInQuery.erase(
                    remove(
                            paramContainer->parametersInQuery.begin(),
                            paramContainer->parametersInQuery.end(),
                            FilterQueryEvaluatorFlag),
                            paramContainer->parametersInQuery.end());
            delete filterQueryContainer->evaluator;
            delete filterQueryContainer;
            paramContainer->filterQueryContainer = NULL;

            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "Filter query is not valid. No filter will be applied on the results."));
        }
    }
    return true;
}

bool QueryValidator::validateParseTreeBooleanStructure(){

	// validation case : if we traverse the parse tree, there must at least one
	// --- AND or OR in the path to each NOT. Because cannot handle pure NOT operators
	// --- and there must be another operator so that NOT prunes the results of that operator.
	// --- This means that the root of the tree cannot be NOT
	if(! validateParseTreeStructureWithRegardToComputability() ){
        paramContainer->messages.push_back(
                std::make_pair(MessageError,
                        "Query boolean structure is not supported."));
		return false;
	}


	return true;
}

bool QueryValidator::validateParseTreeStructureWithRegardToComputability(){
	ParseTreeNode * root = paramContainer->parseTreeRoot;
	if(root == NULL){ // if the tree is empty there is no problem with NOT so we return true
		return true;
	}

	return isParseSubtreeComputableRecursive(root);

}

// check recursively if root is computable.
// Rules are:
// NOT is not computable
// AND is computable if at least one child is computable
// OR is computable only if all children are computable
bool QueryValidator::isParseSubtreeComputableRecursive(ParseTreeNode * node){
	switch (node->type) {
		case LogicalPlanNodeTypeTerm:
			return true;
		case LogicalPlanNodeTypeNot:
			return false;
		case LogicalPlanNodeTypeAnd:
			for(unsigned childOffset = 0 ; childOffset < node->children.size() ; ++childOffset){
				if(isParseSubtreeComputableRecursive(node->children.at(childOffset))){
					return true;
				}
			}
			return false;
		case LogicalPlanNodeTypeOr:
			for(unsigned childOffset = 0 ; childOffset < node->children.size() ; ++childOffset){
				if(isParseSubtreeComputableRecursive(node->children.at(childOffset)) == false){
					return false;
				}
			}
			return true;
			break;

                 default:
			break;
	}
	return true;
}


}
}
