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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __COWVECTOR_H__
#define __COWVECTOR_H__

#include <boost/serialization/vector.hpp>
#include <boost/serialization/vector.hpp>
#include "ts_shared_ptr.h"

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
    vectorview() {};

    vectorview(vectorview<T>& vv)
    : m_size(vv.m_size)
    {
        m_array = vv.m_array;
        viewType = true;
        needToFreeOldArray = false;
    }

    vectorview(size_t capacity)
    : m_size(0)
    {
        m_array =new array<T>(capacity);
        viewType = true;
        needToFreeOldArray = false;
    }

    ~vectorview()
    {
        // only the last readview release the array
        if(viewType == false)
            delete m_array;
    }

    void setNeedToFreeOldArray(bool flag)
    {
        needToFreeOldArray = flag;
    }

    void setViewType(bool flag)
    {
        viewType = flag;
    }

    void push_back(const T& inElement) {
        //only writeview can push_back
        assert(viewType == true);
        (*this)[m_size] = inElement;
    }

    void forceCreateCopy()
    {
        size_t capacity = m_array->capacity;
        array<T>* acopy = new array<T>(capacity);
        memcpy(acopy->extent, m_array->extent, m_size*sizeof(T));
        if(needToFreeOldArray == false)
            needToFreeOldArray = true;
        else
            delete m_array;
        m_array = acopy;
    }

    T& at(unsigned i)
    {
        size_t capacity = m_array->capacity;
        // readview can only access the element [0, m_size-1] in the vectorview, writeview don't have such restrict
        assert(viewType == true || i < m_size);
        if (i >= capacity) {
            while(i >= capacity) {
                capacity *= 2;
                capacity += 1;
            }
            array<T>* acopy = new array<T>(capacity);
            memcpy(acopy->extent, m_array->extent, m_size*sizeof(T));
            if(needToFreeOldArray == false)
                needToFreeOldArray = true;
            else
                delete m_array;
            m_array = acopy;
        }
        if (i >= m_size) {
            m_size = i + 1;
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

    void getArrayForWriteView(shared_ptr<array<T> > &writeView)
    //array<T>* getArrayForInvertedListSortAndMerge() // DO NOT use in readView.
    {
        writeView = this->m_array;
        //return *this->m_array;
    }

    void clear()
    {
    	m_size = 0;
    }

    size_t size() const { return m_size; }

    array<T>* getArray() const {return m_array;}

    void setSize(size_t size){m_size = size;}

private:
    array<T>* m_array;
    size_t m_size;
    // viewType if it's true it 's a writeview, else it's a readview
    bool viewType;
    // whether we need to free the old array when reallocate more space.
    bool needToFreeOldArray;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->m_size;
        ar << boost::serialization::make_array(this->m_array->extent, this->m_size);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar >> this->m_size;

        array<T>* acopy = new array<T>(this->m_size);
        ar >> boost::serialization::make_array(acopy->extent, this->m_size);
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
class cowvector {

private:
    shared_ptr<vectorview<T> > m_readView;
    vectorview<T>* m_writeView;
    mutable pthread_spinlock_t m_spinlock;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->m_readView;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        /*if (this->m_readView.get() != NULL)
            delete this->m_readView.get();*/
        ar >> this->m_readView;
        m_writeView = m_readView.get();
    }

    template<class Archive>
    void serialize(
            Archive & ar,
            const unsigned int file_version
    ){
        boost::serialization::split_member(ar, *this, file_version);
    }

public:
    // TODO remove the serialization dependancy
    cowvector() {
        m_readView.reset(new vectorview<T>(1));
        m_writeView = m_readView.get();
        m_writeView->setNeedToFreeOldArray(true);
        pthread_spin_init(&m_spinlock, 0);
    };
    cowvector(size_t capacity) //Constructor
    {
        if (capacity == 0 )
            capacity = 1;
        m_readView.reset(new vectorview<T>(capacity));
        m_writeView = m_readView.get();
        m_writeView->setNeedToFreeOldArray(true);
        pthread_spin_init(&m_spinlock, 0);
    }

    virtual ~cowvector()
    {
        if(m_readView.get() != m_writeView)
            delete m_writeView;
        pthread_spin_destroy(&m_spinlock);
    }

    void getReadView(shared_ptr<vectorview<T> >& view) const
    {
        pthread_spin_lock(&m_spinlock);
        view = m_readView;
        pthread_spin_unlock(&m_spinlock);
    }

    vectorview<T>*&  getWriteView()
    {
        return m_writeView;
    }

    void commit() //Set readView to writeView and create new a writeView from readViewCopy.
    {
        pthread_spin_lock(&m_spinlock);
        // if the readview and writeview is pointing the same vectorview(initail state) we only need to new a vecterview for writeview
        if(m_readView.get() != m_writeView)
        {
            // if the vecterview of readview and writeview point to the same array, we just need change the size.
            if(m_readView->getArray() == m_writeView->getArray())
                m_readView->setSize(m_writeView->size());
            else{// otherwise we need to reset the readview and new a new vectorview for the writeview
                m_readView.reset(m_writeView);
                //change the viewType to be readview
                m_readView->setViewType(false);
                m_writeView = new vectorview<T>(*m_readView);
            }
        }
        else
        {
            // change the viewType to be readview
            m_readView->setViewType(false);
            m_writeView = new vectorview<T>(*m_readView);
        }
        pthread_spin_unlock(&m_spinlock);
    }
};



}}
#endif /* __COWVECTOR_H__ */
