// $Id: IndexSearcherInternal.h 3335 2013-05-11 03:41:56Z huaijie $

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

namespace bimaple
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

    //Get Schema pointer from IndexSearcherInternal
    IndexSearcherInternal(IndexReaderWriter *indexer);
    virtual ~IndexSearcherInternal() {};

    // find the next k answer starting from "offset". Can be used for
    // pagination. Returns the number of records found
    int search(const Query *query, QueryResults* queryResults, const int offset, const int nextK);

    // find top-k answer. returns the number of records found
    int search(const Query *query, QueryResults* queryResults, const int topK);

    // Added for stemmer
    // For GetAllResultsQuery
    int search(const Query *query, QueryResults* queryResults);


    // for doing a range  query with a rectangle but without keyword information
    void search(const Rectangle &queryRectangle, QueryResults *queryResults);


    // for doing a  range query with a circle but without keyword information
    void search(const Circle &queryCircle, QueryResults *queryResults);


    std::string getInMemoryData(unsigned internalRecordId) const
    {
        return this->indexData->forwardIndex->getInMemoryData(internalRecordId);
    }

    PrefixActiveNodeSet *computeActiveNodeSet(Term *term) const;
    void computeTermVirtualList(QueryResultsInternal *queryResults) const;

    ///Used by TermVirtualList
    const InvertedIndex *getInvertedIndex(){
        return this->indexData->invertedIndex;
    }

    void cacheClear();

    //DEBUG function. Used in CacheIntegration_Test
    bool cacheHit(const Query *query);

    //For testing
    const Trie* getTrie() const
    {
        return this->indexData->trie;
    }

private:

    const IndexData *indexData;
    IndexReadStateSharedPtr_Token indexReadToken;
    IndexReaderWriter *indexer;

    Cache *cacheManager;

    bool isValidTermPositionHit(unsigned postitionIndexOffset,int searchableAttributeId) const;

    int searchGetAllResultsQuery(const Query *query, QueryResults* queryResults);

    int searchTopKQuery(const Query *query, const int offset,
            const int nextK, QueryResults* queryResults);

    int searchMapQuery(const Query *query, QueryResults* queryResults);

    bool randomAccess(std::vector<TermVirtualList* > *virtualListVector,std::vector<float> &queryResultTermScores,
            std::vector<std::string> &queryResultMatchingKeywords, std::vector<unsigned> &queryResultEditDistances, const Query *query, unsigned recordId, unsigned skip, unsigned start);
};

}}

#endif /* __INDEXSEARCHERINTERNAL_H__ */
