// $Id: BusyBit.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __BUSYBIT_H__
#define __BUSYBIT_H__

#include "mypthread.h"

namespace srch2
{
namespace instantsearch
{

class BusyBit {
private:
    mutable pthread_mutex_t m_spinlock;
    bool busyBit;

    void setBusyBit(bool in)
    {
        pthread_mutex_lock(&m_spinlock);
        this->busyBit = in;
        pthread_mutex_unlock(&m_spinlock);
    }

    bool getBusyBit() const
    {
        bool out;
        pthread_mutex_lock(&m_spinlock);
        out = this->busyBit;
        pthread_mutex_unlock(&m_spinlock);
        return out;
    }

public:
    BusyBit()
    {
    	pthread_mutex_init(&m_spinlock, 0);
        this->busyBit = 0;
    }

    ~BusyBit() {
    	pthread_mutex_destroy(&m_spinlock);
    }

    bool isFree() const
    {
        return (getBusyBit() == 0);
    }

    bool isBusy() const
    {
        return (getBusyBit() == 1);
    }

    void setFree()
    {
        setBusyBit(0);
    }

    void setBusy()
    {
        setBusyBit(1);
    }
};

}}

#endif /* __BUSYBIT_H__ */
