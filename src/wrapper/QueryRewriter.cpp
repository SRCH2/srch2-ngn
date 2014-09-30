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
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/ResultsPostProcessor.h>
#include "postprocessing/PhraseSearchFilter.h"
#include "ConfigManager.h"

#include "util/DateAndTimeHandler.h"
#include "util/Assert.h"
#include "TreeOperations.h"
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>


using std::string;
using std::vector;
using srch2is::Query;

namespace srch2is = srch2::instantsearch;

namespace srch2 {

namespace httpwrapper {

QueryRewriter::QueryRewriter(const CoreInfo_t *config,
        const Schema & schema, const Analyzer & analyzer,
        ParsedParameterContainer * paramContainer,
        const AttributeAccessControl & attrAcl) :
        schema(schema), analyzer(analyzer), attributeAcl(attrAcl) {
    this->paramContainer = paramContainer;
    indexDataConfig = config;
}

bool QueryRewriter::rewrite(LogicalPlan & logicalPlan) {

	// If search type is RetrievByIdSearchType, no need to continue rewriting.
	if(this->paramContainer->hasParameterInQuery(RetrieveByIdSearchType)){
		logicalPlan.setQueryType(srch2is::SearchTypeRetrieveById);
		logicalPlan.setDocIdForRetrieveByIdSearchType(this->paramContainer->docIdForRetrieveByIdSearchType);
		return true;
	}

	//If query contains list of attributes to return, we set the vector<string> attributesToReturn vector inside LogicalPlan.
	if(this->paramContainer->responseAttributesList.size() != 0){
	    logicalPlan.setAttrToReturn(this->paramContainer->responseAttributesList);
	}
    // go through the queryParameters and call the analyzer on the query if needed.

    /*
     * Example:
     * for example a user can provide a query like : foo~0.5 AND bar
     * for this query:
     * rawQueryKeyword vector is <foo , bar>
     * SimilarityThreshold vector is <0.5,-1> // should rewrite -1 to what's comming from configuration file
     * boostLevel vector is <> , no rewrite, planGen understands empty vector
     * prefixComplete vector <>
     */
    // make sure keyword parallel vectors have enough information, if not write information from config file
    prepareKeywordInfo();

    // prepare field filter bits
    // Field filter should be changed from field names to field bit filter
    // Example: q= title,name:foo AND body.abstract:bar
    // title,name means title OR name and must be translated to a 32 bit representation
    // body.abstract means body AND abstract
    prepareFieldFilters();

    // rewrite facet query:
    // 1. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file
    //    if it's not even in configuration file remove that field from facets and save error message.
    // 2. If facet is disabled by configuration facet information should be removed
    prepareFacetFilterInfo();

    // apply the analyzer on the query, ]
    // The following code should e temporary.
    if(applyAnalyzer() == false){
    	return false;
    }


	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	/*
	 * This function must be rewritten. In addition to keyword rewritings,
	 * we must also validate the tree structure of LogicalPlan.
	 * QueryPlanGen must be removed and merged with this class.
	 * In here, we should
	 * 1. Apply rewrite rules on term info in the leaf nodes of the tree
	 * 2. Using the code which comes from query plan gen, we should attach Term objects to the LogicalPlanTree
	 * 3. Apply rewrite rules to re-organize the tree.
	 */
    prepareLogicalPlan(logicalPlan);
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////

    return true;
}

void QueryRewriter::prepareKeywordInfo() {

	ParseTreeNode * leafNode;
	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();

        if (paramContainer->hasParameterInQuery(KeywordBoostLevel)
                && leafNode->termIntermediateStructure->keywordBoostLevel < 0) { // -1 is the place holder for numerical parameters ...
        	leafNode->termIntermediateStructure->keywordBoostLevel =
                    indexDataConfig->getQueryTermBoost();
        }

        if (paramContainer->hasParameterInQuery(KeywordSimilarityThreshold)
                && leafNode->termIntermediateStructure->keywordSimilarityThreshold < 0) {
        	leafNode->termIntermediateStructure->keywordSimilarityThreshold =
                    indexDataConfig->getQueryTermSimilarityThreshold();
        }

        if (paramContainer->hasParameterInQuery(QueryPrefixCompleteFlag)
                && leafNode->termIntermediateStructure->keywordPrefixComplete
                        == srch2is::TERM_TYPE_NOT_SPECIFIED) {
        	leafNode->termIntermediateStructure->keywordPrefixComplete =
                    indexDataConfig->getQueryTermPrefixType() ?
                            srch2is::TERM_TYPE_COMPLETE :
                            srch2is::TERM_TYPE_PREFIX;
            // true means complete
        }

        if (paramContainer->hasParameterInQuery(FieldFilter)
                && leafNode->termIntermediateStructure->fieldFilterOp
                        == srch2is::OP_NOT_SPECIFIED) {
        	leafNode->termIntermediateStructure->fieldFilterOp = srch2is::BooleanOperatorOR; // default field filter operation is OR
            // TODO : get it from configuration file
        }
	}
}

bool QueryRewriter::applyAnalyzer() {
    Analyzer & analyzerNotConst = const_cast<Analyzer &>(analyzer); // because of bad design on analyzer
    vector<ParseTreeNode *> keywordPointersToErase; // stop word indexes, to be removed later
    // first apply the analyzer
	ParseTreeNode * leafNode;
	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();

		// Currently we only get the first token coming out of analyzer chain for each
		// keyword. In future we should handle synonyms TODO
		string keywordAfterAnalyzer = "";
		if (leafNode->termIntermediateStructure->isPhraseKeywordFlag){
			PhraseInfo pi;
			std::vector<AnalyzedTermInfo> analyzedQueryKeywords;
			analyzerNotConst.tokenizeQuery(leafNode->termIntermediateStructure->rawQueryKeyword, analyzedQueryKeywords);
			keywordAfterAnalyzer.clear();
			for (int i=0; i < analyzedQueryKeywords.size(); ++i){
				if (i)
					keywordAfterAnalyzer.append(" ");
				keywordAfterAnalyzer.append(analyzedQueryKeywords[i].term);
				pi.phraseKeyWords.push_back(analyzedQueryKeywords[i].term);
				pi.phraseKeywordPositionIndex.push_back(analyzedQueryKeywords[i].position);
			}
			pi.proximitySlop = leafNode->termIntermediateStructure->phraseSlop;
			pi.attributeIdsList = leafNode->termIntermediateStructure->fieldFilterList;
			if (analyzedQueryKeywords.size() > 0) {
				paramContainer->PhraseKeyWordsInfoMap[keywordAfterAnalyzer] = pi;
			}

		}else{
			TermType termType = leafNode->termIntermediateStructure->keywordPrefixComplete;
			keywordAfterAnalyzer = analyzerNotConst.applyFilters(leafNode->termIntermediateStructure->rawQueryKeyword , termType == srch2is::TERM_TYPE_PREFIX);
		}
		if (keywordAfterAnalyzer.compare("") == 0) { // analyzer removed this keyword, it's assumed to be a stop word
			keywordPointersToErase.push_back(leafNode);
		} else { // just apply the analyzer
			leafNode->termIntermediateStructure->rawQueryKeyword = keywordAfterAnalyzer;
		}

	}

