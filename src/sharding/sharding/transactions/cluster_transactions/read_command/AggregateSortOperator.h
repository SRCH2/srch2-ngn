#ifndef __AGGREGATE_SORT_OPERATOR_H__
#define __AGGREGATE_SORT_OPERATOR_H__

#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "ClusterPhysicalOperators.h"
#include "NetworkOpertator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

class ClusterSortOperator : public ClusterPhysicalOperator{
public:

	struct ClusterSortOperatorItemCmp {

			SortEvaluator* sortEvaluator;

			ClusterSortOperatorItemCmp() {};

	        bool operator() (const QueryResult *lhs, const QueryResult *rhs) const {
	        	if(this->sortEvaluator == NULL){
	        		if(lhs->isExactResult() != rhs->isExactResult()){
	        			return lhs->isExactResult();
	        		}
	        		return lhs->_score > rhs->_score;
	        	}else{
	        		return this->sortEvaluator->compare(lhs->valuesOfParticipatingRefiningAttributes,
	        				lhs->internalRecordId,
	        				rhs->valuesOfParticipatingRefiningAttributes,
	        				rhs->internalRecordId) > 0;
	        	}
	        }
	    };


	~ClusterSortOperator();
	void setSortEvaluator(SortEvaluator* sortEvaluator){
		this->sortEvaluator = sortEvaluator;
	}
	bool open(ClusterPhysicalPlanExecutionParameter * params);
	QueryResult* getNext(ClusterPhysicalPlanExecutionParameter * params);
	bool close(ClusterPhysicalPlanExecutionParameter * params);

	bool addChild(NetworkOperator* networkOperator);

	ClusterSortOperator(){
		this->sortEvaluator = NULL;
	}
private:
	vector<NetworkOperator*> children;
	vector<QueryResult*> sortedItems;
	vector<QueryResult*>::iterator sortedItemsItr;
	SortEvaluator* sortEvaluator;
};

}
}


#endif // __AGGREGATE_SORT_OPERATOR_H__
