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


#ifndef __CORE_POSTPROCESSING_SORTFILTER_H__
#define __CORE_POSTPROCESSING_SORTFILTER_H__

#include <vector>
#include <algorithm>
#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"

namespace srch2
{
namespace instantsearch
{

class SortEvaluator
{
public:
	virtual int compare(const std::map<std::string, TypedValue> & left , unsigned leftInternalRecordId,const std::map<std::string, TypedValue> & right, unsigned rightInternalRecordId) const = 0 ;
	virtual const std::vector<std::string> * getParticipatingAttributes() const = 0;
	virtual ~SortEvaluator(){};
	SortOrder order;
};

class SortFilter : public ResultsPostProcessorFilter
{
public:
	// TODO : we don't need query in new design
	void doFilter(IndexSearcher * indexSearcher, const Query * query,
			 QueryResults * input, QueryResults * output);
	~SortFilter();
	SortEvaluator * evaluator;
};

}
}
#endif // __CORE_POSTPROCESSING_SORTFILTER_H__

