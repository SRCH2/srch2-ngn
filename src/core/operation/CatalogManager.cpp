

#include "CatalogManager.h"

namespace srch2
{
namespace instantsearch
{

CatalogManager::CatalogManager(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie){
	this->forwardIndex = forwardIndex;
	this->invertedIndex = invertedIndex;
	this->trie = trie;
}

CatalogManager::~CatalogManager(){

}

// calculates required information for building physical plan.
// this information is accessible later by passing nodeId of LogicalPlanNodes.
void CatalogManager::calculateInfoForLogicalPlan(const LogicalPlan * logicalPlan){
	/*
	 * this function calculates this information:
	 * 1. Active node sets for terminal nodes of LogicalPlan
	 * 2. Estimated number of results for each internal node of LogicalPlan. (keeps this info in estimatedNumberOfResults)
	 */
}

// returns the estimated number of results for a
// LogicalPlanNode that its id is nodeId.
// returns false if this id is not found.
bool CatalogManager::getEstimatedNumberOfResults(unsigned nodeId, unsigned & estimate){

}



}
}
