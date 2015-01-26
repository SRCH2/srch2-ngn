//$Id: cowvector.h 3456 2013-06-14 02:11:13Z jiaying $

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

 * Copyright  2010 SRCH2 Inc. All rights reserved
 */

#ifndef __CORE_UTIL_COWVECTOR_H__
#define __CORE_UTIL_COWVECTOR_H__

#include <boost/serialization/vector.hpp>
#include <boost/serialization/vector.hpp>
#include <cstring>
#include "util/Assert.h"
#include "../mypthread.h"
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <vector>
using boost::shared_ptr;

namespace srch2
{
namespace instantsearch
{

template <class T>
struct array {
public:
    T *extent;
    size_t capacity;

    array(size_t c)
    : extent(new T[c]),
      capacity(c) {    }
    ~array()
    {
        delete [] extent;
    }
};

template <class T>
class vectorview {
public:
    //vectorview() {};

    vectorview(vectorview<T>& vv)
    {
        m_array = vv.m_array;
        this->setSize(vv.size());
        this->setWriteView();
        this->setNeedToFreeArray(false);
    }

    vectorview(size_t capacity = 1)
    {
        m_array =new array<T>(capacity);
        this->setSize(0);
        this->setWriteView();
        this->setNeedToFreeArray(false);
    }

    ~vectorview()
    {
        // only the last readview can release the array
        if(this->isReadView() && this->getNeedToFreeArray())
            delete m_array;
    }

    bool getNeedToFreeArray()
    {
        return m_sizeAndFlags & 1 << 30;
    }

    void setNeedToFreeArray(bool flag)
    {
        if(flag)
            m_sizeAndFlags |= 1 << 30;
        else
            m_sizeAndFlags &= ~(1 << 30);
    }

    void setWriteView()
    {
        m_sizeAndFlags |= 1 << 31;
    }

    void setReadView()
    {
        m_sizeAndFlags &= ~(1 << 31);
    }

    bool isReadView()
    {
        return !isWriteView();
    }

    bool isWriteView()
    {
        return m_sizeAndFlags & (1 << 31);
    }

    void push_back(const T& inElement) {
        //only writeview can push_back
        assert(this->isWriteView());
        (*this)[this->size()] = inElement;
    }

    void forceCreateCopy()
    {
        size_t capacity = m_array->capacity;
        array<T>* acopy = new array<T>(capacity);
        memcpy(acopy->extent, m_array->extent, this->size()*sizeof(T));
        if(this->getNeedToFreeArray() == false)
            this->setNeedToFreeArray(true);
        else
            delete m_array;
        m_array = acopy;
    }

    T& at(unsigned i)
    {
        size_t capacity = m_array->capacity;
        // readview can only access the element [0, m_size-1] in the vectorview, writeview don't have such restrict
        assert(this->isWriteView() || i < this->size());
        if (i >= capacity) {
            while(i >= capacity) {
                capacity *= 2;
                capacity += 1;
            }
            array<T>* acopy = new array<T>(capacity);
            memcpy(acopy->extent, m_array->extent, this->size()*sizeof(T));
            // If the flag is false, then this array could be used by readers, so we don't free the space. We set the flag to "true" so that next time we reallocate the space, we need to release the space.
            if(this->getNeedToFreeArray() == false) {
                this->setNeedToFreeArray(true);
            }
            else {
            	// the else block is executed when the write view has multiple reallocations before the merge.
                delete m_array;
            }
            m_array = acopy;
        }
        if (i >= this->size()) {
            this->setSize(i + 1);
        }
        return m_array->extent[i];
    }

    T& operator [](unsigned i)
    {
    	return this->at(i);
    }

    const T& getElement(unsigned i) const
    {
        return m_array->extent[i];
    }

    void clear()
    {
    	this->setSize(0);
    }

    size_t size() const { return m_sizeAndFlags & 0x3FFFFFFF; }

    array<T>* getArray() const {return m_array;}

    void setSize(size_t size){m_sizeAndFlags &= 0xC0000000; m_sizeAndFlags |= (size & 0x3FFFFFFF);}

private:
    array<T>* m_array;
    // We use the 31^th bit to represent whether it's a write view (bit = 1) or a read view (bit = 0).
    // We use the 30^th bit to represent the boolean flag "needToFreeOldArray" indicating whether we need to
    // free the old array when we need to reallocate the array.
    // We use the remaining 30 bits to represent the size of the array, i.e., number of valid elements in the array (not the capacity).
    size_t m_sizeAndFlags;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        size_t size = this->size();
        ar << size;
        ar << boost::serialization::make_array(this->m_array->extent, this->size());
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        size_t size;
        ar >> size;
        this->setSize(size);

        array<T>* acopy = new array<T>(this->size());
        ar >> boost::serialization::make_array(acopy->extent, this->size());
        if (m_array)
           delete m_array;
        m_array = acopy;
    }

    template<class Archive>
    void serialize(
            Archive & ar,
            const unsigned int file_version
    ){
        boost::serialization::split_member(ar, *this, file_version);
    }

};

template <class T>
class ReadViewManager {
private:
	// readViewGenerations vector keeps all generations of read_views .
	//
	//  Example:
	//  gen 0 : RV1, RV*
	//  gen 1 : RV2, RV3, RV4, RV*
	//  gen 2 : RV5, RV6, RV*
	//
	//  - Every group in a generation shares the same allocated array
	//  - Those RVs in a generation have distinct read view size.
	//  - RV* is a special read_view which can free the allocated array. Other RV's cannot free the array.
	//  - All RVs except RV* are stored in this array if they had one or more than one reader thread when the merge was executed.

