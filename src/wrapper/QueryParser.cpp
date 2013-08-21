#include "QueryParser.h"
#include "ParsedParameterContainer.h"
#include "ParserUtility.h"
#include <evhttp.h>
#include <string>
#include "boost/regex.hpp"
#include "util/Logger.h"
#include "util/Assert.h"
#include "RegexConstants.h"
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
const char* const QueryParser::lengthBoostParamName = "lengthBoost"; //srch2
const char* const QueryParser::prefixMatchPenaltyParamName = "pmp"; //srch2
const char* const QueryParser::filterQueryParamName = "fq"; //solr
const char* const QueryParser::isFuzzyParamName = "fuzzy"; //srch2
// local parameter params
const char* const QueryParser::lpKeyValDelimiter = "="; //solr
const char* const QueryParser::lpQueryBooleanOperatorParamName =
        "defaultFieldOperator"; //srch2
const char* const QueryParser::lpKeywordFuzzyLevelParamName =
        "defaultfuzzyLevel"; // srch2
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
//searchType
const char* const QueryParser::searchType = "searchType";

QueryParser::QueryParser(const evkeyvalq &headers,
        ParsedParameterContainer * container) :
        headers(headers) {
    this->container = container;
    this->isParsedError = false;
    this->isLpFieldFilterBooleanOperatorAssigned = false;
    this->lpKeywordFuzzyLevel = -1.0;
    this->lpKeywordBoostLevel = -1;
    this->lpKeywordPrefixComplete =
            srch2::instantsearch::TERM_TYPE_NOT_SPECIFIED; // stores the fallback termType for keywords
    this->container->isTermBooleanOperatorSet = false;
    this->container->isFqBooleanOperatorSet = false;
    this->isSearchTypeSet = false;
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
     * 10. this->lengthBoostParser();
     * 11. this->prefixMatchPenaltyParser();
     * 12. based on the value of search type (if it's defined in local parameters we take it
     *    otherwise we get it from conf file) leave the rest of parsing to one of the parsers
     * 12.1: search type : Top-K
     *      call topKParameterParser();
     * 12.2: search type : All results
     *      call getAllResultsParser();
     * 12.3: search type : GEO
     *      call geoParser();
     */

    // do some parsing
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
    this->lengthBoostParser();
    this->prefixMatchPenaltyParser();
    this->extractSearchType();
    if (this->container->hasParameterInQuery(GeoSearchType)) {
        this->geoParser();
    } else if (this->container->hasParameterInQuery(GetAllResultsSearchType)) {
        this->getAllResultsParser();
    } else {
        this->topKParameterParser();
    }
    return !this->isParsedError; // return true for success, false for parse error
}

