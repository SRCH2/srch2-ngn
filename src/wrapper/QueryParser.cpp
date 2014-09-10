#include "QueryParser.h"
#include "ParsedParameterContainer.h"
#include "ParserUtility.h"
#include <evhttp.h>
#include <string>
#include "boost/regex.hpp"
#include "util/Logger.h"
#include "util/Assert.h"
#include "RegexConstants.h"
#include "QueryFieldBoostParser.h"

using namespace std;
using srch2::util::Logger;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {
//solr parameters link. http://wiki.apache.org/solr/CommonQueryParameters
const char* const QueryParser::fieldListDelimiter = ","; //solr
const char* const QueryParser::fieldListParamName = "fl"; //solr
const char* const QueryParser::debugControlParamName = "debugQuery"; //solr
const char* const QueryParser::debugParamName = "debug"; //solr
const char* const QueryParser::startParamName = "start"; //solr
const char* const QueryParser::rowsParamName = "rows"; //solr
const char* const QueryParser::timeAllowedParamName = "timeAllowed"; //solr
const char* const QueryParser::ommitHeaderParamName = "omitHeader"; //solr
const char* const QueryParser::responseWriteTypeParamName = "wt"; //solr
const char* const QueryParser::sortParamName = "sort"; //solr  (syntax is not like solr)
const char* const QueryParser::sortFiledsDelimiter = ","; // solr
const char* const QueryParser::orderParamName = "orderby"; //srch2
const char* const QueryParser::orderDescending = "desc"; //srch2
const char* const QueryParser::orderAscending = "asc"; //srch2
const char* const QueryParser::keywordQueryParamName = "q"; //solr
const char* const QueryParser::suggestionKeywordParamName = "k"; //solr
const char* const QueryParser::lengthBoostParamName = "lengthBoost"; //srch2
const char* const QueryParser::prefixMatchPenaltyParamName = "pmp"; //srch2
const char* const QueryParser::filterQueryParamName = "fq"; //solr
const char* const QueryParser::queryFieldBoostParamName = "qf"; //solr
const char* const QueryParser::isFuzzyParamName = "fuzzy"; //srch2
const char* const QueryParser::docIdParamName = "docid"; //srch2

// local parameter params
const char* const QueryParser::lpKeyValDelimiter = "="; //solr
const char* const QueryParser::lpQueryBooleanOperatorParamName =
        "defaultFieldOperator"; //srch2
const char* const QueryParser::lpKeywordSimilarityThresholdParamName =
        "defaultSimilarityThreshold"; // srch2
const char* const QueryParser::lpKeywordBoostLevelParamName =
        "defaultBoostLevel"; // srch2
const char* const QueryParser::lpKeywordPrefixCompleteParamName =
        "defaultPrefixComplete"; //srch2
const char* const QueryParser::lpFieldFilterParamName = "defaultSearchFields"; //srch2
const char* const QueryParser::lpFieldFilterDelimiter = ","; //srch2
// rectangular geo params
const char* const QueryParser::leftBottomLatParamName = "lblat"; //srch2
const char* const QueryParser::leftBottomLongParamName = "lblong"; //srch2
const char* const QueryParser::rightTopLatParamName = "rtlat"; //srch2
const char* const QueryParser::rightTopLongParamName = "rtlong"; //srch2
// circular geo params
const char* const QueryParser::centerLatParamName = "clat"; //srch2
const char* const QueryParser::centerLongParamName = "clong"; //srch2
const char* const QueryParser::radiusParamName = "radius"; //srch2
const char* const QueryParser::facetParamName = "facet"; //solr
const char* const QueryParser::facetRangeGap = "gap";
const char* const QueryParser::facetRangeEnd = "end";
const char* const QueryParser::facetRangeStart = "start";
const char* const QueryParser::facetField = "facet.field";
const char* const QueryParser::facetRangeField = "facet.range";
const char* const QueryParser::highlightSwitch = "hl";
// access control
const char* const QueryParser::roleIdParamName = "roleId";

//searchType
const char* const QueryParser::searchType = "searchType";

void decodeString(const char *inputStr, string& outputStr) {
    size_t st;
    char * decodedOutPut = evhttp_uridecode(inputStr, 0, &st);
    outputStr.assign(decodedOutPut);
    /*
     *  evhttp_uridecode allocates a new buffer for decoded string using an input string and
     *  returns the pointer to the new buffer. It is a responsibility of the caller to "free"
     *  the pointer returned by evhttp_uridecode
     */
    free(decodedOutPut);
}

QueryParser::QueryParser(const evkeyvalq &headers,
        ParsedParameterContainer * container) :
        headers(headers) {
    this->container = container;
    this->isParsedError = false;
    this->isLpFieldFilterBooleanOperatorAssigned = false;
    this->lpKeywordSimilarityThreshold = -1.0;
    this->lpKeywordBoostLevel = -1;
    this->lpKeywordPrefixComplete =
            srch2::instantsearch::TERM_TYPE_NOT_SPECIFIED; // stores the fallback termType for keywords
    this->container->isTermBooleanOperatorSet = false;
    this->container->isFqBooleanOperatorSet = false;
    this->isSearchTypeSet = false;
}

QueryParser::QueryParser(const evkeyvalq &headers) :
    headers(headers) {

}

bool QueryParser::parseForSuggestions(string & keyword, float & fuzzyMatchPenalty,
        int & numberOfSuggestionsToReturn , std::vector<std::pair<MessageType, std::string> > & messages){
       // 1. get the keyword string.
        Logger::debug("parsing the main query.");
        const char * keywordTmp = evhttp_find_header(&headers,
                QueryParser::suggestionKeywordParamName);
        if (keywordTmp) { // if this parameter exists
            string keywordStr;
            decodeString(keywordTmp, keywordStr);
            boost::algorithm::trim(keywordStr); // trim the keywordString.
            bool hasParserSuccessfully = parseKeyword(keywordStr,keyword);
            if(!hasParserSuccessfully){
                messages.push_back(std::make_pair(MessageError, " No keyword is recognized for computing suggestions."));
                return false;
            }
            boost::algorithm::trim(keywordStr); // trim the keywordString.

            if(keywordStr.length() == 0){
                fuzzyMatchPenalty = 1; // 1 indicates that the user want the suggestions to be exact, example: k=can
            }else{
                string normalizerString;
                hasParserSuccessfully = parseFuzzyModifier(keywordStr ,normalizerString );
                if(! hasParserSuccessfully){
                    messages.push_back(std::make_pair(MessageWarning, "Bad format is used for edit distance normalizer. No fuzzy suggestions will be returned."));
                }else{
                    if(normalizerString.length() == 1){ // fuzzy modifier is only '~' w/o any values
                        fuzzyMatchPenalty = -1; // -1 indicates that used did not enter any values for this one, example: k=can
                    }else{
                        fuzzyMatchPenalty = static_cast<float>(strtod(normalizerString.c_str() + 1,NULL));
                    }
                }
            }
        }else{
            messages.push_back(std::make_pair(MessageError, "No keyword is recognized for computing suggestions."));
            return false;
        }

        //2. get number of suggestions to be returned
        /* aka: rows parser
         * if there is a number of results
         * fills the container up
         *
         * example: rows=20
         */
        const char * rowsTemp = evhttp_find_header(&headers,
                QueryParser::rowsParamName);
        if (rowsTemp) { // if this parameter exists
            Logger::debug("rowsTemp parameter found, parsing it.");
            string rowsStr;
            decodeString(rowsTemp, rowsStr);
            // convert the rowsStr to integer. e.g. rowsStr will contain string 20
            if (isUnsigned(rowsStr)) {
                numberOfSuggestionsToReturn = static_cast<int>(strtol(rowsStr.c_str(),NULL,10));
            } else {
                numberOfSuggestionsToReturn = -1; // -1 indicates that this parameter is not given by the user
                // raise error
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                "rows parameter should be a positive integer. We will use the default value for this parameter."));
            }
        }else{
            numberOfSuggestionsToReturn = -1; // -1 indicates that this parameter is not given by the user
        }
        Logger::debug("returning from numberOfResultsParser function");
        return true;
}

// parses the URL to a query object
// TODO: change this to bool
bool QueryParser::parse() {

    /*
     * parses the URL to a query object , fills out the container
     *  We have all the header information here which is the pairs of query parameters
     * 0. call isFuzzyParser(); // always call this before calling mainQueryParser.
     * 1. call the main query parser : mainQueryParser();
     * 2. call the debug parser: debugQueryParser();
     * 3. call the field list parser : fieldListParser();
     * 4. call the start parser: startOffsetParameterParser();
     * 5. call the row parser: numberOfResultsParser();
     * 6. call the time allowed parser: timeAllowedParameterParser();
     * 7. call the omit header parser : omitHeaderParameterParser();
     * 8. call the response writer parser : responseWriteTypeParameterParser();
     * 9. call the filter query parser : filterQueryParameterParser();
     * 10. call the query field boost parser : queryFieldBoostParser();
     * 11. this->lengthBoostParser();
     * 12. this->prefixMatchPenaltyParser();
     * 13. call the geo parser: geoParser();
     * 14. based on the value of search type (if it's defined in local parameters we take it
     *    otherwise we get it from conf file) leave the rest of parsing to one of the parsers
     * 14.1: search type : Top-K
     *      call topKParameterParser();
     * 14.2: search type : All results
     *      call getAllResultsParser();
     */

    // do some parsing

    try {
        if(this->docIdParser()){
            return true;
        }
        this->isFuzzyParser();
        this->mainQueryParser();
        this->debugQueryParser();
        this->fieldListParser();
        this->startOffsetParameterParser();
        this->numberOfResultsParser();
        this->timeAllowedParameterParser();
        this->omitHeaderParameterParser();
        this->responseWriteTypeParameterParser();
        this->filterQueryParameterParser();
        this->queryFieldBoostParser();
        this->lengthBoostParser();
        this->prefixMatchPenaltyParser();
        this->geoParser();
        this->extractSearchType();
        this->highlightParser();
        this->accessControlParser();
        if (this->container->hasParameterInQuery(
                GetAllResultsSearchType)) {
            this->getAllResultsParser();
        } else {
            this->topKParameterParser();
        }
        if(!this->isParsedError){ // no error so far
            this->isParsedError = ! attachParseTreeAndMainQueryParallelVectors();
        }

    } catch (exception& e) {
        Logger::error(e.what());
        // error msg
        this->isParsedError = true;
        this->container->messages.push_back(
                make_pair(MessageError,
                        "Ooops! Something went wrong while parsing the query. If you face this again, please contact srch2."));
    }
    return !this->isParsedError; // return true for success, false for parse error
}

