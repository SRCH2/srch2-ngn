//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */

#ifndef _WRAPPER_PARSEDPARAMETERCONTAINER_H_
#define _WRAPPER_PARSEDPARAMETERCONTAINER_H_

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "WrapperConstants.h"
#include "FilterQueryEvaluator.h"
#include "SortFilterEvaluator.h"

namespace srch2 {
namespace httpwrapper {

class FilterQueryContainer {
public:
    FilterQueryContainer() {
        evaluator = NULL;
    }
    ~FilterQueryContainer() {
        // do not free evaluator here, it's freed in filter
    }
    // this object is created in planGenerator but freed when the filter is being destroyed.
    FilterQueryEvaluator * evaluator;
};


/*
 * TODO : Extend this design to have multiple sort orders
 * for multiple sort fields ...
 */
class SortQueryContainer
{

public:
    SortQueryContainer() {
        evaluator = NULL;
    }
    ~SortQueryContainer() {
        // do not free evaluator here, it's freed in filter
    }
    // this object is created in planGenerator but freed when the filter is being destroyed.
    SortFilterEvaluator * evaluator;
};

class FacetQueryContainer {

public:
    // these vectors must be parallel and same size all the time
    std::vector<srch2::instantsearch::FacetType> types;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
};

class TopKParameterContainer {
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;

    // no parameters known as of now

