// $Id: Cache.h 3294 2013-05-01 03:45:51Z jiaying $
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

#pragma once
#ifndef __CACHE_H__
#define __CACHE_H__

#include <instantsearch/GlobalCache.h>
#include <instantsearch/Term.h>
#include "util/BusyBit.h"
#include "operation/ActiveNode.h"

namespace bimaple
{
namespace instantsearch
{

class PrefixActiveNodeSetCacheItem
{
public:
    PrefixActiveNodeSet* prefixActiveNodeSet;
    unsigned hashedQuery;///TODO: check if hashedQuery is good enough
    bool notRecent;
    unsigned noOfBytes;

    PrefixActiveNodeSetCacheItem()
    {
        this->prefixActiveNodeSet = NULL;
        this->hashedQuery = 0;
        this->notRecent = 1;
        this->noOfBytes = 0; //TODO: implement getNumberOfBytes inside ActiveNode.h
    }

    virtual ~PrefixActiveNodeSetCacheItem()
    {
        if ( prefixActiveNodeSet != NULL)
        {
            prefixActiveNodeSet->setResultsCached(false);
            if (prefixActiveNodeSet->busyBit->isFree() )
            {
                   delete prefixActiveNodeSet;
            }
        }
    }
};

struct CandidateResult
{
    unsigned internalRecordId;
    std::vector<float> termScores;
    std::vector<string> matchingKeywords;
    std::vector<unsigned> editDistances;
};

class ConjunctionCacheResultsEntry
{
public:
    std::vector<Term* > *queryTerms;
    // candidate list
    std::vector<CandidateResult> *candidateList;

    // vector of vector of cursors for each term
    std::vector<std::vector<unsigned>* >* virtualListCursorsVector;

    BusyBit *busyBit;
    bool isCached;

    ConjunctionCacheResultsEntry(std::vector<Term* > *queryTerms, std::vector<CandidateResult> *candidateList, std::vector<std::vector<unsigned>* >* virtualListCursorsVector)
    {
        this->queryTerms = queryTerms;
        this->candidateList =candidateList;
        this->virtualListCursorsVector =virtualListCursorsVector;
        this->busyBit = new BusyBit();
        this->isCached = 0;
    }

    unsigned getNumberOfBytes() const
    {
        unsigned noOfBytes = sizeof(*(this->virtualListCursorsVector));
        if (this->virtualListCursorsVector != NULL)
        {
            for ( std::vector<std::vector<unsigned>* >::const_iterator vectorIter = (this->virtualListCursorsVector)->begin(); vectorIter != (this->virtualListCursorsVector)->end(); vectorIter++ )
            {
                noOfBytes += (*vectorIter)->size() * sizeof((*vectorIter)) + ((*vectorIter)->capacity() - (*vectorIter)->size())*4;
            }
            noOfBytes += this->virtualListCursorsVector->capacity()*4;
        }
        if (this->candidateList != NULL)
        {
            //TODO: Inaccurate size of candidatResult
            noOfBytes += this->candidateList->size() * (sizeof(CandidateResult) +  3*(4*queryTerms->size() + 8) ); //3*(4*queryTerms->size() + 8) = 3*vector*no_of_queryterms
            noOfBytes += sizeof(*(this->candidateList)) + (this->candidateList->capacity() - this->candidateList->size())*4;
        }

        return  noOfBytes + (this->queryTerms->size() * sizeof(Term)) + sizeof(*(this->queryTerms)) + (this->queryTerms->capacity() - this->queryTerms->size())*4;
    }

    virtual ~ConjunctionCacheResultsEntry()
    {
        if ( virtualListCursorsVector != NULL)
        {
            for (unsigned i=0; i<virtualListCursorsVector->size(); ++i)
            {
                delete virtualListCursorsVector->at(i);
            }
            virtualListCursorsVector->clear();
            delete virtualListCursorsVector;
        }
        if ( queryTerms != NULL)
        {
            for (unsigned i=0; i<queryTerms->size(); ++i)
            {
                if (queryTerms->at(i) != NULL)
                    delete queryTerms->at(i);
            }
            delete queryTerms;
        }
        delete candidateList;

        delete busyBit;
    }

};

class ConjunctionCacheItem{
public:
    unsigned hashedQuery;
    bool notRecent;
    unsigned noOfBytes;
    ConjunctionCacheResultsEntry* cacheData;

    ConjunctionCacheItem()
    {
        this->cacheData = NULL;
        this->hashedQuery = 0;
        this->notRecent = 1;
        this->noOfBytes = 0;
    }