bool QueryParser::docIdParser(){
    /*
     * checks to see if "docid" exists in parameters.
     */
    Logger::debug("checking for docid parameter");
    const char * docIdTemp = evhttp_find_header(&headers,
            QueryParser::docIdParamName);
    if (docIdTemp) { // if this parameter exists
        Logger::debug("docid parameter found");
        string docId;
        decodeString(docIdTemp, docId);
        this->container->docIdForRetrieveByIdSearchType = docId;
        this->container->parametersInQuery.push_back(
                srch2::httpwrapper::RetrieveByIdSearchType);
        return true;
    } else {
        Logger::debug("docid parameter not specified");
        return false;
    }
}

void QueryParser::mainQueryParser() { // TODO: change the prototype to reflect input/outputs
    /*
     * example: q={defaultSearchFields=Author defaultSimilarityThreshold=0.8}title:algo* AND publisher:mac* AND lang:engl*^2~0.8
     * 1. calls localParameterParser()
     * 2. calls the termParser();
     */
    // 1. get the mainQuery string.
    Logger::debug("parsing the main query.");
    const char * mainQueryTmp = evhttp_find_header(&headers,
            QueryParser::keywordQueryParamName);
    if (mainQueryTmp && mainQueryTmp[0]) { // if this parameter exists
        string mainQueryStr;
        decodeString(mainQueryTmp, mainQueryStr);
        boost::algorithm::trim(mainQueryStr); // trim the mainQueryString.
        // 2. call the localParameterParser(), this will remove and parse the local param info from the mainQueryStr
        // in LocalParamerterParser, we are logging the mainQuery String
        // so it is ok to pass the mainQueryStr and no need to make a copy of this string
        if (this->localParameterParser(mainQueryStr)) {
            /*
             * At this point we should first extract the AND/OR/NOT parse tree
             */
            bool parsed = parseBooleanExpression( mainQueryStr, this->container->parseTreeRoot );
            if(parsed == false){
                Logger::info(
                        " Parse error, Boolean operators and parentheses are not formatted correctly.");
                this->container->messages.push_back(
                        make_pair(MessageError,
                                "Parse error, Boolean operators and parentheses are not formatted correctly."));
                this->isParsedError = true;
                return;
            }

            ParseTreeNode * leafNode;
            ParseTreeLeafNodeIterator termIterator(this->container->parseTreeRoot);
            while(termIterator.hasMore()){
                leafNode = termIterator.getNext();
                //
                if (this->termParser(leafNode->termIntermediateStructure->termQueryString)) {
                    // term will be parsed and information will be saved in parallel vectors
                }else{
                    break;
                }
            }

            // check if keywordSimilarityThreshold was set by parseTerms
            // true? set the isFuzzyFlag.
            // else , empty the keywordSimilarityThreshold vector
            this->clearMainQueryParallelVectorsIfNeeded();
        }
    } else {
        // no main query parameter present.
        Logger::debug(" parameter q not present.");
    }
}

bool QueryParser::attachParseTreeAndMainQueryParallelVectors(){
    // iterate on leaf nodes and also on vectors and copy their information into the tree
    ParseTreeNode * leafNode;
    ParseTreeLeafNodeIterator termIterator(this->container->parseTreeRoot);
    unsigned vectorsIndex = 0;
    while(termIterator.hasMore()){
        leafNode = termIterator.getNext();

        // raw query keywords
        if(this->rawQueryKeywords.size() < vectorsIndex){
            return false;
        }
        leafNode->termIntermediateStructure->rawQueryKeyword = this->rawQueryKeywords.at(vectorsIndex);
        // keyword similarity threshold
        if (this->container->hasParameterInQuery(KeywordSimilarityThreshold)) {
            leafNode->termIntermediateStructure->keywordSimilarityThreshold = this->keywordSimilarityThreshold.at(vectorsIndex);
        }
        // KeywordBoostLevel
        if (this->container->hasParameterInQuery(KeywordBoostLevel)) {
            leafNode->termIntermediateStructure->keywordBoostLevel = this->keywordBoostLevel.at(vectorsIndex);
        }
        // QueryPrefixCompleteFlag
        if (this->container->hasParameterInQuery(QueryPrefixCompleteFlag)) {
            leafNode->termIntermediateStructure->keywordPrefixComplete = this->keywordPrefixComplete.at(vectorsIndex);
        }
        //FieldFilter
        if (this->container->hasParameterInQuery(FieldFilter)) {
            leafNode->termIntermediateStructure->fieldFilter = this->fieldFilter.at(vectorsIndex);
            leafNode->termIntermediateStructure->fieldFilterOp = this->fieldFilterOps.at(vectorsIndex);
        }
        // Phrase
        if (this->container->hasParameterInQuery(IsPhraseKeyword)) {
            leafNode->termIntermediateStructure->isPhraseKeywordFlag = this->isPhraseKeywordFlags.at(vectorsIndex);
            leafNode->termIntermediateStructure->phraseSlop = this->PhraseSlops.at(vectorsIndex);
        }
        //
        vectorsIndex ++;
    }
    return true;
}

void QueryParser::highlightParser() {

	/*
	 *  Check whether the highlight option is present. By default, highlighting is ON if the attributes
	 *  are marked for highlighting in the config file.
	 */
	 const char * hlTemp = evhttp_find_header(&headers,
			 QueryParser::highlightSwitch);
	 if (hlTemp) {
		 string hlStr;
		 decodeString(hlTemp, hlStr);
		 if (boost::iequals("on", hlStr)) {
			 this->container->isHighlightOn = true;
		 } else if (boost::iequals("off", hlStr)) {
			 this->container->isHighlightOn = false;
		 } else {
			 this->container->isHighlightOn = true;
			 this->container->messages.push_back(
					 make_pair(MessageWarning,
							 "Invalid hl option values. It should be on/off. Setting to default (true)"));
		 }
	 } else {
		 this->container->isHighlightOn = true;
	 }
}

void QueryParser::isFuzzyParser() {
    /*
     * checks to see if "fuzzy" exists in parameters.
     */
    Logger::debug("checking for fuzzy parameter");
    const char * fuzzyTemp = evhttp_find_header(&headers,
            QueryParser::isFuzzyParamName);
    if (fuzzyTemp) { // if this parameter exists
        Logger::debug("fuzzy parameter found");
        string fuzzy;
        decodeString(fuzzyTemp, fuzzy);
        this->container->parametersInQuery.push_back(
                srch2::httpwrapper::IsFuzzyFlag);
        if (boost::iequals("true", fuzzy)) {
            this->container->isFuzzy = true;
            Logger::debug("fuzzy parameter set in container to true");
        } else if (boost::iequals("false", fuzzy)) {
            this->container->isFuzzy = false;
            Logger::debug("fuzzy parameter set in container to false");
        } else {
            this->container->isFuzzy = false;
            Logger::debug("fuzzy parameter set in container to false");
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "unknow fuzzy value. setting it to false"));
        }
    } else {
        Logger::debug("fuzzy parameter not specified");
    }
}

void QueryParser::accessControlParser(){
	/*
	 *   check to see if "acl-id" for access control exists in parameters.
	 */
	Logger::debug("checking for acl-id parameter");
	const char * aclIdTemp = evhttp_find_header(&headers,
			QueryParser::roleIdParamName);
	if (aclIdTemp){ // if acl-id parameter exists.
		Logger::debug("acl-id parameter found");
		string aclId;
		decodeString(aclIdTemp, aclId);
		this->container->parametersInQuery.push_back(srch2::httpwrapper::AccessControl);
		this->container->roleId = aclId;

	} else {
		Logger::debug("acl-id parameter not specified");
		if(this->container->hasRoleCore){
			this->isParsedError = true;
			this->container->messages.push_back(
					make_pair(MessageError,
							"roleId parameter not specified"));
		}
	}
}

void QueryParser::populateFacetFieldsSimple(FacetQueryContainer &fqc) {
    /*
     * populates teh fields vector related to facet.feild
     * example: facet.field=category
     */
    Logger::debug("inside populateFacetFieldSimple function");
    vector<string> facetFields;
    custom_evhttp_find_headers(&headers, facetField, facetFields);
    for (vector<string>::iterator facetField = facetFields.begin();
            facetField != facetFields.end(); ++facetField) {
        fqc.fields.push_back(*facetField);
        fqc.types.push_back(srch2::instantsearch::FacetTypeCategorical);
        // populate parallel vectors with empty string
        fqc.rangeEnds.push_back("");
        fqc.rangeGaps.push_back("");
        fqc.rangeStarts.push_back("");

        // now see if there is a numberOfTopGroupsToReturn given by the user, if not
        // use -1 which indicates returning all the facet groups
        Logger::debug("inside populateParallelRangeVectors function");
        const string numberOfGroupsKeyString = QueryParser::getFacetCategoricalNumberOfTopGroupsToReturn(*facetField);
        const char* numberOfGroupsStrTemp = evhttp_find_header(&headers,
                numberOfGroupsKeyString.c_str());
        if (numberOfGroupsStrTemp) {
            Logger::debug("facetNumberOfGroups parameter found, parsing it.");
            string facetNumberOfGroupsStr;
            decodeString(numberOfGroupsStrTemp, facetNumberOfGroupsStr);
            if(isUnsigned(facetNumberOfGroupsStr)){
                Logger::debug(
                        "facetNumberOfGroups parameter found, pushing it to numberOfTopGroupsToReturn to fqc");
                fqc.numberOfTopGroupsToReturn.push_back(
                        static_cast<int>(strtol(facetNumberOfGroupsStr.c_str(),
                                NULL, 10)));
            } else {
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                numberOfGroupsKeyString+" should get a valid unsigned number. Ignored."));
                fqc.numberOfTopGroupsToReturn.push_back(-1);
            }
        } else {
            Logger::debug(
                    "facetNumberOfGroups parameter not found, pushing -1 to numberOfTopGroupsToReturn to fqc");
            fqc.numberOfTopGroupsToReturn.push_back(-1);
        }
    }
    Logger::debug("returning from populateFacetFieldSimple");
}

void QueryParser::populateFacetFieldsRange(FacetQueryContainer &fqc) {
    /*
     * populates the fields vector related to facet.range.
     * Also, calls the populateParallelRangeVectors function to populate the related parallel vectors.
     * example facet.range=price
     */
    Logger::debug("inside populateFacetFieldsRange function");
    vector<string> facetFields;
    custom_evhttp_find_headers(&headers, facetRangeField, facetFields);
    for (vector<string>::iterator facetField = facetFields.begin();
            facetField != facetFields.end(); ++facetField) {
        fqc.fields.push_back(*facetField);
        fqc.types.push_back(srch2::instantsearch::FacetTypeRange);
        // populate parallel vectors with empty string
        populateParallelRangeVectors(fqc, *facetField);
        // numberOfTopGroupsToReturn is only valid for categorial facets. For a range facet, we put a dummy value "-1"
        fqc.numberOfTopGroupsToReturn.push_back(-1);
    }
    Logger::debug("returning from populateFacetFieldsRange function");
}

