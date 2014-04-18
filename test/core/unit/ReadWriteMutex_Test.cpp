//$Id: ReadWriteMutex_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "util/ReadWriteMutex.h"
#include "util/Assert.h"

#include <unistd.h>
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include "util/mypthread.h"

using namespace std;
using namespace srch2::instantsearch;

ReadWriteMutex rw_mutex(3);

void *writer(void *seconds)
{
	{
		time_t t1, t2;
		time(&t1);
		ExceptionSafeRWLockForWrite autolock(rw_mutex);

		time(&t2);
		*((int *) seconds) = (int)difftime(t2, t1);

		printf("Write Thread\n");
		sleep(2);
	}

    pthread_exit(0);
}

void *reader(void *seconds)
{
    {
    	time_t t1, t2;
    	time(&t1);
    	ExceptionSafeRWLockForRead autolock(rw_mutex);

    	time(&t2);
    	*((int *) seconds) = (int)difftime(t2, t1);

    	printf("Read Thread\n");
    	sleep(2);
    }
    pthread_exit(0);
}

void testReadLock()
{
    pthread_t *threadReaders;
    int n, i, seconds;
    n = 4;
    seconds = 0;

    threadReaders = (pthread_t *) malloc(n * sizeof(*threadReaders));

    for (i = 0; i < n; i++)
    {
        pthread_create(&threadReaders[i], NULL, reader, (void *)&seconds);
    }

    for (i = 0; i < n; i++)
    {
        pthread_join(threadReaders[i], NULL);
    }

    // seconds is the time that the last reader thread spent to get the read lock
    // since the max number of readers is set to 3, the 4th reader thread needs to wait for 1s
    ASSERT(seconds == 2);

    free(threadReaders);

}

void testWriteLock()
{
    pthread_t *threadReaders;
    pthread_t *threadWriters;
    int n, i;
    int reader_seconds = 0;
    int writer_seconds[2];
    n = 2;

    threadReaders = (pthread_t *) malloc((n+1) * sizeof(*threadReaders));
    threadWriters = (pthread_t *) malloc(n * sizeof(*threadWriters));

    // start 2 reader threads
    for (i = 0; i < n; i++)
    {
        pthread_create(&threadReaders[i], NULL, reader, (void *)&reader_seconds);
    }

    sleep(1);

    // start 2 writer threads
    for (i = 0; i < n; i++)
    {
        writer_seconds[i] = 0;
        pthread_create(&threadWriters[i], NULL, writer, (void *)&writer_seconds[i]);
    }

    sleep(4);

    // start the 3rd reader thread
    pthread_create(&threadReaders[n], NULL, reader, (void *)&reader_seconds);

    for (i = 0; i < n; i++)
    {
        pthread_join(threadReaders[i], NULL);
    }

    for (i = 0; i < n; i++)
    {
        pthread_join(threadWriters[i], NULL);
    }

    pthread_join(threadReaders[n], NULL);

    // the first writer thread needs to wait for the first two reader threads to unlock
    // the second writer thread needs to wait for the first writer thread to unlock
    ASSERT ( (writer_seconds[0] == 1 && writer_seconds[1] == 3)
          || (writer_seconds[0] == 3 && writer_seconds[1] == 1) );

    // the third reader thread needs to wait for all the two writer threads to unlock
    ASSERT(reader_seconds == 1);

    free(threadReaders);
    free(threadWriters);

}

int main(int argc, char *argv[])
{
    testReadLock();
    testWriteLock();

    cout<<"\nReadWriteMutex unit tests passed.\n";
    return 0;
}
