/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
