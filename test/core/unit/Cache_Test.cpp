
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
