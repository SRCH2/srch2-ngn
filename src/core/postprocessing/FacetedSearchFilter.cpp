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

#include "instantsearch/FacetedSearchFilter.h"
#include "FacetedSearchFilterInternal.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/QueryEvaluatorInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "util/Assert.h"
#include "util/DateAndTimeHandler.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

FacetedSearchFilter::FacetedSearchFilter() {
    impl = new FacetedSearchFilterInternal();
}

FacetedSearchFilter::~FacetedSearchFilter() {
    delete impl;
}

void FacetedSearchFilter::doFilter(QueryEvaluator *queryEvaluator,
        const Query * query, QueryResults * input, QueryResults * output) {
	this->impl->doFilter(queryEvaluator , query , input , output);

}

void FacetedSearchFilter::initialize(std::vector<FacetType> & facetTypes,
        std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
        std::vector<std::string> & rangeEnds,
        std::vector<std::string> & rangeGaps, std::vector<int> & numberOfGroupsToReturn) {
	this->impl->initialize(facetTypes , fields , rangeStarts , rangeEnds, rangeGaps , numberOfGroupsToReturn);
}

}
}

