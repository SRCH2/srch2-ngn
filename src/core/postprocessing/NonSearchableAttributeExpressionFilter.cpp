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
#include "instantsearch/NonSearchableAttributeExpressionFilter.h"
#include "postprocessing/NonSearchableAttributeExpressionFilterInternal.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/Score.h"
#include "instantsearch/QueryResults.h"

namespace srch2
{
namespace instantsearch
{

NonSearchableAttributeExpressionFilter::NonSearchableAttributeExpressionFilter(){
	impl = new NonSearchableAttributeExpressionFilterInternal();
}
NonSearchableAttributeExpressionFilter::~NonSearchableAttributeExpressionFilter(){

}
void NonSearchableAttributeExpressionFilter::doFilter(IndexSearcher * indexSearcher,  const Query * query,
		QueryResults * input, QueryResults * output){

	IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(indexSearcher);
	Schema * schema = indexSearcherInternal->getSchema();
	ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

	// iterating on results and checking the criteria on each of them
	for(vector<QueryResult *>::iterator resultIter = input->impl->sortedFinalResults.begin(); resultIter != input->impl->sortedFinalResults.end() ; ++resultIter){
		QueryResult * result = *resultIter;

		bool pass = impl->doesPassCriterion(schema, forwardIndex , result);

		// if the result passes the filter criteria it's copied into the output.
		if(pass){
			output->impl->sortedFinalResults.push_back(result);
		}


	}


}



}
}

