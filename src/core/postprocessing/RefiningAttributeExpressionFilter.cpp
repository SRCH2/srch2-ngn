//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

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


#include <vector>


#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/RefiningAttributeExpressionFilter.h"
#include "postprocessing/RefiningAttributeExpressionFilterInternal.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/QueryEvaluatorInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"
#include "instantsearch/QueryResults.h"

namespace srch2
{
namespace instantsearch
{

RefiningAttributeExpressionFilter::RefiningAttributeExpressionFilter(){
	impl = new RefiningAttributeExpressionFilterInternal(this);
}

RefiningAttributeExpressionFilter::~RefiningAttributeExpressionFilter(){
	delete evaluator; // this object is allocated in plan Generator
	delete impl;
}

void RefiningAttributeExpressionFilter::doFilter(QueryEvaluator *queryEvaluator,  const Query * query,
		QueryResults * input, QueryResults * output){

	QueryEvaluatorInternal * queryEvaluatorInternal = queryEvaluator->impl;
	Schema * schema = queryEvaluatorInternal->getSchema();
	ForwardIndex * forwardIndex = queryEvaluatorInternal->getForwardIndex();

	// iterating on results and checking the criteria on each of them
	for(vector<QueryResult *>::iterator resultIter = input->impl->sortedFinalResults.begin(); resultIter != input->impl->sortedFinalResults.end() ; ++resultIter){
		QueryResult * result = *resultIter;
		// if the result passes the filter criteria, it's copied into the output.
		if(impl->doPass(schema, forwardIndex , result)){
			output->impl->sortedFinalResults.push_back(result);
		}
	}
}

}
}

