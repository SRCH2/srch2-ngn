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


#include "instantsearch/ResultsPostProcessor.h"
#include "ResultsPostProcessorInternal.h"
#include "RangeQueryFilter.h"

#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
namespace srch2
{
namespace instantsearch
{


//ResultsPostProcessor::ResultsPostProcessor(IndexSearcher *indexSearcher){
//
//
//	IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal * >(indexSearcher);
//	this->impl->forwardIndex = indexSearcherInternal->getForwardIndex();
//	this->impl->schema = indexSearcherInternal->getSchema();
//}
//
//ResultsPostProcessor::~ResultsPostProcessor(){
//	// de-allocate plans
//}
//
//
//void ResultsPostProcessor::runPlan(Query * query, QueryResults * input, QueryResults *  output){
//// run a plan by iterating on filters and running TODO
//
//	// short circuit in case the plan doesn't have any filters in it.
//	ResultsPostProcessorPlan * plan = query->getPostProcessingPlan();
//
//
//
//
//	// if no plan is set in Query, there is no post processing, just mirror the results
//	if(plan == NULL){
//		input->impl->copyForPostProcessing(output->impl);
//		return;
//	}
//
//	// iterating on filters and applying them on list of results
//	QueryResults * inputOperand = input;
//	plan->beginIteration();
//	while(plan->hasMoreFilters()){
//		ResultsPostProcessorFilter * filter = plan->nextFilter();
//
//		QueryResults outputOperand(input->impl->resultsFactory,input->impl->indexSearcherInternal,input->impl->query);
//		filter->doFilter(this->impl->schema, this->impl->forwardIndex, query, inputOperand, &outputOperand);
//
//		if(!plan->hasMoreFilters()){ // TODO : after adding pointer logic reimplement this logic
//			// copy new results to the output
//			outputOperand.impl->copyForPostProcessing(output->impl);
//		}else{
//			// pass the input to the next filter
//			inputOperand = &outputOperand;
//		}
//	}
//	plan->closeIteration();
//}


ResultsPostProcessorPlan::ResultsPostProcessorPlan(){
	impl = new ResultsPostProcessorPlanInternal();
	impl->iter = impl->plan.end();
}
ResultsPostProcessorPlan::~ResultsPostProcessorPlan(){
	delete impl;
}

void ResultsPostProcessorPlan::addFilterToPlan(ResultsPostProcessorFilter * filter){
	impl->plan.push_back(filter);
}
void ResultsPostProcessorPlan::clearPlan(){
	impl->plan.clear();
}
void ResultsPostProcessorPlan::beginIteration(){
	impl->iter = plan.begin();
}
ResultsPostProcessorFilter * ResultsPostProcessorPlan::nextFilter(){
	if(impl->iter == impl->plan.end()) return NULL;
	ResultsPostProcessorFilter * result = *impl->iter;
	++(impl->iter);
	return result;
}
bool ResultsPostProcessorPlan::hasMoreFilters(){
	if(impl->iter != impl->plan.end()) return true;
	else{
		return false;
	}
}
void ResultsPostProcessorPlan::closeIteration(){
	impl->iter = impl->plan.end();
}

}
}
