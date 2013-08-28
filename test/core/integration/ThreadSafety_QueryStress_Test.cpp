//$Id: ThreadSafety_QueryStress_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $

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

#include <instantsearch/Analyzer.h>
#include "operation/IndexerInternal.h"
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"
#include "IntegrationTestHelper.h"

#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
using std::cout;
using std::endl;

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

#define MAX_THREAD 1000

typedef struct
{
    int id;
    int nproc;
} parm;

struct IndexerDataContainer {
    Indexer *indexer;
    Cache *cache;
    const Analyzer *analyzer;
    std::vector<std::string> queryFile;
};

static struct IndexerDataContainer indexerDataContainer;

//char message[100];    /* storage for message  */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int token = 0;


void queryStressTest(double &time)
{

    // variables to measure the elapsed time
    struct timespec tstart;
    struct timespec tend;

    // create an index searcher
    //srch2is::Cache *cache = new srch2is::Cache();
    IndexSearcher *indexSearcher = IndexSearcher::create(indexerDataContainer.indexer);

    clock_gettime(CLOCK_REALTIME, &tstart);
    for( vector<string>::iterator vectIter = indexerDataContainer.queryFile.begin(); vectIter!= indexerDataContainer.queryFile.end(); vectIter++ )
    {
        pingDummyStressTest(indexerDataContainer.analyzer, indexSearcher,*vectIter);//,resultCount,0);
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    delete indexSearcher;

    time = ts2;
    //cout << "Executed " << file.size() << "queries in " << ts2 << " milliseconds." << endl;
}


void* reader(void *arg)
{
    parm *p = (parm *) arg;
    int id = p->id;
    double time;

    queryStressTest(time);
    printf("READ: Thread id %d. Executed %d in %f milliseconds\n", id, int(indexerDataContainer.queryFile.size()), time );

/*
    sleep(6);
    printf("READ:Entering Thread id %d. Run 2.\n", id);
    testRead(&indexerDataContainer);
*/
    return NULL;
}

void threadReader(int n)
{
    cout << "START: Testing [" << n << "] threads..." << endl;
    pthread_t      *threadReaders1;
    pthread_attr_t  pthread_custom_attr;
    parm *p;
    int i;

    struct timespec tstart;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);
    threadReaders1 = (pthread_t *) malloc(n * sizeof(*threadReaders1));
    pthread_attr_init(&pthread_custom_attr);

    p=(parm *)malloc(sizeof(parm)*n);

    // Start up thread
    for (i = 0; i < n; i++)
    {
        p[i].id = i;
        p[i].nproc = n;
        pthread_create(&threadReaders1[i], &pthread_custom_attr, reader, (void *)(p+i));
    }

    // Synchronize the completion of each thread.
    for (i = 0; i < n; i++)
    {
        pthread_join(threadReaders1[i], NULL);
    }
    free(p);

    clock_gettime(CLOCK_REALTIME, &tend);
    double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    cout << "END: [" << n << "] threads in " << ts2 << "milliseconds." << endl;
}

void test1()
{
    /****Thread Loop***/
    vector<unsigned> n;
    //n.push_back(1);
    n.push_back(2);
    n.push_back(4);
    n.push_back(6);
    n.push_back(8);
/*    n.push_back(10);
    n.push_back(12);
    n.push_back(16);*/

    for(unsigned iter = 0; iter < n.size(); ++iter)
    {
        // variables to measure the elapsed time
        threadReader(n[iter]);
    }

    delete indexerDataContainer.indexer;
}

int main(int argc, char *argv[])
{
    string INDEX_DIR = getenv("index_dir");
    string QUERY_FILE = getenv("query_file");

    //buildFactualIndex(INDEX_DIR, 4000000);
    // create an indexer
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    indexerDataContainer.cache = new srch2is::Cache(134217728,20000); // 134217728 bytes = 1GB
    IndexMetaData *indexMetaData1 = new IndexMetaData( indexerDataContainer.cache, mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    indexerDataContainer.indexer = Indexer::load(indexMetaData1);
    indexerDataContainer.analyzer = getAnalyzer();

    std::string line;
    cout << "Index:" << INDEX_DIR << endl;
    cout << "Query File:" << QUERY_FILE << endl;

    std::ifstream infile (QUERY_FILE.c_str(), std::ios_base::in);
    while (getline(infile, line, '\n'))
    {
        indexerDataContainer.queryFile.push_back(line);
    }

    cout << "Case 1: Multiple Readers - Query Stress Testing..." << endl;
    //Multiple Reader and One writer
    test1();
    cout << "Test1 Passed" << endl;

    return 0;
}