void QueryParser::mainQueryParser() { // TODO: change the prototype to reflect input/outputs

    /*
     * example: q={param=key param2=key2}field1=keyword1 AND field2=keyword2* AND field3=keyword3*^2~.8
     * 1. Parse the string and find
     * 		1.1 local parameters
     * 		1.2 keyword string
     * 2. calls localParameterParser()
     * 3. calls the keywordParser();
     */
    // 1. get the mainQuery string.
    Logger::info("parsing the main query.");
    const char * mainQueryTmp = evhttp_find_header(&headers,
            QueryParser::keywordQueryParamName);
    if (mainQueryTmp) { // if this parameter exists
        size_t st;
        string mainQueryStr = evhttp_uridecode(mainQueryTmp, 0, &st);
        // check if mainQueryStr is valid.
        boost::algorithm::trim(mainQueryStr); // trim the mainQueryString.
        // 2. call the localParameterParser(), this will remove and parse the local param info from the mainQueryStr
        // for reveiwer: we can pass a duplicate of the mainQueryString too. I don't see a point why, so passing the mainQueryStr itself. Any thoughts?
        if (this->localParameterParser(mainQueryStr)) {
            // the mainQueryStr now doesn't have the lopcalparameter part.
            // 3. call the keywordParser().
            bool parseNextTerm = true;
            while (parseNextTerm) {
                if (this->keywordParser(mainQueryStr)) {
                    string boolOperator = "";
                    bool isParsed = parseTermBoolOperator(mainQueryStr,
                            boolOperator);
                    if (isParsed) {
                        string msgStr = "boolean operator is " + boolOperator;
                        Logger::debug(msgStr.c_str());
                        parseNextTerm = true;
                        Logger::debug("LOOPING AGAIN");
                        // fill up boolean operator
                    } else {
                        // no boolean operator found.
                        // if the input string length is >0 throw error.
                        parseNextTerm = false;
                        if (mainQueryStr.length() > 0) {
                            // raise error message
                            Logger::info(
                                    " Parse error, expecting boolean operator while parsing terms.");
                            this->container->messages.push_back(
                                    make_pair(MessageError,
                                            "Parse error, expecting boolean operator while parsing terms."));
                            this->isParsedError = true;
                        }
                    }
                }else{
                    parseNextTerm = false;
                }
            }
            // check if keywordFuzzyLevel was set by parseTerms
            // true? set the isFuzzyFlag.
            // else , empty the keywordFuzzyLevel vector
            this->clearMainQueryParallelVectorsIfNeeded();
        }
    } else {
        // no main query parameter present.
        Logger::debug(" parameter q not present.");
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
        size_t st;
        string fuzzy = evhttp_uridecode(fuzzyTemp, 0, &st);
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

void QueryParser::populateFacetFieldsSimple(FacetQueryContainer &fqc) {
    /*
     * populates teh fields vector related to facet.feild
     * example: facet.field=category
     */
    Logger::debug("inside populateFacetFieldSimple function");
    const char* key = "facet.field"; // TODO: usse a constant
    vector<string> facetFields;
    custom_evhttp_find_headers(&headers, key, facetFields);
    for (vector<string>::iterator facetField = facetFields.begin();
            facetField != facetFields.end(); ++facetField) {
        fqc.fields.push_back(*facetField);
        fqc.types.push_back(srch2::instantsearch::FacetTypeCategorical);
        // populate parallel vectors with empty string
        fqc.rangeEnds.push_back("");
        fqc.rangeGaps.push_back("");
        fqc.rangeStarts.push_back("");
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
    const char* key = "facet.range"; // TODO: use a constant
    vector<string> facetFields;
    custom_evhttp_find_headers(&headers, key, facetFields);
    for (vector<string>::iterator facetField = facetFields.begin();
            facetField != facetFields.end(); ++facetField) {
        fqc.fields.push_back(*facetField);
        fqc.types.push_back(srch2::instantsearch::FacetTypeRange);
        // populate parallel vectors with empty string
        populateParallelRangeVectors(fqc, *facetField);
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
        size_t st;
        string rangeStart = evhttp_uridecode(rangeStartTemp, 0, &st);
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
        size_t st;
        string rangeEnd = evhttp_uridecode(rangeEndTemp, 0, &st);
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
        size_t st;
        string rangeGap = evhttp_uridecode(rangeGapTemp, 0, &st);
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
        size_t st;
        string lengthBoost = evhttp_uridecode(lengthBoostTmp, 0, &st);
        if (isFloat(lengthBoost)) {
            this->container->parametersInQuery.push_back(
                    srch2::httpwrapper::LengthBoostFlag);
            this->container->lengthBoost = atof(lengthBoost.c_str());
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
     * example: pmp=.8
     */
    Logger::debug("inside prefixMatchPenaltyParser function");
    const char * prefixMatchPenaltyTmp = evhttp_find_header(&headers,
            QueryParser::prefixMatchPenaltyParamName);
    if (prefixMatchPenaltyTmp) { // if this parameter exists
        Logger::debug("prefixMatchPenalty parameter found, parsing it.");
        size_t st;
        string prefixMatchPenalty = evhttp_uridecode(prefixMatchPenaltyTmp, 0,
                &st);
        if (isFloat(prefixMatchPenalty)) {
            this->container->parametersInQuery.push_back(
                    srch2::httpwrapper::PrefixMatchPenaltyFlag);
            this->container->prefixMatchPenalty = atof(
                    prefixMatchPenalty.c_str());
        } else {
            //raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            string(prefixMatchPenaltyParamName)
                                    + " should be a valid float number"));
            this->isParsedError = true;
        }
    }
    Logger::debug("returnig from prefixMatchPenaltyParser function");
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
        size_t st;
        string fl = evhttp_uridecode(flTemp, 0, &st);
        this->container->parametersInQuery.push_back(
                srch2::httpwrapper::ReponseAttributesList);
        char * fieldStr = strdup(fl.c_str());
        char * pch = strtok(fieldStr, QueryParser::fieldListDelimiter);
        while (pch) {
            string field = pch;
            if (field.compare("*") == 0) {
                this->container->responseAttributesList.clear();
                this->container->responseAttributesList.push_back("*");
                return;
            }
            this->container->responseAttributesList.push_back(field);
            pch = strtok(fieldStr, NULL);
        }
        delete fieldStr;
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
        size_t st;
        string debugQuery = evhttp_uridecode(debugQueryTemp, 0, &st);
        if (boost::iequals(debugQuery, "true")) {
            this->container->isDebugEnabled = true;
            this->container->parametersInQuery.push_back(IsDebugEnabled); // change the IsDebugEnabled to DebugEnabled in Enum ParameterName ?
            // look for debug paramter. it decides the debug level, if it is not set, set the debug level to complete.
            const char * debugLevelTemp = evhttp_find_header(&headers,
                    QueryParser::debugParamName);
            if (debugLevelTemp) { // if this parameter exists
                size_t st;
                string debugLevel = evhttp_uridecode(debugLevelTemp, 0, &st);
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
        size_t st;
        string startStr = evhttp_uridecode(startTemp, 0, &st);
// convert the startStr to integer.
        if (isUnsigned(startStr)) {
            this->container->resultsStartOffset = atoi(startStr.c_str()); // convert the string to char* and pass it to atoi
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
        size_t st;
        string rowsStr = evhttp_uridecode(rowsTemp, 0, &st);
// convert the rowsStr to integer. e.g. rowsStr will contain string 20
        if (isUnsigned(rowsStr)) {
            this->container->numberOfResults = atoi(rowsStr.c_str()); // convert the string to char* and pass it to atoi
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
        size_t st;
        string timeAllowedStr = evhttp_uridecode(timeAllowedTemp, 0, &st);
// convert the Str to integer.
        if (isUnsigned(timeAllowedStr)) {
            this->container->maxTimeAllowed = atoi(timeAllowedStr.c_str()); // convert the string to char* and pass it to atoi
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
        size_t st;
        string ommitHeader = evhttp_uridecode(ommitHeaderTemp, 0, &st);
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
        size_t st;
        string responseWriteType = evhttp_uridecode(responseWriteTypeTemp, 0,
                &st);
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
     * example: fq=Field1:[10 TO 100] AND field2:[* TO 100] AND field3:keyword
     *
     */
    Logger::debug("inside filterQueryParameterParser function");
    const char* key = QueryParser::filterQueryParamName;
    const char* fqTmp = evhttp_find_header(&headers, key);
    if (fqTmp) {
        Logger::debug("filterQueryParameter found.");
        size_t st;
        string fq = evhttp_uridecode(fqTmp, 0, &st);
        // tokenise on the basis of boolean operator
        // set the boolean operation in fqv.
        // for each token call the fqv.addCriterion method.
        FilterQueryContainer* filterQueryContainer = new FilterQueryContainer();
        FilterQueryEvaluator* fqe = new FilterQueryEvaluator(
                &this->container->messages,
                &this->container->termFQBooleanOperator);
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
        size_t st;
        string facet = evhttp_uridecode(facetTemp, 0, &st);
        // we have facet,
        //parse other facet related parameters if this is true
        if (boost::iequals("true", facet)) {
            // facet param is true
            FacetQueryContainer *fqc = new FacetQueryContainer();
            populateFacetFieldsSimple(*fqc);
            populateFacetFieldsRange(*fqc);
            //// set the parametersInQuery
            if (this->container->hasParameterInQuery(GeoSearchType)) {
                this->container->geoParameterContainer->parametersInQuery
                        .push_back(FacetQueryHandler);
                this->container->geoParameterContainer->facetQueryContainer =
                        fqc;
            } else if (this->container->hasParameterInQuery(
                    GetAllResultsSearchType)) {
                this->container->getAllResultsParameterContainer
                        ->parametersInQuery.push_back(FacetQueryHandler);
                this->container->getAllResultsParameterContainer
                        ->facetQueryContainer = fqc;
            }
            // parse other facet fields
        } else if (boost::iequals("false", facet)) {
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
        size_t st;
        string sortString = evhttp_uridecode(sortTemp, 0, &st);
        // we have sortString, we need to tokenize this string and populate the
        // parameters in SortQueryEvaluator class object.
        char *sortStringDup = strdup(sortString.c_str()); // strtok takes char* and not const char* so creating duplicate of input.
        char *fieldToken = strtok(sortStringDup, sortFiledsDelimiter);
        vector<string> fieldTokens;
        Logger::debug("tokenizing pair using delimiter");
        while (fieldToken) {
            // this first token is filed name and second is its value
            // get the local parameter field
            string sToken = string(fieldToken);
            Logger::debug("triming the token %s", sToken.c_str());
            boost::algorithm::trim(sToken);
            Logger::debug("token trimed %s", sToken.c_str());
            fieldTokens.push_back(sToken);
            fieldToken = strtok(NULL, lpKeyValDelimiter);
        }
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
            if (this->container->hasParameterInQuery(GeoSearchType)) {
                this->container->geoParameterContainer->parametersInQuery
                        .push_back(SortQueryHandler);
                this->container->geoParameterContainer->sortQueryContainer =
                        sortQueryContainer;
            } else if (this->container->hasParameterInQuery(
                    GetAllResultsSearchType)) {
                this->container->getAllResultsParameterContainer
                        ->parametersInQuery.push_back(SortQueryHandler);
                this->container->getAllResultsParameterContainer
                        ->sortQueryContainer = sortQueryContainer;
            }
        }
        delete sortStringDup; // free copy
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
        size_t st;
        order = evhttp_uridecode(orderTemp, 0, &st);
    }
    Logger::debug("returning from orderByParser function");
    return order;
}

bool QueryParser::localParameterParser(string &input) {
    /*
     * this function parses the local parameters section of all parts
     * input:
     *      1. local parameters string : {key=val key2=val2}
     * output:
     *      1. it fills up the metadata of the queryHelper object
     */
    Logger::debug(
            "inside localParameterParser parsing the localparameters in the input string %s",
            input.c_str());
    if (input.at(0) == '{') {
        Logger::debug("localparamter string found, extracting it.");
        input = input.substr(1);
        string lpField = "";
        while (parseLpKey(input, lpField)) {
            if (parseLpDelimeter(input)) {
                string lpValue = "";
                if (parseLpValue(input, lpValue)) {
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
        if (input.at(0) == '}') {
            input = input.substr(1);
            boost::algorithm::trim(input);
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
    } else if (0 == lpKey.compare(lpKeywordFuzzyLevelParamName)) { // i tried using vecotr.at(index) showed me compile errors.
        if (isFloat(lpVal)) {
            float f = atof(lpVal.c_str()); //TODO: add the validation
            this->lpKeywordFuzzyLevel = f;
        } else {
            //warning
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            string(lpKeywordFuzzyLevelParamName)
                                    + " should be a valid float number. Ignoring it."));
        }
    } else if (0 == lpKey.compare(lpKeywordBoostLevelParamName)) { // i tried using vecotr.at(index) showed me compile errors.
        if (isUnsigned(lpVal)) {
            int boostLevel = atof(lpVal.c_str()); //TODO: add the validation
            this->lpKeywordBoostLevel = boostLevel;
        } else {
            //warning
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            string(lpKeywordBoostLevelParamName)
                                    + " should be a valid non negetive integer. Ignoring it."));
        }
    } else if (0 == lpKey.compare(lpKeywordPrefixCompleteParamName)) { //TODO: look into this again, why do we need this parameter?
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
        // val might be a comma separated string of fields.  field1,field2..
        // tokenize it on ',' and set the vector in container.
        char *fieldStr = strdup(lpVal.c_str()); // the strtok function takes char* and not const char* so create dulpicate of val as char*.
        char * fieldToken = strtok(fieldStr, lpFieldFilterDelimiter);
        while (fieldToken) {
            string sToken = string(fieldToken);
            boost::algorithm::trim(sToken); // trim the token
            this->lpFieldFilter.push_back(sToken); // set field in container
            fieldToken = strtok(NULL, lpFieldFilterDelimiter); // get the next filed. (the syntax is weird, but thats how it works.)
        }
        delete fieldStr; // free the fieldStr, duplicate of val.
    } else {
        // this localparameter is not supported. raise a MessageWarning msg.
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "Invalid local parameter " + lpKey + ", ignoring it."));
    }
}
bool QueryParser::keywordParser(string &input) {
    /*
     * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
     * example: field:keyword^3 AND keyword2 AND keyword* AND keyword*^3 AND keyword^2~.5
     * output: fills up the container
     */
    Logger::info("inside keyword parser.");
    Logger::debug("input received is %s", input.c_str());
    /*
     *
     */
    if(input.length() == 0){
        Logger::info("PARSE ERROR, returning from  keywordParser.");
        this->container->messages.push_back(
                make_pair(MessageError,
                        "Parse error, expecting keyword, not found "));
        this->isParsedError = true;
        return false;
    }
    if (input.at(0) == '\"') {
        // we do not support phrase search as of now
        Logger::info(
                "PARSE ERROR, unexpected character \" found at the begining of the keyword.");
        this->container->messages.push_back(
                make_pair(MessageError,
                        "Parse error, unexpected character \" found at the begining of the keyword."));
        this->isParsedError = true;
        return false;
    }
    string field = "";
    bool isParsed = this->parseField(input, field);
    if (!isParsed) {
        // TODO: use lpdefault fields
        this->populateFieldFilterUsingLp();
    } else {
        // populate the field
        // remove the trailing :
        field = field.substr(0, field.length() - 1);
        this->populateFieldFilterUsingQueryFields(field);
    }
    string keywordStr = "";
    isParsed = parseKeyword(input, keywordStr);
    if (isParsed) {
        // keyword obtained, get modifiers
        // populate the rawKeyword
        this->populateRawKeywords(keywordStr);
        // separate for each . *, ^ and ~
    } else {
        // check if they keyword is just *
        string asteric = "";
        isParsed = parseKeyWordForAsteric(input, asteric);
        if (isParsed) {
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
    string prefixMod = "";
    isParsed = parsePrefixModifier(input, prefixMod);
    if (isParsed) {
        this->setInQueryParametersIfNotSet(QueryPrefixCompleteFlag);
        // '*' is present
        Logger::debug("prefix modifier used in query");
        this->container->keywordPrefixComplete.push_back(
                srch2::instantsearch::TERM_TYPE_PREFIX); // use the lp variable value
    } else {
        // use the fallback specified in localparameters.
        this->container->keywordPrefixComplete.push_back(
                this->lpKeywordPrefixComplete);
    }
    string boostMod = "";
    isParsed = parseBoostModifier(input, boostMod);
    this->populateBoostInfo(isParsed, boostMod);
    string fuzzyMod = "";
    isParsed = parseFuzzyModifier(input, fuzzyMod);
    this->populateFuzzyInfo(isParsed, fuzzyMod);
    Logger::info("returning from  keywordParser.");
    return true;
}

void QueryParser::populateTermBooleanOperator(const string &termOperator) {
    /*
     * populates the termBooleanOperators in the container
     */
    Logger::debug("inside  populateTermBooleanOperator.");
    if (boost::iequals("OR", termOperator) || termOperator.compare("||") == 0) {
        // we do not support OR as of now so raising a MessageWarning and setting it to AND.
        // generate MessageWarning and use AND
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "We do not supprt OR  specified, ignoring it and using 'AND'."));
        this->container->termBooleanOperator =
                srch2::instantsearch::BooleanOperatorAND;
    } else if (boost::iequals("AND", termOperator)
            || termOperator.compare("&&") == 0) {
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
    this->container->rawQueryKeywords.push_back(input);
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
     * checks if the fuzzylevel is present in the input string
     * example: keyword~.8 has fuzzylevel as .8. keyword~ has no fuzzylevel specified.
     */
    Logger::debug("inside checkForFuzzyNums");
    boost::regex re(CHECK_FUZZY_NUMBER_REGEX_STRING);
    boost::regex_search(input, matches, re);
    Logger::debug("returning from checkForFuzzyNums");
}
void QueryParser::extractNumbers(const string &input, boost::smatch& matches) {
    /*
     * extracts the numbers from the input string
     * example:  it will extract '8' from '~.8'.
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
        this->container->fieldFilter.push_back(this->lpFieldFilter);
        if (this->isLpFieldFilterBooleanOperatorAssigned) {
            // LpFieldFilterBooleanOperator is assigned. fill
            this->container->fieldFilterOps.push_back(
                    this->lpFieldFilterBooleanOperator);
        } else {
            this->container->fieldFilterOps.push_back(
                    srch2::instantsearch::OP_NOT_SPECIFIED);
        }
    } else {
        this->container->fieldFilter.push_back(vector<string>());
        this->container->fieldFilterOps.push_back(
                srch2::instantsearch::OP_NOT_SPECIFIED);
    }
    Logger::debug("returning from populateFieldFilterUsingLp");
}

void QueryParser::populateFieldFilterUsingQueryFields(const string &input) {
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
        this->container->fieldFilterOps.push_back(
                srch2::instantsearch::BooleanOperatorAND);
    } else if (input.find('+') != string::npos) {
        fieldBoolOpDelimeterRegexString =
                FIELD_OR_BOOL_OP_DELIMETER_REGEX_STRING;
        this->container->fieldFilterOps.push_back(
                srch2::instantsearch::BooleanOperatorOR);
    } else {
        // no boolean operators here.
        // create a vector and add it to the container.
        vector<string> candidate;
        candidate.push_back(input); // this candidate can be any alphanumeric or *.
        this->container->fieldFilter.push_back(candidate);
        // fill the corresponding fieldFilterOps parallel vector
        this->container->fieldFilterOps.push_back(
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
    this->container->fieldFilter.push_back(fields);
    Logger::debug("returning from populateFieldFilterUsingQueryFields");
}

void QueryParser::topKParameterParser() {
    /*
     * this function parsers only the parameters which are specific to Top-K
     */
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
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     *
     */
    this->facetParser();
    this->sortParser();
}
void QueryParser::extractSearchType() {
    /*
     *  figures out what is the searchtype of the query. No need of passing the searchType parameter anymore in lp.
     *  if lat/long query params are specified its a geo
     *      parses the geo parameters like leftBottomLatitude,leftBottomLongitude,rightTopLatitude,rightTopLongitude
     *      centerLatitude,centerLongitude,radius
     *      Based on what group of geo parameters are present it sets geoType to CIRCULAR or RECTANGULAR
     *  else:
     *      if sort|facet are specified, its a getAllResult
     *  else:
     *  it's a Top-K
     *  set the parametersInQuery.
     *
     */
    // if serachType mentioned in queryParameter use that.
    const string geoType = "geo";
    const string getAllType = "getAll";
    const string topKType = "topK";
    Logger::debug("inside extractSearchType function");
    const char * searchTypeTmp = evhttp_find_header(&headers,
            QueryParser::searchType);
    string searchType = "";
    if (searchTypeTmp) { // if this parameter exists
        Logger::debug("searchType found, parsing it");
        this->isSearchTypeSet = true;
        size_t st;
        searchType = evhttp_uridecode(searchTypeTmp, 0, &st);
    }
    Logger::debug("returning from responseWriteTypeParameterParser function");
    // else extrct it. if no search type is given and cannot decide between topk and getAll, use topK, raise a warning
    Logger::debug("inside extractSearchType, checking for geo parameter");
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
        // it's a reactangular geo search
        if (this->isSearchTypeSet && !boost::iequals(geoType, searchType)) {
            // raise warning
            this->container->messages.push_back(
                    make_pair(MessageWarning,
                            "searchType parameter for this query should be set to geo, found "
                                    + searchType
                                    + " .Using geo as searchType"));
        }
        this->container->parametersInQuery.push_back(GeoSearchType);
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
            // its a circular geo search
            if (this->isSearchTypeSet && !boost::iequals(geoType, searchType)) {
                // raise warning
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                "searchType parameter for this query should be set to geo, found "
                                        + searchType
                                        + " .Using geo as searchType"));
            }
            this->container->parametersInQuery.push_back(GeoSearchType);
            this->container->geoParameterContainer =
                    new GeoParameterContainer();
            this->container->geoParameterContainer->parametersInQuery.push_back(
                    GeoTypeCircular);
            //set GeoParameterContainer properties.
            this->setGeoContainerProperties(centerLatTemp, centerLongTemp,
                    radiusParamTemp);
        } else {
            const char * sortTemp = evhttp_find_header(&headers,
                    QueryParser::sortParamName);
            const char * facetTemp = evhttp_find_header(&headers,
                    QueryParser::facetParamName);
            if (sortTemp || facetTemp) {
                // it's a getAllResesult search
                if (this->isSearchTypeSet
                        && !boost::iequals(getAllType, searchType)) {
                    // raise warning
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    "searchType parameter for this query should be set to getAll, found "
                                            + searchType
                                            + " .Using getAll as searchType"));
                }
                this->container->parametersInQuery.push_back(
                        GetAllResultsSearchType);
                this->container->getAllResultsParameterContainer =
                        new GetAllResultsParameterContainer();
            } else {
                if (this->isSearchTypeSet) {
                    if (boost::iequals(getAllType, searchType)) {
                        // it's a getAll
                        this->container->parametersInQuery.push_back(
                                GetAllResultsSearchType);
                        this->container->getAllResultsParameterContainer =
                                new GetAllResultsParameterContainer();
                    } else if (boost::iequals(topKType, searchType)) {
                        // it's a Top-K search
                        this->container->parametersInQuery.push_back(
                                TopKSearchType);
                        this->container->topKParameterContainer =
                                new TopKParameterContainer();
                    } else if (boost::iequals(geoType, searchType)) {
                        Logger::info(
                                "searchType provided in queryParamter is geo. Not all required geo paramters are provided. Evaluating falback options");
                        if (!this->container->rawQueryKeywords.empty()) {
                            // keywords are provided, we can fall back to topK
                            this->container->messages.push_back(
                                    make_pair(MessageWarning,
                                            "not enough circular or rectangular geo parameters were found, falling back to topK search"));
                            this->container->parametersInQuery.push_back(
                                    TopKSearchType);
                            this->container->topKParameterContainer =
                                    new TopKParameterContainer();
                        }

                    } else {
                        // searchType provided is not known, fall back to top-k
                        this->container->messages.push_back(
                                make_pair(MessageWarning,
                                        "Unknown searchType " + searchType
                                                + " provided, falling back to topK"));
                        this->container->parametersInQuery.push_back(
                                TopKSearchType);
                        this->container->topKParameterContainer =
                                new TopKParameterContainer();
                    }
                } else {
                    // no searchType provided use topK
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    "no searchType is provided, using topK"));
                    this->container->parametersInQuery.push_back(
                            TopKSearchType);
                    this->container->topKParameterContainer =
                            new TopKParameterContainer();
                }

            }
        }
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
    string leftBottomLatStr = evhttp_uridecode(leftBottomLat, 0, &st1);
    string leftBottomLongStr = evhttp_uridecode(leftBottomLong, 0, &st2);
    string rightTopLatStr = evhttp_uridecode(rightTopLat, 0, &st3);
    string rightTopLongStr = evhttp_uridecode(rightTopLong, 0, &st4);
// convert the rowsStr to integer.
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->leftBottomLatitude = atof(
                leftBottomLatStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "lblat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->leftBottomLongitude = atof(
                leftBottomLongStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "lblong should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLatitude = atof(
                rightTopLatStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "rtlat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLongitude = atof(
                rightTopLongStr.c_str()); // convert the string to char* and pass it to atof
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
    string centerLatStr = evhttp_uridecode(centerLat, 0, &st1);
    string centerLongStr = evhttp_uridecode(centerLong, 0, &st2);
    string radiusParamStr = evhttp_uridecode(radiusParam, 0, &st3);
// convert the rowsStr to integer.
    if (isFloat(centerLatStr)) {
        this->container->geoParameterContainer->centerLatitude = atof(
                centerLatStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError, "clat should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(centerLongStr)) {
        this->container->geoParameterContainer->centerLongitude = atof(
                centerLongStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "clong should be a valid float number"));
        this->isParsedError = true;
    }
    if (isFloat(radiusParamStr)) {
        this->container->geoParameterContainer->radius = atof(
                radiusParamStr.c_str()); // convert the string to char* and pass it to atof
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
void QueryParser::setFuzzyLevelInContainer(const float f) {
    /*
     * sets the fuzzylevel in the container->keywordFuzzyLevel variable.
     * check if isFuzzyFlag is set
     *      true-> check if is fuzzy is true or false,
     *                  true -> use the fuzzylevel as specified
     *                  else -> set 0.0 as fuzzylevel
     *      false-> set the fuzzy level as specified
     */
    Logger::debug("inside setFuzzyLevelInContainer");
    if (this->container->hasParameterInQuery(IsFuzzyFlag)) {
        // this is set, fuzzyFlag came from  query parameter.
        if (this->container->isFuzzy) {
            // use the fuzzylevel provided with keyword
            this->container->keywordFuzzyLevel.push_back(f);
        } else {
            // set fuzzy level to 0
            this->container->keywordFuzzyLevel.push_back(0.0f);
        }
    } else {
        // set the values as specified with keyword
        this->container->keywordFuzzyLevel.push_back(f);
    }
    Logger::debug("returning from setFuzzyLevelInContainer");
}
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
            unsigned boostNum = atoi(numMatches[0].str().c_str()); // convert to integer
            Logger::debug("boost value is %d", boostNum);
            this->container->keywordBoostLevel.push_back(boostNum); // push to the container.
        } else {
            // there is no value specified
            Logger::debug(
                    "boost value is not specified, using the lp value or -1");
            this->container->keywordBoostLevel.push_back(
                    this->lpKeywordBoostLevel); // sets the localParameter specified value. it's initial value is -1.
        }
    } else {
        Logger::debug("boost value is not specified, using the lp value or -1");
        this->container->keywordBoostLevel.push_back(this->lpKeywordBoostLevel); // selts the localParameter specified value
    }
}
bool QueryParser::parseFuzzyModifier(string &input, string &output) {
    boost::regex re(FUZZE_MODIFIER_REGEX_STRING); //TODO: compile this regex when the engine starts.
    return doParse(input, re, output);
}
void QueryParser::populateFuzzyInfo(bool isParsed, string &input) {
    if (isParsed) {
        Logger::debug("fuzzy modifier used in query");
        this->setInQueryParametersIfNotSet(KeywordFuzzyLevel);
        boost::smatch matches;
        this->checkForFuzzyNums(input, matches); // check if boost value is present
        if (matches[0].matched) {
            // get the fuzzy value;
            Logger::debug("fuzzy value is specified extracting it");
            boost::smatch numMatches;
            this->extractNumbers(matches[0].str(), numMatches);
            float fuzzyNum = atof(("." + numMatches[0].str()).c_str()); // convert to float
            Logger::debug("fuzzy value is %f", fuzzyNum);
            this->setFuzzyLevelInContainer(fuzzyNum);
        } else {
            // there is no value specified
            Logger::debug(
                    "fuzzy value is not specified, using the lp value or -1.0");
            this->setFuzzyLevelInContainer(this->lpKeywordFuzzyLevel); // selts the localParameter specified value
        }
    } else {
        Logger::debug("fuzzy value is not specified, use 0");
        this->setFuzzyLevelInContainer(0.0f);
    }
}

