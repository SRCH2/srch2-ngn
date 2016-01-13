
#include "CacheManager.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include "operation/physical_plan/PhysicalPlan.h"
#include <string>
#include <map>
#include <stddef.h>

#include <iostream>
#include <sstream>

using std::vector;
using std::map;

namespace srch2
{
namespace instantsearch
{


bool PhysicalOperatorsCache::getPhysicalOperatorsInfo(string & key,  boost::shared_ptr<PhysicalOperatorCacheObject> & in){
	// TODO : Physical operators cache is not safe w.r.t. readview/writeview
	return false;
//	return this->cacheContainer->get(key , in);
}
void PhysicalOperatorsCache::setPhysicalOperatosInfo(string & key , boost::shared_ptr<PhysicalOperatorCacheObject> object){
	// TODO : Physical operators cache is not safe w.r.t. readview/writeview
//	this->cacheContainer->put(key , object);
}
int PhysicalOperatorsCache::clear(){
	return this->cacheContainer->clear();
}

int ActiveNodesCache::findLongestPrefixActiveNodes(Term *term, boost::shared_ptr<PrefixActiveNodeSet> &in){
    // return 0; // If uncommented, disable caching temporarily for debugging purposes

	// find the longest prefix with active nodes in the cache
	unsigned termThreshold = term->getThreshold();
	string *keyword = term->getKeyword();
	for (int i = keyword->size(); i >= 2; i --)
	{
		string prefix = keyword->substr(0, i);
		std::string exactOrFuzzy =  termThreshold == 0?"0":"1";
		string key = prefix + "$" + exactOrFuzzy;
		// Cache key is : keyword+0 (for exact) or keyword+1 (for fuzzy)
		// for example: terminator => "terminator$0"
		//         and  terminator~0.5 => "terminator$1"
		boost::shared_ptr<PrefixActiveNodeSet> cacheHit;
		if(this->cacheContainer->get(key , cacheHit) == true && cacheHit->getEditDistanceThreshold() >= termThreshold){
			in = cacheHit;
			return 1;
		}
	}
    // no prefix has a cached PrefixActiveNodeSet
	return 0;

}


int ActiveNodesCache::setPrefixActiveNodeSet(boost::shared_ptr<PrefixActiveNodeSet> &prefixActiveNodeSet){
    // return 1; // If uncommented, disable caching temporarily for debugging purposes

	vector<CharType> *prefix = prefixActiveNodeSet->getPrefix();
	std::stringstream ss ;
	ss << prefixActiveNodeSet->getEditDistanceThreshold();
	std::string exactOrFuzzy = ss.str();
	string key = getUtf8String(*prefix) + "$" + exactOrFuzzy;
	this->cacheContainer->put(key , prefixActiveNodeSet);
	return 1;
}
int ActiveNodesCache::clear(){
	return this->cacheContainer->clear();
}

ActiveNodesCache * CacheManager::getActiveNodesCache(){
	return this->aCache;
}

QueryResultsCache * CacheManager::getQueryResultsCache(){
	return this->qCache;
}

PhysicalOperatorsCache * CacheManager::getPhysicalOperatorsCache(){
	return this->pCache;
}

PhysicalPlanRecordItemFactory * CacheManager::getPhysicalPlanRecordItemFactory(){
	return this->physicalPlanRecordItemFactory;
}

bool QueryResultsCache::getQueryResults(string & key, boost::shared_ptr<QueryResultsCacheEntry> & in){
	return this->cacheContainer->get(key , in);
}
void QueryResultsCache::setQueryResults(string & key , boost::shared_ptr<QueryResultsCacheEntry> object){
	this->cacheContainer->put(key , object);
}
int QueryResultsCache::clear(){
	return this->cacheContainer->clear();
}

int CacheManager::clear(){
	return this->aCache->clear() && this->qCache->clear() && this->pCache->clear() && this->physicalPlanRecordItemFactory->clear();
}



}}
