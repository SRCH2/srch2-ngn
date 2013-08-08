#include "QueryParser.h"
#include "ParsedParameterContainer.h"
#include "ParserUtility.h"
#include <evhttp.h>
#include <string>
#include "boost/regex.hpp"
#include "util/Logger.h"
#include "util/Assert.h"
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

QueryParser::QueryParser(const evkeyvalq &headers,
        ParsedParameterContainer * container) :
        headers(headers) {
    this->container = container;
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
    this->extractSearchType(); // add a query parameter searchType, not a mendatory parameter
    if (this->container->hasParameterInSummary(GeoSearchType)) {
        this->geoParser();
    } else if (this->container->hasParameterInSummary(
            GetAllResultsSearchType)) {
        this->getAllResultsParser();
    } else {
        this->topKParameterParser();
    }
    return !this->container->isParsedError; // return true for success, false for parse error
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
        if (this->verifyMainQuery(mainQueryStr)) {
            // 2. call the localParameterParser(), this will remove and parse the local param info from the mainQueryStr
            // for reveiwer: we can pass a duplicate of the mainQueryString too. I don't see a point why, so passing the mainQueryStr itself. Any thoughts?
            this->localParameterParser(&mainQueryStr);
            // the mainQueryStr now doesn't have the lopcalparameter part.
            // 3. call the keywordParser().
            this->keywordParser(mainQueryStr);
        } else {
            // invalid query return
            Logger::info(" Parsing error:: main query is invalid.");
            // raise an error message, set the error flag.
            // TODO: read the msg string from a separate file.
            this->container->messages.push_back(
                    make_pair(MessageError,
                            " Parsing error:: main query is invalid"));
            this->container->isParsedError = true;
        }
    } else {
        // no main query parameter present.
        Logger::debug(" parameter q not present.");
    }
}

