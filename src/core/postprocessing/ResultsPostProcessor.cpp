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

#include "ResultsPostProcessorInternal.h"
#include <instantsearch/QueryEvaluator.h>
#include "operation/QueryEvaluatorInternal.h"
#include <instantsearch/ResultsPostProcessor.h>
#include <sstream>

using namespace std;

namespace srch2
{
namespace instantsearch
{

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
	roleId = "";
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

void ResultsPostProcessingInfo::setRoleId(string & roleId){
	this->roleId = roleId;
}

string* ResultsPostProcessingInfo::getRoleId(){
	return &(this->roleId);
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
	ss << roleId.c_str() << endl;
	return ss.str();
}

}
}
