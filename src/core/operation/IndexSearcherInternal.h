// $Id: IndexSearcherInternal.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#ifndef __INDEXSEARCHERINTERNAL_H__
#define __INDEXSEARCHERINTERNAL_H__

#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Term.h>
#include <instantsearch/Ranker.h>
#include <query/QueryResultsInternal.h>

//#include "operation/Cache.h"
//#include "IndexerInternal.h"
//#include "operation/ActiveNode.h"
//#include "operation/TermVirtualList.h"
//#include "index/Trie.h"
//#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"
#include "geo/QuadNodeInternalStructures.h"
//#include "license/LicenseVerifier.h"
//#include "quadtree/QuadTree.h"
#include "operation/IndexData.h"
#include <vector>

namespace srch2
{
namespace instantsearch
{

class QueryResultsInternal;
class QueryResults;
class TermVirtualList;
class IndexReaderWriter;
class InvertedIndex;

class IndexSearcherInternal : public IndexSearcher
{
public:

	static const unsigned HISTOGRAM_POPULARITY_THRESHOLD;

    //Get Schema pointer from IndexSearcherInternal
    IndexSearcherInternal(IndexReaderWriter *indexer);
    virtual ~IndexSearcherInternal() {};

    int suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions);

    unsigned estimateNumberOfResults(const Query *query);

    // find the next k answer starting from "offset". Can be used for
    // pagination. Returns the number of records found
    int search(const Query *query, QueryResults* queryResults, const int offset, const int nextK,
    		unsigned estimatedNumberOfResultsThresholdGetAll = 10000 , unsigned numberOfEstimatedResultsToFindGetAll = 2000);

    // find top-k answer. returns the number of records found
    int search(const Query *query, QueryResults* queryResults, const int topK,
    		unsigned estimatedNumberOfResultsThresholdGetAll=10000 , unsigned numberOfEstimatedResultsToFindGetAll=2000);

    // Added for stemmer
    // For GetAllResultsQuery
    int search(const Query *query, QueryResults* queryResults);


    // for doing a range  query with a rectangle but without keyword information
    void search(const Rectangle &queryRectangle, QueryResults *queryResults);


    // for doing a  range query with a circle but without keyword information
    void search(const Circle &queryCircle, QueryResults *queryResults);

    // for retrieving only one result by having the primary key
    void search(const std::string & primaryKey, QueryResults *queryResults);

    std::string getInMemoryData(unsigned internalRecordId) const {
        return this->indexData->forwardIndex->getInMemoryData(internalRecordId);
    }

    PrefixActiveNodeSet *computeActiveNodeSet(Term *term) const;
    void computeTermVirtualList(QueryResults *queryResults ,
    		vector<PrefixActiveNodeSet *> * activeNodes = NULL ,
    		const vector<float> * scoreOfTopRecords = NULL) const;

    // This function uses the histogram information of the trie to estimate the number of records which have this term
    unsigned getEstimatedNumberOfRecordsWithThisTerm(Term *term , PrefixActiveNodeSet * activeNodes) const;

    // This functions defined the policy of estimating the results or computing them.
    bool isTermTooPopular(Term *term, PrefixActiveNodeSet * activeNodes, unsigned & popularity ) const;

    // This function find the runtime score of the top record of the inverted list of most popular suggestion of this term
    float findTopRunTimeScoreOfLeafNodes(Term *term , float prefixMatchPenalty , PrefixActiveNodeSet * activeNodes) const;

    ///Used by TermVirtualList
    const InvertedIndex *getInvertedIndex() {
        return this->indexData->invertedIndex;
    }

    ForwardIndex * getForwardIndex() {
        return this->indexData->forwardIndex;
    }

    Schema * getSchema() {
        return this->indexData->schemaInternal;
    }

    void cacheClear();

    //DEBUG function. Used in CacheIntegration_Test
    bool cacheHit(const Query *query);

    //For testing
    const Trie* getTrie() const {
        return this->indexData->trie;
    }

private:

    const IndexData *indexData;
    IndexReadStateSharedPtr_Token indexReadToken;
    IndexReaderWriter *indexer;

    Cache *cacheManager;

    bool isValidTermPositionHit(unsigned postitionIndexOffset,int searchableAttributeId) const;

    /*
     * estimatedNumberOfResultsThresholdGetAll & numberOfEstimatedResultsToFindGetAll :
     * If the estimated number of results for a query is larger than the value of this argument,
     * this function decides to estimate the results by finding topK. In this case
     * this function uses numberOfEstimatedResultsToFindGetAll as K.
     */
    int searchGetAllResultsQuery(const Query *query, QueryResults* queryResults,
    		unsigned estimatedNumberOfResultsThresholdGetAll=10000 , unsigned numberOfEstimatedResultsToFindGetAll=2000);

    /*
     * if vector<PrefixActiveNodeSet *> * activeNodesVector is passed to this function, it uses the value instead of
     * re-computing all active nodes. If the value is NULL, it computes the activenodes itself.
     *
     */
    int searchTopKQuery(const Query *query, const int offset,
                        const int nextK, QueryResults* queryResults , vector<PrefixActiveNodeSet *> * activeNodesVector = NULL);

    int searchTopKFindResultsForOnlyOnePopularKeyword(const Query *query, PrefixActiveNodeSet * activeNodes, unsigned k, QueryResults * queryResults);

    int searchMapQuery(const Query *query, QueryResults* queryResults);

    void addMoreNodesToExpansion(const TrieNode* trieNode, unsigned distance, unsigned bound, MapSearcherTerm &mapSearcherTerm);

    bool randomAccess(std::vector<TermVirtualList* > *virtualListVector,std::vector<float> &queryResultTermScores,
                      std::vector<std::string> &queryResultMatchingKeywords, std::vector<unsigned> &queryResultBitmaps, std::vector<unsigned> &queryResultEditDistances, const Query *query, unsigned recordId, unsigned skip, unsigned start);

    // This is a helper function of nextRecord. It takes a record ID from the 0-th list, and looks for the remaining lists for the next record ID by doing an intersection operation ("AND" logic).
    int getNextMatchingRecordID(int recordID, vector<TermVirtualList* >* virtualListVector);

    // return the next record that exists in all the virtual lists ("AND of these keyword lists). If there are no more records, return NO_MORE_RECORDS
    int getNextRecordID(vector<TermVirtualList* >* virtualListVector);

    // this functions traverses the trie to find the most popular suggestions of a term
    // returns the number of suggestions found.
    void findKMostPopularSuggestionsSorted(Term *term ,
    		PrefixActiveNodeSet * activeNodes,
    		unsigned numberOfSuggestionsToReturn ,
    		std::vector<std::pair<std::pair< float , unsigned > , const TrieNode *> > & suggestionPairs) const;

    unsigned estimateNumberOfResults(const Query *query, std::vector<PrefixActiveNodeSet *>& activeNodes) const;
    float getPrefixPopularityProbability(PrefixActiveNodeSet * activeNodes , unsigned threshold) const;
};

}
}

#endif /* __INDEXSEARCHERINTERNAL_H__ */