bool QueryParser::verifyMainQuery(const string &input) {
    /*
     * verifyies the syntax of the main query string.
     */
    //TODO: change the field and keyword regex string for differet types.
    /*
     * text: should be inside quotes ""
     * float: no quotes
     * int: no quotes
     * date: see parserUtility.h for date formats.
     */
    Logger::info("varifying the main query.");
    // TODO: move this regex block outside this class. We dont want this regex to be compiled everytime a query comes.
    const string lpRegexString =
            "\\{(\\w+\\s*=\\s*\\w+){1}(\\s+\\w+\\s*=\\s*\\w+)*[\\}]";
    std::string fieldRegexString =
            "(\\w+((\\.{0,1}\\w+)+|(\\+{0,1}\\w+)+)\\s*|\\*\\s*)"; // verifies the syntax of field.   e.g. checks the systax of "field" in field:keyword
    const string boostModRegexString = "\\^{0,1}\\d*"; // verifies the boost syntax. e.g. ^3 or ^
    const string fuzzyModRegexString = "~{0,1}(\\.\\d){0,1}"; // verifies the fuzzyness syntax. e.g. ~4 or ~
    const string modRegexString = boostModRegexString + fuzzyModRegexString; // combining both boost and fuzzy. user should specify them in this order only. e.g. ^3~.4.
    std::string keywordRegexString =
            "\\s*(\\.{0,1}\\w+(\\.{0,1}\\w+)+\\.{0,1}\\*{0,1}|\\*)";
    ; // verifies the syntax of keyword
    const string keywordWithModsRegexString = keywordRegexString
            + modRegexString; // keyword + mod  (keyword^4~.3 or keyword^~.3 or keyword^2~ etc.)
    std::string termModRegexString = "(" + keywordWithModsRegexString + "|"
            + fieldRegexString + ":" + keywordWithModsRegexString + ")"; // verifies the syntax of full "file:keyword"

    std::string queryRegexString = "^(" + lpRegexString + "){0,1}\\s*"
            + termModRegexString + "(\\s+(AND|&&)\\s+" + termModRegexString
            + ")*\\s*"; // verifies the systax of complete query string.
    // e.g. "{localparameter1 = default2 qf = asd} field:keyword^~.4 AND field:keyword^ && filed:keyword^4  && aa11.4.ff:aa AND asda && aa11+4+ff:aa1.11.11  && filed:keyword^4~  && filed:keyword^~"); //various combination

    boost::regex queryRegex(queryRegexString);
    // the above regex block needs to be compiled once only
    /*
     *  explaination of above regex:
     *  if localparameter is present then, query string should START with localparameter.
     *  ^(" + lpRegexString + "){0,1}  =>  here {0,1} makes sure localparameter is present at most once.
     *  query can have a keyword or a field:keyword pair.
     *  field should not start with a '.' (dot). '.' and '+' have special meaning in the field names.
     *  field1.field2:keyword would mean that engine will result the record only if  'keyword' is present in both field1 and field2
     *  field1+field2:keyword would mean that engine will result the record if  'keyword' is present in field1 or field2
     *  Note: we can have only '+' or '.' not both. e.g. field1.field2+field3:keyword is invalid syntax
     *  field can have alphnumerical characters
     */
    Logger::info("verifying main query done.");
    return boost::regex_match(input, queryRegex);
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
        this->container->summary.push_back(srch2::httpwrapper::IsFuzzyFlag);
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
        fqc.types.push_back(srch2::instantsearch::Simple);
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
        fqc.types.push_back(srch2::instantsearch::Range);
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
    stringstream startKey;
    startKey << "f." << field << ".facet.range.start"; // TODO use normal string concat
    string startKeyStr = startKey.str();
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
    stringstream endKey;
    endKey << "f." << field << ".facet.range.end";
    string endKeyStr = endKey.str();
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
    stringstream gapKey;
    gapKey << "f." << field << ".facet.range.end";
    string gapKeyStr = gapKey.str();
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
            this->container->summary.push_back(
                    srch2::httpwrapper::LengthBoostFlag);
            this->container->lengthBoost = atof(lengthBoost.c_str());
        } else {
            //raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "lengthBoost should be a valid float number"));
            this->container->isParsedError = true;
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
            this->container->summary.push_back(
                    srch2::httpwrapper::PrefixMatchPenaltyFlag);
            this->container->prefixMatchPenalty = atof(
                    prefixMatchPenalty.c_str());
        } else {
            //raise error
            this->container->messages.push_back(
                    make_pair(MessageError, string(prefixMatchPenaltyParamName) +
                            " should be a valid float number"));
            this->container->isParsedError = true;
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
        this->container->summary.push_back(
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
            this->container->summary.push_back(IsDebugEnabled); // change the IsDebugEnabled to DebugEnabled in Enum ParameterName ?
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
            // populate the summary
            this->container->summary.push_back(ResultsStartOffset);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "start parameter should be a positive integer"));
            this->container->isParsedError = true;
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
            // populate the summary
            this->container->summary.push_back(NumberOfResults);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "rows parameter should be a positive integer"));
            this->container->isParsedError = true;
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
            // populate the summary
            this->container->summary.push_back(MaxTimeAllowed);
        } else {
            // raise error
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "timeAllowed parameter should be a positive integer"));
            this->container->isParsedError = true;
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
// populate the summary
        this->container->summary.push_back(IsOmitHeader); // should we change this ParameterName to OmitHeader?
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
// populate the summary
        this->container->summary.push_back(ResponseFormat); // should we change this ParameterName to OmitHeader?
    }
    Logger::debug("returning from responseWriteTypeParameterParser function");
}

