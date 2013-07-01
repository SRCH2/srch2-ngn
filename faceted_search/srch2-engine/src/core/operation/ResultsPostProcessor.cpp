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


#include "ResultsPostProcessor.h"
#include "RangeQueryFilter.h"

namespace srch2
{
namespace instantsearch
{

ResultsPostProcessor::ResultsPostProcessor( Schema * schema, ForwardIndex * forwardIndex){

	this->forwardIndex = forwardIndex;
	this->schema = schema;
}

ResultsPostProcessor::~ResultsPostProcessor(){
	// de-allocate plans
}
void ResultsPostProcessor::doProcess(const Query * query,  ResultsPostProcessorOperand * input, ResultsPostProcessorOperand & output){
	// choose a plan based on the information in query
	ResultsPostProcessorPlan * plan = createPlan(query);
	// run the plan on results
	runPlan(plan, query, input, output);

	// delete the plan
	delete plan;
}

// based on the information in Query object, this function creates the plan (which is a list of filters
// to be applied on result list one by one)
ResultsPostProcessorPlan * ResultsPostProcessor::createPlan(const Query * query){
// create a plan based on query


	ResultsPostProcessorPlan * result = NULL;
	if(query->getPostProcessingFilter() == RANGE_CHECK){ // TODO : we must create enums
		result = new ResultsPostProcessorPlan();
		result->addFilterToPlan(new RangeQueryFilter());
	}else if(query->getPostProcessingFilter() == NO_FILTER){
		// TODO : must create plan based on query
		result = new ResultsPostProcessorPlan();
	}

	return result;

}
void ResultsPostProcessor::runPlan(ResultsPostProcessorPlan * plan,  const Query * query, ResultsPostProcessorOperand * input, ResultsPostProcessorOperand & output){
// run a plan by iterating on filters and running TODO

	// short circuit in case the plan doesn't have any filters in it.
	output = *input;


	// iterating on filters and applying them on list of results
	ResultsPostProcessorOperand * inputOperand = input;
	plan->beginIteration();
	while(plan->hasMoreFilters()){
		ResultsPostProcessorFilter * filter = plan->nextFilter();

		ResultsPostProcessorOperand outputOperand;
		filter->doFilter(this->schema, this->forwardIndex, query, inputOperand, outputOperand);

		if(!plan->hasMoreFilters()){
			// copy new results to the output
			output = outputOperand;
		}else{
			// pass the input to the next filter
			inputOperand = &outputOperand;
		}
	}
	plan->closeIteration();
}


}
}
