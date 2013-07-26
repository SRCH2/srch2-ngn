#include "QueryParser.h"
#include "ParsedParameterContainer.h"
#include "ParserUtility.h"
#include <evhttp.h>
#include <string>
#include "boost/regex.hpp"

using namespace std;

namespace srch2 {
namespace httpwrapper {

const char* const QueryParser::fieldListDelimiter = ",";

const char* const QueryParser::fieldListParamName = "fl";
const char* const QueryParser::debugControlParamName = "debugQuery";
const char* const QueryParser::debugParamName = "debug";
const char* const QueryParser::startParamName = "start";
const char* const QueryParser::rowsParamName = "rows";
const char* const QueryParser::timeAllowedParamName = "timeAllowed";
const char* const QueryParser::ommitHeaderParamName = "omitHeader";
const char* const QueryParser::responseWriteTypeParamName = "wt";
const char* const QueryParser::sortParamName = "sort";
const char* const QueryParser::sortFiledsDelimiter = ",";
const char* const QueryParser::keywordQueryParamName = "q";
const char* const QueryParser::localParamDelimiter = "=";
const char* const lpQueryBooleanOperatorParamName = "defaultOperator";
const char* const lpKeywordFuzzyLevelParamName = "defaultfuzzyLevel";
const char* const lpKeywordBoostLevelParamName = "defaultBoostLevel";
const char* const lpKeywordPrefixCompleteParamName = "defaultPrefixComplete";
const char* const lpFieldFilterParamName = "defaultSearchFields";
const char* const lpFieldFilterDelimiter = ",";

QueryParser::QueryParser(const evkeyvalq &headers,
        ParsedParameterContainer * container) {
    this->container = container;
    this->headers = headers;
}

// parses the URL to a query object
void QueryParser::parse() {

    /*
     *  We have all the header information here which is the pairs of query parameters
     * 1. call the main query parser : mainQueryParser();
     * 2. call the debug parser: debugQueryParser();
     * 3. call the field list parser : fieldListParser();
     * 4. call the start parser: startOffsetParameterParser();
     * 5. call the row parser: numberOfResultsParser();
     * 6. call the time allowed parser: timeAllowedParameterParser();
     * 7. call the omit header parser : omitHeaderParameterParser();
     * 8. call the response writer parser : responseWriteTypeParameterParser();
     * 9. call the filter query parser : filterQueryParameterParser();
     * TODO : see what we should do about similarityBoost and lengthBoost
     * 10. based on the value of search type (if it's defined in local parameters we take it
     *    otherwise we get it from conf file) leave the rest of parsing to one of the parsers
     * 10.1: search type : Top-K
     * 		call topKParameterParser();
     * 10.2: search type : All results
     * 		call getAllResultsParser();
     * 10.3: search type : GEO
     * 		call getGeoParser();
     */

    // do some parsing
}

void QueryParser::mainQueryParser() { // TODO: change the prototype to reflect input/outputs

    /*
     * 1. Parse the string and find
     * 		1.1 local parameters
     * 		1.2 keyword string
     * 2. calls localParameterParser()
     * 3. calls the keywordParser();
     */
    // 1. get the mainQuery string.
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
            this->keywordParser(&mainQueryStr);
        } else {
            // invalid query return
        }
    } else {
        //
    }

}

bool verifyMainQuery(const string &input) {
    // TODO: move this regex block outside this class. We dont want this regex to be compiled everytime a query comes.
    const string lpRegexString =
            "\\{(\\w+\\s*=\\s*\\w+){1}(\\s+\\w+\\s*=\\s*\\w+)*[\\}]";
    std::string fieldRegexString = "(\\w+((\\.{0,1}\\w+)+|(\\+{0,1}\\w+)+)|\\*)"; // verifies the syntax of field.   e.g. checks the systax of "field" in field:keyword
    const string boostModRegexString = "\\^{0,1}\\d*"; // verifies the boost syntax. e.g. ^3 or ^
    const string fuzzyModRegexString = "~{0,1}(\\.\\d){0,1}"; // verifies the fuzzyness syntax. e.g. ~4 or ~
    const string modRegexString = boostModRegexString + fuzzyModRegexString; // combining both boost and fuzzy. user should specify them in this order only. e.g. ^3~.4.
    std::string keywordRegexString =
            "(\\.{0,1}\\w+(\\.{0,1}\\w+)+\\.{0,1}\\*{0,1}|\\*)"; // verifies the syntax of keyword
    const string keywordWithModsRegexString = keywordRegexString
            + modRegexString; // keyword + mod  (keyword^4~.3 or keyword^~.3 or keyword^2~ etc.)
    std::string termModRegexString = "(" + keywordWithModsRegexString + "|"
            + fieldRegexString + ":" + keywordWithModsRegexString + ")"; // verifies the syntax of full "file:keyword"

    std::string queryRegexString = "^(" + lpRegexString + "){0,1}\\s*"
            + termModRegexString + "(\\s+(AND|&&)\\s+" + termModRegexString + ")*\\s*"; // verifies the systax of complete query string.
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
    return boost::regex_match(input, queryRegex);
}

