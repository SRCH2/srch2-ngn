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

#ifndef __CORE_UTIL_READWRITEMUTEX_H__
#define __CORE_UTIL_READWRITEMUTEX_H__

#include "mypthread.h"
#include "util/Logger.h"
#include <semaphore.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <cstdlib>
//fcntl.h and stdio.h are implicitly included in Mac OSX g++ but on Linux it should be added explicitly.
//It is safe for both platforms to include them both here.

#define SEMAPHORE_NAME_LENGTH 20
#define TIME_STAMP_LENGTH 9

namespace srch2
{
namespace instantsearch
{

class ReadWriteMutex
{
public:
	inline ReadWriteMutex(int maxReaders = 1000) {
        max_readers = maxReaders;
        //As we can see in the following link. sem_init is not supported on the mac. So in order to create semaphores we should use sem_open.
        //http://stackoverflow.com/questions/1413785/sem-init-on-os-x
        //Base on the issue discussed in http://stackoverflow.com/questions/8063613/c-macs-os-x-semaphore-h-trouble-with-sem-open-and-sem-wait
        //We put sem_unlick before sem_open to make sure this sem_open creates the semaphore.
        //Since each semaphore should have a unique name, there is a gen_random_name function that creates random name.
        //The name length can not be too long otherwise it gives us the error number 63 which is "File name too long"

        //sem_init(&m_semaphore, 0, max_readers);
        //pthread_spin_init(&m_spinlock, 0);
        gen_random_name(semaphoreName);
        sem_unlink(semaphoreName);
        m_semaphore = sem_open(semaphoreName, O_CREAT,0,max_readers);
        if (m_semaphore == SEM_FAILED) {
            srch2::util::Logger::error("Semaphore creation failed with an error no %d", errno);
        }

        pthread_mutex_init(&mutex, 0);
    }

    inline void lockRead() {
      cout << "^^^^^^^lockRead: before sem_wait()" << endl;
        sem_wait(m_semaphore);
	cout << "^^^^^^^lockRead: after sem_wait()" << endl;
    }

    inline void unlockRead() {
      cout << "#######unlockRead: before sem_wait()" << endl;
        sem_post(m_semaphore);
	cout << "#######unlockRead: after sem_wait()" << endl;
    }

    inline void lockWrite() {
        //pthread_spin_lock(&m_spinlock);
      cout << "++++++++++++++++lockWrite: before pthread_mutex_lock" << endl;
        pthread_mutex_lock(&mutex);

        for (int i = 0; i < max_readers;) {
            if(sem_wait(m_semaphore) == 0) ++i;
        }
	cout << "++++++++++++++++unlockWrite: after sem_wait" << endl;
    }

    inline void unlockWrite() {
      cout << "%%%%%%%unlockWrite: before sem_post()" << endl;
        for (int i = 0; i < max_readers; i++) {
            sem_post(m_semaphore);
        }

        //pthread_spin_unlock(&m_spinlock);
        pthread_mutex_unlock(&mutex);
	cout << "%%%%%%%unlockWrite: after pthread_mutex_unlock()" << endl;
    }

    inline int writeLockWithCondTimedWait(pthread_cond_t *cond, const struct timespec *ts) {
        int rc;

        pthread_mutex_lock(&mutex);

        rc = pthread_cond_timedwait(cond, &mutex, ts);

        for (int i = 0; i < max_readers;) {
            if(sem_wait(m_semaphore) == 0) ++i;
        }

        return rc;
    }

    inline void cond_signal(pthread_cond_t *cond) {
        pthread_cond_signal(cond);
    }

    ~ReadWriteMutex() {

    	/*
    	 *  Semaphore created by sem_open should be closed via sem_close(). sem_unlink removes
    	 *  the semaphore's name from the system. Although we create a unique semaphore name for
    	 *  each ReadWriteMutex object , we should unlink the semaphore for better clean up.
    	 *  Caution: sem_destroy should not called on named semaphore. It should be called only
    	 *  on semaphore created by sem_init(). calling sem_destroy on semaphore created by sem_open
    	 *  has undefined behavior in certain unix implementations. On MacOSX the semaphore is not
    	 *  destroyed.
    	 *  http://pubs.opengroup.org/onlinepubs/007908775/xsh/sem_destroy.html
    	 */
        sem_close(m_semaphore);
        sem_unlink(semaphoreName);
        //pthread_spin_destroy(&m_spinlock);
        pthread_mutex_destroy(&mutex);
    }
private:
    char semaphoreName[SEMAPHORE_NAME_LENGTH+TIME_STAMP_LENGTH];
    int max_readers;
    sem_t* m_semaphore;
    //pthread_spinlock_t m_spinlock;
    pthread_mutex_t mutex;

    inline void gen_random_name(char *s) {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        for (int i = 0; i < SEMAPHORE_NAME_LENGTH; ++i) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        // add the timestamp
        unsigned now = time(0);
        sprintf(semaphoreName +SEMAPHORE_NAME_LENGTH, "%x", now);
    }
};

}
}

#endif /* __CORE_UTIL_READWRITEMUTEX_H__ */
