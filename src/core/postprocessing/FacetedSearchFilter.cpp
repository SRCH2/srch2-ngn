
#include "instantsearch/FacetedSearchFilter.h"
#include "FacetedSearchFilterInternal.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"

namespace srch2
{
namespace instantsearch
{

FacetedSearchFilter::~FacetedSearchFilter(){

}


void FacetedSearchFilter::doFilter(IndexSearcher *indexSearcher,  const Query * query,
		QueryResults * input, QueryResults * output){

	IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(indexSearcher);
	Schema * schema = indexSearcherInternal->getSchema();
	ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();



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
				impl->facetByCountAggregation(attributeValue ,
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



}
}

