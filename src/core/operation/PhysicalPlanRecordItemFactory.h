// $Id: PhysicalPlanRecordItemFactory.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
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

#ifndef __PHYSICALPLANRECORDITEMFACTORY_H__
#define __PHYSICALPLANRECORDITEMFACTORY_H__

#define INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP 10000

#include "util/Assert.h"

#include <map>
#include <boost/unordered_set.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

/*
 * This class is the 'tuple' in this iterator model.
 * When the physical plan is being executed, the pointers to
 * PhysicalPlanRecordItem objects are passed around.
 * The object allocation of this class is only performed through
 * PhysicalPlanRecordItemFactory which can open a new PhysicalPlanRecordItemPool.
 *
 * Since tuple objects are created for each and every "record" visited in query processing,
 * it's expensive and keep allocating and destroying these objects. So we use a factory/pool
 * architecture to save this cost. The pointer to a tuple object is retrieved from a pool and
 * physical operators use this pointer to work with a tuple. When a tuple is not needed by an operator
 * (e.g. record is not verified by F.I.) the pointer is simply discarded because operators don't have to
 * worry about tuple memory deallocation since it's done by the pool.
 */
class PhysicalPlanRecordItem{
public:
	// getters
	inline unsigned getRecordId() const {
		return this->recordId;
	}
	inline float getRecordStaticScore() const{
		return this->recordStaticScore;
	}
	inline float getRecordRuntimeScore() const{
		return this->recordRuntimeScore;
	}
	inline void getRecordMatchingPrefixes(vector<TrieNodePointer> & matchingPrefixes) const{
		matchingPrefixes.insert(matchingPrefixes.end(),this->matchingPrefixes.begin(),this->matchingPrefixes.end());
	}
	inline void getRecordMatchEditDistances(vector<unsigned> & editDistances) const{
		editDistances.insert(editDistances.end(),this->editDistances.begin(),this->editDistances.end());
	}
	inline void getRecordMatchAttributeBitmaps(vector<vector<unsigned> > & attributeIdsList) const{
		for (unsigned i = 0; i < this->attributeIdsList.size(); ++i) {
			attributeIdsList.push_back(vector<unsigned>());
			attributeIdsList.back().assign(this->attributeIdsList[i].begin(), this->attributeIdsList[i].end());
		}
	}
	inline void getPositionIndexOffsets(vector<unsigned> & positionIndexOffsets)const {
		positionIndexOffsets.insert(positionIndexOffsets.end(),this->positionIndexOffsets.begin(),this->positionIndexOffsets.end());
	}
	inline void getTermTypes(vector<TermType> & rTermTypes) const {
		rTermTypes.insert(rTermTypes.end(),this->termTypes.begin(),this->termTypes.end());
	}
	inline bool getIsGeo(){
		return this->geoFlag;
	}
	vector<TermType> & getTermTypesRef(){
		return termTypes;
	}

