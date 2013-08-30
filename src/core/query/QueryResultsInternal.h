
// $Id: QueryResultsInternal.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#ifndef __QUERYRESULTSINTERNAL_H__
#define __QUERYRESULTSINTERNAL_H__

#include <instantsearch/QueryResults.h>
#include "operation/TermVirtualList.h"
#include <instantsearch/Stat.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/Score.h>

#include "index/ForwardIndex.h"
#include "util/Assert.h"
#include "util/Logger.h"

#include <vector>
#include <queue>
#include <string>
#include <set>
#include <map>


using srch2::util::Logger;

namespace srch2
{
namespace instantsearch
{

class IndexSearcherInternal;
class QueryResultFactoryInternal;
class FacetedSearchFilter;


class QueryResult {
public:
    string externalRecordId;
    unsigned internalRecordId;
    Score _score;
    std::vector<std::string> matchingKeywords;
    std::vector<unsigned> attributeBitmaps;
    std::vector<unsigned> editDistances;
    std::map<std::string,Score> valuesOfParticipatingNonSearchableAttributes;
    // only the results of MapQuery have this
    double physicalDistance; // TODO check if there is a better way to structure the "location result"
    Score getResultScore() const
    {
    	return _score;
    }
    friend class QueryResultFactoryInternal;

private:
    QueryResult(const QueryResult& copy_from_me){
    	externalRecordId = copy_from_me.externalRecordId;
    	internalRecordId = copy_from_me.internalRecordId;
    	_score = copy_from_me._score;
    	matchingKeywords = copy_from_me.matchingKeywords;
    	attributeBitmaps = copy_from_me.attributeBitmaps;
    	editDistances = copy_from_me.editDistances;
    }
    QueryResult(){
    };
};

class QueryResultComparator
{
public:

  bool operator() (const QueryResult*  lhs, const QueryResult*  rhs) const
  {
      float leftRecordScore, rightRecordScore;
      unsigned leftRecordId  = lhs->internalRecordId;
      unsigned rightRecordId = rhs->internalRecordId;
		Score _leftRecordScore = lhs->_score;
		Score _rightRecordScore = rhs->_score;
		return DefaultTopKRanker::compareRecordsGreaterThan(_leftRecordScore,  leftRecordId,
		                                    _rightRecordScore, rightRecordId);
  }
};


class QueryResultFactoryInternal{
public:
	QueryResult * createQueryResult(){
		QueryResult * newResult = new QueryResult();
		queryResultPointers.push_back(newResult);
		return newResult;
	}
	~QueryResultFactoryInternal(){
	    Logger::debug("Query results are being destroyed in factory destructor." );
		for(std::vector<QueryResult *>::iterator iter = queryResultPointers.begin();
					iter != queryResultPointers.end() ; ++iter){
			delete *iter;
		}
	}
	std::vector<QueryResult *> queryResultPointers;
};



////////////////////////////////////// QueryResultsInternal Header //////////////////////////////////
class QueryResultsInternal
{
public:
	friend class QueryResults;
    friend class ResultsPostProcessor;
	QueryResultsInternal();
	void init(QueryResultFactory * resultsFactory , const IndexSearcherInternal *indexSearcherInternal, Query *query);

    QueryResultsInternal(QueryResultFactory * resultsFactory , const IndexSearcherInternal *indexSearcherInternal, Query *query);
    virtual ~QueryResultsInternal();

    std::vector<TermVirtualList* > *getVirtualListVector() { return virtualListVector; };




    
    

    void setNextK(const unsigned k); // set number of results in nextKResultsHeap
    void insertResult(QueryResult * queryResult); //insert queryResult to the priority queue
    bool hasTopK(const float maxScoreForUnvisitedRecords);
    
    void fillVisitedList(std::set<unsigned> &visitedList); // fill visitedList with the recordIds in sortedFinalResults
    void finalizeResults(const ForwardIndex *forwardIndex);
    const Query* getQuery() const {
        return this->query;
    }
    
    QueryResultFactory * getReultsFactory(){
    	return resultsFactory;
    }

    // DEBUG function. Used in CacheIntegration_Test
    bool checkCacheHit(IndexSearcherInternal *indexSearcherInternal, Query *query);
    
    
    std::vector<QueryResult *> sortedFinalResults;
    std::vector<TermVirtualList* > *virtualListVector;
    
	// map of attribute name to : "aggregation results for categories"
	// map<string, vector<Score>>
    /*
     * Example:
     * If the facet is created on two fields : Model and Price, that Model is categorical and Price is range
     * and price.range.start = 10, price.range.gap = 5 , price.range.end = 20 then this structure
     * which keeps the final results will contain :
     * Model =>
     *          <IBM,10>,<DELL,23>,<SONY,12>,<APPLE,25>
 *     Price =>
 *              <less than 10,14>,<10,80>,<15,24>,<20,30>
     */
	std::map<std::string , std::vector<std::pair<std::string, float> > > facetResults;
    Stat *stat;
    
 private:
    Query* query;
    unsigned nextK;

    const IndexSearcherInternal *indexSearcherInternal;

    QueryResultFactory * resultsFactory;


    // OPT use QueryResults Pointers.
    // TODO: DONE add an iterator to consume the results by converting ids using getExternalRecordId(recordId)
    std::priority_queue<QueryResult *, std::vector<QueryResult *>, QueryResultComparator > nextKResultsHeap;


};
}}
#endif /* __QUERYRESULTSINTERNAL_H__ */
