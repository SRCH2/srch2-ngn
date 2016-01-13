
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
