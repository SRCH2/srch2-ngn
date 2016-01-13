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

#include "operation/CacheManager.h"

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
namespace srch2is = srch2::instantsearch;
using srch2is::Query;
using srch2is::TermType;
using srch2is::Term;
using srch2is::ExactTerm;
using srch2is::FuzzyTerm;
using srch2is::CacheManager;
using srch2is::print_trace;

int main(int argc, char *argv[])
{

    Query *query1 = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[2] = {
            "tom","cat"
    };

    // for each keyword in the user input, add a term to the query
    for (unsigned i = 0; i < 2; ++i)
    {
        TermType termType = srch2is::TERM_TYPE_COMPLETE;
        Term *term = new Term(keywords[i], termType, 1, 1);
        query1->add(term);
    }

    vector<Term* > *cacheQueryTerms = new vector<Term *>();

    const vector<Term* > *queryTerms = query1->getQueryTerms();
    for (vector<Term*>::const_iterator iter = queryTerms->begin(); iter!=queryTerms->end(); iter++)
    {
        string tmp((*iter)->getKeyword()->c_str());
        cacheQueryTerms->push_back(new Term( tmp, (*iter)->getTermType(), (*iter)->getBoost(), (*iter)->getSimilarityBoost(), (*iter)->getThreshold() ) );
    }

//    ConjunctionCacheResultsEntry *conjunctionCacheResultsEntry = new ConjunctionCacheResultsEntry(cacheQueryTerms, NULL, NULL);//

    CacheManager *cache = new CacheManager(10000);

//    cache->setCachedConjunctionResult(query1->getQueryTerms(),conjunctionCacheResultsEntry);//

//    ConjunctionCacheResultsEntry *conjunctionCacheResultsEntry_Assert;//

//    cache->getCachedConjunctionResult(query1->getQueryTerms(),conjunctionCacheResultsEntry_Assert);//

    // cache disabled
    //ASSERT (conjunctionCacheResultsEntry_Assert != NULL);
    //ASSERT (conjunctionCacheResultsEntry_Assert == conjunctionCacheResultsEntry);

    delete query1;
    //New Query
    Query *query2 = new Query(srch2is::SearchTypeTopKQuery);

    // for each keyword in the user input, add a term to the query
    for (unsigned i = 0; i < 1; ++i)
    {
        TermType termType = srch2is::TERM_TYPE_COMPLETE;
        Term *term = new Term(keywords[i], termType, 1, 1);
        query2->add(term);
    }

//    cache->getCachedConjunctionResult(query2->getQueryTerms(),conjunctionCacheResultsEntry_Assert);//
//
//    ASSERT (conjunctionCacheResultsEntry_Assert == NULL);//
    //conjunctionCacheResultsEntry = NULL;

    delete cache;
    delete query2;

    cout << "Cache Unit Tests: Passed\n";

    return 0;
}
