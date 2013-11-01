
// $Id: Cache.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "Cache.h"
#include "util/Assert.h"
#include "util/Logger.h"

#include <string>
#include <map>
#include <stddef.h>

#include <iostream>
#include <sstream>

using std::vector;
using std::map;

namespace srch2
{
namespace instantsearch
{
/**
 * Given a string, hashes it to a unsigned integer using djb2 hash function.
 */
unsigned Cache::_hash( const std::vector<Term *> *queryTerms, unsigned end )
{
    std::stringstream query;

    for ( unsigned iter = 0; iter < end; iter++)
    {
        const Term *term = queryTerms->at(iter);
        string *str = term->getKeyword();
        query << *str << " $" << term->getThreshold() << " "; // To differentiate between exact and fuzzy terms.
    }
    //std::cout << query.str() << "|";
    //std::cout<<"hash("<<query.str().c_str()<<")=" << _hashDJB2(query.str().c_str()) << std::endl;
    return _hashDJB2(query.str().c_str());
}


unsigned Cache::_hashDJB2(const char *str)
{
    unsigned hash = 5381;
    unsigned c;
    do
    {
        c = *str;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }while(*str++);
    return hash;
}

bool Cache::_vectorOfTermPointerEqualsComparator( const vector<Term*> *leftVector, const vector<Term*> *rightVector , unsigned iter)
{
    // CHENLI: DEBUG
    return false; // always returns false so that no cache hit

    for ( unsigned i = 0 ; i < iter; i++ )
    {
        Term *leftTerm = leftVector->at(i);
        Term *rightTerm = rightVector->at(i);
        if ( !_termPointerComparator(leftTerm, rightTerm) )
        {
            return false;
        }
    }
    return true;
}

bool Cache::_termPointerComparator(const Term *leftTerm, const Term *rightTerm)
{
    if ( leftTerm->getAttributeToFilterTermHits() == rightTerm->getAttributeToFilterTermHits()
            && leftTerm->getBoost() == rightTerm->getBoost()
            && leftTerm->getTermType() == rightTerm->getTermType()
            && leftTerm->getSimilarityBoost() == rightTerm->getSimilarityBoost()
            && leftTerm->getThreshold() == rightTerm->getThreshold()
    )
    {
        string *leftString = rightTerm->getKeyword();
        string *rightString = leftTerm->getKeyword();

        //CHENLI: DEBUG
        //std::cout << "Comparing " << *leftString << " and " << *rightString << std::endl;

        if ((*leftString).compare(*rightString) == 0)
        {
            std::stringstream ss;
            ss << "T2|" << *leftString << "|" << *rightString << "|" <<  leftTerm->getAttributeToFilterTermHits() << "|" << rightTerm->getAttributeToFilterTermHits()
                << "|" << leftTerm->getBoost() << "|" << rightTerm->getBoost()
                << "|" << leftTerm->getTermType() << "|" << rightTerm->getTermType()
                << "|" << leftTerm->getSimilarityBoost() << "|" << rightTerm->getSimilarityBoost()
                << "|" <<  leftTerm->getThreshold() << "|" << rightTerm->getThreshold() << std::endl;
            Logger::debug(ss.str().c_str());
            return true;
        }
    }
    return false;
}

ConjunctionCache::ConjunctionCache(unsigned long byteSizeOfCache, unsigned noOfCacheEntries)
{
    ConjunctionCacheItem conjunctionCacheItem;

    this->conjunctionCacheArm = 0;
    this->maxBytesOfConjuntionCache = (unsigned) (( (float)byteSizeOfCache)/2);
    this->currentBytesOfConjuntionCache = 0;
    this->noOfConjuntionCacheItems = (unsigned) (( (float)noOfCacheEntries)/2);
    //std::cout << this->noOfConjuntionCacheItems << "|" << conjunctionCacheItem.noOfBytes << std::endl;
    this->conjunctionCacheDirectory.assign(this->noOfConjuntionCacheItems, conjunctionCacheItem);
}

ActiveNodeCache::ActiveNodeCache(unsigned long byteSizeOfCache, unsigned noOfCacheEntries)
{
    PrefixActiveNodeSetCacheItem prefixActiveNodeSetCacheItem;

    this->prefixActiveNodeCacheArm = 0;
    this->maxBytesOfActiveNodeCache = (unsigned) (( (float)byteSizeOfCache)/2);
    this->currentBytesOfActiveNodeCache = 0;
    this->noOfActiveNodeCacheItems = (unsigned) (( (float)noOfCacheEntries)/2);
    this->cachedActiveNodeSetDirectory.assign(this->noOfActiveNodeCacheItems, prefixActiveNodeSetCacheItem);
}

//TODO: Remove caching for 0 hits
Cache::Cache(unsigned long byteSizeOfCache, unsigned noOfCacheEntries)
{
    pthread_mutex_init(&mutex_ConjunctionCache, 0);
    pthread_mutex_init(&mutex_ActiveNodeCache, 0);

    this->aCache = new ActiveNodeCache(byteSizeOfCache, noOfCacheEntries);
    this->cCache = new ConjunctionCache(byteSizeOfCache, noOfCacheEntries);
}

Cache::~Cache()
{
    this->clear();

    pthread_mutex_lock(&mutex_ActiveNodeCache);
    delete this->aCache;
    pthread_mutex_unlock(&mutex_ActiveNodeCache);

    pthread_mutex_lock(&mutex_ConjunctionCache);
    delete this->cCache;
    pthread_mutex_unlock(&mutex_ConjunctionCache);

    pthread_mutex_destroy(&mutex_ActiveNodeCache);
    pthread_mutex_destroy(&mutex_ConjunctionCache);
}

ConjunctionCache::~ConjunctionCache()
{

}

ActiveNodeCache::~ActiveNodeCache()
{

}

void ConjunctionCache::clear()
{
    this->cachedConjunctionResultsMap.clear();
    this->conjunctionCacheDirectory.clear();

    ConjunctionCacheItem conjunctionCacheItem;

    this->conjunctionCacheDirectory.assign(this->noOfConjuntionCacheItems , conjunctionCacheItem);
    this->conjunctionCacheArm = 0;
    //this->maxBytesOfConjuntionCache = byteSizeOfCache;
    this->currentBytesOfConjuntionCache = 0;
    //this->noOfConjuntionCacheItems = noOfCacheEntries;
}

void ActiveNodeCache::clear()
{
    this->cachedActiveNodeSetMap.clear();
    this->cachedActiveNodeSetDirectory.clear();

    PrefixActiveNodeSetCacheItem prefixActiveNodeSetCacheItem;

    this->cachedActiveNodeSetDirectory.assign(this->noOfActiveNodeCacheItems, prefixActiveNodeSetCacheItem);

    this->prefixActiveNodeCacheArm = 0;
    //this->maxBytesOfActiveNodeCache = byteSizeOfCache;
    this->currentBytesOfActiveNodeCache = 0;
    //this->noOfActiveNodeCacheItems = noOfCacheEntries;
}

int Cache::clear()
{
    pthread_mutex_lock(&mutex_ConjunctionCache);
    this->cCache->clear();
    pthread_mutex_unlock(&mutex_ConjunctionCache);

    pthread_mutex_lock(&mutex_ActiveNodeCache);
    this->aCache->clear();
    pthread_mutex_unlock(&mutex_ActiveNodeCache);

    return 1;
}

//PrefixActiveNodeSet *Cache::getPrefixActiveNodeSet(string &prefix)
PrefixActiveNodeSet *ActiveNodeCache::_getPrefixActiveNodeSet(string &prefix, unsigned termThreshold)
{
	// Since exact search is always before fuzzy search, if exact and fuzzy session must use the same cache entries,
	// exact always makes the cache entry busy so fuzzy cannot use it.
	// So, we want to differentiate the cache entries, so we append "0" (exact) or "1" (fuzzy) to the end of prefix before hashing it.
    std::string exactOrFuzzy =  termThreshold == 0?"0":"1";
    unsigned hashedQuery = Cache::_hashDJB2((prefix+exactOrFuzzy).c_str());
    map<unsigned, unsigned>::iterator mapIterator = this->cachedActiveNodeSetMap.find(hashedQuery);
    if (mapIterator != this->cachedActiveNodeSetMap.end()
            ///Boundary checks
            && mapIterator->second < this->cachedActiveNodeSetDirectory.size()
            && this->cachedActiveNodeSetDirectory.at(mapIterator->second).hashedQuery == hashedQuery
            && this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet != NULL
            && this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet->busyBit != NULL
            && this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet->busyBit->isFree()
            /// the cached active node set shouldn't have a smaller threshold
            && this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet->getEditDistanceThreshold() >= termThreshold
            /// Final string comparison between input prefix and prefix in prefixActiveNodeSetCacheItem ///TODO OPT by storing prefix string inside PrefixActiveNodeSet
            && (prefix.compare(getUtf8String(*this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet->getPrefix()).c_str()) == 0) )
    {
        this->cachedActiveNodeSetDirectory.at(mapIterator->second).notRecent = 0;
        //cout << "\t--ActiveNode Cache hit!" << endl;
        this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet->busyBit->setBusy();
        return this->cachedActiveNodeSetDirectory.at(mapIterator->second).prefixActiveNodeSet;
    }
    return NULL;
}

int Cache::findLongestPrefixActiveNodes(Term *term, PrefixActiveNodeSet* &in)
{
    in = NULL;
    {
    pthread_mutex_lock(&mutex_ActiveNodeCache);
    //return -1;
    /*if (pthread_mutex_trylock(&mutex_ActiveNodeCache) != 0)
    {
        return -1;
    }
    else
    {*/
        // find the longest prefix with active nodes in the cache
        unsigned termThreshold = term->getThreshold();
        string *keyword = term->getKeyword();
        for (int i = keyword->size(); i >= 2; i --)
        {
            // create a new prefix. TODO (OPTIMIZATION): avoid multiple string construction
            string prefix = keyword->substr(0, i);
            //std::cout << "|find:" << prefix << "|";
            PrefixActiveNodeSet *prefixActiveNodeSet = this->aCache->_getPrefixActiveNodeSet(prefix, termThreshold);
            // found one only if the term threshold is within the cached threshold
            if (prefixActiveNodeSet != NULL)
            {
                //prefixActiveNodeSet->busyBit->setBusy();
                in = prefixActiveNodeSet;
                pthread_mutex_unlock(&mutex_ActiveNodeCache);
                return 1;
            }
        }
        pthread_mutex_unlock(&mutex_ActiveNodeCache);
    }
    // no prefix has a cached PrefixActiveNodeSet
    return 0;
}



void ActiveNodeCache::_prefixActiveNodeCacheMapCleanUp()
{
    for ( map<unsigned,unsigned>::iterator mapIterator = this->cachedActiveNodeSetMap.begin();
            mapIterator != this->cachedActiveNodeSetMap.end();
            mapIterator++)
    {
        if ( mapIterator->second < this->cachedActiveNodeSetDirectory.size()
                && mapIterator->first != this->cachedActiveNodeSetDirectory.at(mapIterator->second).hashedQuery)
        {
            this->cachedActiveNodeSetMap.erase(mapIterator);
        }
    }
}

void ConjunctionCache::_conjunctionCacheMapCleanUp()
{
    for ( map<unsigned,unsigned>::iterator mapIterator = this->cachedConjunctionResultsMap.begin();
            mapIterator != this->cachedConjunctionResultsMap.end();
            )
    {
        if ( mapIterator->second < this->conjunctionCacheDirectory.size()
                && mapIterator->first != this->conjunctionCacheDirectory.at(mapIterator->second).hashedQuery)
        {
            Logger::debug("cachedConjunctionResultsMap.erase");
            // We increment the iterator before we call erase.
            this->cachedConjunctionResultsMap.erase(mapIterator++);
        } else {
            ++mapIterator;
        }
    }
}


int Cache::setPrefixActiveNodeSet(PrefixActiveNodeSet* &prefixActiveNodeSet)
{
    {
    pthread_mutex_lock(&mutex_ActiveNodeCache);
    /*if (pthread_mutex_trylock(&mutex_ActiveNodeCache) != 0)
    {
        return -1;
    }
    else
    {*/
        ASSERT(prefixActiveNodeSet != NULL);
        vector<CharType> *prefix = prefixActiveNodeSet->getPrefix();

    	// Since exact search is always before fuzzy search, if exact and fuzzy session must use the same cache entries,
    	// exact always makes the cache entry busy so fuzzy cannot use it.
    	// So, we want to differentiate the cache entries, so we append "0" (exact) or "1" (fuzzy) to the end of prefix before hashing it.
        std::string exactOrFuzzy =  prefixActiveNodeSet->getEditDistanceThreshold() == 0?"0":"1";
        unsigned hashedQuery = _hashDJB2((getUtf8String(*prefix)+exactOrFuzzy).c_str());
        map<unsigned, unsigned>::iterator mapIterator = this->aCache->cachedActiveNodeSetMap.find(hashedQuery);

        ///Check If hashKey is present in Map
        if (mapIterator == this->aCache->cachedActiveNodeSetMap.end())
        {
            ///Case 1: there is no hashKey and we have find a place to cache
            /// Iterate over the clock(i.e cacheDirectory) until you find an entry with notRecent = 1
            PrefixActiveNodeSetCacheItem* aCacheItemAtArm = this->aCache->getPrefixActiveNodeSetCacheItemFromDirectory(this->aCache->prefixActiveNodeCacheArm);

            while ( aCacheItemAtArm->prefixActiveNodeSet != NULL && aCacheItemAtArm->notRecent == 0 && aCacheItemAtArm->prefixActiveNodeSet->busyBit->isBusy() )
            {
                aCacheItemAtArm->notRecent = 1;
                this->aCache->prefixActiveNodeCacheArm = (this->aCache->prefixActiveNodeCacheArm + 1) % this->aCache->cachedActiveNodeSetDirectory.size();
                aCacheItemAtArm = this->aCache->getPrefixActiveNodeSetCacheItemFromDirectory(this->aCache->prefixActiveNodeCacheArm);
            }

            /// Delete the "notRecent = 1" entry
            if ( aCacheItemAtArm->prefixActiveNodeSet != NULL && aCacheItemAtArm->prefixActiveNodeSet->busyBit->isFree())
            {
                delete aCacheItemAtArm->prefixActiveNodeSet;
                aCacheItemAtArm->notRecent = 1;
                aCacheItemAtArm->hashedQuery = 0;
                aCacheItemAtArm->noOfBytes = 0;
                aCacheItemAtArm->prefixActiveNodeSet = NULL;
            }
            this->aCache->currentBytesOfActiveNodeCache -= aCacheItemAtArm->noOfBytes;

            unsigned cacheDataItemSize = prefixActiveNodeSet->getNumberOfBytes();
            if (aCacheItemAtArm->prefixActiveNodeSet == NULL
                    && (cacheDataItemSize < (this->aCache->maxBytesOfActiveNodeCache - this->aCache->currentBytesOfActiveNodeCache) ))
            {
                ///Check whether the new cacheEntry can be put into cache
                ///Cache can accommodate the new entry. Assign the new entry, set notRecent = 0 and move the clock arm
                //PrefixActiveNodeSetCacheItem* aCacheItemAtArm = this->aCache->getPrefixActiveNodeSetCacheItemFromDirectory(this->aCache->prefixActiveNodeCacheArm);
                aCacheItemAtArm->notRecent = 0;
                aCacheItemAtArm->hashedQuery = hashedQuery;
                aCacheItemAtArm->noOfBytes = cacheDataItemSize;

                //prefixActiveNodeSet->setBusyBit(0);
                aCacheItemAtArm->prefixActiveNodeSet = prefixActiveNodeSet;

                this->aCache->currentBytesOfActiveNodeCache += aCacheItemAtArm->noOfBytes;

                this->aCache->cachedActiveNodeSetMap[hashedQuery] = this->aCache->prefixActiveNodeCacheArm;
                this->aCache->prefixActiveNodeCacheArm = (this->aCache->prefixActiveNodeCacheArm + 1) % this->aCache->cachedActiveNodeSetDirectory.size();
                prefixActiveNodeSet->setResultsCached(true);
            }
            else ///Cache cannot accommodate the new entry. Delete the new entry and set the not recent =1.
            {
                prefixActiveNodeSet->setResultsCached(false);
            }
        }
        else
        {
            ///Case: Where the hashKey was found in cacheMap
            unsigned activeNodeCacheDirectoryPosition = mapIterator->second;
            PrefixActiveNodeSetCacheItem* aCacheItemHit = this->aCache->getPrefixActiveNodeSetCacheItemFromDirectory(activeNodeCacheDirectoryPosition);
            ///Check whether query hashKey and the hashKey in the cacheDirectory match
            if ( !(aCacheItemHit->hashedQuery == hashedQuery
                    && aCacheItemHit->prefixActiveNodeSet->getEditDistanceThreshold() > prefixActiveNodeSet->getEditDistanceThreshold()) )
            {
                /// If they don't match, update the cacheEntry pointed by "hashKey in cacheMap" with the new cacheEntry
                if (aCacheItemHit->prefixActiveNodeSet != NULL && aCacheItemHit->prefixActiveNodeSet->busyBit->isFree())
                {
                    delete aCacheItemHit->prefixActiveNodeSet;
                    aCacheItemHit->notRecent = 1;
                    aCacheItemHit->hashedQuery = 0;
                    aCacheItemHit->noOfBytes = 0;
                    aCacheItemHit->prefixActiveNodeSet = NULL;
                }
                this->aCache->currentBytesOfActiveNodeCache -= aCacheItemHit->noOfBytes;

                unsigned cacheDataItemSize = prefixActiveNodeSet->getNumberOfBytes();
                if (aCacheItemHit->prefixActiveNodeSet == NULL
                        && (cacheDataItemSize < (this->aCache->maxBytesOfActiveNodeCache - this->aCache->currentBytesOfActiveNodeCache) ))
                {
                    /// Check if the new entry can be accommodated in the cache
                    aCacheItemHit->notRecent = 0;
                    aCacheItemHit->hashedQuery = hashedQuery;
                    aCacheItemHit->noOfBytes = cacheDataItemSize;
                    //prefixActiveNodeSet->setBusyBit(0);
                    prefixActiveNodeSet->setResultsCached(true);
                    aCacheItemHit->prefixActiveNodeSet = prefixActiveNodeSet;
                    this->aCache->currentBytesOfActiveNodeCache += aCacheItemHit->noOfBytes;
                }
                else
                {
                    prefixActiveNodeSet->setResultsCached(false);
                }
            }
            else // case where the object is already cached
            {
                prefixActiveNodeSet->setResultsCached(false);
                ///case where the hashKeys match and so the data is already in cache. So we delete the cacheData given by user.
                ///Cache protocol is to report cacheSet failure, by setting "prefixActiveNodeSet->setResultsCached(false)" and pass the responsibility
                ///to delete the cacheEntry to user.
            }
        }

        ///PrefixActiveNodeCacheMap cleanUp
        /// TODO: Add an explanation
        if (this->aCache->cachedActiveNodeSetMap.size() > this->aCache->noOfActiveNodeCacheItems * 2)
        {
            this->aCache->_prefixActiveNodeCacheMapCleanUp();
        }
        pthread_mutex_unlock(&mutex_ActiveNodeCache);
    }
    return 1;
}

/// TODO: Move the comment to header file. This setter can fail
/// Test the cache_integration_test

int Cache::setCachedConjunctionResult(const vector<Term *> *queryTerms, ConjunctionCacheResultsEntry* conjunctionCacheResultsEntry)
{
    if (pthread_mutex_trylock(&mutex_ConjunctionCache) != 0)
    {
        return -1;
    }
    else
    {
        conjunctionCacheResultsEntry->busyBit->setBusy();
        //std::cout<<"|Set cCache:";

        unsigned hashedQuery = this->_hash(queryTerms, queryTerms->size());
        map<unsigned, unsigned>::iterator mapIterator = this->cCache->cachedConjunctionResultsMap.find(hashedQuery);

        ///Check If hashKey is present in Map
        if (mapIterator == this->cCache->cachedConjunctionResultsMap.end())
        {
            ///Case 1: there is no hashKey and we have find a place to cache

            /// Iterate over the clock(i.e cacheDirectory) until you find an entry with notRecent = 1
            ConjunctionCacheItem* cCacheItemAtArm = this->cCache->getConjunctionCacheItemFromDirectory(this->cCache->conjunctionCacheArm);
            while ( cCacheItemAtArm->cacheData != NULL && cCacheItemAtArm->notRecent == 0 && cCacheItemAtArm->cacheData->busyBit->isBusy() )
            {
                cCacheItemAtArm->notRecent = 1;
                this->cCache->conjunctionCacheArm = (this->cCache->conjunctionCacheArm + 1) % this->cCache->conjunctionCacheDirectory.size();
                cCacheItemAtArm = this->cCache->getConjunctionCacheItemFromDirectory(this->cCache->conjunctionCacheArm);
            }

            /// Delete the "notRecent = 1" entry
            if (cCacheItemAtArm->cacheData != NULL && cCacheItemAtArm->cacheData->busyBit->isFree())
            {
                delete cCacheItemAtArm->cacheData;
                cCacheItemAtArm->notRecent = 1;
                cCacheItemAtArm->hashedQuery = 0;
                cCacheItemAtArm->noOfBytes = 0;
                cCacheItemAtArm->cacheData = NULL;
            }
            this->cCache->currentBytesOfConjuntionCache -= this->cCache->conjunctionCacheDirectory.at(this->cCache->conjunctionCacheArm).noOfBytes;

            ///Check whether the new cacheEntry can be put into cache
            unsigned cacheDataItemSize = conjunctionCacheResultsEntry->getNumberOfBytes();
            if (cCacheItemAtArm->cacheData == NULL
                    && cacheDataItemSize < (this->cCache->maxBytesOfConjuntionCache - this->cCache->currentBytesOfConjuntionCache) )
            {
                ///Cache can accomodate the new entry. Assign the new entry, set notRecent = 0 and move the clock arm
                cCacheItemAtArm->notRecent = 0;
                cCacheItemAtArm->hashedQuery = hashedQuery;
                cCacheItemAtArm->noOfBytes = cacheDataItemSize;
                this->cCache->currentBytesOfConjuntionCache += cCacheItemAtArm->noOfBytes;
                cCacheItemAtArm->cacheData = conjunctionCacheResultsEntry;
                cCacheItemAtArm->cacheData->isCached = 1;
                conjunctionCacheResultsEntry->busyBit->setFree();

                this->cCache->cachedConjunctionResultsMap[hashedQuery] = this->cCache->conjunctionCacheArm;
                this->cCache->conjunctionCacheArm = (this->cCache->conjunctionCacheArm + 1) % this->cCache->conjunctionCacheDirectory.size();
            }
            else
            {
                ///Cache cannot accommodate the new entry. Delete the new entry and set the not recent =1.
                delete conjunctionCacheResultsEntry;
            }
        }
        else
        {
            ///case: Where the hashKey was found in cacheMap
            unsigned conjunctionCacheDirectoryPosition = mapIterator->second;
            ConjunctionCacheItem* cCacheItemHit = this->cCache->getConjunctionCacheItemFromDirectory(conjunctionCacheDirectoryPosition);

            ///Check whether query hashKey and the hashKey in the cacheDirectory match
            if ( cCacheItemHit->hashedQuery != hashedQuery)
            {
                /// If they don't match, update the cacheEntry pointed by "hashKey in cacheMap" with the new cacheEntry
                if (cCacheItemHit->cacheData != NULL && cCacheItemHit->cacheData->busyBit->isFree())
                {
                    delete cCacheItemHit->cacheData;
                    cCacheItemHit->notRecent = 1;
                    cCacheItemHit->hashedQuery = 0;
                    cCacheItemHit->noOfBytes = 0;
                    cCacheItemHit->cacheData = NULL;
                }
                this->cCache->currentBytesOfConjuntionCache -= cCacheItemHit->noOfBytes;

                /// Check if the new entry can be accommodated in the cache
                unsigned cacheDataItemSize = conjunctionCacheResultsEntry->getNumberOfBytes();
                if (cCacheItemHit->cacheData == NULL
                        && cacheDataItemSize < (this->cCache->maxBytesOfConjuntionCache - this->cCache->currentBytesOfConjuntionCache) )
                {
                    cCacheItemHit->notRecent = 0;
                    cCacheItemHit->hashedQuery = hashedQuery;
                    cCacheItemHit->noOfBytes = cacheDataItemSize;
                    cCacheItemHit->cacheData = conjunctionCacheResultsEntry;
                    cCacheItemHit->cacheData->isCached = 1;
                    conjunctionCacheResultsEntry->busyBit->setFree();

                    this->cCache->currentBytesOfConjuntionCache += cCacheItemHit->noOfBytes;
                }
                else
                {
                    delete conjunctionCacheResultsEntry;
                }
            }
            else
            {
                ///case where the hashKeys match and so the data is already in cache. So we delete the cacheData given by user.
                ///TODO: Change cache protocol to report cacheSet failure and pass the responsiblity to delete the cacheEntry to user.
                delete conjunctionCacheResultsEntry;
            }
        }

        ///ConjunctionCacheMap cleanUp
        if (this->cCache->cachedConjunctionResultsMap.size() > this->cCache->noOfConjuntionCacheItems*2)
        {
            this->cCache->_conjunctionCacheMapCleanUp();
        }

        //std::cout << std::endl;
        pthread_mutex_unlock(&mutex_ConjunctionCache);
    }

    return 1;
}

int Cache::getCachedConjunctionResult(const std::vector<Term *> *queryTerms, ConjunctionCacheResultsEntry* &conjunctionCacheResultsEntry)
{
    conjunctionCacheResultsEntry = NULL;
    if (pthread_mutex_trylock(&mutex_ConjunctionCache) != 0)
    {
        return -1;
    }
    else
    {
        ///Try for query cache hits. For example, for a query "A B C", try for cache hits, " A B C", "A B", "A".
        for ( unsigned iter = queryTerms->size(); iter > 0; iter--)
        {
            //std::cout<< "|Get cCache:";
            unsigned hashedQuery = _hash(queryTerms, iter);
            map<unsigned, unsigned>::iterator mapIterator = this->cCache->cachedConjunctionResultsMap.find(hashedQuery);
            if (mapIterator != this->cCache->cachedConjunctionResultsMap.end()
                    /// Check if it matches to one in cacheDirectory
                    && hashedQuery == this->cCache->conjunctionCacheDirectory.at(mapIterator->second).hashedQuery
                    /// Boundary check
                    && mapIterator->second < this->cCache->conjunctionCacheDirectory.size()
                    /// Make sure the cacheData pointed by the cacheDirectory is not NULL
                    && this->cCache->conjunctionCacheDirectory.at(mapIterator->second).cacheData != NULL
                    && this->cCache->conjunctionCacheDirectory.at(mapIterator->second).cacheData->busyBit->isFree()
                    /// Check if the query parameters like boost, similarity, etc match
                    && this->_vectorOfTermPointerEqualsComparator(queryTerms, this->cCache->conjunctionCacheDirectory.at(mapIterator->second).cacheData->queryTerms,iter) )
            {
                /// Check if it matches to one in cacheDirectory
                this->cCache->conjunctionCacheDirectory.at(mapIterator->second).notRecent = 0;
                this->cCache->conjunctionCacheDirectory.at(mapIterator->second).cacheData->busyBit->setBusy();
                conjunctionCacheResultsEntry = this->cCache->conjunctionCacheDirectory.at(mapIterator->second).cacheData;
                //std::cout << "Found" << std::endl;
                pthread_mutex_unlock(&mutex_ConjunctionCache);
                return 1;
            }
        }
        //std::cout << "NO cache hit" << std::endl;
        pthread_mutex_unlock(&mutex_ConjunctionCache);
    }
    return 0;

}

}}
