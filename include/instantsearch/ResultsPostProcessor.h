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


#ifndef _RESULTSPOSTPROCESSOR_H_
#define _RESULTSPOSTPROCESSOR_H_

#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#include "instantsearch/Query.h"
#include "instantsearch/Schema.h"
#include "instantsearch/TypedValue.h"


namespace srch2
{
namespace instantsearch
{

class QueryEvaluator;
class QueryResults;

class ResultsPostProcessorFilter
{
public:
	virtual void doFilter(QueryEvaluator * queryEvaluator, const Query * query,
			 QueryResults * input , QueryResults * output) = 0;

	virtual ~ResultsPostProcessorFilter() {};

};

class ResultsPostProcessorPlanInternal;


// TODO : add an iterator class inside plan
class ResultsPostProcessorPlan
{
public:

	ResultsPostProcessorPlan();
	~ResultsPostProcessorPlan();
	void addFilterToPlan(ResultsPostProcessorFilter * filter);
	void clearPlan();
	void beginIteration();
	ResultsPostProcessorFilter * nextFilter();
	bool hasMoreFilters() const;
	void closeIteration();
private:
	ResultsPostProcessorPlanInternal * impl;
};

class FacetQueryContainer {

public:
    // these vectors must be parallel and same size all the time
    std::vector<srch2::instantsearch::FacetType> types;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
    std::vector<int> numberOfTopGroupsToReturn;

    string toString(){
    	stringstream ss;
    	for(unsigned index = 0 ; index < types.size() ; ++index){
    		ss << types[index] << fields[index].c_str() <<
    				rangeStarts[index].c_str() << rangeGaps[index].c_str() <<  rangeEnds[index].c_str();
    		if(index < numberOfTopGroupsToReturn.size()){
    			ss << numberOfTopGroupsToReturn[index] ;
    		}
    	}
    	return ss.str();
    }
};

class SortEvaluator
{
public:
	// pass left and right value to compare. Additionally pass internal record id of both left
	// and right records which serve as tie breaker.
	virtual int compare(const std::map<std::string, TypedValue> & left , unsigned leftInternalRecordId,const std::map<std::string, TypedValue> & right, unsigned rightInternalRecordId) const = 0 ;
	virtual const std::vector<std::string> * getParticipatingAttributes() const = 0;
	virtual string toString() const = 0;
	virtual ~SortEvaluator(){};
	SortOrder order;
};

class RefiningAttributeExpressionEvaluator
{
public:
	virtual bool evaluate(std::map<std::string, TypedValue> & refiningAttributeValues) = 0 ;
	virtual ~RefiningAttributeExpressionEvaluator(){};
	virtual string toString() = 0;
};

class PhraseInfo{
    public:
        vector<string> phraseKeyWords;
        vector<unsigned> keywordIds;
        vector<unsigned> phraseKeywordPositionIndex;
        unsigned proximitySlop;
        unsigned attributeBitMap;

        string toString(){
        	stringstream ss;
        	for(unsigned i = 0 ; i < phraseKeyWords.size() ; ++i ){
        		ss << phraseKeyWords[i].c_str();
        	}
        	for(unsigned i = 0 ; i < keywordIds.size() ; ++i ){
        		ss << keywordIds[i];
        	}
        	for(unsigned i = 0 ; i < phraseKeywordPositionIndex.size() ; ++i ){
        		ss << phraseKeywordPositionIndex[i];
        	}
        	ss << proximitySlop;
        	ss << attributeBitMap;
        	return ss.str();
        }
};


class PhraseSearchInfoContainer {
public:
	void addPhrase(const vector<string>& phraseKeywords,
			const vector<unsigned>& phraseKeywordsPositionIndex,
			unsigned proximitySlop,
			unsigned attributeBitMap){

		PhraseInfo pi;
		pi.phraseKeywordPositionIndex = phraseKeywordsPositionIndex;
		pi.attributeBitMap = attributeBitMap;
		pi.phraseKeyWords = phraseKeywords;
		pi.proximitySlop = proximitySlop;
		phraseInfoVector.push_back(pi);
	}
	vector<PhraseInfo> phraseInfoVector;

    string toString(){
    	stringstream ss;
    	for(unsigned i=0; i< phraseInfoVector.size() ; ++i){
    		ss << phraseInfoVector[i].toString().c_str();
    	}
    	return ss.str();
    }
};

class ResultsPostProcessingInfo{
public:
	ResultsPostProcessingInfo();
	~ResultsPostProcessingInfo();
	FacetQueryContainer * getfacetInfo();
	void setFacetInfo(FacetQueryContainer * facetInfo);
	SortEvaluator * getSortEvaluator();
	void setSortEvaluator(SortEvaluator * evaluator);

	void setFilterQueryEvaluator(RefiningAttributeExpressionEvaluator * filterQuery);
	RefiningAttributeExpressionEvaluator * getFilterQueryEvaluator();

	void setPhraseSearchInfoContainer(PhraseSearchInfoContainer * phraseSearchInfoContainer);
	PhraseSearchInfoContainer * getPhraseSearchInfoContainer();

	void setRoleId(string & roleId);
	string* getRoleId();


	string toString();
private:
	FacetQueryContainer * facetInfo;
	SortEvaluator * sortEvaluator;
	RefiningAttributeExpressionEvaluator * filterQueryEvaluator;
	PhraseSearchInfoContainer * phraseSearchInfoContainer;
	string roleId;
};


}
}


#endif // _RESULTSPOSTPROCESSOR_H_
