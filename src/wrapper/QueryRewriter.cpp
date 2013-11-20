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
#include <instantsearch/FacetedSearchFilter.h>
#include <instantsearch/RefiningAttributeExpressionFilter.h>
#include <instantsearch/SortFilter.h>
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

QueryRewriter::QueryRewriter(const ConfigManager *indexDataContainerConf,
        const Schema & schema, const Analyzer & analyzer,
        ParsedParameterContainer * paramContainer) :
        schema(schema), analyzer(analyzer) {
    this->paramContainer = paramContainer;
    this->indexDataContainerConf = indexDataContainerConf;
}

void QueryRewriter::rewrite(LogicalPlan & logicalPlan) {

	// If search type is RetrievByIdSearchType, no need to continue rewriting.
	if(this->paramContainer->hasParameterInQuery(RetrieveByIdSearchType)){
		return;
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
    applyAnalyzer();


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


}

void QueryRewriter::prepareKeywordInfo() {

	ParseTreeNode * leafNode;
	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();

        if (paramContainer->hasParameterInQuery(KeywordBoostLevel)
                && leafNode->temporaryTerm->keywordBoostLevel < 0) { // -1 is the place holder for numerical parameters ...
        	leafNode->temporaryTerm->keywordBoostLevel =
                    indexDataContainerConf->getQueryTermBoost();
        }

        if (paramContainer->hasParameterInQuery(KeywordSimilarityThreshold)
                && leafNode->temporaryTerm->keywordSimilarityThreshold < 0) {
        	leafNode->temporaryTerm->keywordSimilarityThreshold =
                    indexDataContainerConf->getQueryTermSimilarityThreshold();
        }

        if (paramContainer->hasParameterInQuery(QueryPrefixCompleteFlag)
                && leafNode->temporaryTerm->keywordPrefixComplete
                        == srch2is::TERM_TYPE_NOT_SPECIFIED) {
        	leafNode->temporaryTerm->keywordPrefixComplete =
                    indexDataContainerConf->getQueryTermPrefixType() ?
                            srch2is::TERM_TYPE_COMPLETE :
                            srch2is::TERM_TYPE_PREFIX;
            // true means complete
        }

        if (paramContainer->hasParameterInQuery(FieldFilter)
                && leafNode->temporaryTerm->fieldFilterOp
                        == srch2is::OP_NOT_SPECIFIED) {
        	leafNode->temporaryTerm->fieldFilterOp = srch2is::BooleanOperatorOR; // default field filter operation is OR
            // TODO : get it from configuration file
        }
	}
}

void QueryRewriter::applyAnalyzer() {
    Analyzer & analyzerNotConst = const_cast<Analyzer &>(analyzer); // because of bad design on analyzer
    vector<ParseTreeNode *> keywordPointersToErase; // stop word indexes, to be removed later
    // first apply the analyzer
	ParseTreeNode * leafNode;
	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();

		// Currently we only get the first token coming out of analyzer chain for each
		// keyword. In future we should handle synonyms TODO
		string keywordAfterAnalyzer = "";
		if (leafNode->temporaryTerm->isPhraseKeywordFlag){
			std::vector<PositionalTerm> analyzedQueryKeywords;
			analyzerNotConst.tokenizeQuery(leafNode->temporaryTerm->rawQueryKeyword, analyzedQueryKeywords);
			keywordAfterAnalyzer.clear();
			vector<unsigned> positionIndexes;
			for (int i=0; i < analyzedQueryKeywords.size(); ++i){
				if (i)
					keywordAfterAnalyzer.append(" ");
				keywordAfterAnalyzer.append(analyzedQueryKeywords[i].term);
				positionIndexes.push_back(analyzedQueryKeywords[i].position);
			}
			if (positionIndexes.size() > 0)
				paramContainer->PhraseKeyWordsPositionMap[keywordAfterAnalyzer] = positionIndexes;
		}else{
			keywordAfterAnalyzer = analyzerNotConst.applyFilters(leafNode->temporaryTerm->rawQueryKeyword);
		}
		if (keywordAfterAnalyzer.compare("") == 0) { // analyzer removed this keyword, it's assumed to be a stop word
			keywordPointersToErase.push_back(leafNode);
		} else { // just apply the analyzer
			leafNode->temporaryTerm->rawQueryKeyword = keywordAfterAnalyzer;
		}

	}

	// now erase the data of erased keywords
    for(vector<ParseTreeNode *>::iterator keywordToErase = keywordPointersToErase.begin() ; keywordToErase != keywordPointersToErase.end() ; ++keywordToErase){
    	TreeOperations::removeFromTree(*keywordToErase , paramContainer->parseTreeRoot);
    }

    termIterator.init();
    unsigned numberOfKeywords = 0;
	while(termIterator.hasMore()){
		numberOfKeywords ++;
	}

    if(numberOfKeywords == 0){
        if(paramContainer->hasParameterInQuery(TopKSearchType) || paramContainer->hasParameterInQuery(GetAllResultsSearchType)){
            paramContainer->messages.push_back(
                    std::make_pair(MessageWarning,
                            "After ignoring stop words no keyword is left to search."));
        }
    }

}

