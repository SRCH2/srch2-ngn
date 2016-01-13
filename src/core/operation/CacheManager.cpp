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
