/*
 * ThreadSafetyReadWriteGeo_Test.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: mahdi
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
#include "include/instantsearch/Constants.h"
#include "util/RecordSerializerUtil.h"

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
using namespace srch2::util;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

string INDEX_DIR = getenv("index_dir");

#define MAX_THREAD 1000

unsigned id1 = 111;
unsigned id2 = 222;

typedef struct
{
    int id;
    int nproc;
    Indexer *indexer;
} parm;

void setRecordValues(Record* record, srch2is::Schema *schema, int id, string authors, string title, string lat, string lng){
    Schema * storedSchema = Schema::create();
    RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    RecordSerializer recSerializer(*storedSchema);

    record->setPrimaryKey(id);
    record->setSearchableAttributeValue("article_authors", authors);
    record->setSearchableAttributeValue("article_title", title);
    record->setRefiningAttributeValue("latitude",lat);
    record->setRefiningAttributeValue("longitude",lng);
    record->setLocationAttributeValue(atof(lat.c_str()),atof(lng.c_str()));
    {
    	recSerializer.addSearchableAttribute("article_authors", authors);
    	recSerializer.addSearchableAttribute("article_title", title);
    	recSerializer.addRefiningAttribute("latitude",(float)atof(lat.c_str()));
    	recSerializer.addRefiningAttribute("longitude", (float)atof(lng.c_str()));
    	RecordSerializerBuffer compactBuffer = recSerializer.serialize();
    	record->setInMemoryData(compactBuffer.start, compactBuffer.length);
    	recSerializer.nextRecord();
    }
    record->setRecordBoost(90);

    delete storedSchema;
}

void addSimpleRecords()
{
    ///Create Schema
    srch2is::Schema *schema = srch2is::Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text
    string latName = "latitude";
    string lngName = "longitude";
    schema->setRefiningAttribute(latName,ATTRIBUTE_TYPE_FLOAT,"0.0");
    schema->setRefiningAttribute(lngName,ATTRIBUTE_TYPE_FLOAT,"0.0");
    schema->setNameOfLatitudeAttribute(latName);
    schema->setNameOfLongitudeAttribute(lngName);

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

    setRecordValues(record, schema, 1001, "Tom Smith and Jack Lennon", "Come Yesterday Once More", "0.0", "0.0");
    index->addRecord(record, analyzer);

    record->clear();
    setRecordValues(record, schema, 1002, "Jimi Hendrix", "Little wing", "50.0", "50.0");
    index->addRecord(record, analyzer);

    record->clear();
    setRecordValues(record, schema, 1003, "Tom Smith and Jack The Ripper", "Come Tomorrow Two More", "100.0", "100.0");
    index->addRecord(record, analyzer);

    record->clear();
    setRecordValues(record, schema, 1004, "Peter and Alex and Tom", "football volleyball soccer swimming", "105.0", "105.0");
    index->addRecord(record, analyzer);

    index->commit();
    index->save(INDEX_DIR);

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

void testWrite(Indexer *indexer, unsigned id)
{
    Analyzer *analyzer = getAnalyzer();

    Record *record = new Record(indexer->getSchema());

    setRecordValues(record, indexer->getSchema(), 2001, "Tom Hendrix", "Little wing", "5.0", "5.0");
    indexer->addRecord(record, analyzer);

    record->clear();
    setRecordValues(record, indexer->getSchema(), 2002, "Harry and Gorge", "Italy France Japan", "101.0", "101.0");
    indexer->addRecord(record, analyzer);

    record->clear();
    setRecordValues(record, indexer->getSchema(), 2003, "Peter and Tom", "Italy France Japan", "106.0", "106.0");
    indexer->addRecord(record, analyzer);
}

void testDelete(Indexer *indexer, unsigned id)
{
    Analyzer *analyzer = getAnalyzer();

    indexer->deleteRecord("2001");

    indexer->deleteRecord("2002");

    indexer->deleteRecord("2003");
}


void testRead2(Indexer *indexer)
{
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    const Analyzer *analyzer = getAnalyzer();

    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1001);
    	recordIds.push_back(2001);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "tom", 0, 0, 10, 2 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1002);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "jimi", 50, 50, 10, 1 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1003);
    	recordIds.push_back(1004);
    	recordIds.push_back(2003);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "tom", 100, 100, 10, 3 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(2002);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "Harry", 100, 100, 10, 1 , recordIds) == true);
    }
    delete queryEvaluator;

}

void testRead1(Indexer *indexer)
{
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    const Analyzer *analyzer = getAnalyzer();

    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1001);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "tom", 0, 0, 10, 1 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1002);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "jimi", 50, 50, 10, 1 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1003);
    	recordIds.push_back(1004);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "tom", 100, 100, 10, 2 , recordIds) == true);
    }
    {
    	vector<unsigned> recordIds;
    	recordIds.push_back(1003);
    	ASSERT ( ping_WithGeo(analyzer, queryEvaluator, "ripper", 100, 100, 10, 1 , recordIds) == true);
    }
    delete queryEvaluator;
}

void* reader_MultiReader(void *arg)
{
	Indexer * indexer = (Indexer *) arg;
    testRead1(indexer);
    return NULL;
}

void* reader_MultiReaderOneWriter(void *arg)
{
	Indexer * indexer = (Indexer *) arg;
    testRead1(indexer);
    sleep(5);
    testRead2(indexer);
    return NULL;
}

void* reader_MultiReaderMultipleWriter(void *arg)
{
	Indexer * indexer = (Indexer *) arg;
    testRead1(indexer);
    sleep(5);
    testRead2(indexer);
    sleep(11);
    testRead1(indexer);
    return NULL;
}

void* writer(void *arg)
{
    parm *p = (parm *) arg;
    int id = p->id;
    int i;
    Indexer *indexer = p->indexer;

    testWrite(indexer,id);
    sleep(3);
    indexer->merge_ForTesting();

	return NULL;
}

void* writer2(void *arg)
{
    parm *p = (parm *) arg;
    int id = p->id;
    int i;
    Indexer *indexer = p->indexer;
    sleep(10);
    testDelete(indexer,id);
    sleep(3);
    indexer->merge_ForTesting();

	return NULL;
}

/*
 *  In this test case multiple readers do the search in a same time.
 */
