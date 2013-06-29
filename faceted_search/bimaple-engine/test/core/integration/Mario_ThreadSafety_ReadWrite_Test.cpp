#include "util/Assert.h"
#include "MapSearchTestHelper.h"

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
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

string INDEX_DIR = getenv("index_dir");

void addSimpleRecords()
{
	///Create Schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = Analyzer::create(srch2::instantsearch::NO_STEMMER_NORMALIZER, "");
	// create an indexer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( GlobalCache::create(134217728,20000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	Point point;
	point.x = 100.0;
	point.y = 100.0;
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0);

	record->clear();
	point.x = 101.0;
	point.y = 101.0;
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0);

	record->clear();
	point.x = 102.0;
	point.y = 102.0;
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0);

	record->clear();
	point.x = 103.0;
	point.y = 103.0;
	record->setPrimaryKey(1004);
	record->setSearchableAttributeValue(1, "Steve Jobs");
	record->setSearchableAttributeValue(2, "iPod iPhone iPad");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0);

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
	int             id;
	int             nproc;
}               parm;

struct IndexerDataContainer {
	Indexer *indexer;
	//IndexSearcher *indexSearcher;
};

static struct IndexerDataContainer indexerDataContainer;

//char            message[100];	/* storage for message  */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int             token = 0;

void query(IndexSearcher *indexSearcher, string keyword, unsigned numberofHits , const vector<unsigned> &recordIDs)
{
	Query *query = new Query(MapQuery);

	Term *term = FuzzyTerm::create(keyword,
			PREFIX,
			1,
			100.0,
			getNormalizedThreshold(keyword.size()));
	query->add(term);

	query->setRange(-200.0, -200.0, 200.0, 200.0);

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults);

	vector<unsigned> ids;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		//cout << "query: " << keyword << " results numebr: " << queryResults->getRecordId(i) << endl;
		ids.push_back((unsigned)atoi(queryResults->getRecordId(i).c_str()));
	}

	ASSERT( queryResults->getNumberOfResults() ==  numberofHits);

	ASSERT( ifAllFoundResultsAreCorrect(recordIDs, ids) );

	ASSERT( ifAllExpectedResultsAreFound(recordIDs, ids) );

	delete queryResults;
	delete query;
}

//Create Index "A". Deserialise "A". Update Index "A". Search "A". Serialize "A"
void testRead(IndexerDataContainer *indexerDataContainerLocal)
{
	//Indexer *indexer = indexerDataContainerLocal->indexer;
	//srch2is::Cache *cache = new srch2is::Cache();
	IndexSearcher *indexSearcher = IndexSearcher::create(indexerDataContainer.indexer);

	//Query: "tom", hits -> 1001, 1003
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1003);
		//ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == true);
		query(indexSearcher, "tom" , 2 , recordIds);
	}
	//Query: "jimi", hit -> 1002
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1002);
		//ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
		query(indexSearcher, "jimi" , 1 , recordIds);
	}

	//Query: "smith", hits -> 1001, 1003
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1003);
		//ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
		query(indexSearcher, "smith" , 2 , recordIds);
	}

	//Query: "smith", hits -> 1001, 1003
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1003);
		//ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
		query(indexSearcher, "smith" , 2 , recordIds);
	}

	//Query: "jobs", hits -> 1004
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1004);
		//ASSERT ( ping(analyzer, indexSearcher, "jobs" , 2 , recordIds) == true);
		query(indexSearcher, "jobs" , 1 , recordIds);
	}

	//Query: "tom", hits -> 1001, 1003
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1003);
		//ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
		query(indexSearcher, "tom" , 2 , recordIds);
	}

	delete indexSearcher;

}

void* reader(void *arg)
{
	parm           *p = (parm *) arg;
	int             id = p->id;
	printf("READ:Entering Thread id %d. Run 1.\n", id);
	testRead(&indexerDataContainer);
	sleep(6);
	printf("READ:Entering Thread id %d. Run 2.\n", id);
	testRead(&indexerDataContainer);
	return NULL;
}

void test0()
{
	pthread_t      *threadReaders;
	pthread_attr_t  pthread_custom_attr;
	parm           *p;
	int             n, i;
	n = 40;

	addSimpleRecords();
	// create an indexer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( GlobalCache::create(134217728,20000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
	indexerDataContainer.indexer = Indexer::load(indexMetaData1);

	threadReaders = (pthread_t *) malloc(n * sizeof(*threadReaders));

	pthread_attr_init(&pthread_custom_attr);

	p=(parm *)malloc(sizeof(parm)*n);

	// Start up thread
	for (i = 0; i < n; i++)
	{
		p[i].id = i;
		p[i].nproc = n;
		pthread_create(&threadReaders[i], &pthread_custom_attr, reader, (void *)(p+i));
	}

	// Synchronize the completion of each thread.
	for (i = 0; i < n; i++)
	{
		pthread_join(threadReaders[i], NULL);
	}
	free(p);

	delete indexerDataContainer.indexer;
}

int main(int argc, char *argv[])
{
	cout << "Case 1: Multiple Readers - Testing..." << endl;
	//Multiple Readers
	test0();
	cout << "Test0 Passed" << endl;

	return 0;
}				/* main */
