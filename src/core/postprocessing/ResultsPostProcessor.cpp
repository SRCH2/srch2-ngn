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

#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
#include <instantsearch/ResultsPostProcessor.h>

using namespace std;


namespace srch2
{
namespace instantsearch
{


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
	impl->iter = impl->plan.begin();
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



//ResultsPostProcessor::ResultsPostProcessor(IndexSearcher *indexSearcher){
//
//

//}
//
//ResultsPostProcessor::~ResultsPostProcessor(){
//	// de-allocate plans
//}
//
//
//void ResultsPostProcessor::runPlan(Query * query, QueryResults * input, QueryResults *  output){

//}




}
}
