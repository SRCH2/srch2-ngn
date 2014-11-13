#ifndef __CLUSTER_PHYSICAL_OPERATORS_H__
#define __CLUSTER_PHYSICAL_OPERATORS_H__

#include "ClusterPhysicalPlan.h"
#include "../../../notifications/Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "include/instantsearch/QueryResults.h"
#include "core/query/QueryResultsInternal.h"

namespace srch2is = srch2::instantsearch;
using namespace std;
using namespace srch2is;
namespace srch2 {
namespace httpwrapper {


struct ClusterPhysicalPlanExecutionParameter {

};

class ClusterPhysicalOperator{
public:
	ClusterPhysicalOperator(){
	}
	virtual ~ClusterPhysicalOperator(){};
	virtual bool open(ClusterPhysicalPlanExecutionParameter * params) = 0;
	virtual QueryResult* getNext(ClusterPhysicalPlanExecutionParameter * params) = 0;
	virtual bool close(ClusterPhysicalPlanExecutionParameter * params) = 0;
};


typedef std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > FacetResults;

class ClusterFacetResultsAggregator{
public:


	void addShardFacetResults(FacetResults * facetResults){
		shardFacetResultsList.push_back(facetResults);
	}

	void aggregateFacetResults(FacetResults * finalResult){
		ASSERT(finalResult != NULL);
		for(unsigned i = 0; i < shardFacetResultsList.size(); ++i){
			FacetResults * shardFacetResult = shardFacetResultsList.at(i);
	        for(FacetResults::const_iterator facetGroupItr = shardFacetResult->begin();
	                facetGroupItr != shardFacetResult->end() ; ++facetGroupItr){

	            // first check to see if this facet group exists
	            std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::iterator existingFacetGroupItr =
	                    finalResult->find(facetGroupItr->first);

	            if( existingFacetGroupItr == finalResult->end()){ // group is new
	            	(*finalResult)[facetGroupItr->first] = facetGroupItr->second;
	            }else{ // new group must be merged with the existing group
	                ASSERT(existingFacetGroupItr->second.first == facetGroupItr->second.first);
	                mergeFacetVectors(existingFacetGroupItr->second.second, facetGroupItr->second.second);
	            }
	        }
		}
	}

private:
	vector<FacetResults *> shardFacetResultsList;


	/*
	 * Merges destination with source and adds new items to source
	 */
	void mergeFacetVectors(std::vector<std::pair<std::string, float> > & source,
	        const std::vector<std::pair<std::string, float> > & destination){
	    for(std::vector<std::pair<std::string, float> >::const_iterator destinationItr = destination.begin();
	            destinationItr != destination.end(); ++destinationItr){
	        //try to find this facet in the source
	        bool found = false;
	        for(std::vector<std::pair<std::string, float> >::iterator sourceItr = source.begin();
	                sourceItr != source.end(); ++sourceItr){
	            if(destinationItr->first.compare(sourceItr->first) == 0){ // the same facet category
	                sourceItr->second += destinationItr->second;
	                found = true;
	                break;
	            }
	        }
	        if(found == false){
	            source.push_back(*destinationItr);
	        }
	    }
	}

};

}
}


#endif /* __CLUSTER_PHYSICAL_OPERATORS_H__ */
