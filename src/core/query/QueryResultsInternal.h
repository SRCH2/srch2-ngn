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
/*
 */

#ifndef __QUERYRESULTSINTERNAL_H__
#define __QUERYRESULTSINTERNAL_H__

#include <instantsearch/QueryResults.h>
#include "operation/TermVirtualList.h"
#include <instantsearch/Stat.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/TypedValue.h>
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

class QueryEvaluatorInternal;
class QueryResultFactoryInternal;
class FacetedSearchFilter;


class QueryResult {
public:
    string externalRecordId;
    unsigned internalRecordId;
    TypedValue _score;
    std::vector<std::string> matchingKeywords;
    std::vector<vector<unsigned> > attributeIdsList;
    std::vector<unsigned> editDistances;
    std::vector<TermType> termTypes;
    std::vector< TrieNodePointer > matchingKeywordTrieNodes;
    // only the results of MapQuery have this
    double physicalDistance; // TODO check if there is a better way to structure the "location result"
    TypedValue getResultScore() const
    {
    	return _score;
    }

    unsigned getNumberOfBytes(){
    	unsigned result = sizeof(QueryResult);
    	result += externalRecordId.capacity();
    	result += _score.getNumberOfBytes() - sizeof(TypedValue);
    	for(unsigned i=0 ; i< matchingKeywords.size(); ++i){
    		result += matchingKeywords[i].capacity();
    	}
    	for (unsigned i = 0; i < attributeIdsList.size(); ++i)
    		result += attributeIdsList[i].capacity() * sizeof(unsigned);
    	result += attributeIdsList.capacity() * sizeof(void *);
    	result += editDistances.capacity() * sizeof(unsigned);
    	result += termTypes.capacity() * sizeof(unsigned);
    	result += matchingKeywordTrieNodes.capacity() * sizeof(TrieNodePointer);
    	return result;
    }
    friend class QueryResultFactoryInternal;

private:
    QueryResult(const QueryResult& copy_from_me){
    	externalRecordId = copy_from_me.externalRecordId;
    	internalRecordId = copy_from_me.internalRecordId;
    	_score = copy_from_me._score;
    	matchingKeywords = copy_from_me.matchingKeywords;
    	attributeIdsList = copy_from_me.attributeIdsList;
    	editDistances = copy_from_me.editDistances;
    	termTypes = copy_from_me.termTypes;
    	matchingKeywordTrieNodes = copy_from_me.matchingKeywordTrieNodes;
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
		TypedValue _leftRecordScore = lhs->_score;
		TypedValue _rightRecordScore = rhs->_score;
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
	QueryResult * createQueryResult(QueryResult & queryResult){
		QueryResult * newResult = new QueryResult(queryResult);
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
	void init(QueryResultFactory * resultsFactory ,const QueryEvaluatorInternal *queryEvaluatorInternal, Query *query);

    QueryResultsInternal(QueryResultFactory * resultsFactory , const QueryEvaluatorInternal *queryEvaluatorInternal, Query *query);
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

    std::vector<QueryResult *> sortedFinalResults;
    std::vector<TermVirtualList* > *virtualListVector;
    
    // This flag indicates whether the results are approximated
    bool resultsApproximated;

    // This member keeps the estimated number of results in case of top k, if all results are actually calculated, this value is -1
    long int estimatedNumberOfResults;
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
	std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > facetResults;
    Stat *stat;
    std::map<string, vector<unsigned> *> prefixToCompleteStore;
 private:
    Query* query;
    unsigned nextK;
    const QueryEvaluatorInternal *queryEvaluatorInternal;

    QueryResultFactory * resultsFactory;


    // OPT use QueryResults Pointers.
    // TODO: DONE add an iterator to consume the results by converting ids using getExternalRecordId(recordId)
    std::priority_queue<QueryResult *, std::vector<QueryResult *>, QueryResultComparator > nextKResultsHeap;


};
}}
#endif /* __QUERYRESULTSINTERNAL_H__ */