	// now erase the data of erased keywords
    for(vector<ParseTreeNode *>::iterator keywordToErase = keywordPointersToErase.begin() ; keywordToErase != keywordPointersToErase.end() ; ++keywordToErase){
    	TreeOperations::removeFromTree(*keywordToErase , paramContainer->parseTreeRoot);
    }
    // TODO After removing stop words the parse tree should be validated again

    termIterator.init(paramContainer->parseTreeRoot);
    unsigned numberOfKeywords = 0;
	while(termIterator.hasMore()){
		termIterator.getNext();
		numberOfKeywords ++;
	}
	// If query has geo parameters then it can be without any terms. but if it doesn't have geo parameters it should have at least one term for search.
	if(paramContainer->hasParameterInQuery(GeoSearchFlag) == false && numberOfKeywords == 0){
        if(paramContainer->hasParameterInQuery(TopKSearchType) || paramContainer->hasParameterInQuery(GetAllResultsSearchType)){
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "After ignoring stop words no keyword is left to search."));
            return false;
        }
    }
    return true;

}

// this function creates the bit sequence needed for field filter based on the filter names
void QueryRewriter::prepareFieldFilters() {


    if(!indexDataConfig->getSupportAttributeBasedSearch()){
    	ParseTreeNode * leafNode;
    	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
    		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
    	}
        return;
    }
    vector<unsigned> *allowedAttributesForRole = NULL;
    if (paramContainer->attrAclOn) {
    	boost::shared_ptr<vector<unsigned> > allowedAttributesSharedPtr;
    	// Fetch list of attributes accessible by this role-id
    	attributeAcl.fetchSearchableAttrsAcl(paramContainer->roleId, allowedAttributesSharedPtr);
    	allowedAttributesForRole = allowedAttributesSharedPtr.get();
    }

	const vector<unsigned>& schemaNonAclAttrsList = schema.getNonAclSearchableAttrIdsList();

    if(paramContainer->hasParameterInQuery(FieldFilter ) ){
        // some filters are provided in query so these two vectors are the same size as keywords vector

    	ParseTreeNode * leafNode;
    	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();

            srch2is::BooleanOperation op = leafNode->termIntermediateStructure->fieldFilterOp;

            if (leafNode->termIntermediateStructure->fieldFilter.size() == 0) {
            	if (paramContainer->attrAclOn) {
            		// filtering attributes are not specified for this term. Get them from ACL map
            		getFieldFiltersBasedOnAcl(leafNode->termIntermediateStructure->fieldFilterList,
            				leafNode->termIntermediateStructure->fieldFilterAttrOperation,
            				allowedAttributesForRole);
            	} else {
            		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
            	}
            } else {
            	vector<unsigned> attributeFilter;
            	// flag is used to indicate whether the wildcard "*" was found in attributes list in
            	// attribute based search.
                bool hasWildCard = false;
                for (std::vector<std::string>::iterator field = leafNode->termIntermediateStructure->fieldFilter.begin();
                        field != leafNode->termIntermediateStructure->fieldFilter.end(); ++field) {

                    if (field->compare("*") == 0) { // all fields
                    	attributeFilter.clear();
                    	hasWildCard = true;
                        break;
                    }
                    unsigned id = schema.getSearchableAttributeId(*field);
                    // if a user has specified a filtering attribute, then first check whether the
                    // attribute acl is OFF or ON. If attribute acl is OFF then add this field to
                    // attribute filter vector. If attribute acl is ON the check whether it is present
                    // in allowed attributes list for this role-id OR it is present in non-acl attributes
                    // list. If found then use this attribute for filtering. If not found then this
                    // attribute is unaccessible for current search and should not be used for filtering
                    // Note: The unaccessible attributes are just ignored without considering the
                    //  filtering operation (AND or OR). Another alternative option could be to return
                    // no result for when one the filtering attribute is unaccessible and filtering
                    // operation is (AND). This option is not chosen.
                    if ( !paramContainer->attrAclOn ||
                    	(allowedAttributesForRole &&
                    	find(allowedAttributesForRole->begin(), allowedAttributesForRole->end(), id)
                    		!= allowedAttributesForRole->end()) ||
                    	find (schemaNonAclAttrsList.begin(), schemaNonAclAttrsList.end(), id)
                    		!= schemaNonAclAttrsList.end())  {
                    	attributeFilter.push_back(id);
                    }
                }
                if (!hasWildCard) {

                	if (attributeFilter.size() == 0) {
                		// if attributeFilter size is 0 , it indicates that all the attributes specified
                		// in the attribute based search were NOT accessible by a given acl role-id. We
                		// set the operation as NAND to indicate that the term should not match in any of
                		// the attributes of a record. See ForwardIndex.cpp isValidTermHits().
                		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_NAND;
                	} else if (op == srch2is::BooleanOperatorAND) {
                		// if attributeFilter size is > 0. Then use user specified operands AND or OR
                		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_AND;
                	} else {
                		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
                	}
                	leafNode->termIntermediateStructure->fieldFilterList = attributeFilter;
                } else {
                	// if wildcard is used then get filtering attributes from ACL map
                	if (paramContainer->attrAclOn) {
                		getFieldFiltersBasedOnAcl(leafNode->termIntermediateStructure->fieldFilterList,
                				leafNode->termIntermediateStructure->fieldFilterAttrOperation,
                				allowedAttributesForRole);
                	}else {
                		leafNode->termIntermediateStructure->fieldFilterList.clear();
                		leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
                	}
                }
            }
    	}

    }else{
    	// filtering attributes are not specified for any term. Get them from ACL map
    	ParseTreeNode * leafNode;
    	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
    		if (paramContainer->attrAclOn) {
    			getFieldFiltersBasedOnAcl(leafNode->termIntermediateStructure->fieldFilterList,
    					leafNode->termIntermediateStructure->fieldFilterAttrOperation, allowedAttributesForRole);
    		} else {
    			leafNode->termIntermediateStructure->fieldFilterList.clear();
    			leafNode->termIntermediateStructure->fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
    		}
    	}
    }

}

