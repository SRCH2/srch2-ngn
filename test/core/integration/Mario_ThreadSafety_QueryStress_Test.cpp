#include "MapSearchTestHelper.h"
#include "util/Assert.h"

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

#define MAX_THREAD 1000

typedef struct
{
	int             id;
	int             nproc;
}               parm;

struct IndexerDataContainer {
	Indexer *indexer;
	//Cache *cache;
	GlobalCache *cache;
	vector<Rectangle> ranges;
	vector< vector<string> > queries;
};

static struct IndexerDataContainer indexerDataContainer;

//char            message[100];	/* storage for message  */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int             token = 0;

void queryStressTest(double &time)
{

	// variables to measure the elapsed time
	struct timespec tstart;
	struct timespec tend;

	// create an index searcher
	//srch2is::Cache *cache = new srch2is::Cache();

	IndexSearcher *indexSearcher = IndexSearcher::create(indexerDataContainer.indexer);

	clock_gettime(CLOCK_REALTIME, &tstart);
	for( unsigned vectIter = 0; vectIter < indexerDataContainer.queries.size(); vectIter++ )
	{
		Query *query = new Query(MapQuery);

		for (unsigned i = 0; i < indexerDataContainer.queries[vectIter].size(); ++i)
		{
			Term *term = FuzzyTerm::create(indexerDataContainer.queries[vectIter][i],
					PREFIX,
					1,
					100.0,
					getNormalizedThreshold(indexerDataContainer.queries[vectIter][i].size()));
			query->add(term);
		}

		query->setRange(indexerDataContainer.ranges[vectIter].min.x, indexerDataContainer.ranges[vectIter].min.y, indexerDataContainer.ranges[vectIter].max.x, indexerDataContainer.ranges[vectIter].max.y);

		QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcher, query);

		indexSearcher->search(query, queryResults);

		delete queryResults;
		delete query;
	}

	clock_gettime(CLOCK_REALTIME, &tend);
	double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

	delete indexSearcher;

	time = ts2;
	//cout << "Executed " << file.size() << "queries in " << ts2 << " milliseconds." << endl;
}


void* reader(void *arg)
{
	parm           *p = (parm *) arg;
	int             id = p->id;
	double time;

	queryStressTest(time);
	printf("READ: Thread id %d. Executed %d in %f milliseconds\n", id, int(indexerDataContainer.queries.size()), time );

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
	parm           *p;
	int             i;

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
/*	n.push_back(10);
	n.push_back(12);
	n.push_back(16);*/

	for(unsigned iter = 0; iter < n.size(); ++iter)
	{
		// variables to measure the elapsed time
		threadReader(n[iter]);
	}

	//delete indexerDataContainer.indexer; TODO destructor of indexer
}

int main(int argc, char *argv[])
{
	string INDEX_DIR = getenv("index_dir");
	string QUERY_FILE = getenv("query_file");

	// create an indexer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( GlobalCache::create(134217728,20000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

	indexerDataContainer.cache = NULL; // 134217728 bytes = 1GB
	indexerDataContainer.indexer = Indexer::load(indexMetaData1);


	cout << "Index:" << INDEX_DIR << endl;
	cout << "Query File:" << QUERY_FILE << endl;

	string line;
	ifstream infile (QUERY_FILE.c_str(), ios_base::in);
	while (getline(infile, line, '\n'))
	{
		vector<string> query;
		Rectangle range;
		bool pass = parseLine(line, query, range);
		if (!pass)
		{
			cout << line << endl;
			continue;
		}
		indexerDataContainer.queries.push_back(query);
		indexerDataContainer.ranges.push_back (range);
	}

	cout << "Case 1: Multiple Readers - Query Stress Testing..." << endl;
	//Multiple Reader and One writer
	test1();
	cout << "Test1 Passed" << endl;

	return 0;
}				/* main */