void QueryParser::isFuzzyParser() {
    /*
     * checks to see if "fuzzy" exists in parameters.
     */
}

void QueryParser::lengthBoostParser() {

}

void QueryParser::prefixMatchPenaltyParser() {

}

void QueryParser::fieldListParser() {
    /*
     * if there is a field list query parameter
     * then parse it and fill the helper up
     *
     * Example: fl=field1,field2,field3 or fl=*
     */
    //1. first check to see if fl exists in the headers
    const char * flTemp = evhttp_find_header(&headers,
            QueryParser::fieldListParamName);
    if (flTemp) { // if this parameter exists
        size_t st;
        string fl = evhttp_uridecode(flTemp, 0, &st);

        this->container->summary.push_back(
                srch2::httpwrapper::ReponseAttributesList);

        char * pch = strtok(fl.c_str(), QueryParser::fieldListDelimiter);

        while (pch) {

            string field = pch;
            if (field == "*") {
                this->container->responseAttributesList.clear();
                this->container->responseAttributesList.push_back("*");
                return;
            }

            this->container->responseAttributesList.push_back(field);

            //
            pch = strtok(fl.c_str(), NULL);
        }

    }

}

void QueryParser::debugQueryParser() {
    /*
     * if there is a debug query parameter
     * then parse it and fill the helper up
     */
    //1. first check to see if debugQuery exists in the headers
    const char * debugQueryTemp = evhttp_find_header(&headers,
            QueryParser::debugControlParamName);
    if (debugQueryTemp) { // if this parameter exists
        size_t st;
        string debugQuery = evhttp_uridecode(debugQueryTemp, 0, &st);
        if (boost::iequals(debugQuery, 'true')) {
            this->container->isDebugEnabled = true;
            this->container->summary.push_back(IsDebugEnabled); // change the IsDebugEnabled to DebugEnabled in Enum ParameterName ?
            // look for debug paramter. it decides the debug level, if it is not set, set the debug level to complete.
            const char * debugTemp = evhttp_find_header(&headers,
                    QueryParser::debugParamName);
            if (debugTemp) { // if this parameter exists
                size_t st;
                string debug = evhttp_uridecode()(debugTemp, 0, &st);
                //check what is the debug level
                if (boost::iequals("true", debug)) {
                    this->container->queryDebugLevel = CompleteDebug;
                } else if (boost::iequals("query", debug)) {
                    this->container->queryDebugLevel = QueryDebug;
                } else if (boost::iequals("result", debug)) {
                    this->container->queryDebugLevel = ResultsDebug;
                } else if (boost::iequals("timing", debug)) {
                    this->container->queryDebugLevel = TimingDebug;
                } else {
                    // not supported level, generate a warning message and set debug to complete.
                    this->container->messages.insert(
                            std::pair<MessageType, string>(Warning,
                                    "Unknown value for parameter debug. using debug=true"));
                    this->container->queryDebugLevel = CompleteDebug;
                }
            } else {
                // the debug level not specified, use debug level = true
                this->container->queryDebugLevel = CompleteDebug;
            }
        }
    }
}

void QueryParser::startOffsetParameterParser() {
    /*
     * if there is a start offset
     * fills the helper up
     */
    // 1. check for start parameter.
    const char * startTemp = evhttp_find_header(&headers,
            QueryParser::startParamName);
    if (startTemp) { // if this parameter exists
        size_t st;
        string startStr = evhttp_uridecode(startTemp, 0, &st);
        // convert the startStr to integer.
        this->container->resultsStartOffset = atoi(startStr.c_str()); // convert the string to char* and pass it to atoi
        // populate the summary
        this->container->summary.push_back(ResultsStartOffset);
    }
}