/*
 *   This function decides the filter attributes for the current role-id and operation
 *   to be performed among those attributes (AND, OR , NAND). This API is called when a user has
 *   not specified any attributes in a query. First get three sets
 *
 *   set1 : set of non-acl attributes ( accessible by all)
 *   set2 : set of acl attributes (access only to specific role-id based on acl definition)
 *   set3 : set of attributes allowed for this role-id.
 *
 *   allowed attributes set (A) : set1 union set3
 *   not-allowed attributes (NA) set: set2 - set3
 *
 *   if  len(A) > len(NA) then filter attributes are (A) with OR operation
 *   (i.e. keyword should match in any one attribute)
 *
 *   otherwise filter attributes are (NA) with NAND operation.(i.e. keyword should NOT be present
 *   in only these attribute)
 *
 *   The decision to use smaller length set is for optimization so that forward index has to scan
 *   smaller filter list.
 *
 *   e.g
 *   sample schema in config file.
 *   <fields name = f1 .....acl=true >
 *   <fields name = f2 .....acl=true >
 *   <fields name = f3 .....acl=false >
 *   <fields name = f4 .....acl=false >
 *
 *   set1 (non-acl attributes) = [f3 , f4]
 *   set2 (acl attributes) = [f1 , f2]
 *   set3 = [ f2, f3 ]  for role-id = 100  (fetched from attributes ACL map)
 *
 *   allowed attributes set (A) : set1 union set3 : [ f2 f3 f4 ]
 *   not-allowed attributes (NA) set: set2 - set3 : [ f1 ]
 *
 *   and len(NA) = 1  < len(A) = 3
 *
 *   so the filtering attributes for forward index is (NA) i.e. [f1]
 *   and the filtering attributes operation is NAND. i.e. do NOT match in [f1]
 *
 *   If the record's term hit is only on [f1] attribute then the record will not be returned.
 *   See ForwardIndex.cpp : isValidRecordTermHit() for more detail.
 */
void QueryRewriter::getFieldFiltersBasedOnAcl(vector<unsigned>& parseNodeFieldFilter,
		ATTRIBUTES_OP& parseNodeFieldFilterOperaton, vector<unsigned> *allowedAttributesForRole) {

	const vector<unsigned>& schemaAclAttrsList = schema.getAclSearchableAttrIdsList();
	const vector<unsigned>& schemaNonAclAttrsList = schema.getNonAclSearchableAttrIdsList();

	vector<unsigned>  accessibleAttrsList;
	vector<unsigned>  unAccessibleAttrsList;
	if (allowedAttributesForRole != NULL && allowedAttributesForRole->size() > 0) {

		accessibleAttrsList.reserve(schemaNonAclAttrsList.size() + allowedAttributesForRole->size());
		set_union(schemaNonAclAttrsList.begin(), schemaNonAclAttrsList.end(),
				allowedAttributesForRole->begin(), allowedAttributesForRole->end(),
				back_inserter(accessibleAttrsList));

		unAccessibleAttrsList.reserve(schemaAclAttrsList.size());
		set_difference(schemaAclAttrsList.begin(), schemaAclAttrsList.end(),
				allowedAttributesForRole->begin(), allowedAttributesForRole->end(),
				back_inserter(unAccessibleAttrsList));
	} else {
		// when no role is specified or role is not found in acl map then
		// attributes marked as non-acl (public) are accessible
		// and attributes marked as acl (private) are not accessible.
		accessibleAttrsList = schemaNonAclAttrsList;
		unAccessibleAttrsList = schemaAclAttrsList;
	}

	if (accessibleAttrsList.size() == 0) {
		// if all the fields are acl controlled. Then set parseNodeFieldFilter empty
		// and set operation as NAND to indicate that the term should not be found in any of the
		// attributes of the record.
		parseNodeFieldFilter.clear();
		parseNodeFieldFilterOperaton = ATTRIBUTES_OP_NAND;
		return;
	}
	if (unAccessibleAttrsList.size() == 0) {
		// if all the fields are NOT acl controlled. Then set parseNodeFieldFilter empty
		// and set operation as OR to indicate that the term can be found in any of the
		// attributes of the record.
		parseNodeFieldFilter.clear();
		parseNodeFieldFilterOperaton = ATTRIBUTES_OP_OR;
		return;
	}

	// both accessible and unaccessible list cannot be 0 because there are always non-zero fields
	// in schema.
	ASSERT(accessibleAttrsList.size() != 0 && unAccessibleAttrsList.size() != 0);

	if (accessibleAttrsList.size() > unAccessibleAttrsList.size()) {
		parseNodeFieldFilter.assign(
				unAccessibleAttrsList.begin(), unAccessibleAttrsList.end());
		parseNodeFieldFilterOperaton = ATTRIBUTES_OP_NAND;
	} else {
		parseNodeFieldFilter.assign(
				accessibleAttrsList.begin(), accessibleAttrsList.end());
		parseNodeFieldFilterOperaton = ATTRIBUTES_OP_OR;
	}

}

