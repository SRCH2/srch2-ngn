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
#include "operation/IndexSearcherInternal.h"
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

void FacetedSearchFilter::doFilter(IndexSearcher *indexSearcher,
        const Query * query, QueryResults * input, QueryResults * output) {

    IndexSearcherInternal * indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(indexSearcher);
    Schema * schema = indexSearcherInternal->getSchema();
    ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

    // first prepare internal structures based on the input
    this->impl->prepareFacetInputs(indexSearcher);

    // also copy all input results to output to save previous filter works
    output->copyForPostProcessing(input);

    // initialize results of each attribute
    // temporary container for categorical
    std::map<string , std::map<string , float > > categoricalCounts;
//    for (std::map<std::string, std::vector<Score> >::iterator facetAttributeIterator =
//            impl->lowerBoundsOfIntervals.begin();
//            facetAttributeIterator != impl->lowerBoundsOfIntervals.end(); ++facetAttributeIterator) {
    for(std::vector<std::string>::iterator facetField = impl->fields.begin();
            facetField != impl->fields.end() ; ++facetField){
        // inserts the same number of zero scores as the number of lowerbounds
        // in the vector (each one as the initial value of a category)
        // NOTE: if it's a Simple facet field categoricalCounts will be initialized
        if(impl->facetTypes.at(std::distance(impl->fields.begin() , facetField)) == srch2is::FacetTypeCategorical){ // Categorical
            std::map<string , float > counts;
            categoricalCounts[*facetField] = counts;
        }else{ // range
            std::vector<std::pair<std::string, float> > zeroCounts;
            // this line inserts the entry into the map
            output->impl->facetResults[*facetField] = zeroCounts;
            for (int i=0; i<  impl->lowerBoundsOfIntervals[*facetField].size() ; i++) {
                // pushing back zeros directly to the map entry
            	string categoryName = "";
            	categoryName += DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(impl->lowerBoundsOfIntervals[*facetField].at(i).toString());
                output->impl->facetResults[*facetField].push_back(make_pair(categoryName, 0));
            }
        }
    }

    // translate list of attribute names to list of attribute IDs
    std::vector<unsigned> attributeIds;
    for(std::vector<std::string>::iterator facetField = impl->fields.begin();
            facetField != impl->fields.end() ; ++facetField){
        attributeIds.push_back(schema->getNonSearchableAttributeId(*facetField));
    }

    // move on the results once and do all facet calculations.
    for (std::vector<QueryResult *>::iterator resultIter =
            output->impl->sortedFinalResults.begin();
            resultIter != output->impl->sortedFinalResults.end();
            ++resultIter) {
        QueryResult * queryResult = *resultIter;
        // extract all facet related nonsearchable attribute values from this record
        // by accessing the forward index only once.
        bool isValid = false;
        const ForwardList * list = forwardIndex->getForwardList(
                queryResult->internalRecordId, isValid);
        ASSERT(isValid);
        const VariableLengthAttributeContainer * nonSearchableAttributes =
                list->getNonSearchableAttributeContainer();
        // this vector is parallel to attributeIds vector
        std::vector<Score> attributeDataValues;
        nonSearchableAttributes->getBatchOfAttributes(attributeIds, schema, &attributeDataValues);

        // now iterate on attributes and incrementally update the facet results
        for(std::vector<std::string>::iterator facetField = impl->fields.begin();
                facetField != impl->fields.end() ; ++facetField){
            Score & attributeValue = attributeDataValues.at(
                                   std::distance(impl->fields.begin() , facetField));
            // choose the type of aggregation for this attribute
            // increments the correct facet by one
            if(impl->facetTypes.at(std::distance(impl->fields.begin() , facetField)) == srch2is::FacetTypeCategorical){
                // move on computed facet results to see if this value is seen before (increment) or is new (add and initialize)
                impl->doAggregationCategorical(attributeValue, &(categoricalCounts[*facetField]));
            }else{ // range
                impl->doAggregationRange(attributeValue ,
                        impl->lowerBoundsOfIntervals[*facetField] ,
                        &(output->impl->facetResults[*facetField]) ,
                        impl->rangeStartScores.at(std::distance(impl->fields.begin() , facetField)),
                        impl->rangeEndScores.at(std::distance(impl->fields.begin() , facetField)),
                        impl->rangeGapScores.at(std::distance(impl->fields.begin() , facetField)));
            }
        }
    }

    // now copy all categorical results to the query results container (next to range facet info)
    for(std::map<string , std::map<string , float > >::iterator category = categoricalCounts.begin();
            category != categoricalCounts.end() ; ++category){
        std::vector<std::pair<std::string, float> > categoryVector;
        // this statement is to insert this entry to the map
        output->impl->facetResults[category->first] = categoryVector;
        std::vector<std::pair<std::string, float> > & resultsContainer = output->impl->facetResults[category->first];
        for(std::map<string , float >::iterator subCategory = category->second.begin() ;
                subCategory != category->second.end() ; ++subCategory ){
            resultsContainer.push_back(std::make_pair(subCategory->first , subCategory->second));
        }
    }
}

void FacetedSearchFilter::initialize(std::vector<FacetType> & facetTypes,
        std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
        std::vector<std::string> & rangeEnds,
        std::vector<std::string> & rangeGaps) {
    this->impl->fields = fields;
    this->impl->facetTypes = facetTypes;
    this->impl->rangeStarts = rangeStarts;
    this->impl->rangeEnds = rangeEnds;
    this->impl->rangeGaps = rangeGaps;
}

}
}

