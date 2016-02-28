/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
        vector<unsigned> attributeIdsList;
        ATTRIBUTES_OP attrOps;  // flag to indicate conjunction/disjunction in between attributes

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
        	ss << attrOps;
        	for(unsigned i = 0 ; i < attributeIdsList.size() ; ++i ){
        		ss << attributeIdsList[i];
        	}
        	return ss.str();
        }
};


class PhraseSearchInfoContainer {
public:
	void addPhrase(const vector<string>& phraseKeywords,
			const vector<unsigned>& phraseKeywordsPositionIndex,
			unsigned proximitySlop,
			const vector<unsigned>& attributeIdsList, ATTRIBUTES_OP attrOps){

		PhraseInfo pi;
		pi.phraseKeywordPositionIndex = phraseKeywordsPositionIndex;
		pi.attributeIdsList = attributeIdsList;
		pi.attrOps = attrOps;
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