// rewrite facet query:
// 1. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file.
//    if it's not even in configuration file remove that field from facets and save error message.
// 2. If facet is disabled by configuration, facet information should be removed.
void QueryRewriter::prepareFacetFilterInfo() {

	if(paramContainer->hasParameterInQuery(FacetQueryHandler) == false){
		return; // no facet available to validate
	}
    FacetQueryContainer * facetQueryContainer = paramContainer->facetQueryContainer;

    if (facetQueryContainer == NULL) { // it's just for double check, we should never go into this if
    	// there is no facet query to validate but FacetQueryHandler is in parameters vector
    	ASSERT(false);
        return;
    }

    // there is a facet filter, continue with rewriting ...

    // 1. Remove everything if facet is disabled in config file.
    if (!indexDataConfig->isFacetEnabled()) {

        // remove facet flag from queryParameters
		paramContainer->parametersInQuery.erase(
				remove(
						paramContainer->parametersInQuery.begin(),
						paramContainer->parametersInQuery.end(),
						FacetQueryHandler),
				paramContainer->parametersInQuery.end());

        // free facet container
        delete facetQueryContainer;
		paramContainer->facetQueryContainer = NULL;

        return;
    }
    // type should also be rewritten in case it's not given by the user.
    if(facetQueryContainer->types.empty()){
        for (std::vector<std::string>::iterator fieldName =
                facetQueryContainer->fields.begin();
                fieldName != facetQueryContainer->fields.end() ; ++fieldName){

            vector<string>::const_iterator facetIteratorInConfVector = find(
                    indexDataConfig->getFacetAttributes()->begin(),
                    indexDataConfig->getFacetAttributes()->end(),
                    *fieldName);
            int fieldFacetTypeInt =
                    indexDataConfig->getFacetTypes()->at(
                            std::distance(indexDataConfig->getFacetAttributes()->begin() , facetIteratorInConfVector));
            // this field must be removed from facet fields.
            if(fieldFacetTypeInt == 0){
                facetQueryContainer->types.push_back(srch2is::FacetTypeCategorical);
            }else if (fieldFacetTypeInt == 1){
                facetQueryContainer->types.push_back(srch2is::FacetTypeRange);
            }
        }
    }else{ // if the "types" vector is not empty, it's assumed to be equal size with fields (from parser)
        for (std::vector<srch2is::FacetType>::iterator facetType =
                facetQueryContainer->types.begin();
                facetType != facetQueryContainer->types.end(); ++facetType) {
            if(*facetType == srch2is::FacetTypeNonSpecified){
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataConfig->getFacetAttributes()->begin(),
                        indexDataConfig->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(std::distance(facetType ,facetQueryContainer->types.begin() )));
                int fieldFacetTypeInt =
                        indexDataConfig->getFacetTypes()->at(
                                std::distance(indexDataConfig->getFacetAttributes()->begin() , facetIteratorInConfVector));
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
            FilterType fieldType = schema.getTypeOfRefiningAttribute(
                    schema.getRefiningAttributeId(facetQueryContainer->fields.at(facetFieldIndex)));
            if (facetQueryContainer->rangeStarts.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataConfig->getFacetAttributes()->begin(),
                        indexDataConfig->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataConfig->getFacetAttributes()->end()) { // this attribute is in config
                    string startFromConfig =
                            indexDataConfig->getFacetStarts()->at(
                                    facetIteratorInConfVector
                                            - indexDataConfig->getFacetAttributes()->begin());
                    facetQueryContainer->rangeStarts.at(facetFieldIndex) = startFromConfig;
                }
            }else if(fieldType == srch2is::ATTRIBUTE_TYPE_TIME){
            	// here we should use DateAndTimeHandler class to conver start to long representation
            	// we assume it's a good syntax because everything is checked in query validator
            	std::stringstream buffer;
            	buffer << srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(facetQueryContainer->rangeStarts.at(facetFieldIndex));
            	facetQueryContainer->rangeStarts.at(facetFieldIndex) = buffer.str() ;

            }

            if (facetQueryContainer->rangeEnds.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataConfig->getFacetAttributes()->begin(),
                        indexDataConfig->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataConfig->getFacetAttributes()->end()) { // this attribute is in config
                    string endFromConfig =
                            indexDataConfig->getFacetEnds()->at(
                                    facetIteratorInConfVector
                                            - indexDataConfig->getFacetAttributes()->begin());
                    facetQueryContainer->rangeEnds.at(facetFieldIndex) = endFromConfig;
                }
            }else if(fieldType == srch2is::ATTRIBUTE_TYPE_TIME){
            	// here we should use DateAndTimeHandler class to conver start to long representation
            	// we assume it's a good syntax because everything is checked in query validator
            	std::stringstream buffer;
            	buffer << srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(facetQueryContainer->rangeEnds.at(facetFieldIndex));
            	facetQueryContainer->rangeEnds.at(facetFieldIndex) = buffer.str() ;
            }

            if (facetQueryContainer->rangeGaps.at(facetFieldIndex).compare("") == 0) {
                // should get the value from config
                vector<string>::const_iterator facetIteratorInConfVector = find(
                        indexDataConfig->getFacetAttributes()->begin(),
                        indexDataConfig->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataConfig->getFacetAttributes()->end()) { // this attribute is in config
                    string gapFromConfig =
                            indexDataConfig->getFacetGaps()->at(
                                    facetIteratorInConfVector
                                            - indexDataConfig->getFacetAttributes()->begin());
                    facetQueryContainer->rangeGaps.at(facetFieldIndex) = gapFromConfig;
                }
            }//else{
            	// we don't change gap, gap is translated when it's going to be used.
            //}
        }

        //
        facetFieldIndex++;
    }

}

