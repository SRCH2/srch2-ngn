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

#include "ConfigManager.h"
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

	// this constructor is used for keyword search
    QueryParser(const evkeyvalq &headers, ParsedParameterContainer * container);
    // this constructor is used for suggestions
    QueryParser(const evkeyvalq &headers);

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

    // this parser function is used for suggestions. There are only two parameters for /suggest?
    // example: /suggest?k=can~0.6&rows=5
    // in this example keyword=can, editDistanceNormalizationFactor = 0.6 and numberOfSuggestionsToReturn = 5
    bool parseForSuggestions(string & keyword, float & fuzzyMatchPenalty,
    		int & numberOfSuggestionsToReturn , std::vector<std::pair<MessageType, std::string> > & messages);

    // add members related to local parameters
    //lpFieldFilterBooleanOperator is the boolean operator between the lpFieldFiter fields.
    bool isLpFieldFilterBooleanOperatorAssigned; // whether lpFieldFilterBooleanOperator is assigned or not.
    BooleanOperation lpFieldFilterBooleanOperator; // TODO: when we want to add NOT or OR this part should change
    std::vector<std::string> lpFieldFilter; // fallback fields to search a keyword in
    float lpKeywordSimilarityThreshold; // variable to store the fallback SimilarityThreshold specified in Local Parameters
    int lpKeywordBoostLevel; // stores the fallback boostLevel specified in Local Parameters .. TODO: change the type
    srch2::instantsearch::TermType lpKeywordPrefixComplete; // stores the fallback termType for keywords
    // localparamter related variables end
    bool isParsedError; // true -> there was error while parsing, false parsing was successful. no erros. Warnings may still be present.
    bool isSearchTypeSet; // whether the searchType has been set or not.
 //   bool isQfBoosted; // whether the query is dynamically boosted

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
    static const char* const orderDescending; //srch2
    static const char* const orderAscending; //srch2
    static const char* const keywordQueryParamName; //solr
    static const char* const suggestionKeywordParamName;
    static const char* const lengthBoostParamName; //srch2
    static const char* const prefixMatchPenaltyParamName; //srch2
    static const char* const filterQueryParamName; //solr
    static const char* const queryFieldBoostParamName;//solr
    static const char* const isFuzzyParamName; //srch2
    static const char* const docIdParamName;
    // local parameter params
    static const char* const lpKeyValDelimiter; //solr
    static const char* const lpQueryBooleanOperatorParamName; //srch2
    static const char* const lpKeywordSimilarityThresholdParamName; // srch2
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
    //facetParms
    static const char* const facetParamName; //solr
    static const char* const facetRangeGap;
    static const char* const facetRangeEnd;
    static const char* const facetRangeStart;
    static const char* const facetField;
    static const char* const facetRangeField;
    static const string getFacetRangeKey(const string &facetField,
            const string &facetRangeProperty) {
        return "f."+facetField+".facet."+facetRangeProperty;
    }
    static const string getFacetCategoricalNumberOfTopGroupsToReturn(const string &facetField){
    	return "f."+facetField+".rows";
    }
    //searchType
    static const char* const searchType;

    /*
     * example: docid=123
     * if docid is given in the query the rest of parameters will be ignored and the record with this primary key will
     * be returned.
     * If this function returns true it means docid is requested and parsing should be stopped for the rest of parameters.
     * if it returns false it means docid is not among query parameters.
     */
    bool docIdParser();


    /*
     * example: q={defaultSearchFields=Author defaultSimilarityThreshold=0.8}title:algo* AND publisher:mac* AND lang:engl*^2~0.8
     * 1. calls localParameterParser()
     * 2. calls the keywordParser();
     */
    void mainQueryParser();

    /*
     * checks to see if "fuzzy" exists in parameters.
     */
    void isFuzzyParser();

    /*
     * parses the lengthBoost parameter and fills up the container
     * example: 'lengthBoost=.9'
     */
    void lengthBoostParser();

    /*
     * parses the pmp parameter and fills up the container
     * example: 'pmp=0.8'
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
     * Example: 'fl=Author,Title,Name' or 'fl=*'
     */
    void fieldListParser();

    /*
     * if there is a start offset
     * fills the container up
     * example: 'start=40'
     */
    void startOffsetParameterParser();

    /* aka: rows parser
     * if there is a number of results
     * fills the container up
     *
     * example: 'rows=20'
     */
    void numberOfResultsParser();

    /*
     * it looks to see if we have a time limitation
     * if we have time limitation it fills up the container accordingly
     *
     * example: 'timeAllowed=1000'
     * unit is millisec.
     */
    void timeAllowedParameterParser();

    /*
     * it looks to see if we have a omit header
     * if we have omit header it fills up the container accordingly.
     *
     * example: 'omitHeader=True'
     */
    void omitHeaderParameterParser();

    /*
     * it looks to see if we have a responce type
     * if we have reponce type it fills up the container accordingly.
     * exmple: 'wt=JSON'
     */
    void responseWriteTypeParameterParser();

    /*
     * it looks to see if there is any post processing filter
     * if there is then it fills up the container accordingly
     *
     * example: 'fq=price:[10 TO 100] AND popularity:[* TO 100] AND Title:algorithm'
     *
     */
    bool filterQueryParameterParser();

    /*
     * it looks to see if there is any post processing dynamic boosting
     * if there is then it fills up the container accordingly
     *
     * example: 'qf=price^100+popularity^100'
     *
     */
    bool queryFieldBoostParser();

    /*
     * example:  facet=true&facet.field=Author&facet.field=Title&facet.range=price&f.price.range.start=10&f.price.range.end=100&f.price.range.gap=10
     * parses the parameters facet=true/false , and it is true it parses the rest of
     * parameters which are related to faceted search.
     */
    void facetParser();

    /*
     * example: 'sort=Author,Title'
     * looks for the parameter sort which defines the post processing sort job
     */
    void sortParser();

    /*
     * this function parses the local parameters section of all parts
     * input:
     *      1. local parameters string : '{defaultSearchFields=Author defaultSimilarityThreshold=0.7}'
     * output:
     *      1. it fills up the metadata of the queryHelper object
     */
    bool localParameterParser(string &input);

    /*
     * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
     * example: 'Author:gnuth^3 AND algorithms AND java* AND py*^3 AND binary^2~.5'
     * output: fills up the container
     */
    bool termParser(string &input);

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
     * populates the fieldFilter using localparamters.
     * example: 'q=gnuth' , has no fieldFilter specified. it will look into the lpFieldFilters for the
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
     * checks if boost value is present in the input
     * example: 'gnuth^4' has boost value '4'. 'gnuth^' has no boost value
     */
    void checkForBoostNums(const string &input, boost::smatch &matches);
    /*
     * extracts the numbers from the input string
     * example:  it will extract '8' from '~0.8'.
     */
    void extractNumbers(const string &input, boost::smatch &matches);
    /*
     * checks if the SimilarityThreshold is present in the input string
     * example: 'gnuth~0.8' has SimilarityThreshold as '0.8'. 'gnuth~' has no SimilarityThreshold specified.
     */
    void checkForFuzzyNums(const string &input, boost::smatch &matches);
    /*
     * populates the raw keywords, that is the keyword without modifiers.
     * modifiers are *,^ and ~.
     * example: 'gnuth^3' has 'gnuth' as a rawkeyword. this function populates the RawKeywords vector in container.
     */
    void populateRawKeywords(const string &input);
    /*
     * populates the termBooleanOperators in the container
     * checks if the termOperator is one of 'OR,||,AND,&&'
     *  true: sets the container's termBooleanOperator to the corresponding enum.
     *  function raises warning if termOperator is anything other than 'AND' or '&&'.
     *  and sets the termBooleanOperator to BooleanOperatorAND enum value.
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
     * sets the SimilarityThreshold in the container->keywordSimilarityThreshold variable.
     * check if isFuzzyFlag is set
     *      true-> check if is fuzzy is true or false,
     *                  true -> use the SimilarityThreshold as specified
     *                  else -> set 0.0 as SimilarityThreshold
     *      false-> set the fuzzy level as specified
     */
    void setSimilarityThresholdInContainer(const float f);
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
     * example: 'orderby=desc'
     * looks for the parameter orderby in headers and parses it.
     */
    string orderByParser();
    /*
     * populates teh fields vector related to facet.feild
     * example: 'facet.field=category'
     */
    void populateFacetFieldsSimple(FacetQueryContainer &fqc);
    /*
     * populates the fields vector related to facet.range.
     * Also, calls the populateParallelRangeVectors function to populate the related parallel vectors.
     * example 'facet.range=price'
     */
    void populateFacetFieldsRange(FacetQueryContainer &fqc);
    /*
     * populates the parallel vectors related to the facet parser.
     */
    void populateParallelRangeVectors(FacetQueryContainer &fqc, string &field);
    /*
     * populates teh termFQBooleanOperators in container.
     */
    void populateFilterQueryTermBooleanOperator(const string &termOperator);
    /*
     * parses the localparameter value from input string. It changes input string and populates the value
     * example. for input: 'Author defaultBoostLevel = 2}' ,the paramter 'value' will contain 'Author'
     *  and the 'input' will be changed to 'defaultBoostLevel = 2}'
     */
    bool parseLpValue(string &input, string &value);

    /*
     *parses the localparameter delemiter from input string. It changes input string.
     */
    bool parseLpDelimeter(string &input);
    /*
     * parses the localparameter key from input string. It changes input string and populates the field
     * example. for input: 'defaultSearchFields = Author defaultBoostLevel = 2}' ,the paramter field will be
     * 'defaultSearchFields' and the input will be changed to '= Author defaultBoostLevel = 2}'
     */
    bool parseLpKey(string &input, string &field);
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
    void setLpKeyValinContainer(const string &lpKey, const string &lpVal);
    /*
     * parses the boolean operator, the string must begin with it
     * example: 'AND defaultSearchFields:Author'.
     * output will be 'AND'
     */
    bool parseTermBoolOperator(string &input, string &output);
    /*
     * parses the field
     * example: 'Author:gnuth'
     * output will be 'Author'
     *
     */
    bool parseField(string &input, string &field);
    /**
     * parses the keyword string
     * example: 'gnuth*^~'
     * output will be 'gnuth'
     */
    bool parseKeyword(string &input, string &output);
    /*
     * parses the '*' keyword
     */
    bool parseKeyWordForAsteric(string &input, string &output);
    /*
     * parses the prefix modifier
     * example '*^4'
     * output will be '*'
     */
    bool parsePrefixModifier(string &input, string &output);
    /*
     * parses the boost modifier
     * eample: '^4~0.8'
     * output will be '^4'
     * input will be changed to ~0.8
     */
    bool parseBoostModifier(string &input, string &output);
    /*
     * populate boost info
     * Example: '^4'
     * if 'isParsed' is 'True', checks if the 'input' contains a number. In this example it contains '4'.
     * it will populate the keywordBoostLevel vector with 4.
     * Incase the input string was just '^', it will populate the keywordBoostLevel
     *  with the  lPdefaultBoostLevel value as specified in
     * local Parameters.
     */
    void populateBoostInfo(bool isParsed, string &input);
    /*
     * populate fuzzy info
     * Example: '~.4'
     * if 'isParsed' is 'True', checks if the 'input' contains a 'dot' followed by a number. In this example it contains '.4'.
     * it will populate the keywordSimilarityThreshold vector with 4.
     * Incase the input string was just '~', it will populate the keywordSimilarityThreshold
     *  with the  lPdefaultSimilarityThreshold value as specified in
     * local Parameters.
     */
    void populateFuzzyInfo(bool isParsed, string &input);
    /*
     *populate proximity info
     * Example: '~4'
     * if 'isParsed' is 'True', checks if the 'input' contains a number. In this example it contains '4'.
     * it will populate the keywordSimilarityThreshold vector with 4.
     * Incase the input string was just '~', it will populate the keywordSimilarityThreshold
     *  with the  lPdefaultSimilarityThreshold value as specified in
     * local Parameters.
     */
    void populateProximityInfo(bool isParsed, string &input);
    /*
     * example: input : '~0.8 AND Author:gnuth'
     * parses the fuzzy modifier.
     * input will be changed to 'AND Author:gnuth'
     * output will be '~0.8'
     */
    bool parseFuzzyModifier(string &input, string &output);
    /*
     * example '~8 AND Author:gnuth'
     * parses the proximity modifier.
     * input will be changed to 'AND Author:gnuth'
     * output will be '~8'
     */
    bool parseProximityModifier(string &input, string &output);
    /*
     * clears the keyword related parallel vectors if required.
     * Checks if the an entry is present for the following vectors in the parametersInQuery vector.
     * If no entry is present for a vector, that vector will be cleared
     * Following are the vectors:
     * keywordSimilarityThreshold
     * keywordBoostLevel
     * keywordPrefixComplete
     * fieldFilter
     * fieldFilterOps
     */
    void clearMainQueryParallelVectorsIfNeeded();
};

}
}

#endif //_WRAPPER_QUERYPARSER_H__
