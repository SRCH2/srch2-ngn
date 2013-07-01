
// $Id: QueryResultsInternal.h 3480 2013-06-19 08:00:34Z jiaying $

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
//#include "operation/TermVirtualList.h"
#include <instantsearch/Stat.h>
#include <instantsearch/Ranker.h>
#include "index/ForwardIndex.h"
#include <vector>
#include <queue>
#include <string>
#include <set>

namespace srch2
{
namespace instantsearch
{
class Query;
class IndexSearcherInternal;
class TermVirtualList;

struct QueryResult {
    string externalRecordId;
    unsigned internalRecordId;
    float score;
    std::vector<std::string> matchingKeywords;
    std::vector<unsigned> attributeBitmaps;
    std::vector<unsigned> editDistances;
    
    // only the results of MapQuery have this
    double physicalDistance; // TODO check if there is a better way to structure the "location result"
    
    // this operator should be consistent with two others in TermVirtualList.h and InvertedIndex.h
    bool operator<(const QueryResult& queryResult) const
    {
        float leftRecordScore, rightRecordScore;
        unsigned leftRecordId  = internalRecordId;
        unsigned rightRecordId = queryResult.internalRecordId;
		leftRecordScore = score;
		rightRecordScore = queryResult.score;
        return DefaultTopKRanker::compareRecordsGreaterThan(leftRecordScore,  leftRecordId,
                                    rightRecordScore, rightRecordId);
    }
    
};
 
class QueryResultsInternal : public QueryResults
{
public:
    QueryResultsInternal(IndexSearcherInternal *indexSearcherInternal, Query *query);
    virtual ~QueryResultsInternal();

    std::vector<TermVirtualList* > *getVirtualListVector() { return virtualListVector; };

    QueryResult* getQueryResult(unsigned position);
    unsigned getNumberOfResults() const;
    std::string getRecordId(unsigned position) const;
    unsigned getInternalRecordId(unsigned position) const;
    std::string getInMemoryRecordString(unsigned position) const;
    
    float getResultScore(unsigned position) const;
    
    void getMatchingKeywords(const unsigned position, vector<string> &matchingKeywords) const;
    void getEditDistances(const unsigned position, vector<unsigned> &editDistances) const;
    void getMatchedAttributeBitmaps(const unsigned position, std::vector<unsigned> &matchedAttributeBitmaps) const;
    void getMatchedAttributes(const unsigned position, std::vector<vector<unsigned> > &matchedAttributes) const;
    
    // only the results of MapQuery have this
    double getPhysicalDistance(const unsigned position) const; // TODO check if there is a better way to structure the "location result"
    
    void setNextK(const unsigned k); // set number of results in nextKResultsHeap
    void insertResult(QueryResult &queryResult); //insert queryResult to the priority queue
    bool hasTopK(const float maxScoreForUnvisitedRecords);
    
    void fillVisitedList(std::set<unsigned> &visitedList); // fill visitedList with the recordIds in sortedFinalResults
    void finalizeResults(const ForwardIndex *forwardIndex);

    const Query* getQuery() const {
        return this->query;
    }
    
    // DEBUG function. Used in CacheIntegration_Test
    bool checkCacheHit(IndexSearcherInternal *indexSearcherInternal, Query *query);
    
    void printStats() const;
    
    void printResult() const;

    void addMessage(const char* msg)
    {
        this->stat->addMessage(msg);
    }
    
    std::vector<QueryResult> sortedFinalResults;
    std::vector<TermVirtualList* > *virtualListVector;
    
    Stat *stat;
    
 private:
    Query* query;
    unsigned nextK;
    
    const IndexSearcherInternal *indexSearcherInternal;

    // OPT use QueryResults Pointers.
    // TODO: DONE add an iterator to consume the results by converting ids using getExternalRecordId(recordId)
    std::priority_queue<QueryResult, std::vector<QueryResult> > nextKResultsHeap;
};
}}
#endif /* __QUERYRESULTSINTERNAL_H__ */
