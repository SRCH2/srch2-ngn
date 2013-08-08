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

 * Copyright ������ 2013 SRCH2 Inc. All rights reserved
 */

#ifndef _WRAPPER_QUERYPARSER_H__
#define _WRAPPER_QUERYPARSER_H__ // TODO: two underscores
#include<cassert>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>

#include "Srch2ServerConf.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Schema.h"
#include "ParsedParameterContainer.h"

namespace srch2is = srch2::instantsearch;
using srch2is::Schema;
using srch2is::Analyzer;

namespace srch2 {
namespace httpwrapper {

class QueryParser {
public:

    QueryParser(const evkeyvalq &headers, ParsedParameterContainer * container);

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

    bool parse();

private:

    ParsedParameterContainer * container;
    const evkeyvalq & headers;

    // constants used by this class
    static const char* const fieldListDelimiter; //solr
    static const char* const fieldListParamName; //solr
    static const char* const debugControlParamName; //solr
    static const char* const debugParamName; //solr
    static const char* const startParamName; //solr
    static const char* const rowsParamName; //solr
    static const char* const timeAllowedParamName; //solr
    static const char* const ommitHeaderParamName; //solr
    static const char* const responseWriteTypeParamName; //solr
    static const char* const sortParamName; //solr  (syntax is not like solr)
    static const char* const sortFiledsDelimiter; // solr
    static const char* const orderParamName; //srch2
    static const char* const keywordQueryParamName; //solr
    static const char* const lengthBoostParamName; //srch2
    static const char* const prefixMatchPenaltyParamName; //srch2
    static const char* const filterQueryParamName; //solr
    static const char* const isFuzzyParamName; //srch2
    // local parameter params
    static const char* const localParamDelimiter; //solr
    static const char* const lpQueryBooleanOperatorParamName; //srch2
    static const char* const lpKeywordFuzzyLevelParamName; // srch2
    static const char* const lpKeywordBoostLevelParamName; // srch2
    static const char* const lpKeywordPrefixCompleteParamName; //srch2
    static const char* const lpFieldFilterParamName; //srch2
    static const char* const lpFieldFilterDelimiter; //srch2
    // rectangular geo params
    static const char* const leftBottomLatParamName; //srch2
    static const char* const leftBottomLongParamName; //srch2
    static const char* const rightTopLatParamName; //srch2
    static const char* const rightTopLongParamName; //srch2
    // circular geo params
    static const char* const centerLatParamName; //srch2
    static const char* const centerLongParamName; //srch2
    static const char* const radiusParamName; //srch2
    static const char* const facetParamName; //solr
    /*
     * example: q={param=key param2=key2}field1=keyword1 AND field2=keyword2* AND field3=keyword3*^2~.8
     * 1. Parse the string and find
     *      1.1 local parameters
     *      1.2 keyword string
     * 2. calls localParameterParser()
     * 3. calls the keywordParser();
     */
    void mainQueryParser();

    /*
     * checks to see if "fuzzy" exists in parameters.
     */
    void isFuzzyParser();

    /*
     * parses the lengthBoost parameter and fills up the container
     * example: lengthBoost=.9
     */
    void lengthBoostParser();

    /*
     * parses the pmp parameter and fills up the container
     * example: pmp=.8
     */
    void prefixMatchPenaltyParser();

    /*
     * example: debugQuery=True&debug=time
     * if there is a debug query parameter
     * then parse it and fill the container up
     */
    void debugQueryParser();

    /*
     * if there is a field list query parameter
     * then parse it and fill the container up
     *
     * Example: fl=field1,field2,field3 or fl=*
     */
    void fieldListParser();

    /*
     * if there is a start offset
     * fills the container up
     * example: start=40
     */
    void startOffsetParameterParser();

    /* aka: rows parser
     * if there is a number of results
     * fills the container up
     *
     * example: rows=20
     */
    void numberOfResultsParser();

    /*
     * it looks to see if we have a time limitation
     * if we have time limitation it fills up the container accordingly
     *
     * example: timeAllowed=1000
     * unit is millisec.
     */
    void timeAllowedParameterParser();

    /*
     * it looks to see if we have a omit header
     * if we have omit header it fills up the container accordingly.
     *
     * example: omitHeader=True
     */
    void omitHeaderParameterParser();

    /*
     * it looks to see if we have a responce type
     * if we have reponce type it fills up the container accordingly.
     * exmple: wt=JSON
     */
    void responseWriteTypeParameterParser();

    /*
     * it looks to see if there is any post processing filter
     * if there is then it fills up the container accordingly
     *
     * example: fq=Field1:[10 TO 100] AND field2:[* TO 100] AND field3:keyword
     *
     */
    void filterQueryParameterParser();

    /*
     * example:  facet=true&facet.field=filedName&facet.field=fieldName2&facet.range=fieldName3&f.fieldName3.range.start=10&f.fieldName3.range.end=100&f.fieldName3.range.gap=10
     * parses the parameters facet=true/false , and it is true it parses the rest of
     * parameters which are related to faceted search.
     */
    void facetParser();