void QueryRewriter::prepareLogicalPlan(LogicalPlan & plan){
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	/*
	 * Basic policies like pushing down AND or copying down field filters are done here.
	 */
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////

    // create query objects
	/*
	 * Exact and fuzzy query should not be created here. The reason we have this code here is
	 * that some old parts of code like GEO are still working and we need to feed them from
	 * wrapper layer by these query objects.
	 */
	// if search type is RetrieveByIdSearchType, only docid must be set in QueryPlan, no other information is needed in QueryPlan
	if(paramContainer->hasParameterInQuery(RetrieveByIdSearchType)){
		plan.setDocIdForRetrieveByIdSearchType(paramContainer->docIdForRetrieveByIdSearchType);
		plan.setQueryType(srch2is::SearchTypeRetrieveById);
		return;
	}
    createExactAndFuzzyQueries(plan);
    // generate post processing plan
    createPostProcessingPlan(plan);

	/*
	 * 0. Add Geo node to the parseTree if it has GeoSearchFlag
	 */
	if(paramContainer->hasParameterInQuery(GeoSearchFlag)){
		addGeoToParseTree();
	}

	/*
	 * 1. rewrite the parseTree
	 */
	rewriteParseTree();

	/*
	 * 2. Build the logicalPlan tree from the parse tree
	 */
	buildLogicalPlan(plan);

}


void QueryRewriter::rewriteParseTree(){
	// rule 1 : merge similar levels in the tree
	paramContainer->parseTreeRoot = TreeOperations::mergeSameOperatorLevels(paramContainer->parseTreeRoot);
}

// This function add the a new node to the parse tree for geo search
// so it add a new and node and make it the root and the previous root of the parse tree and the new geo node become its chilren
//
//                  |---- The previous root of parse tree
//    NewAndNode ---|
//                  |---- New Geo Node
void QueryRewriter::addGeoToParseTree(){
	ParseTreeNode* newGeoNode;
	if(paramContainer->parseTreeRoot != NULL){
		ParseTreeNode* newAndNode = new ParseTreeNode(LogicalPlanNodeTypeAnd, NULL);
		newAndNode->children.push_back(paramContainer->parseTreeRoot);
		paramContainer->parseTreeRoot->parent = newAndNode;
		paramContainer->parseTreeRoot = newAndNode;
		newGeoNode = new ParseTreeNode(LogicalPlanNodeTypeGeo, newAndNode);
		newAndNode->children.push_back(newGeoNode);
		//newGeoNode->geoIntermediateStructure
	}else{ // if the root of parse tree is null, the new geo node become the new root
		newGeoNode = new ParseTreeNode(LogicalPlanNodeTypeGeo, NULL);
		paramContainer->parseTreeRoot = newGeoNode;
	}
	// now we should fill the value of new geo node's variables
	if(paramContainer->geoParameterContainer->hasParameterInQuery(GeoTypeRectangular)){
		newGeoNode->geoIntermediateStructure = new GeoIntermediateStructure(
				paramContainer->geoParameterContainer->leftBottomLatitude,
				paramContainer->geoParameterContainer->leftBottomLongitude,
				paramContainer->geoParameterContainer->rightTopLatitude,
				paramContainer->geoParameterContainer->rightTopLongitude);
	}else if(paramContainer->geoParameterContainer->hasParameterInQuery(GeoTypeCircular)){
		newGeoNode->geoIntermediateStructure = new GeoIntermediateStructure(
				paramContainer->geoParameterContainer->centerLatitude,
				paramContainer->geoParameterContainer->centerLongitude,
				paramContainer->geoParameterContainer->radius);
	}

}

void QueryRewriter::buildLogicalPlan(LogicalPlan & logicalPlan){
	// traverse on the parseTree and build the logical plan tree
	if(paramContainer->parseTreeRoot == NULL){
		return;
	}
	logicalPlan.setTree(buildLogicalPlan(paramContainer->parseTreeRoot , logicalPlan));
}