// this function creates the bit sequence needed for field filter based on the filter names
void QueryRewriter::prepareFieldFilters() {


    if(!indexDataContainerConf->getSupportAttributeBasedSearch()){
    	ParseTreeNode * leafNode;
    	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
    		leafNode->temporaryTerm->fieldFilterNumber = 0x7fffffff;
    	}
        return;
    }
    if(paramContainer->hasParameterInQuery(FieldFilter) ){
        // some filters are provided in query so these two vectors are the same size as keywords vector

    	ParseTreeNode * leafNode;
    	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();

            srch2is::BooleanOperation op = leafNode->temporaryTerm->fieldFilterOp;

            unsigned filter = 0;
            if (leafNode->temporaryTerm->fieldFilter.size() == 0) {
                filter = 0x7fffffff;  // it can appear in all fields
                // TODO : get it from configuration file
            } else {
                bool shouldApplyAnd = true;
                for (std::vector<std::string>::iterator field = leafNode->temporaryTerm->fieldFilter.begin();
                        field != leafNode->temporaryTerm->fieldFilter.end(); ++field) {

                    if (field->compare("*") == 0) { // all fields
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
            leafNode->temporaryTerm->fieldFilterNumber = filter;

    	}

    }else{
    	ParseTreeNode * leafNode;
    	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
    		leafNode->temporaryTerm->fieldFilterNumber = 0x7fffffff;
    	}
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
    if (!indexDataContainerConf->isFacetEnabled()) {

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
                    indexDataContainerConf->getFacetAttributes()->begin(),
                    indexDataContainerConf->getFacetAttributes()->end(),
                    *fieldName);
            int fieldFacetTypeInt =
                    indexDataContainerConf->getFacetTypes()->at(
                            std::distance(indexDataContainerConf->getFacetAttributes()->begin() , facetIteratorInConfVector));
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
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(std::distance(facetType ,facetQueryContainer->types.begin() )));
                int fieldFacetTypeInt =
                        indexDataContainerConf->getFacetTypes()->at(
                                std::distance(indexDataContainerConf->getFacetAttributes()->begin() , facetIteratorInConfVector));
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
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string startFromConfig =
                            indexDataContainerConf->getFacetStarts()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
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
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string endFromConfig =
                            indexDataContainerConf->getFacetEnds()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
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
                        indexDataContainerConf->getFacetAttributes()->begin(),
                        indexDataContainerConf->getFacetAttributes()->end(),
                        facetQueryContainer->fields.at(facetFieldIndex));
                if (facetIteratorInConfVector
                        != indexDataContainerConf->getFacetAttributes()->end()) { // this attribute is in config
                    string gapFromConfig =
                            indexDataContainerConf->getFacetGaps()->at(
                                    facetIteratorInConfVector
                                            - indexDataContainerConf->getFacetAttributes()->begin());
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
		plan.setSearchType(RetrieveByIdSearchType);
		return;
	}
    createExactAndFuzzyQueries(plan);
    // generate post processing plan
    createPostProcessingPlan(plan);

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
	paramContainer->parseTreeRoot = TreeOperations::mergeTwoSimilarLevels(paramContainer->parseTreeRoot);
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
			result = logicalPlan.createTermLogicalPlanNode(root->temporaryTerm->rawQueryKeyword ,
					root->temporaryTerm->keywordPrefixComplete,
					root->temporaryTerm->keywordBoostLevel ,
					indexDataContainerConf->getFuzzyMatchPenalty(),
					0,
					root->temporaryTerm->fieldFilterNumber);
			if(logicalPlan.isFuzzy()){
				Term * fuzzyTerm = new Term(root->temporaryTerm->rawQueryKeyword ,
						root->temporaryTerm->keywordPrefixComplete,
						root->temporaryTerm->keywordBoostLevel ,
						indexDataContainerConf->getFuzzyMatchPenalty(),
						srch2is::Term::getEditDistanceThreshold(getUtf8StringCharacterNumber(
						                    		root->temporaryTerm->rawQueryKeyword) ,
						                    		root->temporaryTerm->keywordSimilarityThreshold));
				fuzzyTerm->addAttributeToFilterTermHits(root->temporaryTerm->fieldFilterNumber);
				result->setFuzzyTerm(fuzzyTerm);
			}
			break;
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
        plan.setSearchType(TopKSearchType);
    } else if (paramContainer->hasParameterInQuery(GetAllResultsSearchType)) { // get all results
        plan.setSearchType(GetAllResultsSearchType);
    } else if (paramContainer->hasParameterInQuery(GeoSearchType)) { // GEO
        plan.setSearchType(GeoSearchType);
    } // else : there is no else because validator makes sure type is set in parser

    // 2. see if it is a fuzzy search or exact search, if there is no keyword (which means GEO search), then fuzzy is always false
	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
	unsigned numberOfKeywords = 0;
	while(termIterator.hasMore()){
		numberOfKeywords++;
	}
    if (paramContainer->hasParameterInQuery(IsFuzzyFlag)) {
        plan.setFuzzy(paramContainer->isFuzzy
                        && (numberOfKeywords != 0));
    } else { // get it from configuration file
        plan.setFuzzy(indexDataContainerConf->getIsFuzzyTermsQuery()
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
        plan.setResultsToRetrieve(paramContainer->numberOfResults);
    } else { // get it from configuration file
        plan.setResultsToRetrieve(
                indexDataContainerConf->getDefaultResultsToRetrieve());
    }

    // 5. based on the search type, get needed information and create the query objects
    switch (plan.getSearchType()) {
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
        ASSERT(false);
        break;
    }

    fillExactAndFuzzyQueriesWithCommonInformation(plan);
}

