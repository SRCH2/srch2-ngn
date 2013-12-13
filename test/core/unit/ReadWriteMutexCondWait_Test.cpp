//$Id: ReadWriteMutexCondWait_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

/* Part of the code is from the following IBM site:
 * http://publib.boulder.ibm.com/infocenter/iseries/v5r4/index.jsp?topic=%2Fapis%2Fusers_77.htm.
 * It's part of the IBM i5/OS V5R4 package, which was
 * released on January 31, 2006 (according to
 * http://www-01.ibm.com/common/ssi/cgi-bin/ssialias?subtype=ca&infotype=an&appname=iSource&supplier=897&letternum=ENUS206-015).
 */

#include "util/ReadWriteMutex.h"
#include "util/Assert.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include "util/mypthread.h"

using namespace std;
using namespace srch2::instantsearch;

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int                 workToDo = 0;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
ReadWriteMutex rw_mutex(3);

bool hitByTheSignal = false;

#define NTHREADS                1
#define WAIT_TIME_SECONDS       5

void *threadfunc(void *parm)
{
    int               rc;
    struct timespec   ts;
    struct timeval    tp;

    /* Usually worker threads will loop on these operations */
    while (1) {
        rc =  gettimeofday(&tp, NULL);

        /* Convert from timeval to timespec */
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;

        while (!workToDo) {
            printf("Thread blocked\n");
            rc = rw_mutex.writeLockWithCondTimedWait(&cond, &ts);
            /* If the wait timed out, in this example, the work is complete, and   */
            /* the thread will end.                                                */
            /* In reality, a timeout must be accompanied by some sort of checking  */
            /* to see if the work is REALLY all complete. In the simple example    */
            /* we will just go belly up when we time out.                          */
            if (rc != 0) {
                printf("Wait timed out!\n");
                rw_mutex.unlockWrite();
                pthread_exit(NULL);
            }
            else {
                hitByTheSignal = true;
                printf("Got a job.\n");
            }
        }

        printf("Thread consumes work here\n");
        workToDo = 0;
        rw_mutex.unlockWrite();
    }

    rw_mutex.unlockWrite();
    return NULL;
}

int main(int argc, char **argv)
{
      int                   rc=0;
      int                   i;
      pthread_t             threadid[NTHREADS];

      printf("Enter Testcase - %s\n", argv[0]);

      printf("Create %d threads\n", NTHREADS);
      for(i=0; i<NTHREADS; ++i) {
          rc = pthread_create(&threadid[i], NULL, threadfunc, NULL);
      }

      sleep(1);
      rw_mutex.lockWrite();

      printf("One work item to give to a thread\n");
      workToDo = 1;

      rw_mutex.unlockWrite();

      rw_mutex.cond_signal(&cond);
      printf("Wait for threads and cleanup\n");

      for (i=0; i<NTHREADS; ++i) {
          rc = pthread_join(threadid[i], NULL);
      }

      pthread_cond_destroy(&cond);

      printf("Main completed\n");

      ASSERT(hitByTheSignal);

      return 0;
}
