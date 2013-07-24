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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef _WRAPPER_PARSEDPARAMETERCONTAINER_H_
#define _WRAPPER_PARSEDPARAMETERCONTAINER_H_

#include <vector>
#include <string>
#include <map>


#include "FilterQueryEvaluator.h"
#include "SortFilterEvaluator.h"

namespace srch2
{
namespace httpwrapper
{



typedef enum{

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

typedef enum{
	AND,
	OR
} QueryBooleanOperator;

typedef enum{
	PREFIX,
	COMPLETE
} QueryPrefixComplete;

typedef enum{
	TimingDebug,
	QueryDebug,
	ResultsDebug,
	CompleteDebug
} QueryDebugLevel;


typedef enum{
	JSON
} ResponseResultsFormat;


typedef enum{
	Ascending,
	Descending
} SortOrder;

typedef enum{
	Simple,
	Range
} FacetType;

typedef enum{
	Error,
	Warning
} MessageType;

class FilterQueryContainer
{
public:
	FilterQueryEvaluator * evaluator;
};


class SortQueryContainer
{

public:

	SortFilterEvaluator * evaluator;
};


class FacetQueryContainer
{

public:
	// these vectors must be parallel and same size all the time
	std::vector<FacetType> types;
	std::vector<std::string> fields;
	std::vector<std::string> rangeStarts;
	std::vector<std::string> rangeEnds;
	std::vector<std::string> rangeGaps;
};

class TopKParameterContainer
{
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> summary;

	// no parameters known as of now
};

class GetAllResultsParameterContainer
{
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> summary;


	// facet parser parameters
	FacetQueryContainer * facetQueryContainer;
	// sort parser parameters
	SortQueryContainer * sortQueryContainer;

};

class GeoParameterContainer
{
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> summary;


	// facet parser parameters
	FacetQueryContainer * facetQueryContainer;
	// sort parser parameters
	SortQueryContainer * sortQueryContainer;

	// geo related parameters
	float leftBottomLatitude, leftBottomLongitude, rightTopLatitude, rightTopLongitude;
	float centerLatitude,centerLongitude,radius;
};



class ParsedParameterContainer
{
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> summary;

	// main query parser parameters

	// TODO add members related to local parameters
	std::vector<std::string> rawQueryKeywords;
	bool isFuzzy;
	float lengthBoost;
	float prefixMatchPenalty;
	QueryBooleanOperator queryBooleanOperator; // TODO: when we want to all NOT or OR this part should change
	std::vector<float> keywordFuzzyLevel;
	std::vector<float> keywordBoostLevel;
	std::vector<QueryPrefixComplete> keywordPrefixComplete;
	std::vector<std::string> fieldFilter;

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
	std::map<MessageType, std::string> messages;

	std::string getMessageString(){
		std::string result = "";
		for(std::map<MessageType, std::string>::iterator m = messages.begin();
										m != messages.end() ; ++m){
			switch (m->first) {
				case Error:
					result += "ERROR : " + m->second + "\n";
					break;
				case Warning:
					result += "WARNING : " + m->second + "\n";
					break;
			}
		}
		return result;
	}
};



}
}



#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