	//
	bool hasParameterInQuery(ParameterName param){
		return
				(std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};

class GetAllResultsParameterContainer {
public:
	GetAllResultsParameterContainer(){
		facetQueryContainer = NULL;
		sortQueryContainer = NULL;
	}
	~GetAllResultsParameterContainer(){
		if(facetQueryContainer != NULL) delete facetQueryContainer;
		if(sortQueryContainer != NULL) delete sortQueryContainer;
	}

	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;


	// facet parser parameters
	FacetQueryContainer * facetQueryContainer;
	// sort parser parameters
	SortQueryContainer * sortQueryContainer;

	bool hasParameterInQuery(ParameterName param){
		return
				(std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}

};

class GeoParameterContainer {
public:
	GeoParameterContainer(){
		facetQueryContainer = NULL;
		sortQueryContainer = NULL;
	}
	~GeoParameterContainer(){
		if(facetQueryContainer != NULL) delete facetQueryContainer;
		if(sortQueryContainer != NULL) delete sortQueryContainer;
	}
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;


	// this object is created in planGenerator but freed when the filter is being destroyed.
	// facet parser parameters
	FacetQueryContainer * facetQueryContainer;
	// sort parser parameters
	SortQueryContainer * sortQueryContainer;

	// geo related parameters
	float leftBottomLatitude, leftBottomLongitude, rightTopLatitude, rightTopLongitude;
	float centerLatitude,centerLongitude,radius;

	bool hasParameterInQuery(ParameterName param){
		return
				(std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};

class ParsedParameterContainer {
public:

    ParsedParameterContainer() {
        filterQueryContainer = NULL;
        topKParameterContainer = NULL;
        getAllResultsParameterContainer = NULL;
        geoParameterContainer = NULL;
        this->isParsedError=false;
        this->isLpFieldFilterBooleanOperatorAssigned=false;
        lpKeywordFuzzyLevel = -1.0;
        lpKeywordBoostLevel = -1;
        lpKeywordPrefixComplete =
                    srch2::instantsearch::TERM_TYPE_NOT_SPECIFIED;
    }

    ~ParsedParameterContainer() {
        if (filterQueryContainer != NULL)
            delete filterQueryContainer;
        if (topKParameterContainer != NULL)
            delete topKParameterContainer;
        if (getAllResultsParameterContainer != NULL)
            delete getAllResultsParameterContainer;
        if (geoParameterContainer != NULL)
            delete geoParameterContainer;
    }
    // while we are parsing we populate this vector by the names of those members
    // which are set. It's a summary of the query parameters.
    std::vector<ParameterName> parametersInQuery;

    // add members related to local parameters
    //lpFieldFilterBooleanOperator is the boolean operator between the lpFieldFiter fields.
    bool isLpFieldFilterBooleanOperatorAssigned ; // whether lpFieldFilterBooleanOperator is assigned or not.
    BooleanOperation lpFieldFilterBooleanOperator; // TODO: when we want to add NOT or OR this part should change
    std::vector<std::string> lpFieldFilter; // fallback fields to search a keyword in
    float lpKeywordFuzzyLevel ; // variable to store the fallback fuzzyLevel specified in Local Parameters
    unsigned lpKeywordBoostLevel ; // stores the fallback boostLevel specified in Local Parameters .. TODO: change the type
    srch2::instantsearch::TermType lpKeywordPrefixComplete ; // stores the fallback termType for keywords
    // localparamter related variables end

    bool isFuzzy; // stores the value of query parameter 'fuzzy'. if fuzzy == True, use keyword's fuzzylevel as specified with keywords. else set fuzzy level to 0
    float lengthBoost; // store the value of lengthboost query parameter
    float prefixMatchPenalty; // stores the value of 'pmp' query parameter.

    // main query parser parameters
    // the following six vectors must be parallel
    std::vector<std::string> rawQueryKeywords; // stores the keywords in the query
    std::vector<float> keywordFuzzyLevel; // stores the fuzzy level of each keyword in the query
    std::vector<unsigned> keywordBoostLevel; // stores the boost level of each keyword in the query
    std::vector<srch2::instantsearch::TermType> keywordPrefixComplete; // stores whether the keyword is prefix or complete or not specified.
    std::vector<std::vector<std::string> > fieldFilter; // stores the fields where engine should search the corresponding keyword
    std::vector<srch2::instantsearch::BooleanOperation> fieldFilterOps; // stores the boolean operator for the corresponding filedFilter fields.
    std::vector<unsigned> fieldFilterNumbers; // to be calculated in QueryRewriter based on field filter vectors// we are not using it
    // debug query parser parameters
    bool isDebugEnabled;
    QueryDebugLevel queryDebugLevel; // variable to store the debug level. see enum QueryDebugLevel for details.

    // field list parser
    std::vector<std::string> responseAttributesList; // if not empty, response should have only these fields. else check config file.

    // start offset parser parameters
    unsigned resultsStartOffset; // start offset in the response vector. usefull in pagination

    // number of results parser
    unsigned numberOfResults; // number of records to return in the response. usefull in pagination

    // time allowed parser parameters
    unsigned maxTimeAllowed; // zero means no time restriction, max engine time allowed.

    // omit header parser parameters
    bool isOmitHeader; // true-> remove the header from the response and return only the records

    // reponse write type parameters
    ResponseResultsFormat responseResultsFormat; // JSON/XML etc

    // filter query parser parameters
    FilterQueryContainer * filterQueryContainer; // contains all filter query related info.

    // different search type specific parameters
    TopKParameterContainer * topKParameterContainer; // contains all Top k only parameters. currently none.
    GetAllResultsParameterContainer * getAllResultsParameterContainer; //getAllResults specefic params
    GeoParameterContainer * geoParameterContainer; // Geo specific params

    /// messages for the query processing pipeline
    // msgString to be added to a vector
    std::vector<std::pair<MessageType, string> > messages; // stores the messages related to warnings and errors.

    /*
     * function create and return the message string.
     */
    std::string getMessageString() {
        std::string result = "";
        for (std::vector<std::pair<MessageType, std::string> >::iterator m =
                messages.begin(); m != messages.end(); ++m) {
            switch (m->first) {
            case MessageError:
                result += "ERROR : " + m->second + "\n";
                break;
            case MessageWarning:
                result += "WARNING : " + m->second + "\n";
                break;
            }
        }
        return result;
    }

    /*
     * function to check whether the param is set in the summary vector or not.
     */
    bool hasParameterInQuery(ParameterName param) const {
        return (std::find(parametersInQuery.begin(), parametersInQuery.end(), param)
                != parametersInQuery.end());
    }

    // term operator vector
    // TODO:no need of this vector, change it to bool.
    BooleanOperation termBooleanOperator; // boolean operator between different query terms. e.g. field1:keyword1 AND field2:keyword2. AND is a termBoolean Operator
    // FilterQuery term operator vector
    //TODO:no need of this vector, change it to bool.
    BooleanOperation termFQBooleanOperator; // boolean operator between different filterQuery terms. fq=field:[10 TO 20] AND field2:keyword
    // parsed error?
    //TODO: move it close to the messages.
    bool isParsedError; // true -> there was error while parsing, false parsing was successful. no erros. Warnings may still be present.
};

}
}

#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
