
// $Id: Term.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include <instantsearch/Term.h>

#include <map>
#include <vector>
#include <string>
#include <stdint.h>
#include "util/encoding.h"

using std::string;
using std::vector;
namespace srch2
{
namespace instantsearch
{

//TODO OPT pass string pointer in FuzzyTerm/ExactTerm constructor rather than copying. But copy terms into cache
struct Term::Impl
{
    string keyword;
    TermType type;
    float boost;
    float similarityBoost;
    uint8_t threshold;
    unsigned searchableAttributeIdToFilter;
};

Term::Term(const string &keywordStr, TermType type, const float boost, const float fuzzyMatchPenalty, const uint8_t threshold)
{
    impl = new Impl;
    impl->keyword = keywordStr;
    impl->type = type;
    impl->boost = boost;
    impl->similarityBoost = fuzzyMatchPenalty;
    impl->threshold = threshold;
    impl->searchableAttributeIdToFilter = -1;
}

Term::~Term()
{
    if (impl != NULL) {
        delete impl;
    }
}

float Term::getBoost() const
{
    return impl->boost;
}

void Term::setBoost(const float boost)
{
    if (boost >= 1 && boost <= 2)
    {
        impl->boost = boost;
    }
    else
    {
        impl->boost = 1;
    }
}

float Term::getSimilarityBoost() const
{
    return impl->similarityBoost;
}

void Term::setSimilarityBoost(const float similarityBoost)
{
    if (similarityBoost >= 0.0 && similarityBoost <= 1.0)
    {
        impl->similarityBoost = similarityBoost;
    }
    else
    {
        impl->similarityBoost = 0.5;
    }
}

void Term::setThreshold(const uint8_t threshold)
{
    switch (threshold)
    {
        case 0: case 1: case 2:    case 3:    case 4:    case 5:
            impl->threshold = threshold;
            break;
        default:
            impl->threshold = 0;
    };
}

uint8_t Term::getThreshold() const
{
    return impl->threshold;
}

string *Term::getKeyword()
{
    return &(impl->keyword);
}

string *Term::getKeyword() const
{
    return &(impl->keyword);
}

TermType Term::getTermType() const
{
    return impl->type;
}

void Term::addAttributeToFilterTermHits(unsigned searchableAttributeId)
{
    this->impl->searchableAttributeIdToFilter = searchableAttributeId;
}

unsigned Term::getAttributeToFilterTermHits() const
{
    return this->impl->searchableAttributeIdToFilter;
}

////////////////////////

Term* ExactTerm::create(const string &keyword, TermType type, const float boost, const float similarityBoost)
{
    return new Term(keyword, type, boost, similarityBoost, 0);
}

Term* FuzzyTerm::create(const string &keyword, TermType type, const float boost, const float similarityBoost, const uint8_t threshold)
{
    return new Term(keyword, type, boost, similarityBoost, threshold);
}

}}

