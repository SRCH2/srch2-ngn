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


#ifndef _CORE_POSTPROCESSING_FACETEDSEARCHFILTER_H_
#define _CORE_POSTPROCESSING_FACETEDSEARCHFILTER_H_


#include <vector>
#include <map>
#include <string>
#include <algorithm>


#include "ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/Score.h"
#include "util/Assert.h"

namespace srch2
{
namespace instantsearch
{



class FacetedSearchFilter : public ResultsPostProcessorFilter
{

public:
	// TODO : we don't need query in new design
	void doFilter(Schema * schema, ForwardIndex * forwardIndex, const Query * query,
			QueryResults * input, QueryResults * output){

		// these two maps must be parallel
		ASSERT(lowerBoundsOfCategories.size() == facetedSearchAggregationTypes.size());

		// first copy all input results to output
		input->impl->copyForPostProcessing(output->impl);

		// initialize results of each attribute
		for(std::map<std::string , std::vector<Score> >::iterator iter = lowerBoundsOfCategories.begin();
				iter != lowerBoundsOfCategories.end(); ++iter){

			std::vector<float> zeroCounts( iter->second.size() , 0);
			switch (facetedSearchAggregationTypes[iter->first]) {
				case srch2::instantsearch::Count:
					//initialize facet vector by zero.
					output->impl->facetResults[iter->first] = zeroCounts;
					break;
				default:
					ASSERT(false);
					break;
			}
		}

		// translate list of attribute names to list of attribue IDs
		std::vector<unsigned> attributeIds;
		for(std::map<std::string , std::vector<Score> >::iterator iter = lowerBoundsOfCategories.begin();
				iter != lowerBoundsOfCategories.end(); ++iter){

			attributeIds.push_back(schema->getNonSearchableAttributeId(iter->first));
		}
		// map might keep things unsorted, or user might enter them unsorted. attributeIds must be ascending:
		std::sort(attributeIds.begin(), attributeIds.end());


		// move on the results once and do all facet calculations.
		for(std::vector<QueryResult *>::iterator resultIter = output->impl->sortedFinalResults.begin();
				resultIter != output->impl->sortedFinalResults.end(); ++resultIter) {

			QueryResult * queryResult = *resultIter;
//			std::cout << "Moving on result : " << queryResult->externalRecordId << std::endl;
			// extract all facet related nonsearchable attribute values from this record
			// by accessing the forward index only once.
			bool isValid = false;
			const ForwardList * list = forwardIndex->getForwardList(queryResult->internalRecordId , isValid);
			ASSERT(isValid);
			const VariableLengthAttributeContainer * nonSearchableAttributes = list->getNonSearchableAttributeContainer();

			// this vector is parallel to attributeIds vector
			std::vector<Score> attributeDataValues;
			nonSearchableAttributes->getBatchOfAttributes(attributeIds, schema, &attributeDataValues);

			// now iterate on attributes and incrementally update the facet results
			for(std::map<std::string , std::vector<float> >::iterator attrIter = output->impl->facetResults.begin();
					attrIter != output->impl->facetResults.end(); ++attrIter){


//				std::cout << "Attribute : " << attrIter->first ;
				unsigned attributeId = schema->getNonSearchableAttributeId(attrIter->first);
//				std::cout << " ,attributeId = " << attributeId;
				// attributeDataValues is parallel with attributeIds and not necessarily parallel with the map
				// so this attributeId must be found in one list and used in the other list.
				unsigned indexOfThisAttributeInAtributeIds = std::find(attributeIds.begin(), attributeIds.end()
						, attributeId ) - attributeIds.begin() ;
//				std::cout << " ,indexInVector = " << indexOfThisAttributeInAtributeIds ;
				Score & attributeValue = attributeDataValues.at(indexOfThisAttributeInAtributeIds);
//				std::cout << " ,value = " << attributeValue.toString() << std::endl;
				// choose the type of aggregation for this attribute
				switch (facetedSearchAggregationTypes[attrIter->first]) {
					case srch2::instantsearch::Count :
						// increments the correct facet by one
						facetByCountAggregation(attributeValue ,
								lowerBoundsOfCategories[attrIter->first] ,
								&(attrIter->second));
						break;
					default:
						break;
				}


			}
//			std::cout << "===================  Result processed. ==================" <<std::endl;

		}

	}
	~FacetedSearchFilter(){

	}

	// temporary public for test purposes
	// map of attribute name to : "vector of attribute values for categorization"
	// map<string, vector<Score>>
	std::map<std::string , std::vector<Score> > lowerBoundsOfCategories;
	// map of attribute name to : "type of aggregation for each category"
	// map<string, FacetedSearchAggregationType>
	std::map<std::string, srch2::instantsearch::FacetedSearchAggregationType> facetedSearchAggregationTypes;
private:


	// this function compares the attribute data value with the lower bounds and
	// increments the correct category size by one.
	// NOTE: lowerBounds list must be sorted.
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



};

}
}
#endif // _CORE_POSTPROCESSING_FACETEDSEARCHFILTER_H_