void QueryParser::clearMainQueryParallelVectorsIfNeeded() {
    Logger::debug("inside clearMainQueryParallelVectorsIfNeeded().");
    if (this->container->hasParameterInQuery(KeywordFuzzyLevel)) {
        this->setInQueryParametersIfNotSet(IsFuzzyFlag);
        this->container->isFuzzy = true;
    } else {
        this->container->keywordFuzzyLevel.clear();
        Logger::debug("keywordFuzzyLevel paralel vector cleared.");
    }
    // check if KeywordBoostLevel was set
    if (!this->container->hasParameterInQuery(KeywordBoostLevel)) {
        this->container->keywordBoostLevel.clear(); // clear the boost level vector.
        Logger::debug("keywordBoostLevel paralel vector cleared.");
    }
    // check if QueryPrefixCompleteFlag was set
    if (!this->container->hasParameterInQuery(QueryPrefixCompleteFlag)) {
        this->container->keywordPrefixComplete.clear();
        Logger::debug("keywordPrefixComplete paralel vector cleared.");
    }
    // cehck if FieldFilter was set
    if (!this->container->hasParameterInQuery(FieldFilter)) {
        // clear filedFilter vector and filedFilterOps vector
        this->container->fieldFilter.clear();
        this->container->fieldFilterOps.clear();
        Logger::debug("fieldFilter and fieldFilterOps paralel vector cleared.");
    }
    Logger::debug("returning from clearMainQueryParallelVectorsIfNeeded().");
}

}
}
