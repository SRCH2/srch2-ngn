
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

#ifndef __UTIL_THREADSAFEMAP_H__
#define __UTIL_THREADSAFEMAP_H__

#include "ReadWriteMutex.h"

#include <map>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

/*
 * This class encapsulates a lock and a map which is protected by that lock.
 * All queries to the map must pass through the API provided by this class, therefore lock is used when accessing the map.
 * Note : we require at most 100 Readers/Writer at the same time.
 */
template <class KEY, class VALUE> class ThreadSafeMap {
private:
    map<KEY, VALUE> data;
    mutable boost::shared_mutex rwMutexForData;

public:

    ThreadSafeMap(){
    }

    ~ThreadSafeMap(){
    }

    void setValue(const KEY & key, const VALUE & value){
    	boost::unique_lock< boost::shared_mutex > lock(rwMutexForData);
    	data[key] = value;
    }

    bool getValue(const KEY & key, VALUE & value) const{
    	boost::shared_lock< boost::shared_mutex > lock(rwMutexForData);
    	typename std::map<KEY, VALUE>::const_iterator mapIter = this->data.find(key);
    	if(mapIter == this->data.end()){
    		return false;
    	}
    	value = mapIter->second;
    	return true;
    }

    void erase(const KEY & key){
    	boost::unique_lock< boost::shared_mutex > lock(rwMutexForData);
    	data.erase(key);
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

#endif // __UTIL_THREADSAFEMAP_H__