void QueryParser::populateParallelRangeVectors(FacetQueryContainer &fqc,
        string &field) {
    /*
     * populates the parallel vectors related to the facet parser.
     */
    Logger::debug("inside populateParallelRangeVectors function");
    const string startKeyStr = QueryParser::getFacetRangeKey(field,
            facetRangeStart);
    const char* rangeStartTemp = evhttp_find_header(&headers,
            startKeyStr.c_str());
    if (rangeStartTemp) {
        Logger::debug("rangeStart parameter found, parsing it.");
        string rangeStart;
        decodeString(rangeStartTemp, rangeStart);
        fqc.rangeStarts.push_back(rangeStart);
    } else {
        Logger::debug(
                "rangeStart parameter not found,pushing empty string to rageStarts member of fqc");
        fqc.rangeStarts.push_back("");
    }
    string endKeyStr = QueryParser::getFacetRangeKey(field, facetRangeEnd);
    const char* rangeEndTemp = evhttp_find_header(&headers, endKeyStr.c_str());
    if (rangeEndTemp) {
        Logger::debug("rangeStart parameter found, parsing it");
        string rangeEnd ;
        decodeString(rangeEndTemp, rangeEnd);
        fqc.rangeEnds.push_back(rangeEnd);
    } else {
        Logger::debug(
                "rangeEnds parameter not found,pushing empty string to rageEnds member of fqc");
        fqc.rangeEnds.push_back("");
    }
    string gapKeyStr = QueryParser::getFacetRangeKey(field, facetRangeGap);
    const char* rangeGapTemp = evhttp_find_header(&headers, gapKeyStr.c_str());
    if (rangeGapTemp) {
        Logger::debug("rangeGap parameter found, parsing it");
        string rangeGap;
        decodeString(rangeGapTemp, rangeGap);
        fqc.rangeGaps.push_back(rangeGap);
    } else {
        Logger::debug(
                "rangeGaps parameter not found,pushing empty string to rangeGaps member of fqc");
        fqc.rangeGaps.push_back("");
    }
    Logger::debug("returning from  populateParallelRangeVectors function");
}
void QueryParser::lengthBoostParser() {
    /*
     * parses the lengthBoost parameter and fills up the container
     * example: lengthBoost=.9
     */
    Logger::debug("inside lengthBoostParser function");
    const char * lengthBoostTmp = evhttp_find_header(&headers,
            QueryParser::lengthBoostParamName);
    if (lengthBoostTmp) { // if this parameter exists
        Logger::debug("lengthBoostParser parameter found, parsing it.");
        string lengthBoost;
        decodeString(lengthBoostTmp,lengthBoost);
        if (isFloat(lengthBoost)) {
            const float lboost = static_cast<float>(strtod(lengthBoost.c_str(),NULL));
            if (lboost <= 1 && lboost >= 0) {
                this->container->parametersInQuery.push_back(
                        srch2::httpwrapper::LengthBoostFlag);
                this->container->lengthBoost = lboost;
            } else {
                //raise error
                this->container->messages.push_back(
                        make_pair(MessageError,
                                "lengthBoost should be a floating point number in between 0 and 1, both inclusive."));
                this->isParsedError = true;
            }

        } else {
            //raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "lengthBoost should be a valid float number"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returnig from lengthBoostParser function");
}

void QueryParser::prefixMatchPenaltyParser() {
    /*
     * parses the pmp parameter and fills up the container
     * example: pmp=0.8
     */
    Logger::debug("inside prefixMatchPenaltyParser function");
    const char * prefixMatchPenaltyTmp = evhttp_find_header(&headers,
            QueryParser::prefixMatchPenaltyParamName);
    if (prefixMatchPenaltyTmp) { // if this parameter exists
        Logger::debug("prefixMatchPenalty parameter found, parsing it.");
        string prefixMatchPenalty;
        decodeString(prefixMatchPenaltyTmp, prefixMatchPenalty);
        if (isFloat(prefixMatchPenalty)) {
            const float ppm = static_cast<float>(strtod(prefixMatchPenalty.c_str(),NULL));
            if (ppm <= 1 && ppm >= 0) {
                this->container->parametersInQuery.push_back(
                        srch2::httpwrapper::PrefixMatchPenaltyFlag);
                this->container->prefixMatchPenalty = ppm;
            } else {
                //raise error
                this->container->messages.push_back(
                        make_pair(MessageError,
                                "ppm should be a floating point number in between 0 and 1, both inclusive."));
                this->isParsedError = true;
            }
        } else {
            //raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            string(prefixMatchPenaltyParamName)
                                    + " should be a valid float number"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returning from prefixMatchPenaltyParser function");
}

void QueryParser::fieldListParser() {
    /*
     * if there is a field list query parameter
     * then parse it and fill the container up
     *
     * Example: fl=field1,field2,field3 or fl=*
     */
    //1. first check to see if fl exists in the headers
    Logger::debug("inside fieldListParser function");
    const char * flTemp = evhttp_find_header(&headers,
            QueryParser::fieldListParamName);
    if (flTemp) { // if this parameter exists
        Logger::debug("field list parameter found, parsing it");
        string fl;
        decodeString(flTemp, fl);
        this->container->parametersInQuery.push_back(
                srch2::httpwrapper::ReponseAttributesList);
        char * fieldStr = strdup(fl.c_str());

        vector<string> tokens;
        boost::split(tokens, fieldStr,
                boost::is_any_of(QueryParser::fieldListDelimiter));
        for (std::vector<string>::iterator fieldItr = tokens.begin();
                fieldItr != tokens.end(); ++fieldItr) {
            string field = *fieldItr;
            if (field.compare("*") == 0) {
                this->container->responseAttributesList.clear();
                this->container->responseAttributesList.push_back("*");
                return;
            }
            this->container->responseAttributesList.push_back(field);
        }
    }
    Logger::debug("returning from fieldListParser function");
}

void QueryParser::debugQueryParser() {
    /*
     * example: debugQuery=True&debug=time
     * if there is a debug query parameter
     * then parse it and fill the container up
     */
//1. first check to see if debugQuery exists in the headers
    Logger::debug("indide debugQueryParser function");
    const char * debugQueryTemp = evhttp_find_header(&headers,
            QueryParser::debugControlParamName);
    if (debugQueryTemp) { // if this parameter exists
        Logger::debug("debugQuery param found, parsing it");
        string debugQuery;
        decodeString(debugQueryTemp, debugQuery);
        if (boost::iequals(debugQuery, "true")) {
            this->container->isDebugEnabled = true;
            this->container->parametersInQuery.push_back(IsDebugEnabled); // change the IsDebugEnabled to DebugEnabled in Enum ParameterName ?
            // look for debug paramter. it decides the debug level, if it is not set, set the debug level to complete.
            const char * debugLevelTemp = evhttp_find_header(&headers,
                    QueryParser::debugParamName);
            if (debugLevelTemp) { // if this parameter exists
                string debugLevel;
                decodeString(debugLevelTemp, debugLevel);
                //check what is the debug level
                if (boost::iequals("true", debugLevel)) {
                    this->container->queryDebugLevel =
                            srch2::httpwrapper::CompleteDebug;
                } else if (boost::iequals("query", debugLevel)) {
                    this->container->queryDebugLevel =
                            srch2::httpwrapper::QueryDebug;
                } else if (boost::iequals("result", debugLevel)) {
                    this->container->queryDebugLevel =
                            srch2::httpwrapper::ResultsDebug;
                } else if (boost::iequals("timing", debugLevel)) {
                    this->container->queryDebugLevel =
                            srch2::httpwrapper::TimingDebug;
                } else {
                    // not supported level, generate a MessageWarning message and set debug to complete.
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    "Unknown value for parameter debug. using debug=true"));
                    this->container->queryDebugLevel =
                            srch2::httpwrapper::CompleteDebug;
                }
            } else {
                // the debug level not specified, use debug level = true
                this->container->queryDebugLevel =
                        srch2::httpwrapper::CompleteDebug;
            }
        }
    }
    Logger::debug("returning from debugQueryParser function");
}

void QueryParser::startOffsetParameterParser() {
    /*
     * if there is a start offset
     * fills the container up
     * example: start=40
     */
// 1. check for start parameter.
    Logger::debug("inside startOffsetParameterParser function");
    const char * startTemp = evhttp_find_header(&headers,
            QueryParser::startParamName);
    if (startTemp) { // if this parameter exists
        Logger::debug("startOffset Parameter found");
        string startStr;
        decodeString(startTemp, startStr);
// convert the startStr to integer.
        if (isUnsigned(startStr)) {
            this->container->resultsStartOffset =
                    static_cast<unsigned int>(strtoul(startStr.c_str(), NULL,
                            10));
            // populate the parametersInQuery
            this->container->parametersInQuery.push_back(ResultsStartOffset);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "start parameter should be a positive integer"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returning from startOffsetParameterParser function");
}

void QueryParser::numberOfResultsParser() {
    /* aka: rows parser
     * if there is a number of results
     * fills the container up
     *
     * example: rows=20
     */
// 1. check for rows parameter.
    Logger::debug("inside numberOfResultsParser function");
    const char * rowsTemp = evhttp_find_header(&headers,
            QueryParser::rowsParamName);
    if (rowsTemp) { // if this parameter exists
        Logger::debug("rowsTemp parameter found, parsing it.");
        string rowsStr;
        decodeString(rowsTemp, rowsStr);
// convert the rowsStr to integer. e.g. rowsStr will contain string 20
        if (isUnsigned(rowsStr)) {
            this->container->numberOfResults =
                    static_cast<unsigned int>(strtoul(rowsStr.c_str(), NULL, 10));
            // populate the parametersInQuery
            this->container->parametersInQuery.push_back(NumberOfResults);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "rows parameter should be a positive integer"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returning from numberOfResultsParser function");
}

void QueryParser::timeAllowedParameterParser() {
    /*
     * it looks to see if we have a time limitation
     * if we have time limitation it fills up the container accordingly
     *
     * example: timeAllowed=1000
     * unit is millisec.
     */
    Logger::debug("inside timeAllowedParameterParser function");
    const char * timeAllowedTemp = evhttp_find_header(&headers,
            QueryParser::timeAllowedParamName);
    if (timeAllowedTemp) { // if this parameter exists
        Logger::debug("timeAllowed parameter found, parsing it.");
        string timeAllowedStr;
        decodeString(timeAllowedTemp, timeAllowedStr);
// convert the Str to integer.
        if (isUnsigned(timeAllowedStr)) {
            this->container->maxTimeAllowed = static_cast<unsigned int>(strtoul(
                    timeAllowedStr.c_str(), NULL, 10));
            // populate the parametersInQuery
            this->container->parametersInQuery.push_back(MaxTimeAllowed);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "timeAllowed parameter should be a positive integer"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returning from timeAllowedParameterParser function");
}

void QueryParser::omitHeaderParameterParser() {
    /*
     * it looks to see if we have a omit header
     * if we have omit header it fills up the container accordingly.
     *
     * example: omitHeader=True
     */
    Logger::debug("inside omitHeaderParameterParser function");
    const char * ommitHeaderTemp = evhttp_find_header(&headers,
            QueryParser::ommitHeaderParamName);
    if (ommitHeaderTemp) { // if this parameter exists
        Logger::debug("ommitHeaderTemp parameter found, parsing it.");
        string ommitHeader;
        decodeString(ommitHeaderTemp, ommitHeader);
// check if "true"
        if (boost::iequals("true", ommitHeader)) {
            this->container->isOmitHeader = true;
        } else {
            this->container->isOmitHeader = false; // this is default.
        }
// populate the parametersInQuery
        this->container->parametersInQuery.push_back(IsOmitHeader); // should we change this ParameterName to OmitHeader?
    }
    Logger::debug("returning from omitHeaderParameterParser function");
}

void QueryParser::responseWriteTypeParameterParser() {
    /*
     * it looks to see if we have a responce type
     * if we have reponce type it fills up the container accordingly.
     * exmple: wt=JSON
     */
    Logger::debug("inside responseWriteTypeParameterParser function");
    const char * responseWriteTypeTemp = evhttp_find_header(&headers,
            QueryParser::responseWriteTypeParamName);
    if (responseWriteTypeTemp) { // if this parameter exists
        Logger::debug("responseWriteTypeParameter found, parsing it");
        string responseWriteType;
        decodeString(responseWriteTypeTemp, responseWriteType);
// check if "true"
        if (boost::iequals("json", responseWriteType)) {
            this->container->responseResultsFormat = JSON;
        } else {
            // create warning, we only support json as of now.
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "Unknown value for parameter wt. using wt=json"));
            this->container->responseResultsFormat = JSON; // this is default.
        }
// populate the parametersInQuery
        this->container->parametersInQuery.push_back(ResponseFormat); // should we change this ParameterName to OmitHeader?
    }
    Logger::debug("returning from responseWriteTypeParameterParser function");
}

bool QueryParser::filterQueryParameterParser() {
    /*
     * it looks to see if there is any post processing filter
     * if there is then it fills up the container accordingly
     *
     * example: 'fq=price:[10 TO 100] AND popularity:[* TO 100] AND Title:algorithm'
     *
     */
    Logger::debug("inside filterQueryParameterParser function");
    const char* key = QueryParser::filterQueryParamName;
    const char* fqTmp = evhttp_find_header(&headers, key);
    // read fq from headers
    if (fqTmp) {
        Logger::debug("filterQueryParameter found.");
        string fq;
        decodeString(fqTmp, fq);
        // create filterQueryContainer object.
        FilterQueryContainer* filterQueryContainer = new FilterQueryContainer();
        // create FilterQueryEvaluator object, this will parse the fq string
        FilterQueryEvaluator* fqe = new FilterQueryEvaluator(
                &this->container->messages);
        filterQueryContainer->evaluator = fqe;
        this->container->filterQueryContainer = filterQueryContainer;
        this->container->parametersInQuery.push_back(FilterQueryEvaluatorFlag);
        boost::algorithm::trim(fq);
        Logger::debug("parsing fq %s", fq.c_str());
        if (!fqe->parseAndAddCriterion(fq)) {
            this->isParsedError = true;
            return false;
        }
    }
    Logger::debug("returning from filterQueryParameterParser function");
    return true;
}

bool QueryParser::queryFieldBoostParser() {
    /*
     * it looks to see if there is any post processing dynamic boosting
     * if there is then it fills up the container accordingly
     *
     * example: 'qf=price^100+popularity^100'
     *
     */
    Logger::debug("inside queryFieldBoostParser function");
    const char* key = QueryParser::queryFieldBoostParamName;
    const char* qfTmp = evhttp_find_header(&headers, key);
    // read qf from headers
    if (qfTmp) {
        Logger::debug("queryFieldBoostParam found.");
        string qfString;
        decodeString(qfTmp, qfString);
        // create filterQueryContainer object.
        QueryFieldBoostContainer* qfboost= new QueryFieldBoostContainer();

        this->container->qfContainer= qfboost;
        this->container->parametersInQuery.push_back(QueryFieldBoostFlag);
        boost::algorithm::trim(qfString);
        Logger::debug("parsing qf %s", qfString.c_str());
        if (!QueryFieldBoostParser::parseAndAddCriterion(*qfboost, qfString)) {
            this->isParsedError = true;
            return false;
        }
    }
    Logger::debug("returning from quertFieldBoostParser function");
    return true;
}

void QueryParser::facetParser() {
    /*
     * example:  facet=true&facet.field=filedName&facet.field=fieldName2&facet.range=fieldName3&f.fieldName3.range.start=10&f.fieldName3.range.end=100&f.fieldName3.range.gap=10
     * parses the parameters facet=true/false , and it is true it parses the rest of
     * parameters which are related to faceted search.
     */
    Logger::debug("inside facetParser function");
    const char * facetTemp = evhttp_find_header(&headers,
            QueryParser::facetParamName);
    if (facetTemp) { // if this parameter exists
        Logger::debug("facet parameter found, parsing it");
        string facet;
        decodeString(facetTemp, facet);
        // we have facet,
        //parse other facet related parameters if this is true
        if (boost::iequals("true", facet) || boost::iequals("only", facet)) {
            // facet param is true
            FacetQueryContainer *fqc = new FacetQueryContainer();
            populateFacetFieldsSimple(*fqc);
            populateFacetFieldsRange(*fqc);
            //// set the parametersInQuery
            this->container->parametersInQuery.push_back(FacetQueryHandler);
            this->container->facetQueryContainer = fqc;
            if( boost::iequals("only", facet)){
                this->container->onlyFacets = true;
            }

            // parse other facet fields
        }else if (boost::iequals("false", facet)) {
            // facet is off. no need to parse any facet params
        } else {
            // unkonown value. raise warning and set facet to false
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "Unknown value for facet paramter, setting facet to false"));
        }
    }
    Logger::debug("returning from facetParser function");
}

void QueryParser::sortParser() {
    /*
     * example: sort=field1,field2,field3
     * looks for the parameter sort which defines the post processing sort job
     */
    Logger::debug("inside sortParser function");
    const char * sortTemp = evhttp_find_header(&headers,
            QueryParser::sortParamName);
    if (sortTemp) { // if this parameter exists
        Logger::debug("sort parameter found, parsing it.");
        string sortString;
        decodeString(sortTemp, sortString);
        // we have sortString, we need to tokenize this string and populate the
        // parameters in SortQueryEvaluator class object.
        vector<string> fieldTokens;
        boost::split(fieldTokens, sortString,
                boost::is_any_of(sortFiledsDelimiter));
        if (!fieldTokens.empty()) {
            SortQueryContainer *sortQueryContainer = new SortQueryContainer();
            sortQueryContainer->evaluator = new SortFilterEvaluator();
            sortQueryContainer->evaluator->field = fieldTokens;
            string order = this->orderByParser();
            if (order.compare("") == 0) {
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::SortOrderNotSpecified;
            } else if (boost::iequals(orderAscending, order)) {
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::SortOrderAscending;
            } else if (boost::iequals(orderDescending, order)) {
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::SortOrderDescending;
            } else {
                // raise warning value not known. using ORDER_NOT_SPECIFIED
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::SortOrderNotSpecified;
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                "Unknown order value. using order from config file"));
            }
            // set the parametersInQuery
            this->container->parametersInQuery
                    .push_back(SortQueryHandler);
            this->container->sortQueryContainer = sortQueryContainer;
        }
    }
    Logger::debug("returning from sortParser function");
}

string QueryParser::orderByParser() {
    /*
     * example: orderby=desc
     * looks for the parameter orderby in headers and parses it.
     */
    Logger::debug("inside orderByParser function");
    const char * orderTemp = evhttp_find_header(&headers,
            QueryParser::orderParamName);
    string order = "";
    if (orderTemp) { // if this parameter exists
        Logger::debug("orderBy Parameter found, parsing it.");
        decodeString(orderTemp, order);
    }
    Logger::debug("returning from orderByParser function");
    return order;
}

bool QueryParser::localParameterParser(string &input) {
    /*
     * this function parses the local parameters section of all parts
     * input:
     *      1. local parameters string : '{defaultSearchFields=Author defaultSimilarityThreshold=0.7}gnuth'
     * output:
     *      1. it fills up the metadata of the queryHelper object
     */
    Logger::debug(
            "inside localParameterParser parsing the localparameters in the input string %s",
            input.c_str());
    if (input.at(0) == '{') {
        Logger::debug("localparamter string found, extracting it.");
        input = input.substr(1); // input is modifed to 'defaultSearchFields=Author defaultSimilarityThreshold=0.7}gnuth'
        // TODO: do a memory profiling to see if substr is cpu costly. Yes? use unsigned as a cursor over the input string.
        string lpField = "";
        while (parseLpKey(input, lpField)) {
            if (parseLpDelimeter(input)) {
                string lpValue = "";
                if (parseLpValue(input, lpValue)) {
                    // looping first time
                    // acoording to example lpField should be 'defaultSearchFields'
                    // and lpKey should be 'Author'
                    this->setLpKeyValinContainer(lpField, lpValue);
                } else {
                    this->container->messages.push_back(
                            make_pair(MessageError,
                                    "Parse error, invalid local parameter string. Found malformed value for localParameter key "
                                            + lpField));
                    this->isParsedError = true;
                    return false;
                }
            } else {
                this->container->messages.push_back(
                        make_pair(MessageError,
                                "Parse error, invalid local parameter string. Expecting '=', after localParameter key "
                                        + lpField));
                this->isParsedError = true;
                return false;
            }
        }
        boost::algorithm::trim(input);
        // now the input should be '}gnuth'
        // removing the leading '}'
        if (input.at(0) == '}') {
            input = input.substr(1);
            boost::algorithm::trim(input);
            // input has been modified to 'gnuth'
        } else {
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "Parse error, invalid local parameter string. closing '}' not found"));
            this->isParsedError = true;
            return false;
        }
    } else {
        // no lp present.
        Logger::debug(
                "no localparameter info to parse in the input string. returning");
    }
    Logger::debug("returning from localParameterParser");
    return true;
}
/*
 * sets the lp key and lp value in the container.
 * @lpKey a localparameter key
 * @lpVal a localparameter value
 * this functions checks for the validity of lpVal based on the lpKey.
 * if the lpKey and lpVal are have valid syntax it sets the corresponding variable in the queryParser.
 * else, it raises warning and ignores the lpKey and lpVal
 * We support following lpKeys
 * 1) "defaultFieldOperator"
 * 2) "defaultSimilarityThreshold"
 * 3) "defaultBoostLevel"
 * 4) "defaultPrefixComplete"
 * 5) "defaultSearchFields"
 *
 * Example: lpKey = 'defaultSearchFields' and lpVal= 'Author'
 */
