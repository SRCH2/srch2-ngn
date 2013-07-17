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


#ifndef _CORE_POSTPROCESSING_RESULTSPOSTPROCESSOR_H_
#define _CORE_POSTPROCESSING_RESULTSPOSTPROCESSOR_H_
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




class ResultsPostProcessorFilter
{
public:
	virtual void doFilter(Schema * schema, ForwardIndex * forwardIndex, const Query * query,
			 QueryResults * input , QueryResults * output) = 0;

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
	ResultsPostProcessor( IndexSearcher *indexSearcher);
// TODO interface should changed to accept/return QueryResults and also it must accept ForwardIndex pointer to access
	void runPlan(Query * query, QueryResults * input, QueryResults * output);
	~ResultsPostProcessor();

private:

	ForwardIndex * forwardIndex ;
	Schema * schema;

	// TODO: we need some structure to store information about filter compatibility

};


}
}


#endif // _CORE_POSTPROCESSING_RESULTSPOSTPROCESSOR_H_
