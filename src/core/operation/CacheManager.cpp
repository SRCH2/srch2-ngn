
// $Id: Cache.cpp 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
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


bool PhysicalOperatorsCache::getPhysicalOperatorsInfo(string & key,  ts_shared_ptr<PhysicalOperatorCacheObject> & in){
	return this->cacheContainer->get(key , in);
}
void PhysicalOperatorsCache::setPhysicalOperatosInfo(string & key , ts_shared_ptr<PhysicalOperatorCacheObject> object){
	this->cacheContainer->put(key , object);
}
int PhysicalOperatorsCache::clear(){
	return this->cacheContainer->clear();
}

int ActiveNodesCache::findLongestPrefixActiveNodes(Term *term, ts_shared_ptr<PrefixActiveNodeSet> &in){

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
		ts_shared_ptr<PrefixActiveNodeSet> cacheHit;
		if(this->cacheContainer->get(key , cacheHit) == true && cacheHit->getEditDistanceThreshold() >= termThreshold){
			in = cacheHit;
			return 1;
		}
	}
    // no prefix has a cached PrefixActiveNodeSet
	return 0;

}


int ActiveNodesCache::setPrefixActiveNodeSet(ts_shared_ptr<PrefixActiveNodeSet> &prefixActiveNodeSet){
	vector<CharType> *prefix = prefixActiveNodeSet->getPrefix();
	std::stringstream ss ;
	ss << prefixActiveNodeSet->getEditDistanceThreshold();
	std::string exactOrFuzzy = ss.str();
//	std::string exactOrFuzzy =  prefixActiveNodeSet->getEditDistanceThreshold() == 0?"0":"1";
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

bool QueryResultsCache::getQueryResults(string & key, ts_shared_ptr<QueryResultsCacheEntry> & in){
	return this->cacheContainer->get(key , in);
}
void QueryResultsCache::setQueryResults(string & key , ts_shared_ptr<QueryResultsCacheEntry> object){
	this->cacheContainer->put(key , object);
}
int QueryResultsCache::clear(){
	return this->cacheContainer->clear();
}

int CacheManager::clear(){
	return this->aCache->clear() && this->qCache->clear() && this->pCache->clear();
}



}}
