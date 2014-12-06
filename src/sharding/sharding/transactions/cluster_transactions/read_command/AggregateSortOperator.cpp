/*
 * AggregateSortOperator.cpp
 *
 *  Created on: Aug 6, 2014
 *      Author: mahdi
 */

#include "AggregateSortOperator.h"
#include "NetworkOpertator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

ClusterSortOperator::~ClusterSortOperator(){
	for(unsigned i = 0; i < this->children.size(); i++){
		delete this->children[i];
	}
	this->children.clear();
	this->sortEvaluator = NULL;
}

bool ClusterSortOperator::open(ClusterPhysicalPlanExecutionParameter * params){
	for (unsigned i = 0; i < this->children.size(); i++){
		this->children[i]->open(params);
		while(true){
			QueryResult *queryResult = this->children[i]->getNext(params);
			if(queryResult == NULL){
				break;
			}
			if(find(this->sortedItems.begin(), this->sortedItems.end(), queryResult) != this->sortedItems.end()){
				Logger::sharding(Logger::Error, "Search includes duplicate results;");
				continue;
			}
			this->sortedItems.push_back(queryResult);
		}
	}
	sort(this->sortedItems.begin(), this->sortedItems.end(), ClusterSortOperator::ClusterSortOperatorItemCmp(this->sortEvaluator));
	this->sortedItemsItr = this->sortedItems.begin();
	return true;
}

QueryResult* ClusterSortOperator::getNext(ClusterPhysicalPlanExecutionParameter * params){
	if(this->sortedItemsItr == this->sortedItems.end()){
		return NULL;
	}
	QueryResult* result = *(this->sortedItemsItr);
	this->sortedItemsItr++;
	return result;
}

bool ClusterSortOperator::close(ClusterPhysicalPlanExecutionParameter * params){
	for(unsigned i = 0; i < this->children.size(); i++){
		this->children[i]->close(params);
	}
	return true;
}

bool ClusterSortOperator::addChild(NetworkOperator* networkOperator){
	ASSERT(networkOperator != NULL);
	this->children.push_back(networkOperator);
	return true;
}

}
}