void QueryParser::setLpKeyValinContainer(const string &lpKey,
        const string &lpVal) {
    if (0 == lpKey.compare(QueryParser::lpQueryBooleanOperatorParamName)) { // default Boolean operator to be used for the fields in the query terms
        if (boost::iequals("OR", lpVal)) {
            this->lpFieldFilterBooleanOperator =
                    srch2::instantsearch::BooleanOperatorOR;
        } else if (boost::iequals("AND", lpVal)) {
            this->lpFieldFilterBooleanOperator =
                    srch2::instantsearch::BooleanOperatorAND;
        } else {
            // generate MessageWarning and use AND
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "Invalud boolean operator specified " + lpVal
                                    + ", ignoring it and using AND."));
            this->lpFieldFilterBooleanOperator =
                    srch2::instantsearch::BooleanOperatorAND;
        }
        this->isLpFieldFilterBooleanOperatorAssigned = true;
    } else if (0 == lpKey.compare(lpKeywordSimilarityThresholdParamName)) {
        if (isFloat(lpVal)) {
            float f = static_cast<float>(strtod(lpVal.c_str(),NULL)); //TODO: add the validation
            this->lpKeywordSimilarityThreshold = f;
        } else {
            //warning
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            string(lpKeywordSimilarityThresholdParamName)
                                    + " should be a valid float number. Ignoring it."));
        }
    } else if (0 == lpKey.compare(lpKeywordBoostLevelParamName)) {
        if (isUnsigned(lpVal)) {
            int boostLevel = static_cast<int>(strtol(lpVal.c_str(),NULL,10));
            this->lpKeywordBoostLevel = boostLevel;
        } else {
            //warning
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            string(lpKeywordBoostLevelParamName)
                                    + " should be a valid non negetive integer. Ignoring it."));
        }
    } else if (0 == lpKey.compare(lpKeywordPrefixCompleteParamName)) {
        this->setInQueryParametersIfNotSet(QueryPrefixCompleteFlag);
        if (boost::iequals("PREFIX", lpVal)) {
            this->lpKeywordPrefixComplete =
                    srch2::instantsearch::TERM_TYPE_PREFIX;
        } else if (boost::iequals("COMPLETE", lpVal)) {
            this->lpKeywordPrefixComplete =
                    srch2::instantsearch::TERM_TYPE_COMPLETE;
        } else {
            // generate MessageWarning and use prefix
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "Invalid choice " + lpVal
                                    + ",we support prefix and complete search on keyword only. ignoring it and using the default value from config file."));
            this->lpKeywordPrefixComplete =
                    srch2::instantsearch::TERM_TYPE_NOT_SPECIFIED;
        }
    } else if (0 == lpKey.compare(lpFieldFilterParamName)) {
        // val might be a comma separated string of fields.  Author,Name..
        // tokenize it on ',' and set the vector in container.
        vector<string> fieldTokens;
        boost::split(fieldTokens, lpVal,
                boost::is_any_of(lpFieldFilterDelimiter));
        for (std::vector<string>::iterator fieldTokenItr = fieldTokens.begin();
                fieldTokenItr != fieldTokens.end(); ++fieldTokenItr) {
            string sToken = *fieldTokenItr;
            boost::algorithm::trim(sToken); // trim the token
            this->lpFieldFilter.push_back(sToken); // set field in container
        }
    } else {
        // this localparameter is not supported. raise a MessageWarning msg.
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "Invalid local parameter " + lpKey + ", ignoring it."));
    }
}
bool QueryParser::termParser(string &input) {
    /*
     * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
     * example: 'Author:gnuth^3 AND algorithms AND java* AND py*^3 AND binary^2~.5'
     * output: fills up the container
     */
    Logger::debug("inside term parser.");
    Logger::debug("input received is %s", input.c_str());
    /*
     *
     */
    if (input.length() == 0) {
        Logger::info("PARSE ERROR, returning from  keywordParser.");
        this->container->messages.push_back(
                make_pair(MessageError,
                        "Parse error, expecting keyword, not found "));
        this->isParsedError = true;
        return false;
    }
    string field = "";
    // example input is  'Author:gnuth^3 AND algorithms AND java* AND py*^3 AND binary^2~.5'
    // get the field part of the term.
    // a term is 'Author:gnuth', field part of term is 'Author' and keyword part of term is 'gnuth'
    bool hasParsedParameter = this->parseField(input, field);
    if (!hasParsedParameter) {
        // no field found, this can happen if the term contains only
        // the keyword part. like 'algorithm'
        // in this case, we will populate the field using the localParameter values.
        // the execution can come here also if the field syntax provided is not correct.
        // the error will be catched later
        Logger::debug(
                "no fieldFilter present in query, looking into localparameter for field filter");
        this->populateFieldFilterUsingLp();
    } else {

        // field is found. this can happen in the case 'Author:gnuth', here 'Author:' is the field
        // populate the field
        // remove the trailing :
        field = field.substr(0, field.length() - 1); // field is now 'Author' earlier it was 'Author:'
        Logger::debug("field filter found %s ", field.c_str());
        // populate the container's fieldFilter Vector using this field
        this->populateFieldFilterUsingQueryFields(field);
    }
    // now get the keyword part of the term
    /*
     * but first check if it can be a phrase search.
     *
     */
    string keywordStr = ""; // this will store the parsed keyword.
    bool isPhraseKeyword = false;
    if (input.length() == 0) {
    	string err = "Parse error: no keyword found after the field = " + field;
    	this->container->messages.push_back(
    			make_pair(MessageError, err));
    	this->isParsedError = true;
    	return false;
    }
    if (input.at(0) == '"') {  // check if the keyword starts with a quote
        /* see if there is another '"'
         * true: remove the leading and trailing '"'
         *      mark a flag for phrase search.
         * false:
         *      raise an error.
         */
        Logger::debug("can be  a possible phrase search");
        input = input.substr(1); // remove the leading '"'
        // find the occurence of '"' and get the position.
        // check if input ends with '"'
        size_t found = input.find('"');
        if (found != string::npos) {
            Logger::debug("\" found at position %d in the input string %s",
                    found, input.c_str());
            // get the keywrod string till this position
            keywordStr = input.substr(0, found);
            // modify the input string
            input = input.substr(found + 1); // +1 to remove the '"'
            // set the isPhraseVector for this keyword
            isPhraseKeyword = true;
            hasParsedParameter = true; // signifies we have parsed the keyword
            this->setInQueryParametersIfNotSet(IsPhraseKeyword);
        } else {
            // raise error.
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "Parse error, single quote found while parsing q for keyword"));
            this->isParsedError = true;
            return false;
        }

    } else {
        hasParsedParameter = parseKeyword(input, keywordStr); // keywordStr will contain the parsed keyword, input will be modified
    }
    this->isPhraseKeywordFlags.push_back(isPhraseKeyword);
    if (hasParsedParameter) {
        // keyword obtained, get modifiers, keywordStr is 'gnuth'
        // populate the rawKeyword vector in container
        // input is modified to 'AND algorithms AND java* AND py*^3 AND binary^2~.5'
        this->populateRawKeywords(keywordStr);
        // separate for each . *, ^ and ~
    } else {
        // check if they keyword is just *
        string asteric = "";
        hasParsedParameter = parseKeyWordForAsteric(input, asteric);
        if (hasParsedParameter) {
            this->populateRawKeywords(asteric);
        } else {
            // raise error, invalid query, keyword expected, not found
            Logger::info("PARSE ERROR, returning from  keywordParser.");
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "Parse error, expecting keyword, not found "));
            this->isParsedError = true;
            return false;
        }
    }
    if (!isPhraseKeyword) {
        // not a pharse keyword, check if prefix modifier is persent.
        // check for prefix modifier, i.e. '*
        string prefixModifier = "";
        hasParsedParameter = parsePrefixModifier(input, prefixModifier);
        if (hasParsedParameter) {
            this->setInQueryParametersIfNotSet(QueryPrefixCompleteFlag);
            // '*' is present
            Logger::debug("prefix modifier used in query");
            this->keywordPrefixComplete.push_back(
                    srch2::instantsearch::TERM_TYPE_PREFIX); // use the lp variable value
        } else {
            // use the fallback specified in localparameters.
            this->keywordPrefixComplete.push_back(
                    this->lpKeywordPrefixComplete);
        }
    } else {
    	// if phrase term "A B " then treat it as complete. The also keeps keywordPrefixComplete
    	// vector's indices consistent.
    	this->keywordPrefixComplete.push_back(
    	                    srch2::instantsearch::TERM_TYPE_COMPLETE);
    }
    // check for boost modifier, i.e. '^'
    string boostModifier = "";
    hasParsedParameter = parseBoostModifier(input, boostModifier);
    this->populateBoostInfo(hasParsedParameter, boostModifier);
    // check for fuzzy modifier. i.e. '~'
    string fuzzyModifier = "";
    if (!isPhraseKeyword) {
        hasParsedParameter = parseFuzzyModifier(input, fuzzyModifier);
        this->populateFuzzyInfo(hasParsedParameter, fuzzyModifier);
        this->PhraseSlops.push_back(0); // no slop for non phrase keywords
    } else {
        hasParsedParameter = parseProximityModifier(input, fuzzyModifier);
        this->populateProximityInfo(hasParsedParameter, fuzzyModifier);
        this->keywordSimilarityThreshold.push_back(0.0f);
    }
    Logger::debug("returning from the termParser.");
    return true;

}

