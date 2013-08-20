#ifndef __CORE_JNI_SRCH2SDK_H__
#define __CORE_JNI_SRCH2SDK_H__
#include <jni.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>

#include <sys/time.h>

using namespace srch2::instantsearch;

namespace srch2 {
namespace sdk{

Indexer* createIndex(string indexDir, bool isGeo);
Indexer* createIndex(string dataFile, string indexDir, int lineLimit,
		bool isGeo);

void commitIndex(Indexer* indexer);
void saveIndex(Indexer *indexer);

Indexer* loadIndex(const string& strIndexPath);

void addRecord(Indexer* index, string key, string value, bool keepInMemory);

QueryResults* query(const Analyzer* analyzer, IndexSearcher* indexSearcher,
		const string& queryString, unsigned ed);

void parseFuzzyQueryWithEdSet(const Analyzer *analyzer, 
        const string &queryString, int ed, Query *query);

int pingForScalabilityTest(const Analyzer *analyzer, IndexSearcher *indexSearcher, 
        const string &queryString, unsigned ed);
 
int pingToCheckIfHasResults(const Analyzer *analyzer, IndexSearcher *indexSearcher, 
        string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng, int ed);

string printQueryResult(QueryResults* queryResults, Indexer* indexer);

}
}

#endif
