
#include "QueryBuilder.h"
#include "postprocessing/RangeQueryFilter.h"
#include "postprocessing/FacetedSearchFilter.h"
#include "postprocessing/NonSearchableAttributeExpressionFilter.h"
#include "postprocessing/SortFilter.h"


namespace srch2
{
namespace instantsearch
{


QueryBuilder::QueryBuilder(const Analyzer *analyzer,
		const srch2::httpwrapper::Srch2ServerConf *indexDataContainerConf,
		const Schema *schema){

	this->analyzer = analyzer;
	this->indexDataContainerConf = indexDataContainerConf;
	this->schema = schema;

}


// parses the URL to a query object
void QueryBuilder::parse(const evkeyvalq &headers,Query * query){


	// do some parsing

	query->setPostProcessingPlan(createPostProcessingPlan(headers, query));
}






// creates a post processing plan based on information from Query
// TODO : query should be removed from the inputs of this function. This function should only return plan based on header
ResultsPostProcessorPlan * QueryBuilder::createPostProcessingPlan(const evkeyvalq &headers,Query * query){

	ResultsPostProcessorPlan * result = NULL;
	if(query->getPostProcessingFilter() == RANGE_CHECK){
		result = new ResultsPostProcessorPlan();
		result->addFilterToPlan(new RangeQueryFilter());
	}else if(query->getPostProcessingFilter() == NO_FILTER){
		// TODO : must create plan based on query
		result = new ResultsPostProcessorPlan();
	}

	return result;


}



}
}
