//$Id: Term.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __TERM_H__
#define __TERM_H__

#include <instantsearch/platform.h>
#include <string>
#include <stdint.h>
#include "util/encoding.h"

namespace srch2
{
namespace instantsearch
{

typedef enum
{
	TERM_TYPE_PREFIX = 0,
	TERM_TYPE_COMPLETE = 1
} TermType;

/**
 * This class defines a Term representing a word from text, which can
 * be used to define a keyword condition in a search query.
 *
 * This abstract base class is extended based on "matching" criteria
 * into two subclasses ExactTerm and FuzzyTerm.  These two
 * extensions can in turn be of types "PREFIX" or "COMPLETE".

 * Example: Consider a record with a word "elephant". The following
 * table shows four matching situations for a query keyword (shown in
 * the table) and the word "elephant" in the record:
 * <TABLE>
 * <TR>
 *         <TD></TD>
 *         <TD> COMPLETE MATCHING </TD>
 *         <TD> PREFIX MATCHING </TD>
 * </TR>
 * <TR>
 *         <TD> EXACT MATCHING </TD>
 *         <TD> elephant </TD>
 *         <TD> elep </TD>
 * </TR>
 * <TR>
 *         <TD> FUZZY MATCHING </TD>
 *         <TD> eliphant </TD>
 *         <TD> elip </TD>
 * </TR>
 * </TABLE>
 *
 *
 * Each term is associated with four values used in the search and
 * ranking process.
 *
 *<TABLE>
 * <TR>
 *         <TD> TERM </TD>
 *         <TD> Boost </TD>
 *         <TD> SimilarityBoost </TD>
 *         <TD> Edit-Distance Threshold </TD>
 * </TR>
 * <TR>
 *         <TD> EXACT </TD>
 *         <TD> [1-100]  </TD>
 *         <TD> 1  </TD>
 *         <TD> 0  </TD>
 * </TR>
 * <TR>
 *         <TD> FUZZY </TD>
 *         <TD> [1-100] </TD>
 *         <TD> [1-100] </TD>
 *      <TD> [0-5] </TD>
 * </TR>
 * </TABLE>
 */
class MYLIB_EXPORT Term
{
public:

	/**
	 * The edit-distance threshold of a fuzzy match is based on
	 *  the length of query keyword to match.
	 *
	 *  TODO add extra API to enable user to give a threshold based on
	 *  length and remove this function from the exposed code.
	 */
    static uint8_t getNormalizedThreshold(unsigned keywordLength)
    {
        // Keyword length:             [1,4]        [5,7]        >= 8
        // Edit-distance threshold:      0            1           2
        if (keywordLength <= 4)
            return 0;
        else if (keywordLength <= 7)
            return 1;
        return 2;
    }

    Term(const std::string &keyword, TermType type,
            const float boost = 1.0, const float similarityBoost = 0.5, const uint8_t threshold = 0);

    /**
     * \ingroup RankingFunctions
     * Each Term is associated with a boost value, which defines the
     * importance of the term. It is in the range [1-100].
     */
    void setBoost(const float boost);
    float getBoost() const;

    /**
     * \ingroup RankingFunctions
     *
     * Each term is associated with an edit distance Threshold
     * for fuzzy search. For ExactTerm, its default value is 0.
     * For a FuzzyTerm, its value is in the range [0-5].
     */
    void setThreshold(const uint8_t threshold);
    uint8_t getThreshold() const;

    /**
     * \ingroup RankingFunctions
     * 
     * When a query is constructed, each of its fuzzy terms is
     * associated with a SimilarityBoost, which will be used to
     * boost the records with a keyword that matches the term
     * exactly.  Its value is in the range [1-100].  For instance,
     * suppose we have a query with a FuzzyTerm "michal" and a
     * threshold "1".  Consider the records with keywords
     * "michal", "michel", "mikhal", or "michael", whose edit
     * distances to the term are all within 1.  We will use the
     * boost value of this term to increase the rank of the
     * records with the keyword "michal" since it is an exact
     * match.
     *
     * For ExactTerm, its SimilarityBoost value is 1.  The reason
     * is that all the answers of this term should have a keyword
     * matching this term exactly, and no boosting is needed.
     *
     */
    void setSimilarityBoost(const float boost);
    float getSimilarityBoost() const;

    /**
     * \returns the String reference in the Term object.
     */


    /*
     * TODO Remove this function.
     */
    std::string *getKeyword();

    std::string *getKeyword() const;

    /**
     * \returns the type of the Term.
     */
    TermType getTermType() const;

    /*
     * Used in attribute based search. If we want a keyword
     * to appear in a certain group of attributes a
     * @parameter searchableAttributeId is added.
     *
     * We can have maximum 31 searchable attributes for attribute based
     * search.
     *
     * TODO modify function and parameter names
     * TODO add some helper functions for attribute based search.
     */
    void addAttributeToFilterTermHits(unsigned searchableAttributeId);

    unsigned getAttributeToFilterTermHits() const;

    /**
     * Destructor to free persistent resources used by the Term.
     *
     * The query is responsible for freeing the space of its
     * Terms. Look at the example code in the main description of
     * Query.
     */
     virtual ~Term();

    private:
        struct Impl;
        Impl *impl;
};

class MYLIB_EXPORT FuzzyTerm
{
public:
	/*
	 *
	 */
    static Term* create(const std::string &queryKeyword, TermType type,
            const float boost = 1, const float similarityBoost = 0.5, const uint8_t threshold = 1);
};

class MYLIB_EXPORT ExactTerm
{
public:
    static Term* create(const std::string &queryKeyword, TermType type,
            const float boost = 1, const float similarityBoost = 0.5);
};

}}

#endif //__TERM_H__
