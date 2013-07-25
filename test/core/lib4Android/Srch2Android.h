#ifndef __SRCH2_TEST_LIB4ANDROID_H__
#define __SRCH2_TEST_LIB4ANDROID_H__
#include <jni.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include "../integration/IntegrationTestHelper.h"
#include "../integration/MapSearchTestHelper.h"

#include <ctime>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

namespace srch2 {
namespace sdk {

const unsigned mergeEveryNSeconds = 3;
const unsigned mergeEveryMWrites = 5;

float getTimeSpan(clock_t begin);

int parseLine(char* line);

int getRAMUsageValue();

Indexer* createIndex(string indexDir, bool isGeo);
Indexer* createIndex(string dataFile, string indexDir, int lineLimit,
		bool isGeo);

void commitIndex(Indexer* indexer);

void addRecord(Indexer* index, string key, string value, bool keepInMemory);

QueryResults* query(const Analyzer* analyzer, IndexSearcher* indexSearcher,
		const string& queryString, unsigned ed,
		srch2::instantsearch::TermType termType);

Indexer* loadIndex(const string& strIndexPath);

void saveIndex(Indexer *indexer);

string printQueryResult(QueryResults* queryResults, Indexer* indexer);

}
}

#endif
