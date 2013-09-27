
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

#ifndef __UTIL_LOCKPROTECTEDMAP_H_
#define __UTIL_LOCKPROTECTEDMAP_H_

#include "ReadWriteMutex.h"

#include <map>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

/*
 * This class encapsulates a lock and a map which is protected by that lock.
 * All queries to the map must pass through the API provided by this class, therefore lock is used when accessing the map.
 */
template <class KEY, class VALUE> class LockProtectedMap {
private:
	map<KEY, VALUE> data;
    ReadWriteMutex  *rwMutexForData;

public:

    LockProtectedMap(){
    	rwMutexForData =  new ReadWriteMutex(100);
    }

    ~LockProtectedMap(){
    	delete rwMutexForData;
    }

    void setValue(const KEY & key, const VALUE & value){
    	rwMutexForData->lockWrite();
    	data[key] = value;
    	rwMutexForData->unlockWrite();
    }

    bool getValue(const KEY & key, VALUE & value) const{
    	rwMutexForData->lockRead();
    	typename std::map<KEY, VALUE>::const_iterator mapIter = this->data.find(key);
    	if(mapIter == this->data.end()){
    		rwMutexForData->unlockRead();
    		return false;
    	}
    	value = mapIter->second;
    	rwMutexForData->unlockRead();
    	return true;
    }

    void erase(const KEY & key){
    	rwMutexForData->lockWrite();
    	data.erase(key);
    	rwMutexForData->unlockWrite();
    }

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(
            Archive & ar,
            const unsigned int file_version
    ){
    	ar & data;
    }

};

}
}

#endif // __UTIL_LOCKPROTECTEDMAP_H_
