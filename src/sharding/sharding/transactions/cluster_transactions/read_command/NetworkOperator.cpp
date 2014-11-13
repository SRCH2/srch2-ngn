#include "NetworkOpertator.h"


namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

bool NetworkOperator::open(ClusterPhysicalPlanExecutionParameter * params){
	return true;
}

void NetworkOperator::load(srch2is::QueryResults *queryResults){
	this->queryResults = queryResults->impl->sortedFinalResults;
	this->it = this->queryResults.begin();
	return;
}

QueryResult* NetworkOperator::getNext(ClusterPhysicalPlanExecutionParameter * params){
	if(it == this->queryResults.end())
		return NULL;
	QueryResult* queryResult = *it;
	it++;
	return queryResult;
}

bool NetworkOperator::close(ClusterPhysicalPlanExecutionParameter * params){
	this->queryResults.clear();
	this->it = this->queryResults.end();
	return true;
}

}
}



