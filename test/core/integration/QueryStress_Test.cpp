
// $Id: QueryStress_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $


#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

using std::string;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// variables to measure the elapsed time
struct timespec tstart;
struct timespec tend;

bool parseLine(string &line, string &query, bool &returnValue1, bool &returnValue2)
{
	vector<string> record;
	csvline_populate(record, line, ',');

	if (record.size() < 3)
	{
		return false;
	}

	query = record[0];
	returnValue1 = atoi(record[1].c_str());
	returnValue2 = atoi(record[2].c_str());

	return true;
}

int main(int argc, char **argv)
{
  string index_dir = getenv("index_dir");

  cout << index_dir << endl;

  //buildFactualIndex(index_dir, 4000000);
  buildFactualIndex(index_dir, 1000);
  //buildIndex(index_dir); // CHENLI
  // Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			"", index_dir);
	   	
	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	std::vector<std::string> file;
	std::string line;
	file.clear();

	std::string query_file = getenv("query_file");

	cout << query_file << endl;

	std::ifstream infile (query_file.c_str(), std::ios_base::in);
	while (getline(infile, line, '\n'))
	{
		string query = line;
/*		bool returnValue1 = 0;
		bool returnValue2 = 0;
		bool pass = parseLine(line, query, returnValue1,returnValue2);
		if (!pass)
			abort();*/
		file.push_back (query);
	}

	clock_gettime(CLOCK_REALTIME, &tstart);
	for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.end(); vectIter++ )
	{
		//cout << *vectIter << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << file.end() - vectIter << endl;
		//unsigned resultCount = 10;
		pingDummyStressTest(analyzer, queryEvaluator,*vectIter);//,resultCount,0);
	}

	clock_gettime(CLOCK_REALTIME, &tend);
	double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

	delete queryEvaluator;
	delete index;
	delete analyzer;

	cout << "Executed " << file.size() << "queries in " << ts2 << " milliseconds." << endl;
	cout << "QueryStressTest passed!" << endl;

	return 0;
}
