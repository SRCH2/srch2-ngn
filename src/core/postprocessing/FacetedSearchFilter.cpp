#include "instantsearch/FacetedSearchFilter.h"
#include "FacetedSearchFilterInternal.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "util/Assert.h"

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
    input->copyForPostProcessing(output);

    // initialize results of each attribute
    for (std::map<std::string, std::vector<Score> >::iterator iter =
            impl->lowerBoundsOfCategories.begin();
            iter != impl->lowerBoundsOfCategories.end(); ++iter) {

        // inserts the same number of zero scores as the number of lowerbounds
        // in the vector (each one as the initial value of a category)
        // NOTE: if it's a Simple facet field no category will be initialized
        std::vector<std::pair<std::string, float> > zeroCounts;
        for (vector<Score>::iterator lb = iter->second.begin();
                lb != iter->second.end(); ++lb) {
            zeroCounts.push_back(make_pair(lb->toString(), 0));
        }
        output->impl->facetResults[iter->first] = zeroCounts;
    }

    // translate list of attribute names to list of attribue IDs
    std::vector<unsigned> attributeIds;
    for (std::map<std::string, std::vector<Score> >::iterator iter =
            impl->lowerBoundsOfCategories.begin();
            iter != impl->lowerBoundsOfCategories.end(); ++iter) {

        attributeIds.push_back(
                schema->getNonSearchableAttributeId(iter->first));
    }
    // map might keep things unsorted, or user might enter them unsorted. attributeIds must be ascending:
    std::sort(attributeIds.begin(), attributeIds.end());

    // move on the results once and do all facet calculations.
    for (std::vector<QueryResult *>::iterator resultIter =
            output->impl->sortedFinalResults.begin();
            resultIter != output->impl->sortedFinalResults.end();
            ++resultIter) {

        QueryResult * queryResult = *resultIter;
        //			std::cout << "Moving on result : " << queryResult->externalRecordId << std::endl;
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
        nonSearchableAttributes->getBatchOfAttributes(attributeIds, schema,
                &attributeDataValues);

        // now iterate on attributes and incrementally update the facet results
        for (std::map<std::string, std::vector<pair<string, float> > >::iterator attrIter =
                output->impl->facetResults.begin();
                attrIter != output->impl->facetResults.end(); ++attrIter) {

            //				std::cout << "Attribute : " << attrIter->first ;
            unsigned attributeId = schema->getNonSearchableAttributeId(
                    attrIter->first);
            //				std::cout << " ,attributeId = " << attributeId;
            // attributeDataValues is parallel with attributeIds and not necessarily parallel with the map
            // so this attributeId must be found in one list and used in the other list.
            unsigned indexOfThisAttributeInAtributeIds = std::find(
                    attributeIds.begin(), attributeIds.end(), attributeId)
                    - attributeIds.begin();
            //				std::cout << " ,indexInVector = " << indexOfThisAttributeInAtributeIds ;
            Score & attributeValue = attributeDataValues.at(
                    indexOfThisAttributeInAtributeIds);
            //				std::cout << " ,value = " << attributeValue.toString() << std::endl;
            // choose the type of aggregation for this attribute
            // increments the correct facet by one
            impl->facetByCountAggregation(attributeValue,
                    impl->lowerBoundsOfCategories[attrIter->first],
                    &(attrIter->second));

        }

    }

}

void FacetedSearchFilter::initialize(std::vector<FacetType> types,
        std::vector<std::string> fields, std::vector<std::string> rangeStarts,
        std::vector<std::string> rangeEnds,
        std::vector<std::string> rangeGaps) {

    this->impl->fields = fields;
    this->impl->types = types;
    this->impl->rangeStarts = rangeStarts;
    this->impl->rangeEnds = rangeEnds;
    this->impl->rangeGaps = rangeGaps;

}

}
}

