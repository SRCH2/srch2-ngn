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


#ifndef __CORE_POSTPROCESSING_NONSEARCHABLEATTRIBUTEFILTERINTERNAL_H__
#define __CORE_POSTPROCESSING_NONSEARCHABLEATTRIBUTEFILTERINTERNAL_H__

#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "query/QueryResultsInternal.h"
#include "instantsearch/NonSearchableAttributeExpressionFilter.h"
namespace srch2
{
namespace instantsearch
{


// Example:
// fq=price:[* TO 100] AND model:JEEP AND CMPLX$price - discount < 100$
// This class checks to see if a result passes these criteria or not ...
class NonSearchableAttributeExpressionFilterInternal
{

public:
	NonSearchableAttributeExpressionFilterInternal(NonSearchableAttributeExpressionFilter * filter);
	// evaluates expression object coming from query using result data to see
	// if it passes the query criterion.
	bool doPass(Schema * schema, ForwardIndex * forwardIndex , const QueryResult * result);

private:
	NonSearchableAttributeExpressionFilter * filter ;
};

}
}
#endif // __CORE_POSTPROCESSING_NONSEARCHABLEATTRIBUTEFILTERINTERNAL_H__
