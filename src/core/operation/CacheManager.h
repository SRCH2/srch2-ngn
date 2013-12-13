// $Id: Cache.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
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
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>

using namespace std;

namespace srch2
{
namespace instantsearch
{

template <class T>
class CacheEntry{
public:
	CacheEntry(string key, boost::shared_ptr<T> objectPointer){
		this->setKey(key);
		this->setObjectPointer(objectPointer);
	}

	void setKey(string key);
	string getKey();

	void setObjectPointer(boost::shared_ptr<T> objectPointer);
	boost::shared_ptr<T> getObjectPointer();
private:
	string key;
	boost::shared_ptr<T> objectPointer;
};

/*
 * this class is the main implementation of cache which contains the cache entries and
 * get/set behaviors.
 */
template <class T>
class CacheContainer{

public:
	CacheContainer(unsigned long byteSizeOfCache = 134217728, unsigned noOfCacheEntries = 20000);
	~CacheContainer(){};

	void put(string key, boost::shared_ptr<T> objectPointer);
	bool get(string key, boost::shared_ptr<T> & objectPointer);

private:
	map<unsigned , CacheEntry<T> * > cacheEntries;

};


/*
 * This class is the cache manager. The CacheManager is the holder of different kinds of Cache, e.g.
 * ActiveNodesCache.
 */
class CacheManager : public GlobalCache
{
public:
    CacheManager(unsigned long byteSizeOfCache = 134217728, unsigned noOfCacheEntries = 20000);
    virtual ~CacheManager();

};



}
}

#endif /* __CACHE_H__ */