void QueryParser::numberOfResultsParser() {
    /* aka: rows parser
     * if there is a number of results
     * fills the helper up
     */
    // 1. check for rows parameter.
    const char * rowsTemp = evhttp_find_header(&headers,
            QueryParser::rowsParamName);
    if (rowsTemp) { // if this parameter exists
        size_t st;
        string rowsStr = evhttp_uridecode(rowsTemp, 0, &st);
        // convert the rowsStr to integer.
        this->container->numberOfResults = atoi(rowsStr.c_str()); // convert the string to char* and pass it to atoi
        // populate the summary
        this->container->summary.push_back(NumberOfResults);
    }
}

void QueryParser::timeAllowedParameterParser() {
    /*
     * it looks to see if we have a time limitation
     * if we have time limitation it fills up the helper accordingly
     */
    const char * timeAllowedTemp = evhttp_find_header(&headers,
            QueryParser::timeAllowedParamName);
    if (timeAllowedTemp) { // if this parameter exists
        size_t st;
        string timeAllowedStr = evhttp_uridecode(timeAllowedTemp, 0, &st);
        // convert the Str to integer.
        this->container->maxTimeAllowed = atoi(timeAllowedStr.c_str()); // convert the string to char* and pass it to atoi
        // populate the summary
        this->container->summary.push_back(MaxTimeAllowed);
    }
}

