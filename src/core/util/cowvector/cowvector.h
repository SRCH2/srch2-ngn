//$Id: cowvector.h 3410 2013-06-05 12:58:08Z jiaying $

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

#include "ts_shared_ptr.h"

using boost::shared_ptr;

namespace bimaple
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

    vectorview(vectorview<T>* vv)
    : m_size(vv->m_size)
    {
        m_array = vv->m_array;
    }

    vectorview(size_t capacity)
    : m_size(0)
    {
        m_array.reset(new array<T>(capacity));
    }

    ~vectorview() {}

    void push_back(const T& inElement) {
        (*this)[m_size] = inElement;
    }

    void forceCreateCopy()
    {
        shared_ptr<array<T> > a;
        m_array.get(a);
        size_t capacity = a->capacity;
        shared_ptr<array<T> > acopy(new array<T>(capacity));
        for(size_t i = 0; i < m_size; ++i) {
            acopy->extent[i] = a->extent[i];
        }
        m_array = acopy;
    }

    T& at(unsigned i)
    {
        shared_ptr<array<T> > a;
        m_array.get(a);
        size_t capacity = a->capacity;
        if (i >= capacity) {
            while(i >= capacity) {
                capacity *= 2;
                capacity += 1;
            }
            shared_ptr<array<T> > acopy(new array<T>(capacity));
            for(size_t i = 0; i < m_size; ++i) {
                acopy->extent[i] = a->extent[i];
            }
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

    void getArrayForWriteView(ts_shared_ptr<array<T> > &writeView)
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

private:
    ts_shared_ptr<array<T> > m_array;
    size_t m_size;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->m_size;
        for(unsigned i = 0; i < this->m_size; ++i)
            ar << this->getElement(i);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar >> this->m_size;

        shared_ptr<array<T> > acopy(new array<T>(this->m_size));
        for(size_t i = 0; i < m_size; ++i) {
            ar >> acopy->extent[i];
        }
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
    ts_shared_ptr<vectorview<T> > m_readView;
    ts_shared_ptr<vectorview<T> > m_writeView;

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
        m_writeView = m_readView;
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
        m_writeView = m_readView;
    };
    cowvector(size_t capacity) //Constructor
    {
        if (capacity == 0 )
            capacity = 1;
        m_readView.reset(new vectorview<T>(capacity));
        m_writeView = m_readView;
    }

    virtual ~cowvector() {}

    void getReadView(ts_shared_ptr<vectorview<T> >& view) const
    {
        view = m_readView;
    }

    void getWriteView(ts_shared_ptr<vectorview<T> >& view)
    {
        view = m_writeView;
    }

    void commit() //Set readView to writeView and create new a writeView from readViewCopy.
    {
        m_readView = m_writeView;
        m_writeView.reset(new vectorview<T>(*m_readView));
    }
};



}}
#endif /* __COWVECTOR_H__ */