void QueryParser::populateTermBooleanOperator(const string &termOperator) {
    /*
     * populates the termBooleanOperators in the container
     * checks if the termOperator is one of 'OR,||,AND,&&'
     *  true: sets the container's termBooleanOperator to the corresponding enum.
     *  function raises warning if termOperator is anything other than 'AND' or '&&'.
     *  and sets the termBooleanOperator to BooleanOperatorAND enum value.
     */
    Logger::debug("inside  populateTermBooleanOperator.");
    if (boost::iequals("OR", termOperator)) {
        // we do not support OR as of now so raising a MessageWarning and setting it to AND.
        // generate MessageWarning and use AND
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "We do not supprt OR  specified, ignoring it and using 'AND'."));
        this->container->termBooleanOperator =
                srch2::instantsearch::BooleanOperatorAND;
    } else if (boost::iequals("AND", termOperator)) {
        this->container->termBooleanOperator =
                srch2::instantsearch::BooleanOperatorAND;
    } else {
        // generate MessageWarning and use AND
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "Invalid boolean operator specified as term boolean operator "
                                + termOperator
                                + ", ignoring it and using 'AND'."));
        this->container->termBooleanOperator =
                srch2::instantsearch::BooleanOperatorAND;
    }
    Logger::debug("returning from populateTermBooleanOperator.");
}

