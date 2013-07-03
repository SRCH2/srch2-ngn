//$Id: Cache_Test.cpp 3097 2012-12-19 00:55:28Z oliverax $

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

#include "operation/Cache.h"

#include <instantsearch/GlobalCache.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

#include <cstring>
#include <assert.h>

using namespace std;
namespace bmis = bimaple::instantsearch;
using bmis::Query;
using bmis::TermType;
using bmis::Term;
using bmis::ExactTerm;
using bmis::FuzzyTerm;
using bmis::Cache;
using bmis::ConjunctionCacheResultsEntry;
using bmis::print_trace;

int main(int argc, char *argv[])
{

    Query *query1 = new Query(bmis::TopKQuery);
    string keywords[2] = {
            "tom","cat"
    };

    // for each keyword in the user input, add a term to the query
    for (unsigned i = 0; i < 2; ++i)
    {
        TermType type = bmis::COMPLETE;
        Term *term = new Term(keywords[i], type, 1, 1);
        query1->add(term);
    }

    vector<Term* > *cacheQueryTerms = new vector<Term *>();

    const vector<Term* > *queryTerms = query1->getQueryTerms();
    for (vector<Term*>::const_iterator iter = queryTerms->begin(); iter!=queryTerms->end(); iter++)
    {
        string tmp((*iter)->getKeyword()->c_str());
        cacheQueryTerms->push_back(new Term( tmp, (*iter)->getTermType(), (*iter)->getBoost(), (*iter)->getSimilarityBoost(), (*iter)->getThreshold() ) );
    }

    ConjunctionCacheResultsEntry *conjunctionCacheResultsEntry = new ConjunctionCacheResultsEntry(cacheQueryTerms, NULL, NULL);

    Cache *cache = new Cache(10000,10000);

    cache->setCachedConjunctionResult(query1->getQueryTerms(),conjunctionCacheResultsEntry);

    ConjunctionCacheResultsEntry *conjunctionCacheResultsEntry_Assert;

    cache->getCachedConjunctionResult(query1->getQueryTerms(),conjunctionCacheResultsEntry_Assert);

    // cache disabled
    //ASSERT (conjunctionCacheResultsEntry_Assert != NULL);
    //ASSERT (conjunctionCacheResultsEntry_Assert == conjunctionCacheResultsEntry);

    delete query1;
    //New Query
    Query *query2 = new Query(bmis::TopKQuery);

    // for each keyword in the user input, add a term to the query
    for (unsigned i = 0; i < 1; ++i)
    {
        TermType type = bmis::COMPLETE;
        Term *term = new Term(keywords[i], type, 1, 1);
        query2->add(term);
    }

    cache->getCachedConjunctionResult(query2->getQueryTerms(),conjunctionCacheResultsEntry_Assert);

    ASSERT (conjunctionCacheResultsEntry_Assert == NULL);
    //conjunctionCacheResultsEntry = NULL;

    delete cache;
    delete query2;

    cout << "Cache Unit Tests: Passed\n";

    return 0;
}
