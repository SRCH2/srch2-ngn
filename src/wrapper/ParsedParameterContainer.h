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

#ifndef _WRAPPER_QUERYBUILDERHELPER_H_
#define _WRAPPER_QUERYBUILDERHELPER_H_

#include <vector>
#include <string>

typedef enum{

	RawQueryKeywords,
	QueryBooleanOperator,
	KeywordFuzzyLevel,
	KeywordBoostLevel,
	QueryPrefixComplete,
	IsDebugEnabled,
	ReponseAttributesList,
	ResultsStartOffset,
	NumberOfResults,
	MaxTimeAllowed,
	IsOmitHeader,
	ResponseFormat,
	FilterQueryEvaluator,
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
	AND
} QueryBooleanOperator;

typedef enum{
	Prefix,
	Complete
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

class FilterQueryEvaluator
{

};


class SortQueryEvaluator
{

};


class FacetQueryEvaluator
{

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
	FacetQueryEvaluator * facetQueryEvaluator;
	// sort parser parameters
	SortQueryEvaluator * sortQueryEvaluator;

};

class GeoParameterContainer
{
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> summary;


	// facet parser parameters
	FacetQueryEvaluator * facetQueryEvaluator;
	// sort parser parameters
	SortQueryEvaluator * sortQueryEvaluator;

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
	QueryBooleanOperator queryBooleanOperator; // TODO: when we want to all NOT or OR this part should change
	std::vector<float> keywordFuzzyLevel;
	std::vector<float> keywordBoostLevel;
	std::vector<QueryPrefixComplete> keywordPrefixComplete;

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
	FilterQueryEvaluator * filterQueryEvaluator;


	// different search type specific parameters
	TopKParameterContainer * topKParameterContainer;
	GetAllResultsParameterContainer * getAllResultsParameterContainer;
	GeoParameterContainer * geoParameterContainer;

};








#endif // _WRAPPER_QUERYBUILDERHELPER_H_
