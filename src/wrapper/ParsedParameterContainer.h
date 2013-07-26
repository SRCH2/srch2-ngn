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

typedef enum {

    RawQueryKeywords,
    IsFuzzyFlag,
    LengthBoostFlag,
    PrefixMatchPenaltyFlag,
    QueryBooleanOperatorFlag,
    KeywordFuzzyLevel,
    KeywordBoostLevel,
    FieldFilter,
    QueryPrefixCompleteFlag,
    IsDebugEnabled,
    ReponseAttributesList,
    ResultsStartOffset,
    NumberOfResults,
    MaxTimeAllowed,
    IsOmitHeader,
    ResponseFormat,
    FilterQueryEvaluatorFlag,
    TopKSearchType,
    GetAllResultsSearchType,
    GeoSearchType,
    // values related to search type specific parameters
    FacetQueryHandler,
    SortQueryHandler,
    GeoTypeRectangular,
    GeoTypeCircular
} ParameterName;

typedef enum {
    AND, OR
} QueryBooleanOperator;

typedef enum {
    PREFIX, COMPLETE
} QueryPrefixComplete;

typedef enum {
    TimingDebug, QueryDebug, ResultsDebug, CompleteDebug
} QueryDebugLevel;

typedef enum {
    JSON
} ResponseResultsFormat;

typedef enum {
    Ascending, Descending
} SortOrder;

typedef enum {
    Simple, Range
} FacetType;

typedef enum {
    Error, Warning
} MessageType;

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

class SortQueryContainer {

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
    std::vector<FacetType> types;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
};

class TopKParameterContainer {
public:
    // while we are parsing we populate this vector by the names of those members
    // which are set. It's a summary of the query parameters.
    std::vector<ParameterName> summary;

    // no parameters known as of now

    //
    bool doesHaveParameterInSummary(ParameterName param) {
        return (std::find(summary.begin(), summary.end(), param)
                != summary.end());
    }
};

class GetAllResultsParameterContainer {
public:
    GetAllResultsParameterContainer() {
        facetQueryContainer = NULL;
        sortQueryContainer = NULL;
    }
    ~GetAllResultsParameterContainer() {
        if (facetQueryContainer != NULL)
            delete facetQueryContainer;
        if (sortQueryContainer != NULL)
            delete sortQueryContainer;
    }

    // while we are parsing we populate this vector by the names of those members
    // which are set. It's a summary of the query parameters.
    std::vector<ParameterName> summary;

    // facet parser parameters
    FacetQueryContainer * facetQueryContainer;
    // sort parser parameters
    SortQueryContainer * sortQueryContainer;

    bool hasParameterInSummary(ParameterName param) {
        return (std::find(summary.begin(), summary.end(), param)
                != summary.end());
    }

};

class GeoParameterContainer {
public:
    GeoParameterContainer() {
        facetQueryContainer = NULL;
        sortQueryContainer = NULL;
    }
    ~GeoParameterContainer() {
        if (facetQueryContainer != NULL)
            delete facetQueryContainer;
        if (sortQueryContainer != NULL)
            delete sortQueryContainer;
    }
    // while we are parsing we populate this vector by the names of those members
    // which are set. It's a summary of the query parameters.
    std::vector<ParameterName> summary;

    // this object is created in planGenerator but freed when the filter is being destroyed.
    // facet parser parameters
    FacetQueryContainer * facetQueryContainer;
    // sort parser parameters
    SortQueryContainer * sortQueryContainer;

    // geo related parameters
    float leftBottomLatitude, leftBottomLongitude, rightTopLatitude,
            rightTopLongitude;
    float centerLatitude, centerLongitude, radius;

    bool hasParameterInSummary(ParameterName param) {
        return (std::find(summary.begin(), summary.end(), param)
                != summary.end());
    }
};

class ParsedParameterContainer {
public:
    ParsedParameterContainer() {
        filterQueryContainer = NULL;
        topKParameterContainer = NULL;
        getAllResultsParameterContainer = NULL;
        geoParameterContainer = NULL;
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
    std::vector<ParameterName> summary;

    // main query parser parameters

    // TODO add members related to local parameters
    QueryBooleanOperator lpQueryBooleanOperator; // TODO: when we want to all NOT or OR this part should change
    float lpKeywordFuzzyLevel;
    float lpKeywordBoostLevel;
    QueryPrefixComplete lpKeywordPrefixComplete;
    std::vector<std::string> lpFieldFilter;

    // localparamter related variables end
    bool isFuzzy;
    float lengthBoost;
    float prefixMatchPenalty;
    //// the following six vectors must be parallel
    std::vector<std::string> rawQueryKeywords;
    std::vector<float> keywordFuzzyLevel;
    std::vector<float> keywordBoostLevel;
    std::vector<srch2::instantsearch::TermType> keywordPrefixComplete;
    std::vector<std::vector<std::string> > fieldFilter;
    std::vector<srch2::instantsearch::BooleanOperation> fieldFilterOps;

    std::vector<unsigned> fieldFilterNumbers; // to be calculated in QueryRewriter based on field filter vectors

    BooleanOperation queryBooleanOperator; // TODO: when we want to all NOT or OR this part should change
    // debug query parser parameters
    bool isDebugEnabled;
    QueryDebugLevel queryDebugLevel;

    // field list parser
    std::vector<std::string> responseAttributesList;

    // start offset parser parameters
    unsigned resultsStartOffset;

    // number of results parser
    unsigned numberOfResults;

    // time allowed parser parameters
    unsigned maxTimeAllowed; // zero means no time restriction

    // omit header parser parameters
    bool isOmitHeader;

    // reponse write type parameters
    ResponseResultsFormat responseResultsFormat;

    // filter query parser parameters
    FilterQueryContainer * filterQueryContainer;

    // different search type specific parameters
    TopKParameterContainer * topKParameterContainer;
    GetAllResultsParameterContainer * getAllResultsParameterContainer;
    GeoParameterContainer * geoParameterContainer;

    /// messages for the query processing pipeline
    std::vector<std::pair<MessageType, std::string> > messages;

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

    bool hasParameterInSummary(ParameterName param) const {
        return (std::find(summary.begin(), summary.end(), param)
                != summary.end());
    }
};

}
}

#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