    virtual ~ConjunctionCacheItem()
    {
        if ( cacheData != NULL)
        {
            if ( cacheData->busyBit->isFree() )
            {
                delete cacheData;
            }
            else
            {
                this->cacheData->isCached = 0;
            }
        }
    }
};

class ActiveNodeCache
{
public:
    ActiveNodeCache(unsigned long byteSizeOfCache, unsigned noOfCacheEntries);
    virtual ~ActiveNodeCache();
    void clear();

    void _prefixActiveNodeCacheMapCleanUp();
    PrefixActiveNodeSet *_getPrefixActiveNodeSet(std::string &prefix, unsigned termThreshold);

    unsigned prefixActiveNodeCacheArm;

    inline PrefixActiveNodeSetCacheItem* getPrefixActiveNodeSetCacheItemFromDirectory(unsigned index)
    {
        return &this->cachedActiveNodeSetDirectory.at(index);
    }

//private:
    /// This map maps a prefix to its cached activeNodeSet
    std::map<unsigned, unsigned> cachedActiveNodeSetMap;
    std::vector<PrefixActiveNodeSetCacheItem> cachedActiveNodeSetDirectory;

    unsigned long maxBytesOfActiveNodeCache;
    unsigned long currentBytesOfActiveNodeCache;
    unsigned noOfActiveNodeCacheItems;
};

class ConjunctionCache
{
public:
    ConjunctionCache(unsigned long byteSizeOfCache, unsigned noOfCacheEntries);
    virtual ~ConjunctionCache();
    void clear();

    void _conjunctionCacheMapCleanUp();

    inline ConjunctionCacheItem* getConjunctionCacheItemFromDirectory(unsigned index)
    {
        return &this->conjunctionCacheDirectory.at(index);
    }

    unsigned conjunctionCacheArm;

//private:
    /// This map maps a query to its cached results
    std::map<unsigned, unsigned> cachedConjunctionResultsMap;
    std::vector<ConjunctionCacheItem> conjunctionCacheDirectory;

    unsigned long maxBytesOfConjuntionCache;
    unsigned long currentBytesOfConjuntionCache;
    unsigned noOfConjuntionCacheItems;


};

/**
 * There are two types of cache:
 * 1. Active node cache
 *         Can be imagined as a map from prefix to its corresponding ActiveNodeSet
 * 2. Query results cache
 *         Can be imagined as a map from vector<Term*> to its corresponding ConjunctionQueryResults
 *
 * The cache replacement is based on LRU and implemented using Clock Algorithm. The cache is bounded by two user variables:
 *     1. NoOfBytesAllowed
 *  2. NoOfCacheItems
 *
 *  For both ActiveNodeCache and QueryResultsCache, two functions "set" and "get" are provided.
 *
 *  Set(cacheItem) function is best try cache.
 *  It the cacheItem cannot be accommodated in the cache due to the cache bounds(NoOfBytesAllowed or NoOfCacheItems), the set
 *  function just deletes the cacheItem. This makes the "cacheItem" very unsafe to be used after calling set(cacheItem) and so
 *  set(cacheItem) must be the last one to be called before exiting indexSearcherInternal.
 *
 *  get function has the usual functionality of returns a reference to the cacheItem if found, otherwise returns a NULL.
 *
 */
class Cache : public GlobalCache
{
public:
    Cache(unsigned long byteSizeOfCache = 134217728, unsigned noOfCacheEntries = 20000);
    virtual ~Cache();

    /// Find the PrefixActiveNodeSet with the longest prefix of the keyword in the prefix
    /// If no prefix has a cached result, return NULL.
    int findLongestPrefixActiveNodes(Term *term, PrefixActiveNodeSet* &in);

    /// set the PrefixActiveNodeSet for a prefix stored in the given PrefixActiveNodeSet
    int setPrefixActiveNodeSet(PrefixActiveNodeSet* &prefixActiveNodeSet);

    /// start with all the terms, if there is no cached results take out the last term and lookup for the new query terms vector.
    int getCachedConjunctionResult(const std::vector<Term *> *queryTerms, ConjunctionCacheResultsEntry* &conjunctionCacheResultsEntry);

    int setCachedConjunctionResult(const std::vector<Term *> *queryTerms, ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry);

    int clear(); // Called whenever index is updated

    static unsigned _hash(const std::vector<Term *> *queryTerms, unsigned end);
    static unsigned _hashDJB2(const char *str);
    static bool _vectorOfTermPointerEqualsComparator(const std::vector<Term*> *leftVector, const std::vector<Term*> *rightVector , unsigned iter);
    static bool _termPointerComparator( const Term *leftTerm, const Term *rightTerm);

private:

    mutable pthread_mutex_t mutex_ActiveNodeCache;
    ActiveNodeCache *aCache;

    mutable pthread_mutex_t mutex_ConjunctionCache;
    ConjunctionCache *cCache;
};

}}

#endif /* __CACHE_H__ */
