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

#ifndef __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__
#define __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "instantsearch/Score.h"
#include "operation/IndexSearcherInternal.h"

namespace srch2 {
namespace instantsearch {

class FacetedSearchFilterInternal
{

public:
    FacetedSearchFilterInternal(){
        isPrepared = false;
    }

    void doAggregationRange(const Score & attributeValue,
            const std::vector<Score> & lowerBounds,
            std::vector<pair<string, float> > * counts , Score & start, Score & end, Score & gap);

    void doAggregationCategorical(const Score & attributeValue,
            std::map<string , float > * count);

    // this function prepares the "lowerbound" structure from the parallel string vectors
    void prepareFacetInputs(IndexSearcher *indexSearcher);

    // TODO : change fields from string to unsigned (attribute IDs)
    std::vector<FacetType> facetTypes;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
    // these members are filled in prepareFacetInputs
    std::vector<Score> rangeStartScores;
    std::vector<Score> rangeEndScores;
    std::vector<Score> rangeGapScores;
    // TODO : in the future we must suppose different range sizes ....
    std::map<std::string, std::vector<Score> > lowerBoundsOfIntervals;
    bool isPrepared;
};

}
}

#endif // __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__