	// setters
	inline void setRecordId(unsigned id) {
		this->recordId = id;
	}
	inline void setRecordStaticScore(float staticScore) {
		this->recordStaticScore = staticScore;
	}
	inline void setRecordRuntimeScore(float runtimeScore) {
		this->recordRuntimeScore = runtimeScore;
	}
	inline void setRecordMatchingPrefixes(const vector<TrieNodePointer> & matchingPrefixes) {
		this->matchingPrefixes = matchingPrefixes;
	}
	inline void setRecordMatchEditDistances(const vector<unsigned> & editDistances) {
		this->editDistances = editDistances;
	}
	inline void setRecordMatchAttributeBitmaps(const vector<vector<unsigned> > & attributeIdsList) {
		this->attributeIdsList = attributeIdsList;
	}
	inline void setPositionIndexOffsets(const vector<unsigned> & positionIndexOffsets){
		this->positionIndexOffsets = positionIndexOffsets;
	}
	inline void setTermTypes(const vector<TermType> & rTermType){
		this->termTypes = rTermType;
	}
	inline void setIsGeo(bool isGeoFlag){
		this->geoFlag = isGeoFlag;
	}
	inline void addTermType(const TermType & rTermType){
		this->termTypes.push_back(rTermType);
	}
    unsigned getNumberOfBytes(){
    	unsigned totalNumberOfBytes = sizeof(PhysicalPlanRecordItem);

    	//matchingPrefixes
    	totalNumberOfBytes += matchingPrefixes.capacity() * sizeof(TrieNodePointer);
    	// no need to loop over matching prefixes because TrieNodes are not considered in cache byte usage

    	// editDistance
    	totalNumberOfBytes += editDistances.capacity() * sizeof(unsigned);
    	// no need to loop over vector

    	// attributeBitmaps
    	for (unsigned i = 0; i < attributeIdsList.size(); ++i)
    		totalNumberOfBytes += attributeIdsList[i].capacity() * sizeof(unsigned);
    	totalNumberOfBytes += attributeIdsList.capacity() * sizeof(void *);

    	// positionIndexOffsets
    	totalNumberOfBytes += positionIndexOffsets.capacity() * sizeof(unsigned);
    	// no need to loop over vector

    	// term types
    	totalNumberOfBytes += termTypes.capacity() * sizeof(TermType);
    	// no need to loop over vector

    	// valuesOfParticipatingRefiningAttributes
    	for(std::map<std::string,TypedValue>::iterator mapItr = valuesOfParticipatingRefiningAttributes.begin();
    			mapItr != valuesOfParticipatingRefiningAttributes.end(); ++mapItr){
    		totalNumberOfBytes += sizeof(string) + mapItr->first.capacity() + mapItr->second.getNumberOfBytes();
    		// we assume the overhead of map is 32 bytes per entry
    		totalNumberOfBytes += 32;
    	}

    	return totalNumberOfBytes;
    }

    void clear(){
    	valuesOfParticipatingRefiningAttributes.clear();
    	matchingPrefixes.clear();
    	editDistances.clear();
    	attributeIdsList.clear();
    	positionIndexOffsets.clear();
    	termTypes.clear();
    }

    PhysicalPlanRecordItem(){
    	this->geoFlag = false;
    };

	~PhysicalPlanRecordItem(){};

    std::map<std::string,TypedValue> valuesOfParticipatingRefiningAttributes;
private:
    bool geoFlag; // this flag shows that this Item is for a term or a geo element
	unsigned recordId;
	float recordStaticScore;
	float recordRuntimeScore;
	vector<TrieNodePointer> matchingPrefixes;
	vector<unsigned> editDistances;
	vector<vector<unsigned> >attributeIdsList;
	vector<unsigned> positionIndexOffsets;
	vector<TermType> termTypes;
};


/*
 * This class is a pool for PhysicalPlanRecordItem objects
 */
class PhysicalPlanRecordItemPool{
public:
	PhysicalPlanRecordItemPool(){
		/* This code is disabled for now to check the memory effect of record pools
		size = 0;
		*/
	}
	// returns the number of objects created in this pool so far
	unsigned getNumberOfObjects(){
		/* This code is disabled for now to check the memory effect of record pools
		return extraObjects.size();
		*/
		return recordItemObjects.size();
	}
	PhysicalPlanRecordItem * createRecordItem(){
		/* This code is disabled for now to check the memory effect of record pools
		if(size >= INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP){
			if(size - INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP >= extraObjects.size()){
				PhysicalPlanRecordItem  * newObj = new PhysicalPlanRecordItem();
				extraObjects.push_back(newObj);
				size++;
				return newObj;
			}else{
				PhysicalPlanRecordItem * toReturn = extraObjects.at(size - INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP);
				toReturn->clear();
				size ++;
				return toReturn;
			}
		}else{
			PhysicalPlanRecordItem * toReturn = &(objects[size]);
			toReturn->clear();
			size ++;
			return toReturn;
		}
		*/
		PhysicalPlanRecordItem * newTuple = new PhysicalPlanRecordItem();
		recordItemObjects.push_back(newTuple);
		return newTuple;
	}
	// if we get a pointer from this function, we are responsible of
	// deallocating it
	PhysicalPlanRecordItem * cloneForCache(PhysicalPlanRecordItem * oldObj){
		PhysicalPlanRecordItem  * newObj = new PhysicalPlanRecordItem();
		newObj->setRecordId(oldObj->getRecordId());
		newObj->setRecordRuntimeScore(oldObj->getRecordRuntimeScore());
		vector<TrieNodePointer> matchingPrefixes;
		oldObj->getRecordMatchingPrefixes(matchingPrefixes);
		newObj->setRecordMatchingPrefixes(matchingPrefixes);
		vector<unsigned> editDistances;
		oldObj->getRecordMatchEditDistances(editDistances);
		newObj->setRecordMatchEditDistances(editDistances);
		vector<vector<unsigned> > attributeIdsList;
		oldObj->getRecordMatchAttributeBitmaps(attributeIdsList);
		newObj->setRecordMatchAttributeBitmaps(attributeIdsList);
		vector<unsigned> positionIndexOffsets;
		oldObj->getPositionIndexOffsets(positionIndexOffsets);
		newObj->setPositionIndexOffsets(positionIndexOffsets);
		oldObj->getTermTypes(newObj->getTermTypesRef());

		return newObj;
	}

