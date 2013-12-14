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

#ifndef __CACHEBASE_H__
#define __CACHEBASE_H__

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
	CacheEntry::CacheEntry(string key, boost::shared_ptr<T> objectPointer){
		this->setKey(key);
		this->setObjectPointer(objectPointer);
	}

	void setKey(string key){
		this->key = key;
	}
	string getKey(){
		return key;
	}

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
		HashedKeyLinkListElement * previous;
		unsigned hashedKey;
		HashedKeyLinkListElement(unsigned hashedKey){
			this->hashedKey = hashedKey;
			this->next = NULL;
			this->previous = NULL;
		}
	};

public:
	CacheContainer(unsigned long byteSizeOfCache = 134217728)
	: BYTE_BUDGET(byteSizeOfCache){
		totalSizeUsed = 0;
		elementsLinkListFirst = elementsLinkListLast = NULL;
	}
	~CacheContainer(){
		HashedKeyLinkListElement * nextToDelete = elementsLinkListFirst;
		while(elementsLinkListFirst != NULL){
			HashedKeyLinkListElement * nextOfNextToDelete = nextToDelete->next;
			delete nextToDelete;
			nextToDelete = nextOfNextToDelete;
		}
		cacheEntries.clear();
	};

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
		map<unsigned , pair< CacheEntry<T> * , HashedKeyLinkListElement * > >::iterator cacheEntryWithSameHashedKey = cacheEntries.find(newEntryHashedKey);
		if(cacheEntryWithSameHashedKey != cacheEntries.end()){ // hashed key exists
			// replace the old cache entry with the new one and update the size info
			unsigned sizeOfEntryToRemove = getNumberOfBytesUsedByEntry(cacheEntryWithSameHashedKey->second.first);
			// update the size
			// it's important that we += first and then -= because it's unsigned
			totalSizeUsed += numberOfBytesNeededForNewEntry;
			ASSERT(totalSizeUsed >= sizeOfEntryToRemove);
			totalSizeUsed -= sizeOfEntryToRemove;
			ASSERT(totalSizeUsed <= BYTE_BUDGET);
			delete cacheEntryWithSameHashedKey->second.first;
			cacheEntryWithSameHashedKey->second.first = newEntry;
			// move the linked list element to front to make it kicked out later
			moveLinkedListElementToLast(cacheEntryWithSameHashedKey->second.second);
			return true;
		}
		// 3. if it's not in map push it to the map, append hashedKey to the linkedlist and update the size
		addNewElementToLinkList(newEntryHashedKey );
		cacheEntries[newEntryHashedKey] = std::make_pair(  newEntry, elementsLinkListLast );
		totalSizeUsed += numberOfBytesNeededForNewEntry;
		ASSERT(totalSizeUsed <= BYTE_BUDGET);


		return true;
	}
	bool get(string key, boost::shared_ptr<T> & objectPointer) const{

		//1. compute the hashed key
		unsigned hashedKeyToFind = hashDJB2(key.c_str());
		map<unsigned , pair< CacheEntry<T> * , HashedKeyLinkListElement * > >::iterator cacheEntry = cacheEntries.find(hashedKeyToFind);
		if(cacheEntry == cacheEntries.end()){ // hashed key doesn't exist
			return false;
		}
		// cache hit , move the corresponding linked list element to the last position to make it
		// get kicked out laster
		moveLinkedListElementToLast(cacheEntry->second.second);
		// and return the object
		objectPointer = cacheEntry->second.first->getObjectPointer();

		return true;
	}


