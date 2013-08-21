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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */


#ifndef _NONSEARCHABLEATTRIBTEEXPRESSIONFILTER_H_
#define _NONSEARCHABLEATTRIBTEEXPRESSIONFILTER_H_


#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/IndexSearcher.h"
#include "instantsearch/Score.h"

namespace srch2
{
namespace instantsearch
{

class NonSearchableAttributeExpressionFilterInternal;


class NonSearchableAttributeExpressionEvaluator
{
public:
	virtual bool evaluate(std::map<std::string, Score> & nonSearchableAttributeValues) = 0 ;
	virtual ~NonSearchableAttributeExpressionEvaluator(){};
};


class NonSearchableAttributeExpressionFilter : public ResultsPostProcessorFilter
{

public:
	NonSearchableAttributeExpressionFilter();
	void doFilter(IndexSearcher * indexSearcher,   const Query * query,
			QueryResults * input, QueryResults * output);
	~NonSearchableAttributeExpressionFilter();

	// this object is allocated and de-allocated ourside this class.
	NonSearchableAttributeExpressionEvaluator * evaluator;

private:

	NonSearchableAttributeExpressionFilterInternal * impl;


};

}
}
#endif // _NONSEARCHABLEATTRIBTEEXPRESSIONFILTER_H_