void QueryParser::filterQueryParameterParser() {
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
        // see if fq matches a valid regex.
        // if yes, move further, else raise error msg
        // tokenise on the basis of boolean operator
        // set the boolean operation in fqv.
        // for each token call the fqv.addCriterion method.
        if (this->verifyFqSyntax(fq)) {
            FilterQueryContainer* filterQueryContainer =
                    new FilterQueryContainer();
            FilterQueryEvaluator* fqe = new FilterQueryEvaluator();
            filterQueryContainer->evaluator = fqe;
            this->container->filterQueryContainer = filterQueryContainer;
            this->container->summary.push_back(FilterQueryEvaluatorFlag);
            // tokenise on the basis of boolean operator
            string operatorRegexString = "\\s+(AND|&&|OR)\\s+"; //TODO: OR and ||
            //TODO: check exprtk library to see what boolean operator string they use.
            boost::regex re(operatorRegexString); //TODO: move this regex compilation from here. It should happen when the engine starts
            boost::sregex_iterator filterTermItr(fq.begin(), fq.end(), re);
            boost::sregex_iterator filterTermEndItr;
            vector<string> filterTerms;
            vector<string> filterTermOperators;
            size_t termStartPosition = 0;
            for (; filterTermItr != filterTermEndItr; ++filterTermItr) {
                size_t termLength = (*filterTermItr).position()
                        - termStartPosition;
                string filterTerm = fq.substr(termStartPosition, termLength);
                filterTerms.push_back(filterTerm);
                termStartPosition = (*filterTermItr).position()
                        + (*filterTermItr).length();
                filterTermOperators.push_back((*filterTermItr).str());
            }
            filterTerms.push_back(fq.substr(termStartPosition)); // push back the last token
            // set the termOperators in container
            if (!filterTermOperators.empty()) {
                this->populateFilterQueryTermBooleanOperator(
                        filterTermOperators.at(0)); // TODO: validation to check, all the boolean operators are either AND(&&) or OR(||)
            }
            // get the first boolean operator and set that in evaluator. It's decided that we will only support one of AND,OR.
            fqe->setOperation(this->container->termFQBooleanOperator);
            // parse the terms
            for (vector<string>::iterator it = filterTerms.begin();
                    it != filterTerms.end(); ++it) {
                string term = *it;
                fqe->addCriterion(term); // TODO: // pass a pointer to a vector<pairs> to this function. check if it's false  set error msg and set isParsedError to true
            }
        } else {
            // raise error saying fq doesn't match the correct syntax
            this->container->messages.push_back(
                    make_pair(MessageError,
                            "Parsing error, filter query doesn't match the correct syntax"));
            this->container->isParsedError = true;
        }

    }
    Logger::debug("returning from filterQueryParameterParser function");
}

