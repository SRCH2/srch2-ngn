
// $Id: ThreadSafetyReadWrite_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

string INDEX_DIR = getenv("index_dir");

void addSimpleRecords()
{
    ///Create Schema
    srch2is::Schema *schema = srch2is::Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");
    // Create an index writer
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges , updateHistogramEveryQWrites,
    		INDEX_DIR);
           
    Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
    
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
    record->setRecordBoost(90);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "Jimi Hendrix");
    record->setSearchableAttributeValue(2, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
    record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
    record->setRecordBoost(10);
    index->addRecord(record, analyzer);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}


#define MAX_THREAD 1000

typedef struct
{
    int id;
    int nproc;
    Indexer *indexer;
} parm;

//char message[100];    /* storage for message  */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int token = 0;


void testRead(Indexer *indexer)
{
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    const Analyzer *analyzer = getAnalyzer();

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        //ASSERT ( ping(analyzer, queryEvaluator, "tom" , 2 , recordIds) == true);
        ping(analyzer, queryEvaluator, "tom" , 2 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }
    //Query: "jimi", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        //ASSERT ( ping(analyzer, queryEvaluator, "jimi" , 1 , recordIds) == true);
        ping(analyzer, queryEvaluator, "jimi" , 1 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        //ASSERT ( ping(analyzer, queryEvaluator, "smith" , 2 , recordIds) == true);
        ping(analyzer, queryEvaluator, "smith" , 2 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "jobs", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        //ASSERT ( ping(analyzer, queryEvaluator, "jobs" , 1 , recordIds) == true);
        ping(analyzer, queryEvaluator, "jobs" , 1 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //indexer->print_index();

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        //ASSERT ( ping(analyzer, queryEvaluator, "smith" , 2 , recordIds) == true);
        ping(analyzer, queryEvaluator, "smith" , 2 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "jobs", hit -> 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        //ASSERT ( ping(analyzer, queryEvaluator, "jobs" , 1 , recordIds) == true);
        ping(analyzer, queryEvaluator, "jobs" , 1 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        recordIds.push_back(1002);
        //ASSERT ( ping(analyzer, queryEvaluator, "jimi" , 2 , recordIds) == true);
        ping(analyzer, queryEvaluator, "jimi" , 2 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        recordIds.push_back(1999);
        //ASSERT ( ping(analyzer, queryEvaluator, "jobs" , 2 , recordIds) == true);
        ping(analyzer, queryEvaluator, "jobs" , 2 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1999);
        recordIds.push_back(1003);
        //ASSERT ( ping(analyzer, queryEvaluator, "tom" , 3 , recordIds) == true);
        ping(analyzer, queryEvaluator, "tom" , 3 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
    }

    delete queryEvaluator;

}

void testWrite(Indexer *indexer, unsigned id)
{
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);

    Analyzer *analyzer = getAnalyzer();

    //Update Index
    Record *record = new Record(indexer->getSchema());
    record->setPrimaryKey(1999*id);
    record->setSearchableAttributeValue(1, "steve jobs tom");
    record->setSearchableAttributeValue(2, "digital magician");
    record->setRecordBoost(90);
    indexer->addRecord(record, analyzer);

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1999);
        recordIds.push_back(1003);
        indexer->merge_ForTesting();
        ping(analyzer, queryEvaluator, "tom" , 3 , recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND);
        //ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    // //Update a Deserialised Index
    // record = new Record(indexer->getSchema());
    // record->setPrimaryKey(1998*id);
    // record->setSearchableAttributeValue(1, "jimi hendrix");
    // record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    // record->setRecordBoost(100);
    // indexer->addRecord(record, 0);

    // // TODO how to merge? indexer->commit();
}

void* writerWithMutex(void *arg)
{
    parm *p = (parm *) arg;
    int id = p->id;
    int i;
    Indexer *indexer = p->indexer;

    //printf("WRITER Thread %d.\n", id);

    if (id != 0)
    {
        /* Create message */
        while (1)
        {
            pthread_mutex_lock(&msg_mutex);
            //printf("WRITER Thread %d. Got Lock.\n", id);
            if (token  == 0)
            {
                testWrite(indexer,id);
                //sprintf(message, "WRITE:Greetings from process %d received!!!", id);
                token++;
                pthread_mutex_unlock(&msg_mutex);
                break;
            }
            pthread_mutex_unlock(&msg_mutex);
            sleep(1);
        }
        /* Use strlen+1 so that '\0' gets transmitted */
    } else
    {    //my_rank == 0
        //printf("WRITER Thread %d.  Doing reading.\n", id);
        for (i = 1; i < p->nproc; i++)
        {
            while (1)
            {
                pthread_mutex_lock(&msg_mutex);
                if (token == 1)
                {
                    //printf("%s\n", message);
                    testRead(indexer);
                    token--;
                    pthread_mutex_unlock(&msg_mutex);
                    break;
                }
                pthread_mutex_unlock(&msg_mutex);
                sleep(1);
            }
        }
    }

    return NULL;
}

void* reader(void *arg)
{
	Indexer * indexer = (Indexer *) arg;
    testRead(indexer);
    sleep(2);
    testRead(indexer);
    return NULL;
}

void test0()
{
    pthread_t *threadReaders;
    pthread_attr_t pthread_custom_attr;
    int n, i;
    n = 50;

    addSimpleRecords();
    // create an index
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer *indexer = Indexer::load(indexMetaData1);

    threadReaders = (pthread_t *) malloc(n * sizeof(*threadReaders));

    pthread_attr_init(&pthread_custom_attr);

    // Start up thread
    for (i = 0; i < n; i++)
    {
        pthread_create(&threadReaders[i], &pthread_custom_attr, reader, (void *)indexer);
    }

    // Synchronize the completion of each thread.
    for (i = 0; i < n; i++)
    {
        pthread_join(threadReaders[i], NULL);
    }

    delete indexer;
}

// Multiple readers, single writer
void test1()
{
    pthread_t *threadReaders1;
    //pthread_t *threadReaders2;
    //pthread_t *threadWriters;
    pthread_t threadWriter;

    pthread_attr_t  pthread_custom_attr;
    parm *p;
    int n, i;

    n = 15;

    addSimpleRecords();

    // create an indexer
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer * indexer = Indexer::load(indexMetaData1);

    threadReaders1 = (pthread_t *) malloc(n * sizeof(*threadReaders1));
    //threadReaders2 = (pthread_t *) malloc(n * sizeof(*threadReaders2));

    pthread_attr_init(&pthread_custom_attr);

    p=(parm *)malloc(sizeof(parm)*n);

    // Start n - 1 readers
    //for (i = 0; i < n; i++)
    for (i = 0; i < n - 1; i++)
    {
        p[i].id = i;
        p[i].nproc = n;
        p[i].indexer = indexer;
        pthread_create(&threadReaders1[i], &pthread_custom_attr, reader, (void *)indexer);
        //pthread_create(&threadReaders2[i], &pthread_custom_attr, reader, (void *)(p+i));
    }

    // start writer
    p[n-1].id = n-1;
    p[n-1].nproc = n;
    p[n-1].indexer = indexer;
    pthread_create(&threadWriter, &pthread_custom_attr, writerWithMutex, (void *)(p+n-1));

    // Synchronize the completion of each thread.
    for (i = 0; i < n-1; i++)
    {
        pthread_join(threadReaders1[i], NULL);
        //pthread_join(threadReaders2[i], NULL);
    }
    pthread_join(threadWriter, NULL);
    free(p);

    delete indexer;
}

void test2()
{
    pthread_t *threadReaders;
    pthread_t *threadWriters;

    pthread_attr_t  pthread_custom_attr;
    parm *p;

    int n, i;

    n = 3;

    if ((n < 1) || (n > MAX_THREAD))
    {
        printf("The no of thread should between 1 and %d.\n", MAX_THREAD);
        exit(1);
    }

    addSimpleRecords();
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer* indexer = Indexer::load(indexMetaData1);
    
    threadReaders = (pthread_t *) malloc(n * sizeof(*threadReaders));
    threadWriters = (pthread_t *) malloc(n * sizeof(*threadWriters));

    pthread_attr_init(&pthread_custom_attr);

    p = (parm *)malloc(sizeof(parm)*n);

    /* Start up thread */
    for (i = 0; i < n; i++) {
        p[i].id = i;
        p[i].nproc = n;
        p[i].indexer = indexer;
        pthread_create(&threadReaders[i], &pthread_custom_attr, reader, (void *)indexer);
        pthread_create(&threadWriters[i], &pthread_custom_attr, writerWithMutex, (void *)(p+i)); 
    }

    // Synchronize the completion of each thread.
    for (i = 0; i < n; i++) {
        pthread_join(threadReaders[i], NULL);
        pthread_join(threadWriters[i], NULL);
    }
    free(p);

    delete indexer;
}

int main(int argc, char *argv[])
{
	try{

		cout << "Case 0: Multiple Readers - Testing..." << endl;
		test0();
		cout << "Test0 Passed" << endl;

		// multiple readers and one writer
		cout << "Case 1: Multiple Readers and One writer - Testing..." << endl;
		test1();
		cout << "Test1 Passed" << endl;

		// mulitple Readers and multiple writers
		cout << "Case 2: Mulitple Readers and Multiple writers - Testing..." << endl;
		test2();
		cout << "Test2 Passed" << endl;
    // multiple readers
	}catch(const exception& ex) {
		cout << ex.what() << endl;
	}
    return 0;
}