void QueryParser::populateRawKeywords(const string &input) {
    /*
     * populates the raw keywords, that is the keyword without modifiers.
     * modifiers are *,^ and ~.
     * example: keyword^3 has keyword as a rawkeyword. this function populates the RawKeywords vector in container.
     */
    Logger::debug("indide populateRawKeywords, parsing for raw keywords");
    this->setInQueryParametersIfNotSet(RawQueryKeywords);
    Logger::debug("raw keyword found: %s", input.c_str());
    this->rawQueryKeywords.push_back(input);
    Logger::debug("returning from populateRawKeywords");
}

void QueryParser::checkForBoostNums(const string &input,
        boost::smatch &matches) {
    /*
     * checks if boost value is present in the input
     * example: keyword^4 has boost value 4. keyword^ has no boost value
     */
    Logger::debug("inside checkForBoostNums");
    boost::regex boostRegex(BOOST_REGEX_STRING); // TODO: for all these functions compile the regex when the engine starts.
    boost::regex_search(input, matches, boostRegex);
    Logger::debug("returning from checkForBoostNums");
}
void QueryParser::checkForFuzzyNums(const string &input,
        boost::smatch &matches) {
    /*
     * checks if the SimilarityThreshold is present in the input string
     * example: keyword~0.8 has SimilarityThreshold as 0.8. keyword~ has no SimilarityThreshold specified.
     * input is ~0.8 or ~
     */
    Logger::debug("inside checkForFuzzyNums");
    boost::regex re(CHECK_FUZZY_NUMBER_REGEX_STRING);
    boost::regex_search(input, matches, re);
    Logger::debug("returning from checkForFuzzyNums");
}
void QueryParser::extractNumbers(const string &input, boost::smatch& matches) {
    /*
     * extracts the numbers from the input string
     * example:  it will extract '8' from '~0.8'.
     */
    Logger::debug("inside extractNumbers");
    boost::regex re(NUMBERS_REGEX_STRING);
    boost::regex_search(input, matches, re);
    Logger::debug("returning from extractNumbers");
}
void QueryParser::populateFieldFilterUsingLp() {
    /*
     * populates the fieldFilter using localparamters.
     * example: q=keyword , has no fieldFilter specified. it will look into the lpFieldFilters for the
     * fallback and use that to populate the fieldFilter for this keyword
     */
    Logger::debug("inside populateFieldFilterUsingLp");
// check if lpFieldFilter is set in container
    if (!this->lpFieldFilter.empty()) {
        // lpFieldFilter is set. use this to create a vector
        this->setInQueryParametersIfNotSet(FieldFilter);
        this->fieldFilter.push_back(this->lpFieldFilter);
        if (this->isLpFieldFilterBooleanOperatorAssigned) {
            // LpFieldFilterBooleanOperator is assigned. fill
            this->fieldFilterOps.push_back(
                    this->lpFieldFilterBooleanOperator);
        } else {
            this->fieldFilterOps.push_back(
                    srch2::instantsearch::OP_NOT_SPECIFIED);
        }
    } else {
        this->fieldFilter.push_back(vector<string>());
        this->fieldFilterOps.push_back(
                srch2::instantsearch::OP_NOT_SPECIFIED);
    }
    Logger::debug("returning from populateFieldFilterUsingLp");
}

void QueryParser::populateFieldFilterUsingQueryFields(const string &input) { // TODO: note what is input
    /*
     * check if '.'s are present
     * check if '+' are present tokenize on . or + and
     * populate a vector<string> fields
     * populate the fieldFilterOps with given boolean operator
     */
    Logger::debug("inside populateFieldFilterUsingQueryFields");
    this->setInQueryParametersIfNotSet(FieldFilter);
    string fieldBoolOpDelimeterRegexString;
    if (input.find('.') != string::npos) {
        fieldBoolOpDelimeterRegexString =
                FIELD_AND_BOOL_OP_DELIMETER_REGEX_STRING;
        this->fieldFilterOps.push_back(
                srch2::instantsearch::BooleanOperatorAND);
    } else if (input.find('+') != string::npos) {
        fieldBoolOpDelimeterRegexString =
                FIELD_OR_BOOL_OP_DELIMETER_REGEX_STRING;
        this->fieldFilterOps.push_back(
                srch2::instantsearch::BooleanOperatorOR);
    } else {
        // no boolean operators here.
        // create a vector and add it to the container.
        vector<string> candidate;
        candidate.push_back(input); // this candidate can be any alphanumeric or *.
        this->fieldFilter.push_back(candidate);
        // fill the corresponding fieldFilterOps parallel vector
        this->fieldFilterOps.push_back(
                srch2::instantsearch::OP_NOT_SPECIFIED);
        Logger::debug(
                "returning from  populateFieldFilterUsingQueryFields as no boolean operators found");
        return;
    }
// fill the corresponding fieldFilter parallel vector
    const boost::regex fieldDelimeterRegex(fieldBoolOpDelimeterRegexString);
    boost::sregex_iterator fieldItr(input.begin(), input.end(),
            fieldDelimeterRegex);
    boost::sregex_iterator fieldEndItr;
    vector<string> fields;
    size_t start = 0;
    for (; fieldItr != fieldEndItr; ++fieldItr) {
        size_t len = (*fieldItr).position() - start;
        string field = input.substr(start, len);
        fields.push_back(field);
        start = (*fieldItr).position() + (*fieldItr).length();
    }
    fields.push_back(input.substr(start)); // push back the last field in the string.
// push back the fields vector in container.
    this->fieldFilter.push_back(fields);
    Logger::debug("returning from populateFieldFilterUsingQueryFields");
}

void QueryParser::topKParameterParser() {
    /*
     * this function parsers only the parameters which are specific to Top-K
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     *
     */
    this->facetParser();
    this->sortParser();
}

void QueryParser::getAllResultsParser() {
    /*
     * this function parsers only the parameters which are specific to get all results search type
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     */
    this->facetParser();
    this->sortParser();
}

void QueryParser::geoParser() {
	/*
	 * extractSearchType function parsers the parameters related to geo search like latitude and longitude .
	 *      if lat/long query params are specified its a geo
     *      parses the geo parameters like leftBottomLatitude,leftBottomLongitude,rightTopLatitude,rightTopLongitude
     *      centerLatitude,centerLongitude,radius
     *      Based on what group of geo parameters are present it sets geoType to CIRCULAR or RECTANGULAR
     *
     *      example query:   http://localhost:8087/search?q=hospital&lblat=135.34&lblong=135.34&rtlat=180&rtlong=180
	 */
	Logger::debug("inside gepParser function");

	Logger::debug("inside geoParser, checking for geo parameter");
	const char * leftBottomLatTemp = evhttp_find_header(&headers,
			QueryParser::leftBottomLatParamName);
	const char * leftBottomLongTemp = evhttp_find_header(&headers,
			QueryParser::leftBottomLongParamName);
	const char * rightTopLatTemp = evhttp_find_header(&headers,
			QueryParser::rightTopLatParamName);
	const char * rightTopLongTemp = evhttp_find_header(&headers,
			QueryParser::rightTopLongParamName);
	if (leftBottomLatTemp && leftBottomLongTemp && rightTopLatTemp
			&& rightTopLongTemp) {
		// we have geo input and it's a reactangular geo search
		this->container->parametersInQuery.push_back(GeoSearchFlag);
		this->container->geoParameterContainer = new GeoParameterContainer();
		this->container->geoParameterContainer->parametersInQuery.push_back(
				GeoTypeRectangular);
		//set GeoParameterContainer properties.
		this->setGeoContainerProperties(leftBottomLatTemp, leftBottomLongTemp,
				rightTopLatTemp, rightTopLongTemp);
	} else {
		const char * centerLatTemp = evhttp_find_header(&headers,
				QueryParser::centerLatParamName);
		const char * centerLongTemp = evhttp_find_header(&headers,
				QueryParser::centerLongParamName);
		const char * radiusParamTemp = evhttp_find_header(&headers,
				QueryParser::radiusParamName);
		if (centerLatTemp && centerLongTemp && radiusParamTemp) {
			// we have geo input and its a circular geo search
			this->container->parametersInQuery.push_back(GeoSearchFlag);
			this->container->geoParameterContainer =
					new GeoParameterContainer();
			this->container->geoParameterContainer->parametersInQuery.push_back(
					GeoTypeCircular);
			//set GeoParameterContainer properties.
			this->setGeoContainerProperties(centerLatTemp, centerLongTemp,
					radiusParamTemp);
		}
	}
	// TODO : Jamshid : if only a subset of values are provided errosr ....
	Logger::debug("returning from geoParser");
}
void QueryParser::extractSearchType() {
    /*
     *  figures out what is the searchtype of the query. No need of passing the searchType parameter anymore in lp.
     *  if sort|facet are specified, its a getAllResult
     *  else:
     *  it's a Top-K
     *  set the parametersInQuery.
     */
// if serachType mentioned in queryParameter use that.
	// TODO : Jamshid : make these constants....
    const string getAllType = "getAll";
    const string topKType = "topK";
    Logger::debug("inside extractSearchType function");
    const char * searchTypeTmp = evhttp_find_header(&headers,
            QueryParser::searchType);
    string searchType = "";
    if (searchTypeTmp) { // if this parameter exists
        Logger::debug("searchType found, parsing it");
        this->isSearchTypeSet = true;
        decodeString(searchTypeTmp, searchType);
    }
    // else extrct it. if no search type is given and cannot decide between topk and getAll, use topK, raise a warning
    if (this->isSearchTypeSet) {
    	if (boost::iequals(getAllType, searchType)) { // search type is given and it's getAll
    		// it's a getAll
    		this->container->parametersInQuery.push_back(
    				GetAllResultsSearchType);
    	} else if (boost::iequals(topKType, searchType)) { // search type is given and it's topK
    		// it's a Top-K search
    		this->container->parametersInQuery.push_back(
    				TopKSearchType);
    	} else {
    		// searchType provided is not known, fall back to top-k
    		this->container->messages.push_back(
    				make_pair(MessageWarning,
    						"Unknown searchType " + searchType
    						+ " provided, falling back to topK"));
    		this->container->parametersInQuery.push_back(
    				TopKSearchType);
    	}
    } else { // search type is not given by the user, and there is no post processing task either
    	// no searchType provided use topK
    	this->container->messages.push_back(make_pair(MessageNotice, "topK query"));
    	this->container->parametersInQuery.push_back(TopKSearchType);
    }
    Logger::debug("returning from extractSearchType");
}
void QueryParser::setGeoContainerProperties(const char* leftBottomLat,
        const char* leftBottomLong, const char* rightTopLat,
        const char* rightTopLong) {
    /*
     * sets the rectangular geo paramters in the geocontainer.
     */
    Logger::debug("inside extrasetGeoContainerProperties");
    size_t st1, st2, st3, st4;
    string leftBottomLatStr;
    decodeString(leftBottomLat, leftBottomLatStr);
    string leftBottomLongStr;
    decodeString(leftBottomLong, leftBottomLongStr);
    string rightTopLatStr;
    decodeString(rightTopLat, rightTopLatStr);
    string rightTopLongStr;
    decodeString(rightTopLong, rightTopLongStr);
// convert the rowsStr to integer.
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->leftBottomLatitude = static_cast<float>(strtod(
                leftBottomLatStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "lblat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->leftBottomLongitude = static_cast<float>(strtod(
                leftBottomLongStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "lblong should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLatitude = static_cast<float>(strtod(
                rightTopLatStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "rtlat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLongitude = static_cast<float>(strtod(
                rightTopLongStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "rtlong should be a valid float number"));
        this->isParsedError = true;
    }

    Logger::debug("returning from extrasetGeoContainerProperties");
}