LogicalPlanNode * QueryRewriter::buildLogicalPlan(ParseTreeNode * root, LogicalPlan & logicalPlan){
	LogicalPlanNode * result = NULL;
	// traverse on the parseTree and build the logical plan tree
	switch (root->type) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeNot:
			result = logicalPlan.createOperatorLogicalPlanNode(root->type);
			break;
		case LogicalPlanNodeTypeTerm:
			if (root->termIntermediateStructure->isPhraseKeywordFlag) {
					/*
					 *   Phrase terms should be broken down to Phrase operator node and Term nodes
					 *   in a logical plan
					 *
					 *   Phrase "a b c" is currently a leaf node {a b c} in the parse tree
					 *   Transform it to a following structure in the logical plan
					 *
					 *                    |-------- {a}
					 *   [PhraseOP] --[ MergOp]	--- {b}
					 *   		          |-------- {c}
					 *
					 *   Note: if term position index is missing then the phrase gets converted to
					 *   AND query. "A B C" -> A and B and C
					 */
					ASSERT(root->children.size() == 0);
					string& phrase = root->termIntermediateStructure->rawQueryKeyword;
					vector<string> phraseKeyWords;
					boost::algorithm::split(phraseKeyWords,	phrase, boost::is_any_of("\t "));
					LogicalPlanNode * mergeNode = logicalPlan.createOperatorLogicalPlanNode(LogicalPlanNodeTypeAnd);
					if (isEnabledWordPositionIndex(this->schema.getPositionIndexType())){
						std::map<string, PhraseInfo >::const_iterator iter =
								paramContainer->PhraseKeyWordsInfoMap.find(phrase);

						ASSERT(iter != paramContainer->PhraseKeyWordsInfoMap.end());
						ASSERT(iter->second.phraseKeywordPositionIndex.size() == phraseKeyWords.size());
						result = logicalPlan.createPhraseLogicalPlanNode(phraseKeyWords,
								iter->second.phraseKeywordPositionIndex,
								root->termIntermediateStructure->phraseSlop,
								root->termIntermediateStructure->fieldFilterList,
								root->termIntermediateStructure->fieldFilterAttrOperation);
						result->children.push_back(mergeNode);
					}else {
						result = mergeNode;
					}

					for (unsigned pIndx =0; pIndx < phraseKeyWords.size(); ++pIndx) {
						LogicalPlanNode * termNode = logicalPlan.createTermLogicalPlanNode(
											phraseKeyWords[pIndx],
											TERM_TYPE_COMPLETE, // always complete
											root->termIntermediateStructure->keywordBoostLevel ,
											1, 				   // no fuzzy match
											0,
											root->termIntermediateStructure->fieldFilterList,
											root->termIntermediateStructure->fieldFilterAttrOperation);
						/*
						 *  Although phrase terms should not have fuzzy terms, it is created and
						 *  assigned to the term node because it avoids issues with histogram manager that
						 *  expect fuzzy terms for each term node when the query level fuzziness is enabled.
						 *  The fuzzy term created is same as exact term.
						 */
						if(logicalPlan.isFuzzy()){
							Term * fuzzyTerm = new Term(root->termIntermediateStructure->rawQueryKeyword ,
									root->termIntermediateStructure->keywordPrefixComplete,
									root->termIntermediateStructure->keywordBoostLevel ,
									1,
									0);
							fuzzyTerm->addAttributesToFilter(
									root->termIntermediateStructure->fieldFilterList,
									root->termIntermediateStructure->fieldFilterAttrOperation);
							termNode->setFuzzyTerm(fuzzyTerm);
						}
						mergeNode->children.push_back(termNode);
					}
			}else{
                result = logicalPlan.createTermLogicalPlanNode(root->termIntermediateStructure->rawQueryKeyword ,
                        root->termIntermediateStructure->keywordPrefixComplete,
                        root->termIntermediateStructure->keywordBoostLevel ,
                        indexDataConfig->getFuzzyMatchPenalty(),
                        0,
                        root->termIntermediateStructure->fieldFilterList,
                        root->termIntermediateStructure->fieldFilterAttrOperation);
                if(logicalPlan.isFuzzy()){
                    Term * fuzzyTerm = new Term(root->termIntermediateStructure->rawQueryKeyword ,
                            root->termIntermediateStructure->keywordPrefixComplete,
                            root->termIntermediateStructure->keywordBoostLevel ,
                            indexDataConfig->getFuzzyMatchPenalty(),
                            computeEditDistanceThreshold(getUtf8StringCharacterNumber(
                                                    root->termIntermediateStructure->rawQueryKeyword) ,
                                                    root->termIntermediateStructure->keywordSimilarityThreshold));
                    fuzzyTerm->addAttributesToFilter(
                    		root->termIntermediateStructure->fieldFilterList,
                    		root->termIntermediateStructure->fieldFilterAttrOperation);
                    result->setFuzzyTerm(fuzzyTerm);
                }
			}
			break;
		case LogicalPlanNodeTypeGeo:
			if(root->geoIntermediateStructure->type == GeoTypeRectangular){
				Rectangle* regionShape = new Rectangle();
				regionShape->min.x = root->geoIntermediateStructure->lblat;
				regionShape->min.y = root->geoIntermediateStructure->lblong;
				regionShape->max.x = root->geoIntermediateStructure->rtlat;
				regionShape->max.y = root->geoIntermediateStructure->rtlong;
				result = logicalPlan.createGeoLogicalPlanNode(regionShape);
			}else if(root->geoIntermediateStructure->type == GeoTypeCircular){
				Point center;
				center.x = root->geoIntermediateStructure->clat;
				center.y = root->geoIntermediateStructure->clong;
				Circle* regionShape = new Circle(center,root->geoIntermediateStructure->radius);
				result = logicalPlan.createGeoLogicalPlanNode(regionShape);
			}else{
				ASSERT(false); // the type of the geoIntermediateStructure is not correct.
			}
			break;
		default:
			ASSERT(true);  // 1.avoids compiler warning. 2. Informs developer that they made some mistake
	}
	for(vector<ParseTreeNode *>::iterator childOfRoot = root->children.begin() ; childOfRoot != root->children.end() ; ++childOfRoot){
		result->children.push_back(buildLogicalPlan(*childOfRoot, logicalPlan));
	}

	return result;
}

