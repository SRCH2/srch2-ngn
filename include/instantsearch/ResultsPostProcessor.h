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
#include "wrapper/SortFilterEvaluator.h"
#include "wrapper/FilterQueryEvaluator.h"


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
    std::vector<int> numberOfTopGroupsToReturn;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;

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

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	void * serializeForNetwork(void * buffer){
		buffer = srch2::util::serializeVectorOfFixedTypes(types, buffer);
		buffer = srch2::util::serializeVectorOfFixedTypes(numberOfTopGroupsToReturn, buffer);
		buffer = srch2::util::serializeVectorOfString(fields, buffer);
		buffer = srch2::util::serializeVectorOfString(rangeStarts, buffer);
		buffer = srch2::util::serializeVectorOfString(rangeEnds, buffer);
		buffer = srch2::util::serializeVectorOfString(rangeGaps, buffer);
		return buffer;
	}

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	static void * deserializeForNetwork(FacetQueryContainer & info, void * buffer){
		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, info.types);
		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, info.numberOfTopGroupsToReturn);
		buffer = srch2::util::deserializeVectorOfString(buffer, info.fields);
		buffer = srch2::util::deserializeVectorOfString(buffer, info.rangeStarts);
		buffer = srch2::util::deserializeVectorOfString(buffer, info.rangeEnds);
		buffer = srch2::util::deserializeVectorOfString(buffer, info.rangeGaps);
		return buffer;
	}

    /*
     * Serialization scheme :
     * | types | numberOfTopGroupsToReturn | fields | rangeStarts | rangeEnds | rangeGaps |
     */
	unsigned getNumberOfBytesForSerializationForNetwork(){
		unsigned numberOfBytes = 0;
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(types);
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(numberOfTopGroupsToReturn);
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(fields);
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeStarts);
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeEnds);
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeGaps);
		return numberOfBytes;
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
	virtual void * serializeForNetwork(void * buffer) const = 0;
	virtual static void * deserializeForNetwork(SortEvaluator & info, void * buffer) = 0;
	virtual unsigned getNumberOfBytesForSerializationForNetwork() const= 0;
	virtual ~SortEvaluator(){};
	SortOrder order;
};

class RefiningAttributeExpressionEvaluator
{
public:
	virtual bool evaluate(std::map<std::string, TypedValue> & refiningAttributeValues) = 0 ;
	virtual ~RefiningAttributeExpressionEvaluator(){};
	virtual string toString() = 0;
	virtual void * serializeForNetwork(void * buffer) const = 0;
	virtual static void * deserializeForNetwork(RefiningAttributeExpressionEvaluator & info, void * buffer) = 0;
	virtual unsigned getNumberOfBytesForSerializationForNetwork() const= 0;
};

class PhraseInfo{
    public:
        unsigned proximitySlop;
        unsigned attributeBitMap;
        vector<unsigned> keywordIds;
        vector<unsigned> phraseKeywordPositionIndex;
        vector<string> phraseKeyWords;

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

