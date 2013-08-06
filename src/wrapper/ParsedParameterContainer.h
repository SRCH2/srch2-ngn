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
#include <algorithm>



#include "WrapperConstants.h"
#include "FilterQueryEvaluator.h"
#include "SortFilterEvaluator.h"

namespace srch2
{
namespace httpwrapper
{




class FilterQueryContainer
{
public:
	FilterQueryContainer(){
		evaluator = NULL;
	}
	~FilterQueryContainer(){
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
	SortQueryContainer(){
		evaluator = NULL;
	}
	~SortQueryContainer(){
		// do not free evaluator here, it's freed in filter
	}
	// this object is created in planGenerator but freed when the filter is being destroyed.
	SortFilterEvaluator * evaluator;
};


class FacetQueryContainer
{

public:
	// these vectors must be parallel and same size all the time
	std::vector<srch2::instantsearch::FacetType> types;
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
	std::vector<ParameterName> parametersInQuery;

	// no parameters known as of now

	//
	bool hasParameterInQuery(ParameterName param){
		return
				(std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};

class GetAllResultsParameterContainer
{
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

class GeoParameterContainer
{
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



class ParsedParameterContainer
{
public:

	ParsedParameterContainer(){
		filterQueryContainer = NULL;
		topKParameterContainer = NULL;
		getAllResultsParameterContainer = NULL;
		geoParameterContainer = NULL;
	}

	~ParsedParameterContainer(){
		if (filterQueryContainer != NULL) delete filterQueryContainer;
		if (topKParameterContainer != NULL) delete topKParameterContainer;
		if (getAllResultsParameterContainer != NULL) delete getAllResultsParameterContainer;
		if (geoParameterContainer != NULL) delete geoParameterContainer;
	}
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	// TODO : change to unordered set
	std::vector<ParameterName> parametersInQuery;

	// main query parser parameters

	// TODO add members related to local parameters
	bool isFuzzy;
	float lengthBoost;
	float prefixMatchPenalty;
	//// the following six vectors must be parallel
	std::vector<std::string> rawQueryKeywords;
	std::vector<float> keywordFuzzyLevel;
	std::vector<float> keywordBoostLevel;
	std::vector<srch2::instantsearch::TermType> keywordPrefixComplete;
	/*
	 * Example : for example if the query is like : name,author:foo AND *:bar AND name.body:foobar
	 * then the filter list will contain :
	 * <
	 * <name, author>,     <*>,    <name,body>
	 * >
	 * and the field operators will be :
	 * <
	 * AND,                AND,    OR
	 * >
	 */
	std::vector<std::vector<std::string> > fieldFilter;
	std::vector<srch2::instantsearch::BooleanOperation> fieldFilterOps;

	std::vector<unsigned> fieldFilterNumbers; // to be calculated in QueryRewriter based on field filter vectors


	BooleanOperation queryBooleanOperator; // TODO: when we want to all NOT or OR this part should change
	// debug query parser parameters
	bool isDebugEnabled; // not used yet
	QueryDebugLevel queryDebugLevel; // not used yet


	// field list parser , list of attributes to be returned to the user
	std::vector<std::string> responseAttributesList; // not used yet

	// start offset parser parameters
	unsigned resultsStartOffset;

	// number of results parser
	unsigned numberOfResults;

	// time allowed parser parameters
	unsigned maxTimeAllowed; // zero means no time restriction // not used yet

	// omit header parser parameters
	bool isOmitHeader; // not used yet

	// response write type parameters
	ResponseResultsFormat responseResultsFormat;

	// filter query parser parameters
	FilterQueryContainer * filterQueryContainer;


	// different search type specific parameters
	TopKParameterContainer * topKParameterContainer;
	GetAllResultsParameterContainer * getAllResultsParameterContainer;
	GeoParameterContainer * geoParameterContainer;



	/// messages for the query processing pipeline
	std::vector<std::pair<MessageType, std::string> > messages;

	std::string getMessageString(){
		std::string result = "";
		for(std::vector<std::pair<MessageType, std::string> >::iterator m = messages.begin();
										m != messages.end() ; ++m){
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

	bool hasParameterInQuery(ParameterName param) const{
		return
				(std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};



}
}



#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
