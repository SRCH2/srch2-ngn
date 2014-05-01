//$Id: ResultsPostProcessor.cpp 3456 2013-06-26 02:11:13Z Jamshid $

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

#include "ResultsPostProcessorInternal.h"
#include <instantsearch/QueryEvaluator.h>
#include "operation/QueryEvaluatorInternal.h"
#include <instantsearch/ResultsPostProcessor.h>
#include <postprocessing/SortFilterEvaluator.h>
#include <postprocessing/FilterQueryEvaluator.h>
#include <sstream>

using namespace std;

namespace srch2
{
namespace instantsearch
{


string FacetQueryContainer::toString(){
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
void * FacetQueryContainer::serializeForNetwork(void * buffer){
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
void * FacetQueryContainer::deserializeForNetwork(FacetQueryContainer & info, void * buffer){
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
unsigned FacetQueryContainer::getNumberOfBytesForSerializationForNetwork(){
	unsigned numberOfBytes = 0;
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(types);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(numberOfTopGroupsToReturn);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(fields);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeStarts);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeEnds);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(rangeGaps);
	return numberOfBytes;
}

ResultsPostProcessorPlan::ResultsPostProcessorPlan(){
	impl = new ResultsPostProcessorPlanInternal();
}

ResultsPostProcessorPlan::~ResultsPostProcessorPlan(){
	delete impl;
}

void ResultsPostProcessorPlan::addFilterToPlan(ResultsPostProcessorFilter * filter){
	impl->filterVector.push_back(filter);
}

void ResultsPostProcessorPlan::clearPlan(){
	impl->filterVector.clear();
}

void ResultsPostProcessorPlan::beginIteration(){
	impl->filterIterator = impl->filterVector.begin();
}

ResultsPostProcessorFilter * ResultsPostProcessorPlan::nextFilter(){
	if(impl->filterIterator == impl->filterVector.end()){
	    return NULL;
	}
	ResultsPostProcessorFilter * resultsPostProcessingFilter = *impl->filterIterator;
	++(impl->filterIterator);
	return resultsPostProcessingFilter;
}

bool ResultsPostProcessorPlan::hasMoreFilters() const{
	if(impl->filterIterator != impl->filterVector.end()){
	    return true;
	}else{
		return false;
	}
}

void ResultsPostProcessorPlan::closeIteration(){
	impl->filterIterator = impl->filterVector.end();
}

ResultsPostProcessingInfo::ResultsPostProcessingInfo(){
	facetInfo = NULL;
	sortEvaluator = NULL;
	filterQueryEvaluator = NULL;
	phraseSearchInfoContainer = NULL;
}
ResultsPostProcessingInfo::~ResultsPostProcessingInfo(){
	if(facetInfo != NULL){
		delete facetInfo;
	}
	if(sortEvaluator != NULL){
		delete sortEvaluator;
	}
	if(filterQueryEvaluator != NULL){
		delete filterQueryEvaluator;
	}
	if(phraseSearchInfoContainer != NULL){
		delete phraseSearchInfoContainer;
	}
}

FacetQueryContainer * ResultsPostProcessingInfo::getfacetInfo(){
	return facetInfo;
}
void ResultsPostProcessingInfo::setFacetInfo(FacetQueryContainer * facetInfo){
	this->facetInfo = facetInfo;
}
SortEvaluator * ResultsPostProcessingInfo::getSortEvaluator(){
	return this->sortEvaluator;
}
void ResultsPostProcessingInfo::setSortEvaluator(SortEvaluator * evaluator){
	this->sortEvaluator = evaluator;
}

void ResultsPostProcessingInfo::setFilterQueryEvaluator(RefiningAttributeExpressionEvaluator * filterQuery){
	this->filterQueryEvaluator = filterQuery;
}
RefiningAttributeExpressionEvaluator * ResultsPostProcessingInfo::getFilterQueryEvaluator(){
	return this->filterQueryEvaluator;
}

void ResultsPostProcessingInfo::setPhraseSearchInfoContainer(PhraseSearchInfoContainer * phraseSearchInfoContainer){
	this->phraseSearchInfoContainer = phraseSearchInfoContainer;
}
PhraseSearchInfoContainer * ResultsPostProcessingInfo::getPhraseSearchInfoContainer(){
	return this->phraseSearchInfoContainer;
}

string ResultsPostProcessingInfo::toString(){
	stringstream ss;
	if(facetInfo != NULL){
		ss << facetInfo->toString().c_str();
	}
	if(sortEvaluator != NULL){
		ss << sortEvaluator->toString().c_str();
	}
	if(filterQueryEvaluator != NULL){
		ss << filterQueryEvaluator->toString().c_str();
	}
	if(phraseSearchInfoContainer != NULL){
		ss << phraseSearchInfoContainer->toString().c_str();
	}
	return ss.str();
}

void * ResultsPostProcessingInfo::serializeForNetwork(void * buffer){
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
void * ResultsPostProcessingInfo::deserializeForNetwork(ResultsPostProcessingInfo & info, void * buffer){

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
unsigned ResultsPostProcessingInfo::getNumberOfBytesForSerializationForNetwork(){
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

string PhraseInfo::toString(){
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
void * PhraseInfo::serializeForNetwork(void * buffer) const {
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
void * PhraseInfo::deserializeForNetwork(void * buffer) {
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
unsigned PhraseInfo::getNumberOfBytesForSerializationForNetwork() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(attributeBitMap);
	numberOfBytes += sizeof(proximitySlop);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(keywordIds);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(phraseKeywordPositionIndex);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(phraseKeyWords);
	return numberOfBytes;
}

void PhraseSearchInfoContainer::addPhrase(const vector<string>& phraseKeywords,
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

string PhraseSearchInfoContainer::toString(){
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
void * PhraseSearchInfoContainer::serializeForNetwork(void * buffer) const {
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
void * PhraseSearchInfoContainer::deserializeForNetwork(PhraseSearchInfoContainer & container, void * buffer) {
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
unsigned PhraseSearchInfoContainer::getNumberOfBytesForSerializationForNetwork() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(unsigned posInfoIndex = 0; posInfoIndex < phraseInfoVector.size(); ++posInfoIndex){
		numberOfBytes += phraseInfoVector.at(posInfoIndex).getNumberOfBytesForSerializationForNetwork();
	}
	return numberOfBytes;
}

}
}