bool QueryParser::verifyFqSyntax(const string &fq) {
    /*
     * verifies the syntax of filter query srting.
     */
    Logger::debug("inside verifyFqSyntax function");
    Logger::debug("returning from verifyFqSyntax function");
    // stub to add verification logic here, currently it returns true,
    //as we are verifying different parts separately
    //TODO: remove this function if it's not needed.
    return true; // not implementing it
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
            //// set the summary
            if (this->container->hasParameterInSummary(GeoSearchType)) {
                this->container->geoParameterContainer->summary.push_back(
                        FacetQueryHandler);
                this->container->geoParameterContainer->facetQueryContainer =
                        fqc;
            } else if (this->container->hasParameterInSummary(
                    GetAllResultsSearchType)) {
                this->container->getAllResultsParameterContainer->summary
                        .push_back(FacetQueryHandler);
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
                        srch2::instantsearch::ORDER_NOT_SPECIFIED;
            } else if (boost::iequals("asc", order)) {
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::Ascending;
            } else if (boost::iequals("desc", order)) {
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::Descending;
            } else {
                // raise warning value not known. using ORDER_NOT_SPECIFIED
                sortQueryContainer->evaluator->order =
                        srch2::instantsearch::ORDER_NOT_SPECIFIED;
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                "Unknown order value. using order from config file"));
            }
            // set the summary
            if (this->container->hasParameterInSummary(GeoSearchType)) {
                this->container->geoParameterContainer->summary.push_back(
                        SortQueryHandler);
                this->container->geoParameterContainer->sortQueryContainer =
                        sortQueryContainer;
            } else if (this->container->hasParameterInSummary(
                    GetAllResultsSearchType)) {
                this->container->getAllResultsParameterContainer->summary
                        .push_back(SortQueryHandler);
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

void QueryParser::localParameterParser(string *input) {
    /*
     * this function parses the local parameters section of all parts
     * input:
     *      1. local parameters string : {key=val key2=val2}
     * output:
     *      1. it fills up the metadata of the queryHelper object
     */
    //TODO: break this funcion into smaller functions
    Logger::debug("parsing the localparameters in the input string %s",
            (*input).c_str());
// check if input string might have a local parameter info
    std::string localParametersString; // string to contain the localparameter string only
    std::string lpRegexString =
            "\\{(\\w+\\s*=\\s*\\w+){1}(\\s+\\w+\\s*=\\s*\\w+)*\\}"; // TODO update from the verifyMainquery
    boost::regex localParameterRegex(lpRegexString); // TODO: compile this regex when the engine starts. that would be more efficient.
    boost::smatch clpMatches;
    if (boost::regex_search(*input, clpMatches, localParameterRegex)) {
// mathc found input has localParameters
        Logger::debug("localparamter string found, extracting it.");
        localParametersString = clpMatches[0].str(); // get the locaparamter part
        Logger::debug("localparamter string is %s",
                localParametersString.c_str());
        // remove the localparameter string part form the input.
        Logger::debug("removing localparameter substring from input");
        *input = boost::regex_replace(*input, localParameterRegex, ""); // input is modified. lp info is being removed.
        Logger::debug(
                "removed localparameter substring from input, input is nodified to %s",
                (*input).c_str());
// now get the pairs from the local parameter string
        string lpPairsRegexString = "\\w+\\s*=\\s*\\w+"; // regex to get field = val pairs from {field1=val1 field2 = val2}
        boost::regex lPPairRegex(lpPairsRegexString); //TODO: compile this regex when the engine starts.
        boost::sregex_token_iterator pairItr(localParametersString.begin(),
                localParametersString.end(), lPPairRegex, 0); // get the iterator for the matches
        boost::sregex_token_iterator end;
// iterate on matched pairs
// parse the key value pairs and populate the container
        Logger::debug(
                "search for all the field=value pairs in localparameter string %s",
                localParametersString.c_str());
        for (; pairItr != end; ++pairItr) {
            string pair = *pairItr;
            Logger::debug("pair found %s", pair.c_str());
            // split by "=" (localParamDelimiter) and fill the container
            char *pairDup = strdup(pair.c_str()); // strtok takes char* and not const char* so creating duplicate of input.
            char *pairToken = strtok(pairDup, lpKeyValDelimiter);
            vector<string> pairKeyVal;
            Logger::debug("tokenizing pair using delimiter");
            while (pairToken) { // should give two tokesns only
                // this first token is field name and second is its value
                // get the local parameter field
                string sToken = string(pairToken);
                Logger::debug("triming the token %s", sToken.c_str());
                boost::algorithm::trim(sToken);
                Logger::debug("token trimed %s", sToken.c_str());
                pairKeyVal.push_back(sToken);
                pairToken = strtok(NULL, lpKeyValDelimiter);
            }
            if (0 == pairKeyVal[0].compare(QueryParser::lpQueryBooleanOperatorParamName)) { // default Boolean operator to be used for the fields in the query terms
                string val = pairKeyVal[1];
                if (boost::iequals("OR", val)) {
                    this->container->lpFieldFilterBooleanOperator =
                            srch2::instantsearch::OR;
                } else if (boost::iequals("AND", val)) {
                    this->container->lpFieldFilterBooleanOperator =
                            srch2::instantsearch::AND;
                } else {
                    // generate MessageWarning and use AND
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    "Invalud boolean operator specified " + val
                                            + ", ignoring it and using AND."));
                    this->container->lpFieldFilterBooleanOperator =
                            srch2::instantsearch::AND;
                }
                this->container->isLpFieldFilterBooleanOperatorAssigned = true;
            } else if (0
                    == pairKeyVal[0].compare(lpKeywordFuzzyLevelParamName)) { // i tried using vecotr.at(index) showed me compile errors.
                string val = pairKeyVal[1];
                if (isFloat(val)) {
                    float f = atof(val.c_str()); //TODO: add the validation
                    this->container->lpKeywordFuzzyLevel = f;
                } else {
                    //warning
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    string(lpKeywordFuzzyLevelParamName)
                                            + " should be a valid float number"));
                }
            } else if (0
                    == pairKeyVal[0].compare(
                            lpKeywordPrefixCompleteParamName)) { //TODO: look into this again, why do we need this parameter?
                string val = pairKeyVal[1];
                if (boost::iequals("PREFIX", val)) {
                    this->container->lpKeywordPrefixComplete =
                            srch2::instantsearch::TERM_TYPE_PREFIX;
                } else if (boost::iequals("COMPLETE", val)) {
                    this->container->lpKeywordPrefixComplete =
                            srch2::instantsearch::TERM_TYPE_COMPLETE;
                } else {
                    // generate MessageWarning and use prefix
                    this->container->messages.push_back(
                            make_pair(MessageWarning,
                                    "Invalid choice " + val
                                            + ",we support prefix and complete search on keyword only. ignoring it and using the default value from config file."));
                    this->container->lpKeywordPrefixComplete =
                            srch2::instantsearch::NOT_SPECIFIED;
                }
            } else if (0 == pairKeyVal[0].compare(lpFieldFilterParamName)) {
                string val = pairKeyVal[1];
                // val might be a comma separated string of fields.  field1,field2..
                // tokenize it on ',' and set the vector in container.
                char *fieldStr = strdup(val.c_str()); // the strtok function takes char* and not const char* so create dulpicate of val as char*.
                char * fieldToken = strtok(fieldStr, lpFieldFilterDelimiter);
                while (fieldToken) {
                    string sToken = string(fieldToken);
                    boost::algorithm::trim(sToken); // trim the token
                    this->container->lpFieldFilter.push_back(sToken); // set field in container
                    fieldToken = strtok(NULL, lpFieldFilterDelimiter); // get the next filed. (the syntax is weird, but thats how it works.)
                }
                delete fieldStr; // free the fieldStr, duplicate of val.
            } else {
                // this localparameter is not supported. raise a MessageWarning msg.
                this->container->messages.push_back(
                        make_pair(MessageWarning,
                                "Invalid local parameter " + pairKeyVal[0]
                                        + ", ignoring it."));
            }
            delete pairDup; // free the result, duplicate of input.
        }
        Logger::debug("returning from localParameterParser");
    } else {
// does not contain any localParameter info.
        Logger::debug(
                "no localparameter info to parse in the input string. returning");
    }
}

