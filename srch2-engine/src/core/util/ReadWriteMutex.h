// $Id$

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

#ifndef __READWRITEMUTEX_H__
#define __READWRITEMUTEX_H__

#include <pthread.h>
#include <semaphore.h>

namespace srch2
{
namespace instantsearch
{

class ReadWriteMutex
{
public:
    ReadWriteMutex(int maxReaders = 1000)
    {
        max_readers = maxReaders;
        sem_init(&m_semaphore, 0, max_readers);
        //pthread_spin_init(&m_spinlock, 0);
        pthread_mutex_init(&mutex, 0);
    }
    
    void lockRead()
    {
        sem_wait(&m_semaphore);
    }

    void unlockRead()
    {
        sem_post(&m_semaphore);
    }

    void lockWrite()
    {
        //pthread_spin_lock(&m_spinlock);
        pthread_mutex_lock(&mutex);

        for (int i = 0; i < max_readers; i++)
        {
            sem_wait(&m_semaphore); 
        }
    }

    void unlockWrite()
    {
        for (int i = 0; i < max_readers; i++)
        {
            sem_post(&m_semaphore); 
        }

        //pthread_spin_unlock(&m_spinlock);
        pthread_mutex_unlock(&mutex);
    }

    int cond_timedwait(pthread_cond_t *cond, const struct timespec *ts)
    {
        int rc;

        pthread_mutex_lock(&mutex);

        rc = pthread_cond_timedwait(cond, &mutex, ts);

        for (int i = 0; i < max_readers; i++)
        {
            sem_wait(&m_semaphore); 
        }

        return rc;
    }

    void cond_signal(pthread_cond_t *cond)
    {
        pthread_cond_signal(cond);
    }

    ~ReadWriteMutex()
    {
        sem_destroy(&m_semaphore);
        //pthread_spin_destroy(&m_spinlock);
        pthread_mutex_destroy(&mutex);
    }
    
private:
    int max_readers;
    sem_t m_semaphore;
    //pthread_spinlock_t m_spinlock;
    pthread_mutex_t mutex;
};

}}

#endif /* __READWRITEMUTEX_H__ */