void QueryRewriter::fillExactAndFuzzyQueriesWithCommonInformation(
        LogicalPlan & plan) {

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
                indexDataContainerConf->getQueryTermLengthBoost());
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setLengthBoost(
                    indexDataContainerConf->getQueryTermLengthBoost());
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
                indexDataContainerConf->getPrefixMatchPenalty());
        if (plan.isFuzzy()) {
            plan.getFuzzyQuery()->setPrefixMatchPenalty(
                    indexDataContainerConf->getPrefixMatchPenalty());
        }
    }

    ParseTreeNode * leafNode;
	ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(KeywordSimilarityThreshold) == false) { // get it from configuration file
			leafNode->temporaryTerm->keywordSimilarityThreshold = indexDataContainerConf->getQueryTermSimilarityThreshold();
		}
	}

	termIterator.init();
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(KeywordBoostLevel) == false) { // get it from configuration file
			leafNode->temporaryTerm->keywordBoostLevel = indexDataContainerConf->getQueryTermBoost();
		}
	}

	termIterator.init();
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(QueryPrefixCompleteFlag) == false) { // get it from configuration file
			leafNode->temporaryTerm->keywordPrefixComplete =
					indexDataContainerConf->getQueryTermPrefixType() ?
                    srch2is::TERM_TYPE_COMPLETE :
                    srch2is::TERM_TYPE_PREFIX;
		}
	}

	termIterator.init();
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (leafNode->temporaryTerm->fieldFilterNumber == 0) { // get it from configuration file
			leafNode->temporaryTerm->fieldFilterNumber = 0x7fffffff;// 0x7fffffff means all attributes with OR operator
		}
	}

	termIterator.init();
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		if (paramContainer->hasParameterInQuery(IsPhraseKeyword) == false) { // get it from configuration file
			leafNode->temporaryTerm->isPhraseKeywordFlag = false;
		}
	}

    // 3. Fill up query objects
    // exact query
	termIterator.init();
	while(termIterator.hasMore()){
		leafNode = termIterator.getNext();
		srch2is::Term *exactTerm;
		if (leafNode->temporaryTerm->isPhraseKeywordFlag == false){

			exactTerm = new srch2is::Term(leafNode->temporaryTerm->rawQueryKeyword,
					leafNode->temporaryTerm->keywordPrefixComplete, leafNode->temporaryTerm->keywordBoostLevel,
					indexDataContainerConf->getFuzzyMatchPenalty(), 0);
			exactTerm->addAttributeToFilterTermHits(leafNode->temporaryTerm->fieldFilterNumber);

			plan.getExactQuery()->add(exactTerm);

		} else {

			vector<string> phraseKeyWords;
			boost::algorithm::split(phraseKeyWords, leafNode->temporaryTerm->rawQueryKeyword, boost::is_any_of("\t "));
			// keywords in phrase are considered to be complete and no fuzziness is allowed
			for (int pIndx =0; pIndx < phraseKeyWords.size(); ++pIndx){
				exactTerm = new srch2is::Term(phraseKeyWords[pIndx],
						srch2is::TERM_TYPE_COMPLETE, leafNode->temporaryTerm->keywordBoostLevel,
						1 , 0);
				exactTerm->addAttributeToFilterTermHits(leafNode->temporaryTerm->fieldFilterNumber);
				plan.getExactQuery()->add(exactTerm);
			}
		}
	}
    // fuzzy query
    if (plan.isFuzzy()) {

    	termIterator.init();
    	while(termIterator.hasMore()){
    		leafNode = termIterator.getNext();
        	if (leafNode->temporaryTerm->isPhraseKeywordFlag == true)
        		continue;                   // No fuzzy for phrase search
            srch2is::Term *fuzzyTerm;
            fuzzyTerm = new srch2is::Term(leafNode->temporaryTerm->rawQueryKeyword,
            		leafNode->temporaryTerm->keywordPrefixComplete, leafNode->temporaryTerm->keywordBoostLevel,
                    indexDataContainerConf->getFuzzyMatchPenalty(),
                    srch2is::Term::getEditDistanceThreshold(getUtf8StringCharacterNumber(
                    		leafNode->temporaryTerm->rawQueryKeyword) , leafNode->temporaryTerm->keywordSimilarityThreshold));
                    // this is the place that we do normalization, in case we want to make this
                    // configurable we should change this place.
            fuzzyTerm->addAttributeToFilterTermHits(leafNode->temporaryTerm->fieldFilterNumber);

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
void QueryRewriter::createExactAndFuzzyQueriesForGetAllTResults(
        LogicalPlan & plan) {
    plan.setExactQuery(new Query(srch2is::SearchTypeGetAllResultsQuery));
    srch2is::SortOrder order =
            (indexDataContainerConf->getOrdering() == 0) ?
                    srch2is::SortOrderAscending : srch2is::SortOrderDescending;
    plan.getExactQuery()->setSortableAttribute(
            indexDataContainerConf->getAttributeToSort(), order);
    // TODO : sortableAttribute and order must be removed from here, all sort jobs must be transfered to
    //        to sort filter, now, when it's GetAllResults, it first finds the results based on an the order given here
    //        and then also applies the sort filter. When this is removed, core also must be changed to not need this
    //        sortable attribute anymore.

    if (plan.isFuzzy()) {
        plan.setFuzzyQuery(new Query(srch2is::SearchTypeGetAllResultsQuery));
        plan.getFuzzyQuery()->setSortableAttribute(
                indexDataContainerConf->getAttributeToSort(), order);
    }
}

void QueryRewriter::createExactAndFuzzyQueriesForGeo(LogicalPlan & plan) {
    plan.setExactQuery(new Query(srch2is::SearchTypeMapQuery));
    if (plan.isFuzzy()) {
        plan.setFuzzyQuery(new Query(srch2is::SearchTypeMapQuery));
    }
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

// creates a post processing plan based on information from Query
void QueryRewriter::createPostProcessingPlan(LogicalPlan & plan) {

    // NOTE: FacetedSearchFilter should be always the last filter.
    // this function goes through the summary and uses the members of parsedInfo to fill out the query objects and
    // also creates and sets the plan

    // 1. allocate the post processing plan, it will be freed when the QueryPlan object is destroyed.
    plan.setPostProcessingPlan(new ResultsPostProcessorPlan());
    // 2. If there is a filter query, allocate the filter and add it to the plan
    if (paramContainer->hasParameterInQuery(FilterQueryEvaluatorFlag)) { // there is a filter query
        srch2is::RefiningAttributeExpressionFilter * filterQuery =
                new srch2is::RefiningAttributeExpressionFilter();
        filterQuery->evaluator =
        		paramContainer->filterQueryContainer->evaluator;
        plan.getPostProcessingPlan()->addFilterToPlan(filterQuery);
    }

    if (paramContainer->hasParameterInQuery(IsPhraseKeyword)) { // Filter query phrase...
    	srch2is::PhraseQueryFilter * pqFilter = new srch2is::PhraseQueryFilter();
		ParseTreeNode * leafNode;
		ParseTreeLeadNodeIterator termIterator(paramContainer->parseTreeRoot);
		while(termIterator.hasMore()){
			leafNode = termIterator.getNext();
    		if (leafNode->temporaryTerm->isPhraseKeywordFlag == false)
    			continue;
    		vector<string> phraseKeywords;
    		boost::algorithm::split(phraseKeywords, leafNode->temporaryTerm->rawQueryKeyword,
                                    boost::is_any_of("\t "));
    		std::map<string, vector<unsigned> >::const_iterator iter =
    				paramContainer->PhraseKeyWordsPositionMap.find(leafNode->temporaryTerm->rawQueryKeyword);
    		ASSERT(iter != paramContainer->PhraseKeyWordsPositionMap.end());
    		ASSERT(iter->second.size() == phraseKeywords.size());
    		pqFilter->addPhrase(phraseKeywords,iter->second, leafNode->temporaryTerm->phraseSlop,
    				leafNode->temporaryTerm->fieldFilterNumber);
		}
        plan.getPostProcessingPlan()->addFilterToPlan(pqFilter);
    }

    // 3. look for Sort and Facet
	// look for SortFiler
	if (paramContainer->hasParameterInQuery(SortQueryHandler)) { // there is a sort filter
		srch2is::SortFilter * sortFilter = new srch2is::SortFilter();
		sortFilter->evaluator =
				paramContainer->sortQueryContainer->evaluator;
		plan.getPostProcessingPlan()->addFilterToPlan(sortFilter);
	}

	// look for Facet filter
	if (paramContainer->hasParameterInQuery(FacetQueryHandler)) { // there is a sort filter
		srch2is::FacetedSearchFilter * facetFilter = new srch2is::FacetedSearchFilter();
		FacetQueryContainer * container =
				paramContainer->facetQueryContainer;
		facetFilter->initialize(container->types, container->fields,
				container->rangeStarts, container->rangeEnds,
				container->rangeGaps , container->numberOfTopGroupsToReturn);

		plan.getPostProcessingPlan()->addFilterToPlan(facetFilter);
	}

}



}
}