void QueryParser::omitHeaderParameterParser() {
    /*
     * it looks to see if we have a omit header
     * if we have omit header it fills up the helper accordingly.
     */
    const char * ommitHeaderTemp = evhttp_find_header(&headers,
            QueryParser::ommitHeaderParamName);
    if (ommitHeaderTemp) { // if this parameter exists
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
}

void QueryParser::responseWriteTypeParameterParser() {
    /*
     * it looks to see if we have a responce type
     * if we have reponce type it fills up the helper accordingly.
     */
    const char * responseWriteTypeTemp = evhttp_find_header(&headers,
            QueryParser::responseWriteTypeParamName);
    if (responseWriteTypeTemp) { // if this parameter exists
        size_t st;
        string responseWriteType = evhttp_uridecode(responseWriteTypeTemp, 0,
                &st);
        // check if "true"
        if (boost::iequals("json", responseWriteType)) {
            this->container->responseResultsFormat = JSON;
        } else {
            // create warning, we only support json as of now.
            this->container->messages.insert(
                    make_pair(Warning,
                            "Unknown value for parameter wt. using wt=json"));
            this->container->responseResultsFormat = JSON; // this is default.
        }
        // populate the summary
        this->container->summary.push_back(ResponseFormat); // should we change this ParameterName to OmitHeader?
    }
}

void QueryParser::filterQueryParameterParser() {
    /*
     * it looks to see if there is any post processing filter
     * if there is then it fills up the helper accordingly
     */
}

void QueryParser::facetParser() {
    /*
     * parses the parameters facet=true/false , and it is true it parses the rest of
     * parameters which are related to faceted search.
     */

}

void QueryParser::sortParser() {
    /*
     * looks for the parameter sort which defines the post processing sort job
     */
    const char * sortTemp = evhttp_find_header(&headers,
            QueryParser::sortParamName);
    if (sortTemp) { // if this parameter exists
        size_t st;
        string sortString = evhttp_uridecode(sortTemp, 0, &st);
        // we have sortString, we need to tokenize this string and populate the
        // parameters in SortQueryEvaluator class object.
    }
}

void QueryParser::localParameterParser(string *input) {
    /* TODO: break this funcion into smaller functions like:
     * it checks if localparamertes are present in the input. extracts the key/value pairs and puts them in the helper.
     */
    // check if input string might have a local parameter info
    std::string localParametersString; // string to contain the localparameter string only
    std::string lpRegexString =
            "\\{(\\w+\\s*=\\s*\\w+){1}(\\s+\\w+\\s*=\\s*\\w+)*\\}";
    boost::regex localParameterRegex(lpRegexString); // TODO: compile this regex when the engine starts. that would be more efficient.
    boost::smatch clpMatches;
    if (boost::regex_search(*input, clpMatches, localParameterRegex)) {
        // mathc found input has localParameters
        // remove the localparameter string part form the input.
        input = boost::regex_replace(*input, localParameterRegex, ""); // input is modified. lp info is being removed.
        localParametersString = clpMatches[0];  // get the locaparamter part
        // now get the pairs from the local parameter string
        string lpPairsRegexString = "\\w+\\s*=\\s*\\w+"; // regex to get field = val pairs from {field1=val1 field2 = val2}
        boost::regex lPPairRegex(lpPairsRegexString); //TODO: compile this regex when the engine starts.
        boost::sregex_token_iterator itr(localParametersString.begin(),
                localParametersString.end(), lpPairsRegexString, 0); // get the iterator for the matches
        boost::sregex_token_iterator end;
        // iterate on matched pairs
        // parse the key value pairs and populate the container
        for (; itr != end; ++itr) {
            string pair = *itr;
            // split by "=" (localParamDelimiter) and fill the container
            char *result = strdup((*input).c_str()); // strtok takes char* and not const char* so creating duplicate of input.
            char * token = strtok(result, localParamDelimiter);
            vector<string> tokens;
            while (token) { // should give two tokesns only
                // this first token is filed name and second is its value
                // get the local parameter field
                boost::algorithm::trim(token);
                tokens.push_back(token);
                token = strtok(NULL, localParamDelimiter);
            }
            if (lpQueryBooleanOperatorParamName == tokens[0]) {
                string val = tokens[1];
                boost::to_upper(val); // convert to upper case.
                if ("OR" == val) {
                    //this->container->lpQueryBooleanOperator = OR; // set default operator as OR,
                    // we do not support OR as of now so raising a warning and setting it to AND.
                    // generate warning and use AND
                    this->container->messages.insert(
                            std::pair<MessageType, string>(Warning,
                                    "We do not supprt OR  specified, ignoring it and using 'AND'."));
                    this->container->lpQueryBooleanOperator = AND;
                } else if ("AND" == val) {
                    this->container->lpQueryBooleanOperator = AND;
                } else {
                    // generate warning and use AND
                    this->container->messages.insert(
                            std::pair<MessageType, string>(Warning,
                                    "Invalid boolean operator specified. " + val
                                            + ", ignoring it and using 'AND'."));
                    this->container->lpQueryBooleanOperator = AND;
                }
            } else if (lpKeywordFuzzyLevelParamName == tokens[0]) { // i tried using vecotr.at(index) showed me compile errors.
                string val = tokens[1];
                float f = atof(val.c_str());
                this->container->lpKeywordFuzzyLevel = f;
            } else if (lpKeywordPrefixCompleteParamName == tokens[0]) {
                string val = tokens[1];
                boost::to_upper(val);
                if ("PREFIX" == val) {
                    this->container->lpKeywordPrefixComplete = PREFIX;
                } else if ("COMPLETE" == val) {
                    this->container->lpKeywordPrefixComplete = COMPLETE;
                } else {
                    // generate warning and use prefix
                    this->container->messages.insert(
                            std::pair<MessageType, string>(Warning,
                                    "Invalid choice " + val
                                            + ",we support prefix and complete search on keyword only. ignoring it and using 'Prefix'."));
                    this->container->lpKeywordPrefixComplete = PREFIX;
                }
            } else if (lpFieldFilterParamName == tokens[0]) {
                string val = tokens[1];
                // val might be a comma separated string of fields.  field1,field2..
                // tokenize it on ',' and set the vector in container.
                char *fieldStr = strdup(val.c_str()); // the strtok function takes char* and not const char* so create dulpicate of val as char*.
                char * fieldToken = strtok(fieldStr, lpFieldFilterDelimiter);
                while (fieldToken) {
                    boost::algorithm::trim(fieldToken); // trim the token
                    this->container->lpFieldFilter.push_back(fieldToken); // set field in container
                    fieldToken = strtok(NULL, lpFieldFilterDelimiter); // get the next filed. (the syntax is weird, but thats how it works.)
                }
                free(fieldStr); // free the fieldStr, duplicate of val.
            } else {
                // this localparameter is not supported. raise a warning msg.
                this->container->messages.insert(
                        std::pair<MessageType, string>(Warning,
                                "Invalid local parameter " + tokens[0]
                                        + ", ignoring it."));
            }
            free(result); // free the result, duplicate of input.
        }

    } else {
        // does not contain any localParameter info.
    }
}

void QueryParser::keywordParser(string * input) {
    /*
     * parses the keyword string and fills up the helper by term information
     */
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
     * TODO: do we need to get any other parameters here ?
     */

}

void QueryParser::getGeoParser() {

    /*
     * this function parsers the parameters related to geo search like latitude and longitude .
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     * 3. parses the geo parameters like leftBottomLatitude,leftBottomLongitude,rightTopLatitude,rightTopLongitude
     * 		centerLatitude,centerLongitude,radius
     * 4. Based on what group of geo parameters are present it sets geoType to CIRCULAR or RECTANGULAR
     */

}

// creates a post processing plan based on information from Query
// TODO : query should be removed from the inputs of this function. This function should only return plan based on header

}
}