void QueryParser::setGeoContainerProperties(const char* centerLat,
        const char* centerLong, const char* radiusParam) {
    /*
     * sets the circular geo paramters in the geocontainer.
     */
    Logger::debug("inside setGeoContainerProperties");
    size_t st1, st2, st3;
    string centerLatStr;
    decodeString(centerLat, centerLatStr);
    string centerLongStr;
    decodeString(centerLong, centerLongStr);
    string radiusParamStr;
    decodeString(radiusParam, radiusParamStr);
// convert the rowsStr to integer.
    if (isFloat(centerLatStr)) {
        this->container->geoParameterContainer->centerLatitude = static_cast<float>(strtod(
                centerLatStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError, "clat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(centerLongStr)) {
        this->container->geoParameterContainer->centerLongitude = static_cast<float>(strtod(
                centerLongStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "clong should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(radiusParamStr)) {
        this->container->geoParameterContainer->radius = static_cast<float>(strtod(
                radiusParamStr.c_str(),NULL)); // convert the string to char* and pass it to strtod
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "radius should be a valid float number"));
        this->isParsedError = true;
    }
    Logger::debug("returning from setGeoContainerProperties");
}
void QueryParser::setInQueryParametersIfNotSet(ParameterName param) {
    /*
     * checks is a paramter is set in the container's parametersInQuery. if not, it sets it.
     */
    Logger::debug("inside setInQueryParametersIfNotSet");
    if (!this->container->hasParameterInQuery(param)) {
        Logger::debug("parameter not in parametersInQuery, setting it.");
        this->container->parametersInQuery.push_back(param);
    }
    Logger::debug("returning from setInQueryParametersIfNotSet");
}
void QueryParser::setSimilarityThresholdInContainer(const float f) {
    /*
     * sets the SimilarityThreshold in the container->keywordSimilarityThreshold variable.
     *  check isFuzzy
     *            true -> use the SimilarityThreshold as specified
     *            else -> set 0.0 as SimilarityThreshold
     */
    Logger::debug("inside setSimilarityThresholdInContainer");
// this is set, fuzzyFlag came from  query parameter.
    if (this->container->isFuzzy) {
        // use the SimilarityThreshold provided with keyword
        this->keywordSimilarityThreshold.push_back(f);
    } else {
        // Similarity threshold is not specified, use 1
        this->keywordSimilarityThreshold.push_back(1.0f);
    }
    Logger::debug("Similarity threshold is not specified, use 1");
}
/*
 * parses the localparameter key from input string. It changes input string and populates the field
 * example. for input: 'defaultSearchFields = Author defaultBoostLevel = 2}' ,the paramter field will be
 * 'defaultSearchFields' and the input will be changed to '= Author defaultBoostLevel = 2}'
 */
bool QueryParser::parseLpKey(string &input, string &field) {
    boost::regex re(LP_KEY_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, field);
}
bool QueryParser::parseLpDelimeter(string &input) {
    boost::regex re(LP_KEY_VAL_DELIMETER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    string delimeter = "";
    return doParse(input, re, delimeter);
}
bool QueryParser::parseLpValue(string &input, string &value) {
    boost::regex re(LP_VALUE_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, value);
}

bool QueryParser::parseTermBoolOperator(string &input, string &output) {
    boost::regex re(TERM_BOOL_OP_REGEX_STRING); //TODO: compile this regex when the engine starts.
    boost::algorithm::trim(input);
    bool isParsed = doParse(input, re, output);
    if (!this->container->isTermBooleanOperatorSet && isParsed) {
        this->populateTermBooleanOperator(output);
        this->container->isTermBooleanOperatorSet = true;
    }
    return isParsed;
}

bool QueryParser::parseField(string &input, string &field) {
    boost::regex re(MAIN_QUERY_TERM_FIELD_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, field);
}
bool QueryParser::parseKeyword(string &input, string &output) {
    boost::regex re(MAIN_QUERY_KEYWORD_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}

bool QueryParser::parseKeyWordForAsteric(string &input, string &output) {
    boost::regex re(MAIN_QUERY_ASTERIC_KEYWORD_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}
bool QueryParser::parsePrefixModifier(string &input, string &output) {
    boost::regex re(PREFIX_MODIFIER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}
bool QueryParser::parseBoostModifier(string &input, string &output) {
    boost::regex re(BOOST_MODIFIER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);

}
void QueryParser::populateBoostInfo(bool isParsed, string &input) {
    if (isParsed) {
        Logger::debug("boost modifier used in query");
        boost::smatch matches;
        this->setInQueryParametersIfNotSet(KeywordBoostLevel);
        this->checkForBoostNums(input, matches); // check if boost value is present
        if (matches[0].matched) {
            // get the boost value;
            Logger::debug("boost value is specified, extracting it.");
            boost::smatch numMatches;
            this->extractNumbers(matches[0].str(), numMatches);
            unsigned boostNum = static_cast<unsigned int>(strtoul(
                    numMatches[0].str().c_str(), NULL, 10));
            Logger::debug("boost value is %d", boostNum);
            this->keywordBoostLevel.push_back(boostNum); // push to the container.
        } else {
            // there is no value specified
            Logger::debug(
                    "boost value is not specified, using the lp value or -1");
            this->keywordBoostLevel.push_back(
                    this->lpKeywordBoostLevel); // sets the localParameter specified value. it's initial value is -1.
        }
    } else {
        Logger::debug("boost value is not specified, using the lp value or -1");
        this->keywordBoostLevel.push_back(this->lpKeywordBoostLevel); // selts the localParameter specified value
    }
}
bool QueryParser::parseFuzzyModifier(string &input, string &output) {
    boost::regex re(FUZZY_MODIFIER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}
bool QueryParser::parseProximityModifier(string &input, string &output) {
    boost::regex re(PROXIMITY_MODIFIER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}
void QueryParser::populateFuzzyInfo(bool isParsed, string &input) {
    if (isParsed) {
        Logger::debug("fuzzy modifier used in query");
        this->setInQueryParametersIfNotSet(KeywordSimilarityThreshold);
        /* Test if input is string "~" */
        if (input.length() != 1) {
            // get the fuzzy value;
            Logger::debug("fuzzy value is specified extracting it");
            /* The String is in form ~.d+, so ignore ~ and move forward */
            float fuzzyNum = static_cast<float>(strtod(input.c_str() + 1,NULL)); // convert to float
            Logger::debug("fuzzy value is %f", fuzzyNum);
            this->setSimilarityThresholdInContainer(fuzzyNum);
        } else {
            // there is no value specified
            Logger::debug(
                    "fuzzy value is not specified, using the lp value");
            this->setSimilarityThresholdInContainer(this->lpKeywordSimilarityThreshold); // selts the localParameter specified value
        }
    } else {
        Logger::debug("fuzzy value is not specified, use 0");
        this->setSimilarityThresholdInContainer(1.0f);
    }
}

void QueryParser::populateProximityInfo(bool isParsed, string &input) {
    if (isParsed) {
        Logger::debug("Proximity slop used in query");
        //this->setInQueryParametersIfNotSet(KeywordSimilarityThreshold);
        // get the fuzzy value;
        Logger::debug("fuzzy value is specified extracting it");
        boost::smatch numMatches;
        this->extractNumbers(input, numMatches);
        unsigned proximityNum = static_cast<unsigned int>(strtoul(
                numMatches[0].str().c_str(), NULL, 10)); // convert to unsigned int
        Logger::debug("proximity value is %d", proximityNum);
        this->PhraseSlops.push_back(proximityNum);
    } else {
        this->PhraseSlops.push_back(0);
    }
}

void QueryParser::clearMainQueryParallelVectorsIfNeeded() {
    Logger::debug("inside clearMainQueryParallelVectorsIfNeeded().");
    if (this->container->hasParameterInQuery(KeywordSimilarityThreshold)) {
        this->setInQueryParametersIfNotSet(IsFuzzyFlag);
        this->container->isFuzzy = true;
    } else {
        this->keywordSimilarityThreshold.clear();
        Logger::debug("keywordSimilarityThreshold paralel vector cleared.");
    }
// check if KeywordBoostLevel was set
    if (!this->container->hasParameterInQuery(KeywordBoostLevel)) {
        this->keywordBoostLevel.clear(); // clear the boost level vector.
        Logger::debug("keywordBoostLevel paralel vector cleared.");
    }
// check if QueryPrefixCompleteFlag was set
    if (!this->container->hasParameterInQuery(QueryPrefixCompleteFlag)) {
        this->keywordPrefixComplete.clear();
        Logger::debug("keywordPrefixComplete paralel vector cleared.");
    }
// cehck if FieldFilter was set
    if (!this->container->hasParameterInQuery(FieldFilter)) {
        // clear filedFilter vector and filedFilterOps vector
        this->fieldFilter.clear();
        this->fieldFilterOps.clear();
        Logger::debug("fieldFilter and fieldFilterOps paralel vector cleared.");
    }
    // check if QueryPrefixCompleteFlag was set
    if (!this->container->hasParameterInQuery(IsPhraseKeyword)) {
        this->isPhraseKeywordFlags.clear();
        Logger::debug("isPhraseKeyword paralel vector cleared.");
    }
    Logger::debug("returning from clearMainQueryParallelVectorsIfNeeded().");
}


bool QueryParser::parseBooleanExpression(string input, ParseTreeNode *& root){
    // Check correctness of parentheses
    if(checkParentheses(input) == false){
        return false;
    }
    // Parse the string and build the parse tree under root
    return parseBooleanExpressionRecursive(NULL, input, root);
}

/*
 * This function checks to see if open/close parentheses are well formatted.
 */
bool QueryParser::checkParentheses(const string & input){
    // We initialize depth to zero and start moving on characters, every time we see '(' we increment depth, and
    // every time we see ')' we decrement depth. Two criteria must meet:
    // 1. Depth must always be positive
    // 2. At the end, depth must be zero again.
    int depthCounter = 0;
    for(unsigned i=0; i < input.length() ; ++i){
        if(input.at(i) == '('){
            depthCounter ++;
        }else if(input.at(i) == ')'){
            depthCounter --;
        }
        if(depthCounter < 0) return false;
    }
    if(depthCounter != 0) return false;
    return true;
}

/*
 * This function is a recursive function to parse and build the parse tree of an expression
 * which contains AND,OR,NOT and nested parentheses.
 * Example: "(A OR1 B) AND1 NOT1 (C OR2 D) AND1 (E AND2 F) OR3 ((NOT2 G OR4 H) AND3 I)"
 * The tree will look like:
 * (NOTE : Numbers are just to help the reader map the query and the figure more easily.
 * So AND1 and AND2 are both just simple ANDs)
 * [AND1]__ [OR1]__ A
 *   |        |____ B
 *   |
 *   |_____ [NOT1]__ [OR2]__ C
 *   |                |____ D
 *   |
 *   |_____ [OR3] __ [AND2]__ E
 *           |         |_____ F
 *           |
 *           |______ [AND3]__ [OR4]__ [NOT2]__ G
 *                     |       |
 *                     |       |_____ H
 *                     |
 *                     |_____ I
 */
bool QueryParser::parseBooleanExpressionRecursive(ParseTreeNode * parent, string input, ParseTreeNode *& expressionNode){

    // 1. make sure parentheses are first and last character (if any) by trimming
    boost::algorithm::trim(input);

    // 2.1. Keep removing outer parentheses from the input
    // Example : ((((EXP1) OR (EXP2)))) => (EXP1) OR (EXP2)
    removeOuterParenthesisPair(input);

    // 2.2. after removing parentheses we should trim the string again.
    boost::algorithm::trim(input);
    /*
    * 3. OR has the next priority (after AND), so now we try to break the string by OR first
    * Example : "(A AND B) OR C AND (NOT D OR E)" =>
    * ==== Step 3 :
    * [AND]_______ (A AND B) OR C
    *  |
    *  |__________ (NOT D OR E)
    * ==== This step :
    * (A AND B) OR C =>
    * [OR]____ (A AND B)
    *  |
    *  |______ C
    * (NOT D OR E) => NOT D OR E =>
    * [OR]____ NOT D
    *  |
    *  |______ E
    */
    vector<string> disjuncts;
    tokenizeAndDontBreakParentheses(input, disjuncts , "OR");
    if(disjuncts.size() >= 2){
        // This OR operator object
        expressionNode = new ParseTreeNode(LogicalPlanNodeTypeOr, parent);
        for(vector<string>::iterator disjunct = disjuncts.begin() ; disjunct != disjuncts.end() ; ++disjunct){
            ParseTreeNode * newChild = NULL;
            // Call this function on each expression and put the result as a child of this OR operator
            if(parseBooleanExpressionRecursive(expressionNode, *disjunct, newChild) == false) {
                delete expressionNode;
                expressionNode = NULL;
                return false;
            }
            expressionNode->children.push_back(newChild);
        }
        return true;
    }

    /*
    * 4. Since AND has the highest priority, first we try to break the string by AND (after breaking it by OR)
    * Example : "(A AND B) OR C AND (NOT D OR E)" =>
    * [AND]_______ (A AND B) OR C
    *  |
    *  |__________ (NOT D OR E)
    */

    vector<string> conjuncts;
    tokenizeAndDontBreakParentheses(input, conjuncts , "AND");
    if(conjuncts.size() >= 2){
        // This AND operator object
        expressionNode = new ParseTreeNode(LogicalPlanNodeTypeAnd, parent);
        for(vector<string>::iterator conjunct = conjuncts.begin() ; conjunct != conjuncts.end() ; ++conjunct){
            ParseTreeNode * newChild = NULL;
            // Call this function on each expression and put the result as a child of this AND operator
            if(parseBooleanExpressionRecursive(expressionNode, *conjunct,  newChild) == false) {
                delete expressionNode;
                expressionNode = NULL;
                return false;
            }
            expressionNode->children.push_back(newChild);
        }
        return true;
    }

    /*
    * 5. If the string is not breakable by AND and OR, we try find NOT in the beginning.
    * Example :
    * NOT D =>
    * [NOT]__ D
    * Example 2:
    * NOT (A OR B) =>
    * [NOT]__ (A OR B)
    */
    string beginningNotRegex = "^(NOT)\\s";
    string inputWithoutNot;
    boost::regex re2(beginningNotRegex);
    if(doParse(input , re2, inputWithoutNot)){
        // This is a "NOT" operator object
        expressionNode = new ParseTreeNode(LogicalPlanNodeTypeNot, parent);
        ParseTreeNode * newChild = NULL;
        // Call this function on the following expression and put the result as the child of this NOT operator
        if(parseBooleanExpressionRecursive(expressionNode, input, newChild) == false) {
            delete expressionNode;
            expressionNode = NULL;
            return false;
        }
        expressionNode->children.push_back(newChild);
		return true;
    }

	/*
	 * 6. And finally if there is not a NOT, it's only a TERM which is left.
	 */
	expressionNode = new ParseTreeNode(LogicalPlanNodeTypeTerm, parent);
	expressionNode->termIntermediateStructure = new TermIntermediateStructure();
	expressionNode->termIntermediateStructure->termQueryString = input;
	return true;
}

/*
 * This function removes the outer parentheses of an expression
 * Example : (((A AND B) OR C)) => (A AND B) OR C
 * Assumption of this function is that parentheses are well formatted.
 */
void QueryParser::removeOuterParenthesisPair(string & input){
    // ASSERT(checkParentheses(input));
    while(true){
        // simple length condition to avoid out-of-bound faults.
        if(input.length() < 2) return;
        // If the first and last characters are not '(' and ')' there is no point in continuing.
        if(input.at(0) != '(' || input.at(input.length()-1) != ')') return;
        // We keep incrementing and decrementing depth when we see '(' and ')'
        // first and last characters are removed if
        // 1. they are parentheses
        // 2. when we reach to the end depth is zero
        // 3. and depth is always greater than zero in the middle of string.
        int depthCounter = 0;
        for(int c = 0 ; c < input.length() ; ++c){
            if(input.at(c) == '(') depthCounter ++;
            else if(input.at(c) == ')') {
                depthCounter --;
                /*
                * Except for the last character, any moment that
                * we see a depth of zero we can stop because it means the
                * first parentheses is not associated with the last one
                * Example :                         (A AND B) OR (C AND D)
                * Depth becomes zero at this point:         ^
                * so we should NOT remove parentheses for this expression because if we do
                * we get "A AND B) OR (C AND D" which is wrong.
                */
                if(depthCounter == 0 && c != input.length()-1){
                    return;
                }
            }
        }
        if(depthCounter != 0) return;
        // remove parentheses
        input = input.substr(1, input.length()-2);
    }
}

/*
 * This function tokenizes a string by a string delimiter (e.g. 'AND') while it is
 * careful not to break any pair of parentheses.
 * For example,if we want to tokenize expression "(A AND B) OR C" by "AND", we shouldn't
 * return "(A" and "B) OR C" and we should only return on token which is the expression itself.
 * NOTE1 : Assumption of this function is that the input is well formatted in terms of
 * parentheses.
 */
void QueryParser::tokenizeAndDontBreakParentheses(const string & inputArg , vector<string> & tokensArg, const string & delimiter){
    // ASSERT(checkParentheses(input));
    /*
     * Steps:
     * 0. Make sure there are two spaces around each parentheses, because we want to tokenize with space first.
     * 1. Break the string by space
     * 2. While moving on words, increment and decrement depth by also checking
     * --- the characters of each word.
     * 3. Every time we see a word=delimiter if depth is zero, we can break the string at that point.
     */

    // 0. make sure ( and ) are separatable by parentheses
    stringstream inputStream;
    string input = "";
    for(unsigned charIndex = 0 ; charIndex < inputArg.length(); ++charIndex){
        if(inputArg.at(charIndex) == '(' || inputArg.at(charIndex) == ')'){
            inputStream <<  " " << inputArg.at(charIndex) << " ";
        }else{
            inputStream << inputArg.at(charIndex) << "" ;
        }
    }
    input = inputStream.str();
    boost::algorithm::trim(input);

    std::list<std::string> stringList;
    boost::char_separator<char> sep(" ");
    boost::tokenizer< boost::char_separator<char> > tokens(input, sep);
    int depthCounter = 0;
    // We always keep appending words to the last element of this temporary vector
    // and when we see a new delimiter (e.g. "AND") we insert a new "" to this vector.
    tokensArg.push_back("");
    BOOST_FOREACH (const string& token, tokens) {
        if(token.compare(delimiter) == 0 && depthCounter == 0){
            boost::algorithm::trim(tokensArg.at(tokensArg.size()-1));
            tokensArg.push_back("");
            continue;
        }
        // append the new word
        tokensArg.at(tokensArg.size()-1) += token + " ";
        // move on characters of this word to update the depth
        for(unsigned charIndex = 0 ; charIndex < token.length() ; ++charIndex){
            if(token.at(charIndex) == '('){
                depthCounter ++;
            }else if(token.at(charIndex) == ')'){
                depthCounter --;
            }
        }
    }
}

}
}
