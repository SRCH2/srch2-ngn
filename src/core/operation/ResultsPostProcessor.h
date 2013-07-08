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


#ifndef __RESULTSPOSTPROCESSOR_H_OPERATION_SRC
#define __RESULTSPOSTPROCESSOR_H_OPERATION_SRC
#include <vector>
#include <map>
#include <iostream>

#include "instantsearch/Query.h"
#include "query/QueryResultsInternal.h"
#include "index/ForwardIndex.h"
#include "instantsearch/Schema.h"

namespace srch2
{
namespace instantsearch
{



class ResultsPostProcessorOperand
{
public:
	vector<QueryResult> results;

	ResultsPostProcessorOperand(){
;
	}

	ResultsPostProcessorOperand(const ResultsPostProcessorOperand & copy){
		results = copy.results;
	}
	void importResults(QueryResultsInternal * input){
//		std::cout << std::endl << "Results imported:" ;
//
//		for(vector<QueryResult>::iterator iter = input->sortedFinalResults.begin(); iter != input->sortedFinalResults.end(); ++iter){
//			std::cout << " "<< iter->externalRecordId ;
//		}
//		std::cout << std::endl;

		results.insert(results.end(), input->sortedFinalResults.begin(), input->sortedFinalResults.end());
	}
	void exportResults(QueryResultsInternal * output){ // TODO this class might need some more information so this function may need to change ....
//		std::cout << std::endl << "Results exported:" ;
//
//		for(vector<QueryResult>::iterator iter = results.begin(); iter != results.end(); ++iter){
//			std::cout << " " << iter->externalRecordId ;
//		}
//		std::cout << std::endl;
		output->sortedFinalResults.insert(output->sortedFinalResults.end(), results.begin(), results.end());
	}
};

class ResultsPostProcessorFilter
{
public:
	virtual void doFilter(Schema * schema, ForwardIndex * forwardIndex, const Query * query,
			ResultsPostProcessorOperand * input, ResultsPostProcessorOperand & output) = 0;

	virtual ~ResultsPostProcessorFilter() {};

};

class ResultsPostProcessorPlan
{
public:

	ResultsPostProcessorPlan(){
		iter = plan.end();
	}
	void addFilterToPlan(ResultsPostProcessorFilter * filter){
		plan.push_back(filter);
	}
	void clearPlan(){
		plan.clear();
	}
	void beginIteration(){
		iter = plan.begin();
	}
	ResultsPostProcessorFilter * nextFilter(){
		if(iter == plan.end()) return NULL;
		ResultsPostProcessorFilter * result = *iter;
		++iter;
		return result;
	}
	bool hasMoreFilters(){
		if(iter != plan.end()) return true;
		else{
			return false;
		}
	}
	void closeIteration(){
		iter = plan.end();
	}
private:
	vector<ResultsPostProcessorFilter *> plan;
	vector<ResultsPostProcessorFilter *>::iterator iter;
};

class ResultsPostProcessor
{
public:
	ResultsPostProcessor( Schema * schema, ForwardIndex * forwardIndex);
// TODO interface should changed to accept/return QueryResults and also it must accept ForwardIndex pointer to access
	void doProcess(const Query * query, ResultsPostProcessorOperand * input, ResultsPostProcessorOperand & output);
	~ResultsPostProcessor();

private:

	ForwardIndex * forwardIndex ;
	Schema * schema;

	// TODO: we need some structure to store information about filter compatibility

	ResultsPostProcessorPlan * createPlan(const Query * query);
	void runPlan(ResultsPostProcessorPlan * plan, const Query * query, ResultsPostProcessorOperand * input, ResultsPostProcessorOperand & output);



};


}
}


#endif // __RESULTSPOSTPROCESSOR_H_OPERATION_SRC
