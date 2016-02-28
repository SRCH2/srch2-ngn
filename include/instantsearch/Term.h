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

#ifndef __TERM_H__
#define __TERM_H__

#include <instantsearch/platform.h>
#include <instantsearch/Constants.h>
#include <string>
#include <stdint.h>
#include "util/encoding.h"
#include "util/Assert.h"

namespace srch2
{
namespace instantsearch
{



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
            const float boost = 1.0, const float fuzzyMatchPenalty = 0.5, const uint8_t threshold = 0);

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


    std::string *getKeyword();

    std::string *getKeyword() const;

    /**
     * \returns the type of the Term.
     */
    TermType getTermType() const;

    /*
     * Used in attribute based search. searchableAttributeIdsList specifies the list of attributes
     * where keyword should be found in a data record.  attrOp flag indicates whether AND/OR
     * logic is applied on filter attributes.
     */
    void addAttributesToFilter(const vector<unsigned>& searchableAttributeIdsList, ATTRIBUTES_OP attrOp);

    /*
     *   getter function for a flag which indicates whether disjunction or conjunction is
     *   applied on filter fields.
     */
    ATTRIBUTES_OP getFilterAttrOperation();

    vector<unsigned>& getAttributesToFilter() const;

    string toString();
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
