//$Id: QueryResults.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#ifndef __QUERYRESULTS_H__
#define __QUERYRESULTS_H__

#include <vector>
#include <string>

#include <instantsearch/platform.h>

namespace srch2
{
namespace instantsearch
{

class Query;
class IndexSearcher;
class Score;

/**
 * This class defines QueryResults that acts as a container to hold
 * the results of IndexSearcher search methods. A single QueryResults
 * object can be used for paginating through results. For example,
 * consider a search query "John Lennon".  We can create a
 * QueryResults object on the query and retrieve the first K
 * results. The same QueryResults object can be used to retrieve the
 * next K results and so on.
 */
class MYLIB_EXPORT QueryResults
{
public:
    /**
     * TODO: change this to constructor. Also verify how much code changes will be
     * required to this.
     * Creates a QueryResults object.
     * @param[in] indexSearcher the reference to an IndexSearcher object.
     * @param[in] query the reference to a Query object.
     */
    static QueryResults* create(IndexSearcher* indexSearcher, Query* query);

    /**
     * Checks if the iterator reaches the end. @return false if
     * there is no next item in the QueryResults.
     */
    /*virtual bool next() = 0;*/

    /**
     * Gets the number of result items in the QueryResults object.
     */
    virtual unsigned getNumberOfResults() const = 0;

    /**
     * Gets the current record id while iterating through the QueryResults object.
     */
    /*virtual int getRecordId() = 0;*/

    /**
     * Gets the record id of the 'position'-th item in the QueryResults object.
     */
    virtual std::string getRecordId(unsigned position) const = 0;

    /**
     * Gets the record id of the 'position'-th item in the QueryResults object.
     * TODO: Move/remove getInternalRecordId to internal include files. There should be a mapping from external
     * Used to access the InMemoryData.
     */
    virtual unsigned getInternalRecordId(unsigned position) const = 0;

    virtual std::string getInMemoryRecordString(unsigned position) const = 0;

    /**
     * Gets the score of the 'position'-th item in the QueryResults object.
     */
    virtual std::string getResultScoreString(unsigned position) const = 0;
    virtual Score getResultScore(unsigned position) const = 0;

    /**
     *
     * Gets the matching keywords of the 'position'-th item in
     * the QueryResults object. For each query keyword return a matching
     * keyword in a record. If the query keyword is prefix then return a
     * matching prefix. In the case of multiple matching keywords for a
     * query keyword,the best match based on edit distance is returned.
     *
     * For example, for the query "ulman compilor", a result
     * record with keywords "ullman principles compiler" will
     * return ["ullman","compiler"] in the matchingKeywords
     * vector.  In particular, "compiler" is the best matching
     * keyword in the record for the second query keyword
     * "compilor".
     */
    virtual void getMatchingKeywords(const unsigned position, std::vector<std::string> &matchingKeywords) const = 0;

    /**
     * Gets the edit distances of the 'position'-th item in the
     * QueryResults object.
     * @param[in] position the position of a result in the returned
     * results (starting from 0).
     * @param[out] editDistances a vector of edit distances of the
     * best matching keywords in this result record with respect
     * to the query keywords.
     *
     * For example, for the query "ulman compilor", a result
     * record with keywords "ullman principles conpiler" will
     * return [1,2] in the editDistances vector.  In particular,
     * "2" means that the best matching keyword ("conpiler") in
     * this record has an edit distance 2 to the second query
     * keyword "compilor".
     */
    virtual void getEditDistances(const unsigned position, std::vector<unsigned> &editDistances) const = 0;

    // The following two functions only work for attribute based search
    virtual void getMatchedAttributeBitmaps(const unsigned position, std::vector<unsigned> &matchedAttributeBitmaps) const = 0;

    virtual void getMatchedAttributes(const unsigned position, std::vector<std::vector<unsigned> > &matchedAttributes) const = 0;
    /*
     *   In Geo search return distance between location of the result and center of the query rank.
     *   TODO: Change the name to getGeoDistance()
     */
    virtual double getPhysicalDistance(const unsigned position) const = 0;

    //TODO: These three functions for internal debugging. remove from the header
    virtual void printStats() const = 0;

    virtual void printResult() const = 0;

    virtual void addMessage(const char* msg) = 0;

    /**
     * Destructor of the QueryResults object.
     */
    virtual ~QueryResults() {};
};
}}

#endif /* __QUERYRESULTS_H__ */
