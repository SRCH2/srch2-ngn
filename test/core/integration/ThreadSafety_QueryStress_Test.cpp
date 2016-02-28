/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <instantsearch/Analyzer.h>
#include "operation/IndexerInternal.h"
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"
#include "IntegrationTestHelper.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "util/mypthread.h"

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
    CacheManager *cache;
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
    //srch2is::CacheManager *cache = new srch2is::CacheManager();
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexerDataContainer.indexer, &runtimeParameters);

    clock_gettime(CLOCK_REALTIME, &tstart);
    for( vector<string>::iterator vectIter = indexerDataContainer.queryFile.begin(); vectIter!= indexerDataContainer.queryFile.end(); vectIter++ )
    {
        pingDummyStressTest(indexerDataContainer.analyzer, queryEvaluator,*vectIter);//,resultCount,0);
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    delete queryEvaluator;

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
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    indexerDataContainer.cache = new srch2is::CacheManager(134217728); // 134217728 bytes = 1GB
    IndexMetaData *indexMetaData1 = new IndexMetaData( indexerDataContainer.cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
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