        /*
         * Serialization scheme :
         * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
         */
    	void * serializeForNetwork(void * buffer) const {
    		buffer = srch2::util::serializeFixedTypes(proximitySlop,  buffer);
    		buffer = srch2::util::serializeFixedTypes(attributeBitMap,  buffer);
    		buffer = srch2::util::serializeVectorOfFixedTypes(keywordIds,  buffer);
    		buffer = srch2::util::serializeVectorOfFixedTypes(phraseKeywordPositionIndex,  buffer);
    		buffer = srch2::util::serializeVectorOfString(phraseKeyWords,  buffer);
    		return buffer;
    	}
        /*
         * Serialization scheme :
         * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
         */
    	void * deserializeForNetwork(void * buffer) {
    		buffer = srch2::util::deserializeFixedTypes(buffer, proximitySlop);
    		buffer = srch2::util::deserializeFixedTypes(buffer, attributeBitMap);
    		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, keywordIds);
    		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, phraseKeywordPositionIndex);
    		buffer = srch2::util::deserializeVectorOfString(buffer, phraseKeyWords);
    		return buffer;
    	}
        /*
         * Serialization scheme :
         * | proximitySlop | attributeBitMap | keywordIds  | phraseKeywordPositionIndex | phraseKeyWords |
         */
    	unsigned getNumberOfBytesForSerializationForNetwork() const{
    		unsigned numberOfBytes = 0;
    		numberOfBytes += sizeof(attributeBitMap);
    		numberOfBytes += sizeof(proximitySlop);
    		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(keywordIds);
    		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(phraseKeywordPositionIndex);
    		numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(phraseKeyWords);
    		return numberOfBytes;
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

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	void * serializeForNetwork(void * buffer) const {
		buffer = srch2::util::serializeFixedTypes(phraseInfoVector.size(),  buffer);
		for(unsigned  pIndex = 0; pIndex < phraseInfoVector.size(); ++pIndex){
			phraseInfoVector.at(pIndex).serializeForNetwork(buffer);
		}
		return buffer;
	}

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	static void * deserializeForNetwork(PhraseSearchInfoContainer & container, void * buffer) {
		unsigned numberOfPositionInfos = 0;
		buffer = srch2::util::deserializeFixedTypes(buffer, numberOfPositionInfos);
		for(unsigned posInfoIndex = 0; posInfoIndex < numberOfPositionInfos; ++posInfoIndex){
			PhraseInfo * pi = new PhraseInfo();
			container.phraseInfoVector.push_back(*pi);
			buffer = pi->deserializeForNetwork(buffer);
		}
		return buffer;
	}

    /*
     * Serialization scheme :
     * | phraseInfoVector |
     */
	unsigned getNumberOfBytesForSerializationForNetwork() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += sizeof(unsigned);
		for(unsigned posInfoIndex = 0; posInfoIndex < phraseInfoVector.size(); ++posInfoIndex){
			numberOfBytes += phraseInfoVector.at(posInfoIndex).getNumberOfBytesForSerializationForNetwork();
		}
		return numberOfBytes;
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


	string toString();

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	void * serializeForNetwork(void * buffer){
		// first serializeForNetwork 4 flags to know which ones are null
		// if poiters are not null save the object
		buffer = srch2::util::serializeFixedTypes(facetInfo != NULL, buffer);
		buffer = srch2::util::serializeFixedTypes(sortEvaluator != NULL, buffer);
		buffer = srch2::util::serializeFixedTypes(filterQueryEvaluator != NULL, buffer);
		buffer = srch2::util::serializeFixedTypes(phraseSearchInfoContainer != NULL, buffer);

		if(facetInfo != NULL){
			buffer = facetInfo->serializeForNetwork(buffer);
		}
		if(sortEvaluator != NULL){
			buffer = sortEvaluator->serializeForNetwork(buffer);
		}
		if(filterQueryEvaluator != NULL){
			buffer = filterQueryEvaluator->serializeForNetwork(buffer);
		}
		if(phraseSearchInfoContainer != NULL){
			buffer = phraseSearchInfoContainer->serializeForNetwork(buffer);
		}

		return buffer;
	}

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	static void * deserializeForNetwork(ResultsPostProcessingInfo & info, void * buffer){

		bool isFacetInfoNotNull = false;
		buffer = srch2::util::serializeFixedTypes(isFacetInfoNotNull, buffer);
		bool isSortEvaluatorNotNull = false;
		buffer = srch2::util::serializeFixedTypes(isSortEvaluatorNotNull, buffer);
		bool isFilterQueryEvaluatorInfoNotNull = false;
		buffer = srch2::util::serializeFixedTypes(isFilterQueryEvaluatorInfoNotNull, buffer);
		bool isPhraseSearchInfoContainerNotNull = false;
		buffer = srch2::util::serializeFixedTypes(isPhraseSearchInfoContainerNotNull, buffer);

		if(isFacetInfoNotNull){
			info.facetInfo = new FacetQueryContainer();
			buffer = FacetQueryContainer::deserializeForNetwork(*(info.facetInfo), buffer);
		}
		if(isSortEvaluatorNotNull){
			info.sortEvaluator = new srch2::httpwrapper::SortFilterEvaluator();
			buffer = srch2::httpwrapper::SortFilterEvaluator::deserializeForNetwork(*(info.sortEvaluator), buffer);
		}
		if(isFilterQueryEvaluatorInfoNotNull){
			info.filterQueryEvaluator = new srch2::httpwrapper::FilterQueryEvaluator(NULL);
			buffer = srch2::httpwrapper::FilterQueryEvaluator::deserializeForNetwork(*(info.filterQueryEvaluator), buffer);
		}
		if(isPhraseSearchInfoContainerNotNull){
			info.phraseSearchInfoContainer = new PhraseSearchInfoContainer();
			buffer = PhraseSearchInfoContainer::deserializeForNetwork(*(info.phraseSearchInfoContainer), buffer);
		}
		return buffer;

	}

	/*
	 * Serialization scheme :
	 * | isNULL | isNULL | isNULL | isNULL | facetInfo | sortEvaluator | filterQueryEvaluator | phraseSearchInfoContainer |
	 */
	unsigned getNumberOfBytesForSerializationForNetwork(){
		unsigned numberOfBytes = 0;
		numberOfBytes += 4 * sizeof(bool);

		if(facetInfo != NULL){
			numberOfBytes += facetInfo->getNumberOfBytesForSerializationForNetwork();
		}
		if(sortEvaluator != NULL){
			numberOfBytes += sortEvaluator->getNumberOfBytesForSerializationForNetwork();
		}
		if(filterQueryEvaluator != NULL){
			numberOfBytes += filterQueryEvaluator->getNumberOfBytesForSerializationForNetwork();
		}
		if(phraseSearchInfoContainer != NULL){
			numberOfBytes += phraseSearchInfoContainer->getNumberOfBytesForSerializationForNetwork();
		}
		return numberOfBytes;
	}

private:
	FacetQueryContainer * facetInfo;
	SortEvaluator * sortEvaluator;
	RefiningAttributeExpressionEvaluator * filterQueryEvaluator;
	PhraseSearchInfoContainer * phraseSearchInfoContainer;
};


}
}


#endif // _RESULTSPOSTPROCESSOR_H_
