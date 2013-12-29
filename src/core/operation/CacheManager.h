// $Id: Cache.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
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

#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include <instantsearch/GlobalCache.h>
#include "operation/ActiveNode.h"
#include "query/QueryResultsInternal.h"
#include "CacheBase.h"

#include <boost/shared_ptr.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

class PhysicalOperatorCacheObject;

class PhysicalOperatorsCache {
	public:
	PhysicalOperatorsCache(unsigned long byteSizeOfCache = 134217728){
		this->cacheContainer = new CacheContainer<PhysicalOperatorCacheObject>(byteSizeOfCache);
	}
	bool getPhysicalOperatosInfo(string key,  ts_shared_ptr<PhysicalOperatorCacheObject> & in);
	void setPhysicalOperatosInfo(string key , ts_shared_ptr<PhysicalOperatorCacheObject> object);
	int clear();
	~PhysicalOperatorsCache(){
		delete this->cacheContainer;
	}
private:
	CacheContainer<PhysicalOperatorCacheObject> * cacheContainer;
};


class ActiveNodesCache {
public:
	ActiveNodesCache(unsigned long byteSizeOfCache = 134217728){
		this->cacheContainer = new CacheContainer<PrefixActiveNodeSet>(byteSizeOfCache);
	}
	int findLongestPrefixActiveNodes(Term *term, ts_shared_ptr<PrefixActiveNodeSet> &in);
	int setPrefixActiveNodeSet(ts_shared_ptr<PrefixActiveNodeSet> &prefixActiveNodeSet);
	int clear();
	~ActiveNodesCache(){
		delete cacheContainer;
	}
private:
	CacheContainer<PrefixActiveNodeSet> * cacheContainer;

};

class QueryResultsCacheEntry{
public:
	QueryResultsCacheEntry(){
		factory = new QueryResultFactoryInternal();
	}
	~QueryResultsCacheEntry(){
		delete factory ;
	}
    std::vector<QueryResult *> sortedFinalResults;
    bool resultsApproximated;
    long int estimatedNumberOfResults;
	std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > facetResults;
	QueryResultFactoryInternal * factory;
    void copyToQueryResultsInternal(QueryResultsInternal * destination){
    	destination->resultsApproximated = resultsApproximated;
    	destination->estimatedNumberOfResults = estimatedNumberOfResults;
    	for(std::vector<QueryResult *>::iterator queryResult = sortedFinalResults.begin(); queryResult != sortedFinalResults.end() ; ++ queryResult){
    		destination->sortedFinalResults.push_back(destination->getReultsFactory()->impl->createQueryResult(*(*queryResult)));
    	}
    	destination->facetResults = facetResults;
    }
    void copyFromQueryResultsInternal(QueryResultsInternal * destination){
    	resultsApproximated = destination->resultsApproximated;
    	estimatedNumberOfResults = destination->estimatedNumberOfResults;
    	for(std::vector<QueryResult *>::iterator queryResult = destination->sortedFinalResults.begin();
    			queryResult != destination->sortedFinalResults.end() ; ++ queryResult){
    		sortedFinalResults.push_back(factory->createQueryResult(*(*queryResult)));
    	}
    	facetResults = destination->facetResults;
    }

    unsigned getNumberOfBytes(){
    	unsigned result = 0;
    	for(std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::iterator attr =
                facetResults.begin(); attr != facetResults.end(); ++attr){
    		result += sizeof(attr->first);
    		result += sizeof(attr->second.first);
    		for(std::vector<std::pair<std::string, float> >::iterator f = attr->second.second.begin() ;
    				f != attr->second.second.end() ; ++f){
    			result += sizeof(f->first) + sizeof(float);
    		}
    	}
    	result += sizeof(bool) + sizeof(long int);
    	for(std::vector<QueryResult *>::iterator q = sortedFinalResults.begin();
    			q != sortedFinalResults.end() ; ++q){
    		result += (*q)->getNumberOfBytes();
    	}
    	return result;
    }
};

class QueryResultsCache {
public:
	QueryResultsCache(unsigned long byteSizeOfCache = 134217728){
		this->cacheContainer = new CacheContainer<QueryResultsCacheEntry>(byteSizeOfCache);
	}

	bool getQueryResults(string key,  ts_shared_ptr<QueryResultsCacheEntry> & in);
	void setQueryResults(string key , ts_shared_ptr<QueryResultsCacheEntry> object);
	int clear();
	~QueryResultsCache(){
		delete this->cacheContainer;
	}
private:
	CacheContainer<QueryResultsCacheEntry> * cacheContainer;
};


/*
 * This class is the cache manager. The CacheManager is the holder of different kinds of Cache, e.g.
 * ActiveNodesCache.
 */
class CacheManager : public GlobalCache
{
public:
    CacheManager(unsigned long byteSizeOfCache = 134217728){
    	aCache = new ActiveNodesCache(byteSizeOfCache);
    	qCache = new QueryResultsCache(byteSizeOfCache);
    	pCache = new PhysicalOperatorsCache(byteSizeOfCache);
    }
    virtual ~CacheManager(){
    	delete aCache;
    	delete qCache;
    	delete pCache;
    }

    int clear();
    ActiveNodesCache * getActiveNodesCache();
    QueryResultsCache * getQueryResultsCache();
    PhysicalOperatorsCache * getPhysicalOperatorsCache();

private:
    ActiveNodesCache * aCache;

    QueryResultsCache * qCache;

    PhysicalOperatorsCache * pCache;

};



}
}

#endif /* __CACHEMANAGER_H__ */