void QueryParser::keywordParser(const string &input) {
    /*
     * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
     * example: field:keyword^3 AND keyword2 AND keyword* AND keyword*^3 AND keyword^2~.5
     * output: fills up the container
     */
    Logger::info("inside keyword parser.");
    Logger::debug("input received is %s", input.c_str());
    string operatorRegexString = "\\s+(AND|&&|OR)\\s+";
    boost::regex re(operatorRegexString); //TODO: move this regex compilation from here. It should happen when the engine starts
    boost::sregex_iterator termItr(input.begin(), input.end(), re);
    boost::sregex_iterator termEndItr;
    vector<string> terms;
    vector<string> termOperators;
    size_t start = 0;
    for (; termItr != termEndItr; ++termItr) {
        size_t len = (*termItr).position() - start;
        string candidate = input.substr(start, len);
        terms.push_back(candidate);
        start = (*termItr).position() + (*termItr).length();
        termOperators.push_back((*termItr).str());
    }
    terms.push_back(input.substr(start)); // push back the last token
    // set the termOperators in container
    if (!termOperators.empty()) {
        this->populateTermBooleanOperator(termOperators.at(0)); // we only support one type of operator : AND
    }
    // parse the terms
    this->parseTerms(terms);
    // check if keywordFuzzyLevel was set by parseTerms
    // true? set the isFuzzyFlag.
    // else , empty the keywordFuzzyLevel vector
    if (this->container->hasParameterInSummary(KeywordFuzzyLevel)) {
        this->setInSummaryIfNotSet(IsFuzzyFlag);
        this->container->isFuzzy = true;
    } else {
        this->container->keywordFuzzyLevel.clear();
    }
    // check if KeywordBoostLevel was set
    if (!this->container->hasParameterInSummary(KeywordBoostLevel)) {
        this->container->keywordBoostLevel.clear(); // clear the boost level vector.
    }
    // check if QueryPrefixCompleteFlag was set
    if (!this->container->hasParameterInSummary(QueryPrefixCompleteFlag)) {
        this->container->keywordPrefixComplete.clear();
    }
    // cehck if FieldFilter was set
    if (!this->container->hasParameterInSummary(FieldFilter)) {
        // clear filedFilter vector and filedFilterOps vector
        this->container->fieldFilter.clear();
        this->container->fieldFilterOps.clear();
    }
    Logger::info("returning from  keywordParser.");
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
        this->container->termBooleanOperator = srch2::instantsearch::AND;
    } else if (boost::iequals("AND", termOperator)
            || termOperator.compare("&&") == 0) {
        this->container->termBooleanOperator = srch2::instantsearch::AND;
    } else {
        // generate MessageWarning and use AND
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "Invalid boolean operator specified as term boolean operator "
                                + termOperator
                                + ", ignoring it and using 'AND'."));
        this->container->termBooleanOperator = srch2::instantsearch::AND;
    }
    Logger::debug("returning from populateTermBooleanOperator.");
}
void QueryParser::populateFilterQueryTermBooleanOperator(
        const string &termOperator) {
    /*
     * populates teh termFQBooleanOperators in container.
     */
    // TODO: check for && and || also
    Logger::debug("inside populateFilterQueryTermBooleanOperators.");
    if (boost::iequals("OR", termOperator) || termOperator.compare("||") == 0) {
        this->container->termFQBooleanOperator = srch2::instantsearch::OR;
    } else if (boost::iequals("AND", termOperator)
            || termOperator.compare("&&") == 0) {
        this->container->termFQBooleanOperator = srch2::instantsearch::AND;
    } else {
        // generate MessageWarning and use AND
        this->container->messages.push_back(
                make_pair(MessageWarning,
                        "Invalid boolean operator specified as term boolean operator "
                                + termOperator
                                + ", ignoring it and using 'AND'."));
        this->container->termFQBooleanOperator = srch2::instantsearch::AND;
    }
    Logger::debug("returning from populateFilterQueryTermBooleanOperators.");
}

