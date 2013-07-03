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

#include "mypthread.h"
#include <semaphore.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

//fcntl.h and stdio.h are implicitly included in Mac OSX g++ but on Linux it should be added explicitly.
//It is safe for both platforms to include them both here.

#define SEMAPHORE_NAME_LENGTH 20

namespace bimaple
{
namespace instantsearch
{

class ReadWriteMutex
{
public:
	ReadWriteMutex(int maxReaders = 1000)
{
		max_readers = maxReaders;
		//As we can see in the following link. sem_init is not supported on the mac. So in order to create semaphores we should use sem_open.
		//http://stackoverflow.com/questions/1413785/sem-init-on-os-x
		//Base on the issue discussed in http://stackoverflow.com/questions/8063613/c-macs-os-x-semaphore-h-trouble-with-sem-open-and-sem-wait
		//I put sem_unlick before sem_open to make sure this sem_open creates the semaphore.
		//Since each semaphore should have a unique name, there is a gen_random_name function that creates random name.
		//The name length can not be too long otherwise it gives us the error number 63 which is "File name too long"

		//sem_init(&m_semaphore, 0, max_readers);
		//pthread_spin_init(&m_spinlock, 0);

		gen_random_name(semaphoreName,SEMAPHORE_NAME_LENGTH);
		sem_unlink(semaphoreName);
		m_semaphore=sem_open(semaphoreName, O_CREAT,0,max_readers);
		if(m_semaphore==SEM_FAILED){
			std::cerr<< "Semaphore creation failed!! with error no " << errno << std::endl;
		}

		pthread_mutex_init(&mutex, 0);
}

	void lockRead()
	{
		sem_wait(m_semaphore);
	}

	void unlockRead()
	{
		sem_post(m_semaphore);
	}

	void lockWrite()
	{
		//pthread_spin_lock(&m_spinlock);
		pthread_mutex_lock(&mutex);

		for (int i = 0; i < max_readers; i++)
		{
			sem_wait(m_semaphore);
		}
	}

	void unlockWrite()
	{
		for (int i = 0; i < max_readers; i++)
		{
			sem_post(m_semaphore);
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
			sem_wait(m_semaphore);
		}

		return rc;
	}

	void cond_signal(pthread_cond_t *cond)
	{
		pthread_cond_signal(cond);
	}

	~ReadWriteMutex()
	{
		sem_destroy(m_semaphore);
		//pthread_spin_destroy(&m_spinlock);
		pthread_mutex_destroy(&mutex);
	}

private:
    char semaphoreName[SEMAPHORE_NAME_LENGTH];
    int max_readers;
    sem_t* m_semaphore;
	//pthread_spinlock_t m_spinlock;
	pthread_mutex_t mutex;

	void gen_random_name(char *s, const int len) {
	    static const char alphanum[] =
	        "0123456789"
	        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	        "abcdefghijklmnopqrstuvwxyz";

	    for (int i = 0; i < len; ++i) {
	        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	    }
	}
};

}}

#endif /* __READWRITEMUTEX_H__ */