	std::vector< std::vector<shared_ptr<vectorview<T> > > > readViewGenerations;

	// counter to keep track of current generation. The counter is incremented every time
	// the write_view's array gets reallocated to increase its capacity.
	unsigned currentGen;
public:
	ReadViewManager() : currentGen(0) {
		readViewGenerations.push_back(std::vector<shared_ptr<vectorview<T> > >());
	}

	// increment generation and allocate vector for read_views of new generation.
	void incrementGeneration() {
		++currentGen;
		readViewGenerations.push_back(std::vector<shared_ptr<vectorview<T> > >());
	}

	// keep alive the read_view passed in by storing it in a vector.
	void storeReadView(const shared_ptr<vectorview<T> >& newReadView) {
		readViewGenerations[currentGen].push_back(newReadView);
	}

	// clean the readviews from the past generations and free the underlying array whenever possible.
	void cleanReadViews() {

		// loop over all the RV generations skipping the last one because it is the most recent one.
		for (signed i = 0; i < readViewGenerations.size() - 1; ++i) {
			std::vector<shared_ptr<vectorview<T> > >& readViewGroups = readViewGenerations[i];
			bool hasReader = false;
			// Go over all the read_views of the "i" generation and check whether any readers exist.
			for (unsigned j = 0; j < readViewGroups.size(); ++j) {
				if (!readViewGroups[j].unique()) {
					hasReader = true;
					break;
				}
			}
			if (hasReader == false) {
				// Clear the vector. It will call the destructor of each stored read_view but
				// only one read_view has the capability to free the underlying array which is
				// set via setNeedToFreeArray() in merge().
				readViewGroups.clear();
			}
		}
	}

};

template <class T>
class cowvector {

private:
    // We use a shared pointer for the read view so that multiple readers can share the space and free the space when the last reader leaves.
    shared_ptr<vectorview<T> > m_readView;
    vectorview<T>* m_writeView;
    mutable pthread_spinlock_t m_spinlock;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << *this->m_readView;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        if(m_readView.get() != m_writeView)
            delete m_writeView;
        ar >> *this->m_readView;
        m_readView->setReadView();
        m_readView->setNeedToFreeArray(true);
        m_writeView = new vectorview<T>(*m_readView);
        m_writeView->setNeedToFreeArray(false);
    }

    template<class Archive>
    void serialize(
            Archive & ar,
            const unsigned int file_version
    ){
        boost::serialization::split_member(ar, *this, file_version);
    }

public:
    cowvector(size_t capacity = 1023) //Constructor
    {
        m_writeView = new vectorview<T>(capacity);
        m_readView.reset(m_writeView);
        pthread_spin_init(&m_spinlock, 0);
    }

    virtual ~cowvector()
    {
        if(m_readView.get() != m_writeView)
            delete m_writeView;
        m_readView->setNeedToFreeArray(true);
        pthread_spin_destroy(&m_spinlock);
    }

    void getReadView(shared_ptr<vectorview<T> >& view) const
    {
        // We need the lock it to prevent the following two operations from happening at the same time.
        // One reader is doing reader = readview, which is reading the readview.
        // At the same time, we can call merge(), in which we can have "readview=writeview", which is modifying the read view.
        pthread_spin_lock(&m_spinlock);
        view = m_readView;
        pthread_spin_unlock(&m_spinlock);
    }

    vectorview<T>*&  getWriteView()
    {
        return m_writeView;
    }

    void commit()
    {
        // change the viewType to be readview
        m_readView->setReadView();
        m_writeView = new vectorview<T>(*m_readView);
    }

    void merge(ReadViewManager<T>* readViewManager = NULL)
    {
        pthread_spin_lock(&m_spinlock);
        // After the commit, the two views are different.
        ASSERT(m_readView.get() != m_writeView);

        if(m_readView->getArray() == m_writeView->getArray()){
        	// no re-allocation occurred during the insertions since last merge.
            m_readView->setNeedToFreeArray(false);
            // check whether the current readview has any readers.
            if (readViewManager && !m_readView.unique()){
            	// One or more than one reader threads exist.
            	readViewManager->storeReadView(m_readView);
            }
        }else{
        	// re-allocation occurred during the insertions since last merge
            m_readView->setNeedToFreeArray(true);
            // This current readview can delete the underlying array of vectorview.
            // but we must delay the deletion until all the other previous readviews
            // of this generation do not have any reader.
            if (readViewManager) {
            	readViewManager->storeReadView(m_readView);
            	readViewManager->incrementGeneration();
            }
        }

        // periodically check to clean read_view and free vector view array.
        if (readViewManager) {
        	readViewManager->cleanReadViews();
        }

        // reset the read_view and let it point to the write_view
        m_readView.reset(m_writeView);

        // change the viewType to be read_view
        m_readView->setReadView();
        // We can safely release the lock now, since the only chance the read view can be modified is during merge().
        // But merge() can only happen when another writer comes in, and we assume at any time only one writer can come in.
        // So this case cannot happen.
        pthread_spin_unlock(&m_spinlock);

        m_writeView = new vectorview<T>(*m_readView);
    }
};



}}
#endif /* __CORE_UTIL_COWVECTOR_H__ */