void QueryParser::parseTerms(const vector<string>&terms) { //TODO: change it to const vector<string &terms>
    /*
     * receives a vector of terms, field:keyword.
     * for each term it calls the parseTerm to parse the term.
     */
    Logger::debug("inside parseTerms function");
    string fieldKeywordDelimeterRegexString = "\\s*:\\s*";
    boost::regex fieldDelimeterRegex(fieldKeywordDelimeterRegexString); // TODO: regex to compile at engine start
    for (std::vector<string>::const_iterator termItr = terms.begin();
            termItr != terms.end(); ++termItr) {
        this->parseTerm(*termItr, fieldDelimeterRegex); // parse each term
    }
    Logger::debug("returning from  parseTerms function");
}

void QueryParser::parseTerm(const string &term, boost::regex &fieldDelimeterRegex) { //TODO: const string
    /*
     * example: field:keyword^3~.8
     * if ":" is present, we have field information, create a vector and populate the fieldFilter vector in container
     * else: check if lpFieldFilter in container has fields. if yes, create a vector of these fields and populate the vector
     * else: create an empty vector and poplate the fieldFilter vector in container.
     * in parallel populate the rawQueryKeywords vector in container.
     * this will need to populate boost and similarity boost vectors too. also add "NOT_DEFINED" in
     * prefixcomplete enum and populate the keywordPrefixComplete vector.
     * NOTE: populating fileds will also need to look for . and + in them and populate the fieldFilterOps vector.
     */
    Logger::debug("inside parseTerm funtion, parsing term: %s", term.c_str());
    string candidateKeyword;
    boost::smatch matches;
    boost::regex_search(term, matches, fieldDelimeterRegex);
    if (matches[0].matched) {
        // it has field. create a vector and populate container->fieldFilter.
        string fieldStr = term.substr(0, matches.position()); // extract the field
        this->populateFieldFilterUsingQueryFields(fieldStr);
        candidateKeyword = term.substr(matches.position() + matches.length()); // extract the keyword
    } else {
        // its a keyword, no field specified. look for the fields in localparameter
        this->populateFieldFilterUsingLp();
        candidateKeyword = term;
    }
    this->parseKeyword(candidateKeyword);
    Logger::debug("returning from  parseTerm function");
}

