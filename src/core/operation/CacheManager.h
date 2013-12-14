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

class Sizable{
public:
	virtual unsigned getSizeInBytes() = 0;

	virtual ~Sizable(){};
};

template <class T>
class CacheEntry{
public:
	CacheEntry::CacheEntry(string key, boost::shared_ptr<T> objectPointer){
		this->setKey(key);
		this->setObjectPointer(objectPointer);
	}

	void setKey(string key);
	string getKey();

	void setObjectPointer(boost::shared_ptr<T> objectPointer){
		this->objectPointer = objectPointer;
	}
	boost::shared_ptr<T> getObjectPointer(){
		return this->objectPointer;
	}

	unsigned getSizeInBytes(){
		return sizeof(this->key) + this->objectPointer->getSizeInBytes();
	}
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

	struct HashedKeyLinkListElement{
		HashedKeyLinkListElement * next;
		unsigned hashedKey;
		HashedKeyLinkListElement(unsigned hashedKey){
			this->hashedKey = hashedKey;
			this->next = NULL;
		}
	};

public:
	CacheContainer(unsigned long byteSizeOfCache = 134217728)
	: BYTE_BUDGET(byteSizeOfCache){
		totalSizeUsed = 0;
		elementsLinkListHead = elementsLinkListTail = NULL;
	}
	~CacheContainer(){};

	bool put(string key, boost::shared_ptr<T> objectPointer){
		CacheEntry<T> * newEntry = new CacheEntry<T>(key , objectPointer);
		unsigned numberOfBytesNeededForNewEntry = getNumberOfBytesUsedByEntry(newEntry);
		if(numberOfBytesNeededForNewEntry > BYTE_BUDGET){
			// we cannot accept this entry, it's bigger than our budget
			delete newEntry;
			return false;
		}
		if(numberOfBytesNeededForNewEntry > BYTE_BUDGET - totalSizeUsed){ // size is bigger than our left budget, we should remove some entries
			// keep removing entries until enough space is left
			while(true){
				LRUCacheReplacementPolicyKickoutOneEntry();
				if(numberOfBytesNeededForNewEntry <= BYTE_BUDGET - totalSizeUsed){
					break;
				}
			}
		}
		// now we can insert this new entry
		// 1. first find the hashedKey
		unsigned newEntryHashedKey = hashDJB2(key.c_str());
		// 2. check to see if this hashKey is in the map
		map<unsigned , CacheEntry<T> * >::iterator cacheEntryWithSameHashedKey = cacheEntries.find(newEntryHashedKey);
		if(cacheEntryWithSameHashedKey != cacheEntries.end()){ // hashed key exists
			// replace the old cache entry with the new one and update the size info
			unsigned sizeOfEntryToRemove = getNumberOfBytesUsedByEntry(cacheEntryWithSameHashedKey->second);
			// update the size
			// it's important that we += first and then -= because it's unsigned
			totalSizeUsed += numberOfBytesNeededForNewEntry;
			ASSERT(totalSizeUsed >= sizeOfEntryToRemove);
			totalSizeUsed -= sizeOfEntryToRemove;
			ASSERT(totalSizeUsed <= BYTE_BUDGET);
			delete cacheEntryWithSameHashedKey->second;
			cacheEntryWithSameHashedKey->second = newEntry;
			// no need to touch linked list although it might make this new entry kicked out earlier,
			// but for simplicity we just return here
			return true;
		}
		// 3. if it's not in map push it to the map, append hashedKey to the linkedlist and update the size
		cacheEntries[newEntryHashedKey] = newEntry;
		addNewElementToLinkList(newEntryHashedKey);
		totalSizeUsed += numberOfBytesNeededForNewEntry;
		ASSERT(totalSizeUsed <= BYTE_BUDGET);


		return true;
	}
	bool get(string key, boost::shared_ptr<T> & objectPointer) const{

		//1. compute the hashed key
		unsigned hashedKeyToFind = hashDJB2(key.c_str());
		map<unsigned , CacheEntry<T> * >::iterator cacheEntry = cacheEntries.find(hashedKeyToFind);
		if(cacheEntry == cacheEntries.end()){ // hashed key doesn't exist
			return false;
		}
		objectPointer = cacheEntry->second->getObjectPointer();

		return true;
	}


private:
	map<unsigned , CacheEntry<T> * > cacheEntries;


	// LRU replacement policy always removes elements from the head
	HashedKeyLinkListElement * elementsLinkListHead;
	// LRU replacement policy always requires adding new entries to the tail
	HashedKeyLinkListElement * elementsLinkListTail;

	const unsigned long BYTE_BUDGET;
	unsigned totalSizeUsed;


	void LRUCacheReplacementPolicyKickoutOneEntry(){
		// 1. get the oldest element to kick out
		unsigned hashedKeyToRemove ;
		if(removeTheOldestElementFromLinkList(hashedKeyToRemove) == false){
			return; // cache is empty , these is nothing to remove
		}

		// 2. remove the entry from the map and update the total byte size used
		map<unsigned , CacheEntry<T> * >::iterator entryToRemove = cacheEntries.find(hashedKeyToRemove);
		ASSERT(entryToRemove != cacheEntries.end());
		unsigned numberOfUsedBytesToGetRidOf = getNumberOfBytesUsedByEntry(*entryToRemove);
		cacheEntries.erase(entryToRemove); // remove it from map
		ASSERT(totalSizeUsed >= numberOfUsedBytesToGetRidOf);
		totalSizeUsed -= numberOfUsedBytesToGetRidOf;
		delete *entryToRemove;
	}

	// does not update the size
	void addNewElementToLinkList(unsigned hashedKey){
		ASSERT((elementsLinkListHead == NULL && elementsLinkListTail == NULL) ||
				(elementsLinkListHead != NULL && elementsLinkListTail != NULL) );

		if(elementsLinkListHead == NULL && elementsLinkListTail == NULL){
			elementsLinkListHead = elementsLinkListTail = new HashedKeyLinkListElement(hashedKey);
			return;
		}

		elementsLinkListTail->next = new HashedKeyLinkListElement(hashedKey);
		elementsLinkListTail = elementsLinkListTail->next;
	}

	// does not update the size
	bool removeTheOldestElementFromLinkList(unsigned removedHashedKey){
		ASSERT((elementsLinkListHead == NULL && elementsLinkListTail == NULL) ||
				(elementsLinkListHead != NULL && elementsLinkListTail != NULL) );
		if(elementsLinkListHead == NULL && elementsLinkListTail == NULL){
			return false; // nothing in linked list to remove
		}
		if(elementsLinkListHead == elementsLinkListTail){ // there is only one element
			removedHashedKey = elementsLinkListHead->hashedKey;
			delete elementsLinkListHead;
			elementsLinkListHead = elementsLinkListTail = NULL;
			return true;
		}
		HashedKeyLinkListElement * newHead = elementsLinkListHead->next;
		removedHashedKey = elementsLinkListHead->hashedKey;
		delete elementsLinkListHead;
		elementsLinkListHead = newHead;
		return true;
	}

	unsigned getNumberOfBytesUsedByEntry(CacheEntry<T> * entry){
		/*
		 * entry + unsigned hashedKey + linkedList element
		 */
		return entry->getSizeInBytes() + sizeof(unsigned) + sizeof(HashedKeyLinkListElement);
	}

	// computes the hash value of a string
	unsigned hashDJB2(const char *str) const
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
