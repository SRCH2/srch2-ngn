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

#ifndef _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_
#define _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_

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

    void doAggregation(const Score & attributeValue,
            const std::vector<Score> & lowerBounds,
            std::vector<pair<string, float> > * counts) {

        if (lowerBounds.empty()) { // Categorical facet
            // move on computed facet results to see if this value is seen before (increment) or is new (add and initialize)
            // TODO : optimization on find and access and increment ...
            for (std::vector<pair<string, float> >::iterator p =
                    counts->begin(); p != counts->end(); ++p) {
                if (p->first.compare(attributeValue.toString()) == 0) {
                    p->second = p->second + 1; // this value is seen before
                    return;
                }
            }
            counts->push_back(make_pair(attributeValue.toString(), 1)); // add a new value
            return;
        }
        // Range facet
        unsigned lowerBoundIndex = 0;
        for (std::vector<Score>::const_iterator lowerBoundIterator =
                lowerBounds.begin(); lowerBoundIterator != lowerBounds.end();
                ++lowerBoundIterator) {
            bool fallsInThisCategory = false;
            // It's assumed that lowerBound list is sorted so the
            // first category also accepts records things which are less than it.
            // in normal case if the value is greater than lowerBound it's potentially in this category.
            // left side of the intervals are closed and right side is open
            if (*lowerBoundIterator <= attributeValue) {
                fallsInThisCategory = true;
            }
            if (fallsInThisCategory) {
                // if this is not the last category, the value should also be less than the next lowerBound
                if (lowerBoundIndex != lowerBounds.size() - 1) {
                    if (*(lowerBoundIterator + 1) <= attributeValue) {
                        fallsInThisCategory = false;
                    }
                }
            }
            if (fallsInThisCategory) {
                counts->at(lowerBoundIndex).second =
                        counts->at(lowerBoundIndex).second + 1;
                return;
            }
            //
            ++lowerBoundIndex;
        }
        // normally we should never reach to this point because each record should fall in at least one of the categories.
        ASSERT(false);

    }

    // this function prepares the "lowerbound" structure from the parallel string vectors
    void prepareFacetInputs(IndexSearcher *indexSearcher) {

        IndexSearcherInternal * indexSearcherInternal =
                dynamic_cast<IndexSearcherInternal *>(indexSearcher);
        Schema * schema = indexSearcherInternal->getSchema();
        ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

        std::vector<Score> rangeStartScores;
        std::vector<Score> rangeEndScores;
        std::vector<Score> rangeGapScores;
        // 1. parse the values into Score.
        unsigned fieldIndex = 0;
        for (std::vector<std::string>::iterator field = this->fields.begin();
                field != this->fields.end(); ++field) {
            if (this->facetTypes.at(fieldIndex) == FacetTypeCategorical) { // Simple facet
                Score start; // just insert placeholders
                rangeStartScores.push_back(start);
                rangeEndScores.push_back(start);
                rangeGapScores.push_back(start);
            } else { // Range facet
                FilterType attributeType = schema->getTypeOfNonSearchableAttribute(
                        schema->getNonSearchableAttributeId(*field));
                Score start;
                // std::distance(this->fields.begin() , field)
                start.setScore(attributeType, this->rangeStarts.at(fieldIndex));
                rangeStartScores.push_back(start);

                Score end;
                end.setScore(attributeType, this->rangeEnds.at(fieldIndex));
                rangeEndScores.push_back(end);

                Score gap;
                gap.setScore(attributeType, this->rangeGaps.at(fieldIndex));
                rangeGapScores.push_back(gap);
            }

            //
            fieldIndex++;
        }

        // 2. create the lowebound vector for each attribute

        unsigned facetTypeIndex = 0;
        for (std::vector<FacetType>::iterator facetTypeIterator = facetTypes.begin();
                facetTypeIterator != facetTypes.end(); ++facetTypeIterator) {
            std::vector<Score> lowerBounds;

            switch (*facetTypeIterator) {
            case FacetTypeCategorical: // lower bounds vector is empty, because lower bounds are not determined before results
                break;
            case FacetTypeRange:
                Score & start = rangeStartScores.at(facetTypeIndex);
                Score & end = rangeEndScores.at(facetTypeIndex);
                Score & gap = rangeGapScores.at(facetTypeIndex);
                ASSERT(start.getType() != srch2::instantsearch::ATTRIBUTE_TYPE_TEXT);

                Score lowerBoundToAdd = start;

                lowerBounds.push_back(lowerBoundToAdd.minimumValue()); // to collect data smaller than start
                while (lowerBoundToAdd < end) {
                    lowerBounds.push_back(lowerBoundToAdd); // data of normal categories
                    lowerBoundToAdd = lowerBoundToAdd + gap;
                }
                lowerBounds.push_back(lowerBoundToAdd); // to collect data greater than end
                break;
            }
            lowerBoundsOfIntervals[fields.at(facetTypeIndex)] = lowerBounds;

            //
            facetTypeIndex++;
        }

    }

    // TODO : change fields from string to unsigned (attribute IDs)
    std::vector<FacetType> facetTypes;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;

    // TODO : in the future we must suppose different range sizes ....
    std::map<std::string, std::vector<Score> > lowerBoundsOfIntervals;

};

}
}

#endif // _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_