    /*
     * example: sort=field1,field2,field3
     * looks for the parameter sort which defines the post processing sort job
     */
    void sortParser();

    /*
     * this function parses the local parameters section of all parts
     * input:
     *      1. local parameters string : {key=val key2=val2}
     * output:
     *      1. it fills up the metadata of the queryHelper object
     */
    void localParameterParser(string* input);

    /*
     * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
     * example: field:keyword^3 AND keyword2 AND keyword* AND keyword*^3 AND keyword^2~.5
     * output: fills up the container
     */
    void keywordParser(const string &input);

    /*
     * this function parsers only the parameters which are specific to Top-K
     */
    void topKParameterParser();

    /*
     * this function parsers only the parameters which are specific to get all results search type
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     */
    void getAllResultsParser();

    /*
     * this function parsers the parameters related to geo search like latitude and longitude .
     * 1. also calls the facet parser. : facetParser();
     * 2. also calls the sort parser: sortParser();
     *
     */
    void geoParser();
    /*
     * verifyies the syntax of the main query string.
     */
    bool verifyMainQuery(const string &input);
    /*
     * receives a vector of terms, field:keyword.
     * for each term it calls the parseTerm to parse the term.
     */
    void parseTerms(vector<string>& terms);
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
    void parseTerm(string &term, boost::regex &fieldDelimeterRegex);
    /*
     * populates the fieldFilter using localparamters.
     * example: q=keyword , has no fieldFilter specified. it will look into the lpFieldFilters for the
     * fallback and use that to populate the fieldFilter for this keyword
     */
    void populateFieldFilterUsingLp();
    /*
     * check if '.'s are present
     * check if '+' are present tokenize on . or + and
     * populate a vector<string> fields
     * populate the fieldFilterOps with given boolean operator
     */
    void populateFieldFilterUsingQueryFields(const string &input);
    /*
     * parses the keywords
     * example: keyword*^3~.7
     * fills up rawkeywords, keyPrefixComp, boost, simBoost.
     */
    void parseKeyword(string &input);
    /*
     * checks if boost value is present in the input
     * example: keyword^4 has boost value 4. keyword^ has no boost value
     */
    void checkForBoostNums(const string &input, boost::smatch &matches);
    /*
     * extracts the numbers from the input string
     * example:  it will extract '8' from '~.8'.
     */
    void extractNumbers(const string &input, boost::smatch &matches);
    /*
     * checks if the fuzzylevel is present in the input string
     * example: keyword~.8 has fuzzylevel as .8. keyword~ has no fuzzylevel specified.
     */
    void checkForFuzzyNums(const string &input, boost::smatch &matches);
    /*
     * populates the raw keywords, that is the keyword without modifiers.
     * modifiers are *,^ and ~.
     * example: keyword^3 has keyword as a rawkeyword. this function populates the RawKeywords vector in container.
     */
    void populateRawKeywords(const string &input);
    /*
     * populates the termBooleanOperator in the container
     */
    void populateTermBooleanOperator(const string &termOperator);
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
    void extractSearchType();
    /*
     * checks is a paramter is set in the container's summary. if not, it sets it.
     */
    void setInQueryParametersIfNotSet(ParameterName param);
    /*
     * sets the fuzzylevel in the container->keywordFuzzyLevel variable.
     * check if isFuzzyFlag is set
     *      true-> check if is fuzzy is true or false,
     *                  true -> use the fuzzylevel as specified
     *                  else -> set 0.0 as fuzzylevel
     *      false-> set the fuzzy level as specified
     */
    void setFuzzyLevelInContainer(const float f);
    /*
     * sets the rectangular geo paramters in the geocontainer.
     */
    void setGeoContainerProperties(const char* leftBottomLat,
            const char* leftBottomLong, const char* rightTopLat,
            const char* rightTopLong);
    /*
     * sets the geo paramters in the geocontainer.
     */
    void setGeoContainerProperties(const char*centerLat, const char* centerLong,
            const char* radiusParam);
    /*
     * example: orderby=desc
     * looks for the parameter orderby in headers and parses it.
     */
    string orderByParser();
    /*
     * populates teh fields vector related to facet.feild
     * example: facet.field=category
     */
    void populateFacetFieldsSimple(FacetQueryContainer &fqc);
    /*
     * populates the fields vector related to facet.range.
     * Also, calls the populateParallelRangeVectors function to populate the related parallel vectors.
     * example facet.range=price
     */
    void populateFacetFieldsRange(FacetQueryContainer &fqc);
    /*
     * populates the parallel vectors related to the facet parser.
     */
    void populateParallelRangeVectors(FacetQueryContainer &fqc, string &field);
    /*
     * verifies the syntax of filter query srting.
     */
    bool verifyFqSyntax(string &fq);
    /*
     * populates teh termFQBooleanOperators in container.
     */
    void populateFilterQueryTermBooleanOperator(const string &termOperator);
};

}
}

#endif //_WRAPPER_QUERYPARSER_H__