void testMultipleReaders(){
	pthread_t *threadReaders;
	pthread_attr_t pthread_custom_attr;
	int n, i;
	n = 10;

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
        pthread_create(&threadReaders[i], &pthread_custom_attr, reader_MultiReader, (void *)indexer);
    }

    // Synchronize the completion of each thread.
    for (i = 0; i < n; i++)
    {
        pthread_join(threadReaders[i], NULL);
    }

    delete indexer;
}

/*
 *  In this test case we have multiple readers and a writer. First this readers
 *  do the search and in a same time the writer insert new records to the engine.
 *  Then the readers sleep for a few seconds and in this time we do the merge for the indexer
 *  Again these readers do the search and they should be get new records in their results.
 */
void testMultipleReadersOneWriter(){
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
    unsigned mergeEveryMWrites = 30;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 30;
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
        pthread_create(&threadReaders1[i], &pthread_custom_attr, reader_MultiReaderOneWriter, (void *)indexer);
        //pthread_create(&threadReaders2[i], &pthread_custom_attr, reader, (void *)(p+i));
    }

    // start writer
    p[n-1].id = id1;
    p[n-1].nproc = n;
    p[n-1].indexer = indexer;
    pthread_create(&threadWriter, &pthread_custom_attr, writer, (void *)(p+n-1));

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
/*
 *    In this test case we have multiple readers and two writer.
 *    First these readers do the search and in the same time first writer insert new records
 *    to the engine. Then these readers sleep for a few seconds and after we merged the indexer
 *    they start to do the search again and they should get these new records in their results.
 *    Now the second writer remove some records from the engine. After that readers again sleep
 *    for a few second and again they start searching and they should not get deleted records in
 *    their results.
 */
void testMultipleReadersMultipleWriters(){
    pthread_t *threadReaders1;
    //pthread_t *threadReaders2;
    //pthread_t *threadWriters;
    pthread_t threadWriter1;
    pthread_t threadWriter2;

    pthread_attr_t  pthread_custom_attr;
    parm *p;
    int n, i;

    n = 15;

    addSimpleRecords();

    // create an indexer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 30;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 30;
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
    for (i = 0; i < n - 2; i++)
    {
        p[i].id = i;
        p[i].nproc = n;
        p[i].indexer = indexer;
        pthread_create(&threadReaders1[i], &pthread_custom_attr, reader_MultiReaderOneWriter, (void *)indexer);
        //pthread_create(&threadReaders2[i], &pthread_custom_attr, reader, (void *)(p+i));
    }

    // start first writer
    p[n-2].id = id1;
    p[n-2].nproc = n;
    p[n-2].indexer = indexer;
    pthread_create(&threadWriter1, &pthread_custom_attr, writer, (void *)(p+n-2));

    // start second writer
    p[n-1].id = id2;
    p[n-1].nproc = n;
    p[n-1].indexer = indexer;
    pthread_create(&threadWriter2, &pthread_custom_attr, writer2, (void *)(p+n-1));

    // Synchronize the completion of each thread.
    for (i = 0; i < n-2; i++)
    {
        pthread_join(threadReaders1[i], NULL);
        //pthread_join(threadReaders2[i], NULL);
    }
    pthread_join(threadWriter1, NULL);
    pthread_join(threadWriter2, NULL);
    free(p);

    delete indexer;
}

int main(int argc, char *argv[]){
	try{
		cout << "Multiple Readers Testing ..." << endl;
		testMultipleReaders();
		cout << "Multiple Readers Test Passed" << endl;

		cout << "Multiple Readers and One Writer Testing ..." << endl;
		testMultipleReadersOneWriter();
		cout << "Multiple Readers and One Writer Test Passed" << endl;

		cout << "Multiple Readers and Multiple Writers Testing ..." << endl;
		testMultipleReadersMultipleWriters();
		cout << "Multiple Readers and Multiple Writers Test Passed" << endl;

	}catch(const exception& ex){

	}
	return 0;
}