void QueryRewriter::createExactAndFuzzyQueries(LogicalPlan & plan) {
    // move on summary of the container and build the query object

    //1. first find the search type
    if (paramContainer->hasParameterInQuery(TopKSearchType)) { // search type is TopK
        plan.setQueryType(srch2is::SearchTypeTopKQuery);
    } else if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results
        plan.setQueryType(srch2is::SearchTypeGetAllResultsQuery);
    }
	// else : there is no else because validator makes sure type is set in parser

    // 2. see if it is a fuzzy search or exact search, if there is no keyword (which means GEO search), then fuzzy is always false
	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
	unsigned numberOfKeywords = 0;
	while(termIterator.hasMore()){
		termIterator.getNext();
		numberOfKeywords++;
	}
    if (paramContainer->hasParameterInQuery(IsFuzzyFlag)) {
        plan.setFuzzy(paramContainer->isFuzzy
                        && (numberOfKeywords != 0));
    } else { // get it from configuration file
        plan.setFuzzy(indexDataConfig->getIsFuzzyTermsQuery()
                        && (numberOfKeywords != 0));
    }

    // 3. set the offset of results to retrieve
    if (paramContainer->hasParameterInQuery(ResultsStartOffset)) {
        plan.setOffset(paramContainer->resultsStartOffset);
    } else { // get it from configuration file
        plan.setOffset(0); // default is zero
    }

    // 4. set the number of results to retrieve
    if (paramContainer->hasParameterInQuery(NumberOfResults)) {
        plan.setNumberOfResultsToRetrieve(paramContainer->numberOfResults);
    } else { // get it from configuration file
        plan.setNumberOfResultsToRetrieve(
        		indexDataConfig->getDefaultResultsToRetrieve());
    }

    // 5. based on the search type, get needed information and create the query objects
    // TODO: check if we can merge this two function and remove the switch-case
    switch (plan.getQueryType()) {
    case srch2is::SearchTypeTopKQuery:
        createExactAndFuzzyQueriesForTopK(plan);
        break;
    case srch2is::SearchTypeGetAllResultsQuery:
        createExactAndFuzzyQueriesForGetAllTResults(plan);
        break;
    default:
        ASSERT(false);
        break;
    }

    fillExactAndFuzzyQueriesWithCommonInformation(plan);
}

void QueryRewriter::fillExactAndFuzzyQueriesWithCommonInformation(
        LogicalPlan & plan) {

	// 0. Extract the common information from the container
	//  Geo query information
	if(paramContainer->hasParameterInQuery(GeoSearchFlag)){
		GeoParameterContainer * gpc = paramContainer->geoParameterContainer;
		if (gpc->hasParameterInQuery(GeoTypeRectangular)) {
			plan.getExactQuery()->setRange(gpc->leftBottomLatitude,
					gpc->leftBottomLongitude, gpc->rightTopLatitude,
					gpc->rightTopLongitude);
			if (plan.isFuzzy()) {
				plan.getFuzzyQuery()->setRange(gpc->leftBottomLatitude,
						gpc->leftBottomLongitude, gpc->rightTopLatitude,
						gpc->rightTopLongitude);
			}
		} else if (gpc->hasParameterInQuery(GeoTypeCircular)) {
			plan.getExactQuery()->setRange(gpc->centerLatitude,
					gpc->centerLongitude, gpc->radius);
			if (plan.isFuzzy()) {
				plan.getFuzzyQuery()->setRange(gpc->centerLatitude,
						gpc->centerLongitude, gpc->radius);
			}
		}
	}

    // 1. first check to see if there is any keyword in the query
    if (! paramContainer->hasParameterInQuery(RawQueryKeywords)) {
        return;
    }

    // 2. Extract the common information from the container
    // length boost
    if (paramContainer->hasParameterInQuery(LengthBoostFlag)) {
        plan.getExactQuery()->setLengthBoost(paramContainer->lengthBoost);
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setLengthBoost(paramContainer->lengthBoost);
        }
    } else { // get it from configuration file
        plan.getExactQuery()->setLengthBoost(
        		indexDataConfig->getQueryTermLengthBoost());
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setLengthBoost(
            		indexDataConfig->getQueryTermLengthBoost());
        }
    }

    // prefix match penalty flag
    if (paramContainer->hasParameterInQuery(PrefixMatchPenaltyFlag)) {
        plan.getExactQuery()->setPrefixMatchPenalty(
        		paramContainer->prefixMatchPenalty);
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setPrefixMatchPenalty(
            		paramContainer->prefixMatchPenalty);
        }
    } else { // get it from configuration file
        plan.getExactQuery()->setPrefixMatchPenalty(
        		indexDataConfig->getPrefixMatchPenalty());
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setPrefixMatchPenalty(
            		indexDataConfig->getPrefixMatchPenalty());
        }
    }

    ParseTreeNode * leafNode;
	ParseTreeLeafNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(KeywordSimilarityThreshold) == false) { // get it from configuration file
			leafNode->termIntermediateStructure->keywordSimilarityThreshold = indexDataConfig->getQueryTermSimilarityThreshold();
		}
	}

	termIterator.init(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(KeywordBoostLevel) == false) { // get it from configuration file
			leafNode->termIntermediateStructure->keywordBoostLevel = indexDataConfig->getQueryTermBoost();
		}
	}

	termIterator.init(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(QueryPrefixCompleteFlag) == false) { // get it from configuration file
			leafNode->termIntermediateStructure->keywordPrefixComplete =
					indexDataConfig->getQueryTermPrefixType() ?
                    srch2is::TERM_TYPE_COMPLETE :
                    srch2is::TERM_TYPE_PREFIX;
		}
	}



	termIterator.init(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(IsPhraseKeyword) == false) { // get it from configuration file
			leafNode->termIntermediateStructure->isPhraseKeywordFlag = false;
		}
	}

    // 3. Fill up query objects
    // exact query
	termIterator.init(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		srch2is::Term *exactTerm;
		if (leafNode->termIntermediateStructure->isPhraseKeywordFlag == false){

			exactTerm = new srch2is::Term(leafNode->termIntermediateStructure->rawQueryKeyword,
					leafNode->termIntermediateStructure->keywordPrefixComplete, leafNode->termIntermediateStructure->keywordBoostLevel,
					indexDataConfig->getFuzzyMatchPenalty(), 0);
			exactTerm->addAttributesToFilter(
					leafNode->termIntermediateStructure->fieldFilterList,
					leafNode->termIntermediateStructure->fieldFilterAttrOperation);

			plan.getExactQuery()->add(exactTerm);

		} else {

			vector<string> phraseKeyWords;
			boost::algorithm::split(phraseKeyWords, leafNode->termIntermediateStructure->rawQueryKeyword, boost::is_any_of("\t "));
			// keywords in phrase are considered to be complete and no fuzziness is allowed
			for (int pIndx =0; pIndx < phraseKeyWords.size(); ++pIndx){
				exactTerm = new srch2is::Term(phraseKeyWords[pIndx],
						srch2is::TERM_TYPE_COMPLETE, leafNode->termIntermediateStructure->keywordBoostLevel,
						1 , 0);
				exactTerm->addAttributesToFilter(
						leafNode->termIntermediateStructure->fieldFilterList,
						leafNode->termIntermediateStructure->fieldFilterAttrOperation);
				plan.getExactQuery()->add(exactTerm);
			}
		}
	}
    // fuzzy query
    if (plan.isFuzzy()) {

    	termIterator.init(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
        	if (leafNode->termIntermediateStructure->isPhraseKeywordFlag == true)
        		continue;                   // No fuzzy for phrase search
            srch2is::Term *fuzzyTerm;
            fuzzyTerm = new srch2is::Term(leafNode->termIntermediateStructure->rawQueryKeyword,
            		leafNode->termIntermediateStructure->keywordPrefixComplete, leafNode->termIntermediateStructure->keywordBoostLevel,
            		indexDataConfig->getFuzzyMatchPenalty(),
                    computeEditDistanceThreshold(getUtf8StringCharacterNumber(leafNode->termIntermediateStructure->rawQueryKeyword) , leafNode->termIntermediateStructure->keywordSimilarityThreshold));
                    // this is the place that we do normalization, in case we want to make this
                    // configurable we should change this place.
            fuzzyTerm->addAttributesToFilter(
            		leafNode->termIntermediateStructure->fieldFilterList,
            		leafNode->termIntermediateStructure->fieldFilterAttrOperation);

            plan.getFuzzyQuery()->add(fuzzyTerm);
    	}
    }

}

