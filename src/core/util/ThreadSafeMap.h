
#ifndef __UTIL_THREADSAFEMAP_H__
#define __UTIL_THREADSAFEMAP_H__


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