void QueryParser::parseKeyword(const string &input) {
    /*
     * parses the keywords
     * example: keyword*^3~.7
     * fills up rawkeywords, keyPrefixComp, boost, simBoost.
     */
// check if '^' is present
    if (input.find('^') != string::npos) {
        // '^ is present'
        this->setInSummaryIfNotSet(KeywordBoostLevel);
        Logger::debug("boost modifier used in query");
        boost::smatch matches;
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
                    this->container->lpKeywordBoostLevel); // sets the localParameter specified value. it's initial value is -1.
        }
    } else {
        Logger::debug("boost value is not specified, using the lp value or -1");
        this->container->keywordBoostLevel.push_back(
                this->container->lpKeywordBoostLevel); // selts the localParameter specified value
    }
    if (input.find('~') != string::npos) {
        // '~' is present
        this->setInSummaryIfNotSet(KeywordFuzzyLevel);
        Logger::debug("fuzzy modifier used in query");
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
            this->setFuzzyLevelInContainer(
                    this->container->lpKeywordFuzzyLevel); // selts the localParameter specified value
        }
    } else {
        Logger::debug("fuzzy value is not specified, use 0");
        this->setFuzzyLevelInContainer(0.0f);
    }
    if (input.length() > 1 && input.find('*') != string::npos) {
        this->setInSummaryIfNotSet(QueryPrefixCompleteFlag);
        // '*' is present
        Logger::debug("prefix modifier used in query");
        this->container->keywordPrefixComplete.push_back(
                srch2::instantsearch::TERM_TYPE_PREFIX);
    } else {
        // use the fallback specified in localparameters.
        this->container->keywordPrefixComplete.push_back(
                this->container->lpKeywordPrefixComplete);
    }
    QueryParser::populateRawKeywords(input);
}
void QueryParser::populateRawKeywords(const string &input) {
    /*
     * populates the raw keywords, that is the keyword without modifiers.
     * modifiers are *,^ and ~.
     * example: keyword^3 has keyword as a rawkeyword. this function populates the RawKeywords vector in container.
     */
    Logger::debug("indide populateRawKeywords, parsing for raw keywords");
    string regexString = "\\w+";
    boost::smatch matches;
    boost::regex re(regexString);
    boost::regex_search(input, matches, re);
    if (matches[0].matched) {
        this->setInSummaryIfNotSet(RawQueryKeywords);
        Logger::debug("raw keyword found: %s", matches[0].str().c_str());
        this->container->rawQueryKeywords.push_back(matches[0].str());
    } else if (0 == input.compare("*")) { //TODO check this while testing
        this->setInSummaryIfNotSet(RawQueryKeywords);
        Logger::debug("raw keyword found: *");
        this->container->rawQueryKeywords.push_back("*");
    } else {
        //no keyword specifed. // i think code will never reach here. need to ask
        ASSERT(false);
        this->container->rawQueryKeywords.push_back("");
        Logger::debug("No keyword specified");
    }
    Logger::debug("returning from populateRawKeywords");
}

