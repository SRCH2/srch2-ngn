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
#include "util/SerializationHelper.h"


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

	virtual ResultsPostProcessorFilter * getNewCopy() const = 0;

	virtual ~ResultsPostProcessorFilter() {};

};

class ResultsPostProcessorPlanInternal;


// TODO : add an iterator class inside plan
class ResultsPostProcessorPlan
{
public:

	ResultsPostProcessorPlan();
	ResultsPostProcessorPlan(const ResultsPostProcessorPlan & plan);
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
    std::vector<int> numberOfTopGroupsToReturn;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;

    FacetQueryContainer(const FacetQueryContainer & contianer){
    	this->types = contianer.types;
    	this->numberOfTopGroupsToReturn = contianer.numberOfTopGroupsToReturn;
    	this->fields = contianer.fields;
    	this->rangeStarts = contianer.rangeStarts;
    	this->rangeEnds = contianer.rangeEnds;
    	this->rangeGaps = contianer.rangeGaps;
    }

    FacetQueryContainer(){};


    string toString();

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	void * serializeForNetwork(void * buffer);

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	static void * deserializeForNetwork(FacetQueryContainer & info, void * buffer);

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	unsigned getNumberOfBytesForSerializationForNetwork();

};

class SortEvaluator
{
public:
	// pass left and right value to compare. Additionally pass internal record id of both left
	// and right records which serve as tie breaker.
	virtual int compare(const std::map<std::string, TypedValue> & left , unsigned leftInternalRecordId,const std::map<std::string, TypedValue> & right, unsigned rightInternalRecordId) const = 0 ;
	virtual const std::vector<std::string> * getParticipatingAttributes() const = 0;
	virtual string toString() const = 0;
	virtual void * serializeForNetwork(void * buffer) const = 0;
	virtual unsigned getNumberOfBytesForSerializationForNetwork() const= 0;
	virtual ~SortEvaluator(){};
	virtual SortEvaluator * getNewCopy() const = 0;
	SortOrder order;
};

class RefiningAttributeExpressionEvaluator
{
public:
	virtual bool evaluate(std::map<std::string, TypedValue> & refiningAttributeValues) = 0 ;
	virtual ~RefiningAttributeExpressionEvaluator(){};
	virtual string toString() = 0;
	virtual void * serializeForNetwork(void * buffer) const = 0;
	virtual unsigned getNumberOfBytesForSerializationForNetwork() const= 0;
	virtual RefiningAttributeExpressionEvaluator * getNewCopy() const = 0;
};

class PhraseInfo{
    public:
	PhraseInfo(const PhraseInfo & info){
		this->proximitySlop = info.proximitySlop;
		this->attributeIdsList = info.attributeIdsList;
		this->attrOps = info.attrOps;
		this->keywordIds = info.keywordIds;
		this->phraseKeywordPositionIndex = info.phraseKeywordPositionIndex;
		this->phraseKeyWords = info.phraseKeyWords;
	}
	PhraseInfo(){

	}
	unsigned proximitySlop;
	vector<unsigned> keywordIds;
	vector<unsigned> phraseKeywordPositionIndex;
	vector<string> phraseKeyWords;
    vector<unsigned> attributeIdsList;
    ATTRIBUTES_OP attrOps;  // flag to indicate conjunction/disjunction in between attributes

	string toString();

	/*
	 * Serialization scheme :
	 * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
	 */
	void * serializeForNetwork(void * buffer) const ;
	/*
	 * Serialization scheme :
	 * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
	 */
	void * deserializeForNetwork(void * buffer) ;
	/*
	 * Serialization scheme :
	 * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
	 */
	unsigned getNumberOfBytesForSerializationForNetwork() const;

};


class PhraseSearchInfoContainer {
public:
	void addPhrase(const vector<string>& phraseKeywords,
			const vector<unsigned>& phraseKeywordsPositionIndex,
			unsigned proximitySlop,
			const vector<unsigned>& attributeIdsList, ATTRIBUTES_OP attrOps);
	PhraseSearchInfoContainer(){};
	PhraseSearchInfoContainer(const PhraseSearchInfoContainer & container){
		this->phraseInfoVector = container.phraseInfoVector;
	};

	vector<PhraseInfo> phraseInfoVector;

    string toString();

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	void * serializeForNetwork(void * buffer) const ;

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	static void * deserializeForNetwork(PhraseSearchInfoContainer & container, void * buffer);

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	unsigned getNumberOfBytesForSerializationForNetwork() const;
};

class ResultsPostProcessingInfo{
public:
	ResultsPostProcessingInfo();
	ResultsPostProcessingInfo(const ResultsPostProcessingInfo & info){
		if(info.facetInfo == NULL){
			this->facetInfo = NULL;
		}else{
			this->facetInfo = new FacetQueryContainer(*(info.facetInfo));
		}
		if(info.sortEvaluator == NULL){
			this->sortEvaluator = NULL;
		}else{
			this->sortEvaluator = info.sortEvaluator->getNewCopy();
		}
		if(info.filterQueryEvaluator == NULL){
			this->filterQueryEvaluator = NULL;
		}else{
			this->filterQueryEvaluator = info.filterQueryEvaluator->getNewCopy();
		}
		if(info.phraseSearchInfoContainer == NULL){
			this->phraseSearchInfoContainer = NULL;
		}else{
			this->phraseSearchInfoContainer = new PhraseSearchInfoContainer(*(info.phraseSearchInfoContainer));
		}
	}
	~ResultsPostProcessingInfo();
	FacetQueryContainer * getfacetInfo();
	void setFacetInfo(FacetQueryContainer * facetInfo);
	SortEvaluator * getSortEvaluator();
	void setSortEvaluator(SortEvaluator * evaluator);

	void setFilterQueryEvaluator(RefiningAttributeExpressionEvaluator * filterQuery);
	RefiningAttributeExpressionEvaluator * getFilterQueryEvaluator();

	void setPhraseSearchInfoContainer(PhraseSearchInfoContainer * phraseSearchInfoContainer);
	PhraseSearchInfoContainer * getPhraseSearchInfoContainer();


	string toString();

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	void * serializeForNetwork(void * buffer);

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	static void * deserializeForNetwork(ResultsPostProcessingInfo & info, void * buffer);

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	unsigned getNumberOfBytesForSerializationForNetwork();

private:
	FacetQueryContainer * facetInfo;
	SortEvaluator * sortEvaluator;
	RefiningAttributeExpressionEvaluator * filterQueryEvaluator;
	PhraseSearchInfoContainer * phraseSearchInfoContainer;
};


}
}


#endif // _RESULTSPOSTPROCESSOR_H_
