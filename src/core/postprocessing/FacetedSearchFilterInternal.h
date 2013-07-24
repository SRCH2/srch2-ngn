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

namespace srch2
{
namespace instantsearch
{

class FacetedSearchFilterInternal

{

public:

	void facetByCountAggregation(const Score & attributeValue ,
			const std::vector<Score> & lowerBounds ,
			std::vector<float>  * counts ){

		unsigned i = 0;
		for(std::vector<Score>::const_iterator lowerBound = lowerBounds.begin() ; lowerBound!=lowerBounds.end() ; ++lowerBound){
			bool fallsInThisCategory = false;
			// It's assumed that lowerBound list is sorted so the
			// first category also accepts records things which are less than it.
			if(i == 0) {
				fallsInThisCategory = true;
			}else{ // in normal case if the value is greater than lowerBound it's potentially in this category.
				if(*lowerBound <= attributeValue){
					fallsInThisCategory = true;
				}
			}
			if(fallsInThisCategory){
				// if this is not the last category, the value should also be less than the next lowerBound
				if(i != lowerBounds.size()-1){
					if(*(lowerBound + 1) <= attributeValue){
						fallsInThisCategory = false;
					}
				}
			}
			if(fallsInThisCategory){
				counts->at(i) = counts->at(i) + 1;
				return;
			}
			//
			++i;
		}
		// normally we should never reach to this point because each record should fall in at least one of the categories.
		ASSERT(false);
	}

	//
	std::vector<FacetType> types;
	std::vector<std::string> fields;
	std::vector<std::string> rangeStarts;
	std::vector<std::string> rangeEnds;
	std::vector<std::string> rangeGaps;

};

}
}

#endif // _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_