void QueryParser::checkForBoostNums(const string &input,
        boost::smatch &matches) {
    /*
     * checks if boost value is present in the input
     * example: keyword^4 has boost value 4. keyword^ has no boost value
     */
    Logger::debug("inside checkForBoostNums");
    string boostRegexString = "\\^\\d+";
    boost::regex boostRegex(boostRegexString); // TODO: for all these functions compile the regex when the engine starts.
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
    string regexString = "\\~\\.\\d+";
    boost::regex re(regexString);
    boost::regex_search(input, matches, re);
    Logger::debug("returning from checkForFuzzyNums");
}
void QueryParser::extractNumbers(const string &input, boost::smatch& matches) {
    /*
     * extracts the numbers from the input string
     * example:  it will extract '8' from '~.8'.
     */
    Logger::debug("inside extractNumbers");
    string regexString = "\\d+";
    boost::regex re(regexString);
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
    if (!this->container->lpFieldFilter.empty()) {
        // lpFieldFilter is set. use this to create a vector
        this->setInSummaryIfNotSet(FieldFilter);
        this->container->fieldFilter.push_back(this->container->lpFieldFilter);
        if (this->container->isLpFieldFilterBooleanOperatorAssigned) {
            // LpFieldFilterBooleanOperator is assigned. fill
            this->container->fieldFilterOps.push_back(
                    this->container->lpFieldFilterBooleanOperator);
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
    this->setInSummaryIfNotSet(FieldFilter);
    const string fieldAndBoolOpDelimeterRegexString = "\\.";
    const string fieldOrBoolOpDelimeterRegexString = "\\+";
    string fieldBoolOpDelimeterRegexString;
    if (input.find('.') != string::npos) {
        fieldBoolOpDelimeterRegexString = fieldAndBoolOpDelimeterRegexString;
        this->container->fieldFilterOps.push_back(srch2::instantsearch::AND);
    } else if (input.find('+') != string::npos) {
        fieldBoolOpDelimeterRegexString = fieldOrBoolOpDelimeterRegexString;
        this->container->fieldFilterOps.push_back(srch2::instantsearch::OR);
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
        string candidate = input.substr(start, len);
        fields.push_back(candidate);
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
     *  set the summary.
     *
     */

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
        this->container->summary.push_back(GeoSearchType);
        this->container->summary.push_back(GeoTypeRectangular);
        this->container->geoParameterContainer = new GeoParameterContainer();
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
            this->container->summary.push_back(GeoSearchType);
            this->container->summary.push_back(GeoTypeCircular);
            this->container->geoParameterContainer =
                    new GeoParameterContainer();
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
                this->container->summary.push_back(GetAllResultsSearchType);
            } else {
                // it's a Top-K search
                this->container->summary.push_back(TopKSearchType);
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
        this->container->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->leftBottomLongitude = atof(
                leftBottomLongStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "lblong should be a valid float number"));
        this->container->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLatitude = atof(
                rightTopLatStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "rtlat should be a valid float number"));
        this->container->isParsedError = true;
    }
    if (isFloat(leftBottomLatStr)) {
        this->container->geoParameterContainer->rightTopLongitude = atof(
                rightTopLongStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "rtlong should be a valid float number"));
        this->container->isParsedError = true;
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
        this->container->geoParameterContainer->leftBottomLatitude = atof(
                centerLatStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError, "clat should be a valid float number"));
        this->container->isParsedError = true;
    }
    if (isFloat(centerLongStr)) {
        this->container->geoParameterContainer->leftBottomLongitude = atof(
                centerLongStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "clong should be a valid float number"));
        this->container->isParsedError = true;
    }
    if (isFloat(radiusParamStr)) {
        this->container->geoParameterContainer->rightTopLatitude = atof(
                radiusParamStr.c_str()); // convert the string to char* and pass it to atof
    } else {
        this->container->messages.push_back(
                make_pair(MessageError,
                        "radius should be a valid float number"));
        this->container->isParsedError = true;
    }
    Logger::debug("returning from setGeoContainerProperties");
}
void QueryParser::setInSummaryIfNotSet(ParameterName param) {
    /*
     * checks is a paramter is set in the container's summary. if not, it sets it.
     */
    Logger::debug("inside setInSummaryIfNotSet");
    if (!this->container->hasParameterInSummary(param)) {
        Logger::debug("parameter not in summary, setting it.");
        this->container->summary.push_back(param);
    }
    Logger::debug("returning from setInSummaryIfNotSet");
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
    if (this->container->hasParameterInSummary(IsFuzzyFlag)) {
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

}
}
