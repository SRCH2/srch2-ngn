//$Id: ts_shared_ptr.h 3441 2013-06-11 20:30:12Z sinakeshtkar $

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

#ifndef __TS_SHARED_PTR_H__
#define __TS_SHARED_PTR_H__

#include "../mypthread.h"

#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

namespace bimaple
{
namespace instantsearch
{

template <class T>
class ts_shared_ptr {
private:
    mutable pthread_spinlock_t m_spinlock;
    boost::shared_ptr<T> m_ptr;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->m_ptr;
    }

public:
    ts_shared_ptr()
    {
        pthread_spin_init(&m_spinlock, 0);
    }

    ~ts_shared_ptr()
    {
        pthread_spin_destroy(&m_spinlock);
    }

    void reset(T *optr)
    {
        pthread_spin_lock(&m_spinlock);
        m_ptr.reset(optr);
        pthread_spin_unlock(&m_spinlock);
    }

    ts_shared_ptr<T>& operator =(const boost::shared_ptr<T>& optr)
    {
        pthread_spin_lock(&m_spinlock);
        m_ptr = optr;
        pthread_spin_unlock(&m_spinlock);
        return *this;
    }

    ts_shared_ptr<T>& operator =(const ts_shared_ptr<T>& optr)
    {
        boost::shared_ptr<T> p;
        optr.get(p);
        pthread_spin_lock(&m_spinlock);
        m_ptr = p;
        pthread_spin_unlock(&m_spinlock);
        return *this;
    }

    void get(boost::shared_ptr<T>& optr) const
    {
        pthread_spin_lock(&m_spinlock);
        optr = m_ptr;
        pthread_spin_unlock(&m_spinlock);
    }

    T *get() const
    {
        T* tmp;
        pthread_spin_lock(&m_spinlock);
        tmp = m_ptr.get();
        pthread_spin_unlock(&m_spinlock);
        return tmp;
    }

    T* operator *() const
    {
        T* tmp;
        pthread_spin_lock(&m_spinlock);
        tmp =  m_ptr.get();
        pthread_spin_unlock(&m_spinlock);
        return tmp;
    }

    T *operator ->() const
    {
        T* tmp;
        pthread_spin_lock(&m_spinlock);
        tmp = m_ptr.get();
        pthread_spin_unlock(&m_spinlock);
        return tmp;
    }
};

}}
#endif /* __TS_SHARED_PTR_H__ */
