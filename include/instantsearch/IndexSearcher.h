//$Id: IndexSearcher.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __INDEXSEARCHER_H__
#define __INDEXSEARCHER_H__

#include <instantsearch/platform.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/Indexer.h>
#include <string>
#include <record/LocationRecordUtil.h>
#include <operation/Cache.h>

namespace srch2
{
namespace instantsearch
{

class Indexer;
class Query;
class QueryResults;

/**
 * IndexSearcher provides an interface to do search using the
 * index. The IndexSearcher internally is a wrapper around the
 * IndexReader and supports search. It also does caching and ranking.
 */
class MYLIB_EXPORT IndexSearcher
{
public:
    /**
     * Creates an IndexSearcher object.
     * @param indexer - An object holding the index structures and cache.
     */
    static IndexSearcher *create(Indexer *indexer);

    /*
     * Finds the suggestions of the engine for keyword using fuzzyMatchPenalty.
     * Returns the number of suggestions found.
     */
    virtual int suggest(const string & keyword, const float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions ) = 0;

    /**
     * Finds the next topK answers starting from
     * offset. This function can be used to support pagination of
     * search results. queryResults is a QueryResults
     * object, which must be created using the same query object
     * as the first argument in this function.
     *
     * returns the number of records found (at most topK).
     */
    virtual int search(const Query *query, QueryResults *queryResults, const int offset, const int nextK) = 0;

    ///Added for stemmer
    //virtual int searchWithStemmer(const Query *query, QueryResults *queryResults, const int offset, const int topK, bool &isStemmed) = 0;


    /**
     * Finds the first topK best answers.
     */
    virtual int search(const Query *query, QueryResults *queryResults, const int topK) = 0;

    /**
     * Does Map Search
     */
    virtual int search(const Query *query, QueryResults *queryResults) = 0;

    // for doing a geo range query with a circle
    virtual void search(const Circle &queryCircle, QueryResults *queryResults) = 0;

    // for doing a geo range query with a rectangle
    virtual void search(const Rectangle &queryRectangle, QueryResults *queryResults) = 0;

    // for retrieving only one result by having the primary key
    virtual void search(const std::string & primaryKey, QueryResults *queryResults) = 0;

    /// Added for stemmer
    //virtual int searchWithStemmer(const Query *query, QueryResults *queryResults, const int nextK = 0, bool &isStemmed) = 0;

    /// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
    virtual std::string getInMemoryData(unsigned internalRecordId) const = 0;

    virtual void cacheClear() = 0;
    /**
     * Destructor to free persistent resources used by the IndexSearcher.
     */
    virtual ~IndexSearcher() {};
};
}}

#endif //__INDEXSEARCHER_H__