	/*
	 * This factory will take care of deallocation of these pointers.
	 */
	PhysicalPlanRecordItem * clone(PhysicalPlanRecordItem * oldObj){
		PhysicalPlanRecordItem  * newObj = createRecordItem();
		newObj->setRecordId(oldObj->getRecordId());
		newObj->setRecordRuntimeScore(oldObj->getRecordRuntimeScore());
		vector<TrieNodePointer> matchingPrefixes;
		oldObj->getRecordMatchingPrefixes(matchingPrefixes);
		newObj->setRecordMatchingPrefixes(matchingPrefixes);
		vector<unsigned> editDistances;
		oldObj->getRecordMatchEditDistances(editDistances);
		newObj->setRecordMatchEditDistances(editDistances);
		vector<vector<unsigned> > attributeIdsList;
		oldObj->getRecordMatchAttributeBitmaps(attributeIdsList);
		newObj->setRecordMatchAttributeBitmaps(attributeIdsList);
		vector<unsigned> positionIndexOffsets;
		oldObj->getPositionIndexOffsets(positionIndexOffsets);
		newObj->setPositionIndexOffsets(positionIndexOffsets);
		oldObj->getTermTypes(newObj->getTermTypesRef());
		return newObj;
	}

	~PhysicalPlanRecordItemPool(){
		clear();
	}

	void clear(){
		/* This code is disabled for now to check the memory effect of record pools
		vector<PhysicalPlanRecordItem *> emptyVector;
		if(extraObjects.size() > 0){
			for(unsigned i =0 ; i< extraObjects.size() ; ++i){
				if(extraObjects.at(i) == NULL){
					ASSERT(false);
				}else{
					delete extraObjects.at(i);
				}
			}
			extraObjects.clear();
			// Clear does not free internal memory of vector. swap internal memory with empty
			// vector. empty vector will free the memory when it goes out of scope.
			extraObjects.swap(emptyVector);
		}
		*/
		refresh();
	}

	/*
	 * Refresh prepares this pool for another fresh query. Since we
	 * don't want to delete any objects at this point (it's expensive) we only set the size to
	 * zero.
	 */
	void refresh(){
		/* This code is disabled for now to check the memory effect of record pools
		size = 0;
		*/
		for(int r = 0 ; r < recordItemObjects.size() ; ++r){
			delete recordItemObjects.at(r);
		}
		recordItemObjects.clear();
	}
private:

	/*
	 * Each pool has 10000 tuples created in the beginning. If a reader
	 * keeps asking for more tuples (more than INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP),
	 * we start allocating new tuples and save them in extraObjects vector.
	 */
	/* This code is disabled for now to check the memory effect of record pools
	PhysicalPlanRecordItem objects[INITIAL_NUMBER_OF_RECORD_ITEMS_IN_A_GROUP];
	vector<PhysicalPlanRecordItem *> extraObjects;
	unsigned size;
	*/

	vector<PhysicalPlanRecordItem *> recordItemObjects;
};

class PhysicalPlanRecordItemFactory{
public:

	PhysicalPlanRecordItemFactory(){
		handleCounter = 0;
	}
	~PhysicalPlanRecordItemFactory(){
		// lock
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// remove inactive pools
		for(boost::unordered_set<PhysicalPlanRecordItemPool *>::iterator idlePoolItr =
				idlePools.begin(); idlePoolItr != idlePools.end(); ++idlePoolItr){
			delete *idlePoolItr;
		}
		// remove active pools
		// NOTE: The instance of PhysicalPlanRecordItemFactory is kept in Cache so it will be destroyed
		// when the whole system is shutting down. So at that point it's safe to delete even active pools
		for(map<unsigned , PhysicalPlanRecordItemPool *>::iterator busyPoolItr = busyPools.begin();
				busyPoolItr != busyPools.end() ; ++busyPoolItr){
			delete busyPoolItr->second;
		}
		// unlock
		lock.unlock();
	}