void QueryRewriter::createExactAndFuzzyQueriesForTopK(LogicalPlan & plan){
    // allocate the objects
    plan.setExactQuery(new Query(srch2is::SearchTypeTopKQuery));
    if (plan.isFuzzy()) {
    	plan.setFuzzyQuery(new Query(srch2is::SearchTypeTopKQuery));
    }
}
void QueryRewriter::createExactAndFuzzyQueriesForGetAllTResults(LogicalPlan & plan) {
    plan.setExactQuery(new Query(srch2is::SearchTypeGetAllResultsQuery));
    srch2is::SortOrder order =
            (indexDataConfig->getOrdering() == 0) ?
                    srch2is::SortOrderAscending : srch2is::SortOrderDescending;
    plan.getExactQuery()->setSortableAttribute(
    		indexDataConfig->getAttributeToSort(), order);
    // TODO : sortableAttribute and order must be removed from here, all sort jobs must be transfered to
    //        to sort filter, now, when it's GetAllResults, it first finds the results based on an the order given here
    //        and then also applies the sort filter. When this is removed, core also must be changed to not need this
    //        sortable attribute anymore.

    if (plan.isFuzzy()) {
        plan.setFuzzyQuery(new Query(srch2is::SearchTypeGetAllResultsQuery));
        plan.getFuzzyQuery()->setSortableAttribute(
        		indexDataConfig->getAttributeToSort(), order);
    }
}

// creates a post processing plan based on information from Query
void QueryRewriter::createPostProcessingPlan(LogicalPlan & plan) {

    // NOTE: FacetedSearchFilter should be always the last filter.
    // this function goes through the summary and uses the members of parsedInfo to fill out the query objects and
    // also creates and sets the plan

    // 1. allocate the post processing plan, it will be freed when the QueryPlan object is destroyed.
    plan.setPostProcessingPlan(new ResultsPostProcessorPlan());
    plan.setPostProcessingInfo(new ResultsPostProcessingInfo());
    // 2. If there is a filter query, allocate the filter and add it to the plan
    if (paramContainer->hasParameterInQuery(FilterQueryEvaluatorFlag)) { // there is a filter query
        plan.getPostProcessingInfo()->setFilterQueryEvaluator(paramContainer->filterQueryContainer->evaluator);
    }

    // 3. look for Sort and Facet
	// look for SortFiler
	if (paramContainer->hasParameterInQuery(SortQueryHandler)) { // there is a sort filter
		plan.getPostProcessingInfo()->setSortEvaluator(paramContainer->sortQueryContainer->evaluator);
	}

	// look for Facet filter
	if (paramContainer->hasParameterInQuery(FacetQueryHandler)) { // there is a sort filter
		FacetQueryContainer * container =
				paramContainer->facetQueryContainer;

		plan.getPostProcessingInfo()->setFacetInfo(container);
	}

	//4. if there is a role id set the role id for access control
	if(paramContainer->hasParameterInQuery(AccessControl)){ // there is access control
		if(paramContainer->hasRoleCore)
			plan.getPostProcessingInfo()->setRoleId(paramContainer->roleId);
	}

}



}
}
