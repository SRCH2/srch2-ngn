//$Id: FacetedSearchFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

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


#ifndef _FACETEDSEARCHFILTER_H_
#define _FACETEDSEARCHFILTER_H_


#include <vector>
#include <map>
#include <string>
#include <algorithm>


#include "ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "instantsearch/Score.h"
#include "instantsearch/IndexSearcher.h"
#include <instantsearch/Constants.h>


namespace srch2
{
namespace instantsearch
{


class FacetedSearchFilterInternal;

class FacetedSearchFilter : public ResultsPostProcessorFilter
{

public:
	// TODO : we don't need query in new design
	void doFilter(IndexSearcher *indexS48earcher, const Query * query,
			QueryResults * input, QueryResults * output);
	~FacetedSearchFilter();


	void initialize(std::vector<srch2is::FacetType> types,
			std::vector<std::string> fields,
			std::vector<std::string> rangeStarts,
			std::vector<std::string> rangeEnds,
			std::vector<std::string> rangeGaps);

private:

	FacetedSearchFilterInternal * impl;








};

}
}
#endif // _FACETEDSEARCHFILTER_H_