private:
	map<unsigned , pair< CacheEntry<T> * , HashedKeyLinkListElement * > > cacheEntries;


	// LRU replacement policy always removes elements from the beginning
	HashedKeyLinkListElement * elementsLinkListFirst;
	// LRU replacement policy always requires adding new entries to the tail (after last)
	HashedKeyLinkListElement * elementsLinkListLast;

	const unsigned long BYTE_BUDGET;
	unsigned totalSizeUsed;


	void LRUCacheReplacementPolicyKickoutOneEntry(){
		// 1. get the oldest element to kick out, which is the first element on linked list
		unsigned hashedKeyToRemove ;
		if(removeTheOldestElementFromLinkList(hashedKeyToRemove) == false){
			return; // cache is empty , these is nothing to remove
		}

		// 2. remove the entry from the map and update the total byte size used
		map<unsigned , pair< CacheEntry<T> * , HashedKeyLinkListElement * > >::iterator entryToRemove = cacheEntries.find(hashedKeyToRemove);
		ASSERT(entryToRemove != cacheEntries.end());
		unsigned numberOfUsedBytesToGetRidOf = getNumberOfBytesUsedByEntry(entryToRemove->second.first);
		cacheEntries.erase(entryToRemove); // remove it from map
		ASSERT(totalSizeUsed >= numberOfUsedBytesToGetRidOf);
		totalSizeUsed -= numberOfUsedBytesToGetRidOf;
		delete entryToRemove->second.first;
	}

	// does not update the size
	void addNewElementToLinkList(unsigned hashedKey){
		ASSERT((elementsLinkListFirst == NULL && elementsLinkListLast == NULL) ||
				(elementsLinkListFirst != NULL && elementsLinkListLast != NULL) );

		if(elementsLinkListFirst == NULL && elementsLinkListLast == NULL){
			elementsLinkListFirst = elementsLinkListLast = new HashedKeyLinkListElement(hashedKey);
			return;
		}

		ASSERT(elementsLinkListLast->next == NULL);

		elementsLinkListLast->next = new HashedKeyLinkListElement(hashedKey);
		elementsLinkListLast->next->previous = elementsLinkListLast;
		elementsLinkListLast = elementsLinkListLast->next;
	}

	// does not update the size , just removes the first element of linked list
	bool removeTheOldestElementFromLinkList(unsigned & removedHashedKey){
		ASSERT((elementsLinkListFirst == NULL && elementsLinkListLast == NULL) ||
				(elementsLinkListFirst != NULL && elementsLinkListLast != NULL) );
		if(elementsLinkListFirst == NULL && elementsLinkListLast == NULL){
			return false; // nothing in linked list to remove
		}
		if(elementsLinkListFirst == elementsLinkListLast){ // there is only one element
			removedHashedKey = elementsLinkListFirst->hashedKey;
			delete elementsLinkListFirst;
			elementsLinkListFirst = elementsLinkListLast = NULL;
			return true;
		}
		HashedKeyLinkListElement * newFirst = elementsLinkListFirst->next;
		ASSERT(newFirst->previous == elementsLinkListFirst);
		newFirst->previous = NULL;
		removedHashedKey = elementsLinkListFirst->hashedKey;
		delete elementsLinkListFirst;
		elementsLinkListFirst = newFirst;
		return true;
	}

	void moveLinkedListElementToLast(HashedKeyLinkListElement * element){
		ASSERT(element != NULL);
		ASSERT(! (elementsLinkListFirst == NULL || elementsLinkListLast == NULL));
		if(elementsLinkListFirst == elementsLinkListLast){
			// only one entry, no need to move anything
			return;
		}
		if(element == elementsLinkListLast){ // the element is already on the last position
			ASSERT(element->next == NULL);
			// no need to do anything
			return;
		}
		if(element == elementsLinkListFirst){ // the element is the first element
			ASSERT(element->previous == NULL);
			ASSERT(element->next->previous == element);
			elementsLinkListFirst = element->next;
			elementsLinkListFirst->previous = NULL;
			ASSERT(elementsLinkListLast->next == NULL);
			elementsLinkListLast->next = element;
			element->previous = elementsLinkListLast;
			elementsLinkListLast = element;
			return;
		}
		// element is in the middle
		// 1. connect previous to next and next to previous
		element->previous->next = element->next;
		element->next->previous = element->previous;
		// 2. put element in the front and make it HEAD
		ASSERT(elementsLinkListLast->next == NULL);
		elementsLinkListLast->next = element;
		element->previous = elementsLinkListLast;
		elementsLinkListLast = element;
		return;
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



}
}

#endif // __CACHEBASE_H__
