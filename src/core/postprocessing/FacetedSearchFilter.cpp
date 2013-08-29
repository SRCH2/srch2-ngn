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
    output->copyForPostProcessing(input);

    // initialize results of each attribute
    for (std::map<std::string, std::vector<Score> >::iterator facetAttributeIterator =
            impl->lowerBoundsOfIntervals.begin();
            facetAttributeIterator != impl->lowerBoundsOfIntervals.end(); ++facetAttributeIterator) {

        // inserts the same number of zero scores as the number of lowerbounds
        // in the vector (each one as the initial value of a category)
        // NOTE: if it's a Simple facet field no category will be initialized
        std::vector<std::pair<std::string, float> > zeroCounts;
        for (vector<Score>::iterator lb = facetAttributeIterator->second.begin();
                lb != facetAttributeIterator->second.end(); ++lb) {
            zeroCounts.push_back(make_pair(lb->toString(), 0));
        }
        output->impl->facetResults[facetAttributeIterator->first] = zeroCounts;
    }

    // translate list of attribute names to list of attribute IDs
    std::vector<unsigned> attributeIds;
    for (std::map<std::string, std::vector<Score> >::iterator iter =
            impl->lowerBoundsOfIntervals.begin();
            iter != impl->lowerBoundsOfIntervals.end(); ++iter) {
        attributeIds.push_back(schema->getNonSearchableAttributeId(iter->first));
    }

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
        for(std::map<std::string, std::vector<Score> >::iterator facetField =
                impl->lowerBoundsOfIntervals.begin();
                facetField != impl->lowerBoundsOfIntervals.end(); ++facetField) {
            Score & attributeValue = attributeDataValues.at(
                                   std::distance(impl->lowerBoundsOfIntervals.begin(),
                                           facetField));
            // choose the type of aggregation for this attribute
            // increments the correct facet by one
            impl->doAggregation(attributeValue,
                    facetField->second,
                    &(output->impl->facetResults[facetField->first]) ,
                    std::distance(impl->lowerBoundsOfIntervals.begin(),facetField));

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