	bool clear(){
		// lock
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// clear inactive pools (removes extraObjects from them)
		for(boost::unordered_set<PhysicalPlanRecordItemPool *>::iterator idlePoolItr =
				idlePools.begin(); idlePoolItr != idlePools.end(); ++idlePoolItr){
			(*idlePoolItr)->clear();
		}
		// unlock
		lock.unlock();
		return true;
	}

	unsigned openRecordItemPool(){

		// lock
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// prepare the handle = number of pools we have so far
		unsigned newHandleForNewBusyPool = handleCounter ++;

		// check to see if we have any inactive pool
		PhysicalPlanRecordItemPool * newBusyPool = NULL;
		if(idlePools.size() > 0){
			// Choose the pool which has the most number of objects created
			// we want to reduce the chance of needing more objects by giving out
			// big pools first
			boost::unordered_set<PhysicalPlanRecordItemPool *>::iterator poolToReturn = idlePools.end();
			for(boost::unordered_set<PhysicalPlanRecordItemPool *>::iterator poolItr = idlePools.begin();
					poolItr != idlePools.end() ; ++ poolItr){
				if(poolToReturn == idlePools.end()){
					poolToReturn = poolItr;
				}else{
					if((*poolItr)->getNumberOfObjects() > (*poolToReturn)->getNumberOfObjects()){
						poolToReturn = poolItr;
					}
				}
			}

			// remove pool from inactive pools
			newBusyPool =  *(poolToReturn);
			idlePools.erase(poolToReturn);
		}else{ // we don't have inactive pools, so we should make a new one
			// create a new pool
			newBusyPool = new PhysicalPlanRecordItemPool();
		}
		// add (newHandle,pool) to active pools
		busyPools[newHandleForNewBusyPool] = newBusyPool;
		// unlock
		lock.unlock();
		// return handle
		return newHandleForNewBusyPool;
	}
	PhysicalPlanRecordItemPool * getRecordItemPool(unsigned handle){
		// lock
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// get pool from map by using handle
		map<unsigned , PhysicalPlanRecordItemPool *>::iterator poolToReturnItr = busyPools.find(handle);
		if(poolToReturnItr == busyPools.end()){
			// unlock
			lock.unlock();
			return NULL;
		}else{
			PhysicalPlanRecordItemPool * poolToReturn = poolToReturnItr->second;
			// unlock
			lock.unlock();
			// return pool
			return poolToReturn;
		}
	}
	void closeRecordItemPool(unsigned handle){
		//lock
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// find the pool
		map<unsigned , PhysicalPlanRecordItemPool *>::iterator poolItr = busyPools.find(handle);
		ASSERT(poolItr != busyPools.end());
		PhysicalPlanRecordItemPool * poolToClose = poolItr->second;

		// erase pool from active pools
		busyPools.erase(poolItr);
		// refresh this pool for later use
		poolToClose->refresh();
		// insert pool in inactive pools
		idlePools.insert(poolToClose);
		//unlock
		lock.unlock();
	}
private:
	// lock
	mutable boost::shared_mutex _access;

	/*
	 * Factory contains two types of pools : busy and idle.
	 * Busies are those that are opened but not closed yet. These pools are
	 * never deallocated because maybe a reader is using them. Whenever a busy
	 * pool is closed, we return it back to idle pools.
	 * Idle pools are those that are created upon the request of an
	 * old query but they are closed and not opened anymore. When a new
	 * query comes, we first check to see if we have any idle pools.
	 * if we do we open this pool and move it to busy ones, otherwise we create a new pool.
	 */

	// map from handles to pool objects
	map<unsigned , PhysicalPlanRecordItemPool *> busyPools;

	// set which keeps all pools
	boost::unordered_set<PhysicalPlanRecordItemPool *> idlePools;

	// handle value
	unsigned handleCounter;
};

}
}

#endif // __PHYSICALPLANRECORDITEMFACTORY_H__


