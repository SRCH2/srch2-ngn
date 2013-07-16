//$Id: Scalability_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $

#include <jni.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <android/log.h>
#include <ctime>

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"
#include "util/Logger.h"

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using srch2::util::Logger;

const unsigned mergeEveryNSeconds = 3;
const unsigned mergeEveryMWrites = 5;

float getTimeSpan(clock_t begin) {
	return (((float) (clock() - begin)) / CLOCKS_PER_SEC);
}

int parseLine(char* line) {
	int i = strlen(line);
	while (*line < '0' || *line > '9')
		line++;
	line[i - 3] = '\0';
	i = atoi(line);
	return i;
}

int getRAMUsageValue() { //Note: this value is in KB!
	FILE* file = fopen("/proc/self/status", "r");
	int result = -1;
	if (file == NULL) {
		Logger::error("File %s open failed", "/proc/self/status");
		return result;
	}
	char line[128];
	while (fgets(line, 128, file) != NULL) {
		if (strncmp(line, "VmRSS:", 6) == 0) {
			result = parseLine(line);
			break;
		}
	}
	fclose(file);
	return result;
}

// Read data from file, build the index, and save the index to disk
void buildIndex(string data_file, string index_dir, int lineLimit) {
	/// Set up the Schema
	Schema *schema = Schema::create(srch2is::DefaultIndex);
	schema->setPrimaryKey("primaryKey");
	schema->setSearchableAttribute("description", 2);

	/// Create an Analyzer
	AnalyzerInternal *analyzer = new StandardAnalyzer(
			srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

	/// Create an index writer
	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	Record *record = new Record(schema);

	unsigned docsCounter = 0;
	string line;

	ifstream data(data_file.c_str());

	/// Read records from file
	/// the file should have two fields, seperated by '^'
	/// the first field is the primary key, the second field is a searchable attribute
	clock_t timebegin = clock();
	int cline = 0;
	while (getline(data, line)) {
		if (++cline > lineLimit) {
			break;
		}
		unsigned cellCounter = 0;
		stringstream lineStream(line);
		string cell;

		while (getline(lineStream, cell, '^') && cellCounter < 2) {
			if (cellCounter == 0) {
				record->setPrimaryKey(cell.c_str());
			} else {
				record->setSearchableAttributeValue(0, cell);
			}

			cellCounter++;
		}

		indexer->addRecord(record, 0);

		docsCounter++;

		record->clear();
	}

	Logger::console("#Docs Read: %d, time spend: %.5f seconds", docsCounter,
			getTimeSpan(timebegin));
	indexer->commit();
	indexer->save();

	Logger::console("Index saved. total time spend: %.5f seconds",
			getTimeSpan(timebegin));
	Logger::console("Index mem usage %d kb. ", getRAMUsageValue());
	data.close();

	delete indexer;
	delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Read data from file, build the index, and save the index to disk
void buildGeoIndex(string data_file, string index_dir, int lineLimit) {
	/// Set up the Schema
	Schema *schema = Schema::create(srch2is::LocationIndex);
	schema->setPrimaryKey("primaryKey");
	schema->setSearchableAttribute("description", 2);

	/// Create an Analyzer
	AnalyzerInternal *analyzer = new StandardAnalyzer(
			srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

	/// Create an index writer
	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	Record *record = new Record(schema);

	unsigned docsCounter = 0;
	string line;

	ifstream data(data_file.c_str());

	/// Read records from file
	/// the file should have two fields, seperated by '^'
	/// the first field is the primary key, the second field is a searchable attribute
	clock_t timebegin = clock();
    int cline = 0;
	while (getline(data, line)) {
        if (++cline > lineLimit){
            break;
        }
		unsigned cellCounter = 0;
		stringstream lineStream(line);
		string cell;
		float lat = 0.0, lng = 0.0;

		while (getline(lineStream, cell, '^') && cellCounter < 4) {
			if (cellCounter == 0) {
				record->setPrimaryKey(cell.c_str());
			} else if (cellCounter == 1) {
				record->setSearchableAttributeValue(0, cell);
			} else if (cellCounter == 2) {
				lng = atof(cell.c_str());
			} else if (cellCounter == 3) {
				lat = atof(cell.c_str());
			}

			cellCounter++;
		}

		record->setLocationAttributeValue(lat, lng);

		indexer->addRecord(record, 0);

		docsCounter++;

		record->clear();
	}

	Logger::console("#Docs Read: %d, time spend: %.5f seconds", docsCounter,
			getTimeSpan(timebegin));
	indexer->commit();
	indexer->save();

	Logger::console("Index saved. total time spend: %.5f seconds",
			getTimeSpan(timebegin));

	Logger::console("Index mem usage %d kb. ", getRAMUsageValue());

	data.close();

	delete indexer;
	delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Warm up the index, so that the first query in the test won't be slow
void warmUp(const Analyzer *analyzer, IndexSearcher *indexSearcher) {
	pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, PREFIX);
	pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, PREFIX);
	pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, PREFIX);
}

// Warm up the geo index, so that the first query in the test won't be slow
void warmUpGeo(const Analyzer *analyzer, IndexSearcher *indexSearcher) {
	pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0,
			60.0, -90.0, 1, PREFIX);
	pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0,
			60.0, -90.0, 1, PREFIX);
	pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0,
			60.0, -90.0, 1, PREFIX);
}

// Read queries from file and do the search
void readQueriesAndDoQueries(string path, string type, const Analyzer *analyzer,
		IndexSearcher *indexSearcher, unsigned ed,
		srch2::instantsearch::TermType termType) {
	string line;

	ifstream keywords(path.c_str());
	vector < string > keywordVector;

	unsigned counter = 0;

	while (getline(keywords, line) && counter < MAX_QUERY_NUMBER) {
		keywordVector.push_back(line);
		counter++;
	}

	keywords.close();

	timespec t1;
	timespec t2;

	int empty = 0;

	clock_gettime(CLOCK_REALTIME, &t1);

	for (vector<string>::iterator vectIter = keywordVector.begin();
			vectIter != keywordVector.end(); vectIter++) {
		timespec t1_inner;
		timespec t2_inner;

//      Logger::console("Query: %s", (*vectIter).c_str());
//      clock_gettime(CLOCK_REALTIME, &t1_inner);

		bool hasRes = pingForScalabilityTest(analyzer, indexSearcher, *vectIter,
				ed, termType);

//      clock_gettime(CLOCK_REALTIME, &t2_inner);
//      double time_span_inner = (double) ((t2_inner.tv_sec - t1_inner.tv_sec)
//              * 1000)
//              + ((double) (t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
//      Logger::console("curren search done in %.3f milliseconds",
//              time_span_inner);

		if (!hasRes) {
			empty++;
		}
	}

	clock_gettime(CLOCK_REALTIME, &t2);

	double time_span = (double) ((t2.tv_sec - t1.tv_sec) * 1000)
			+ ((double) (t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

	Logger::console("Type: %s", type.c_str());
	Logger::console("Searched %d queries in %.3f milliseconds",
			keywordVector.size(), time_span);
	Logger::console("Each query %.3f milliseconds",
			time_span / keywordVector.size());
	Logger::console("%d queries have no result.", empty);
}

// Read geo queries from file and do the search
void readGeoQueriesAndDoQueries(string path, string type,
		const Analyzer *analyzer, IndexSearcher *indexSearcher, unsigned ed,
		srch2::instantsearch::TermType termType) {
	string line;

	ifstream queries(path.c_str());
	vector < string > queryVector;

	unsigned counter = 0;

	while (getline(queries, line) && counter < MAX_QUERY_NUMBER) {
		queryVector.push_back(line);
		counter++;
	}

	queries.close();

	timespec t1;
	timespec t2;

	int empty = 0;

	float radius = 0.5;

	clock_gettime(CLOCK_REALTIME, &t1);

	for (vector<string>::iterator vectIter = queryVector.begin();
			vectIter != queryVector.end(); vectIter++) {
		int split = vectIter->find_first_of("^");

		string geo_part = vectIter->substr(split + 1);
		int geo_split = geo_part.find_first_of("+");
		float lng = atof(geo_part.substr(0, geo_split).c_str());
		float lat = atof(geo_part.substr(geo_split + 1).c_str());

//      timespec t1_inner;
//      timespec t2_inner;
//
//      Logger::console("Query: %s", (*vectIter).c_str());
//      clock_gettime(CLOCK_REALTIME, &t1_inner);

		bool hasRes = pingToCheckIfHasResults(analyzer, indexSearcher,
				vectIter->substr(0, split), lat - radius, lng - radius,
				lat + radius, lng + radius, ed, termType);

//      clock_gettime(CLOCK_REALTIME, &t2_inner);
//      double time_span_inner = (double) ((t2_inner.tv_sec - t1_inner.tv_sec)
//              * 1000)
//              + ((double) (t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
//      Logger::console("curren search done in %.3f milliseconds",
//              time_span_inner);

		if (!hasRes) {
			//cout << "Query " << *vectIter << " has no results" << endl;
			empty++;
		}
	}

	clock_gettime(CLOCK_REALTIME, &t2);

	double time_span = (double) ((t2.tv_sec - t1.tv_sec) * 1000)
			+ ((double) (t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

	Logger::console("Type: %s", type.c_str());
	Logger::console("Searched %d queries in %.3f milliseconds",
			queryVector.size(), time_span);
	Logger::console("Each query %.3f milliseconds",
			time_span / queryVector.size());
	Logger::console("%d queries have no result.", empty);
}

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jint Java_com_srch2_mobile_ndksearch_Srch2Lib_buildIndex(JNIEnv* env,
		jobject javaThis, jstring dataFile, jstring indexPath, jboolean isGeo) {
	const char *nativeStringDataFile = env->GetStringUTFChars(dataFile, NULL);
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexPath, NULL);
	//return buildIndex(nativeStringDataFile, nativeStringIndexPath);
	return 0;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jstring Java_com_srch2_mobile_ndksearch_Srch2Lib_query(JNIEnv* env,
		jobject javaThis, jstring queryFile, jstring indexPath,
		jboolean isGeo) {
	int num = 1024;
	char str[100];
	sprintf(str, "Hello from query code! = %d", num);
	return env->NewStringUTF(str);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_scalabilityTestBuildConfigurableLines(
		JNIEnv* env, jobject javaThis, jstring testFileDir, jstring indexDir,
		jstring logfile, jint lineLimit, jboolean isGeo) {
	Logger::console("Test BuildConfigurableLines begins. lines: %d", lineLimit);
	const char *nativeStringDataFile = env->GetStringUTFChars(testFileDir,
			NULL);
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);
	const char *nativeStringLogFile = env->GetStringUTFChars(logfile, NULL);
	FILE* fpLog = fopen(nativeStringLogFile, "a");
	Logger::setOutputFile(fpLog);

//	char cachedStrIndexPath[1024];
//	char cachedStrDataFile[1024];
//	strcpy(cachedStrIndexPath, nativeStringIndexPath);
//	strcpy(cachedStrDataFile, nativeStringDataFile);
//
//	string strIndexPath(cachedStrIndexPath);
//	string strTestFile(cachedStrDataFile);

	string strIndexPath(nativeStringIndexPath);
	string strTestFile(nativeStringDataFile);

	srch2::instantsearch::TermType termType = PREFIX;

	if (!isGeo) {
		string data_file = strTestFile + "/data.txt";

		Logger::console("Read data from %s", data_file.c_str());
		Logger::console("Save index to %s", strIndexPath.c_str());

		buildIndex(data_file, strIndexPath, lineLimit);
	} else {
		string geo_data_file = strTestFile + "/geo_data.txt";

		Logger::console("Read data from %s", geo_data_file.c_str());
		Logger::console("Save index to %s", strIndexPath.c_str());

		buildGeoIndex(geo_data_file, strIndexPath, lineLimit);
	}
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_scalabilityTest(
		JNIEnv* env, jobject javaThis, jstring testFileDir, jstring indexDir,
		jstring logfile, jboolean isGeo) {
	Logger::console("Test begins.");
	const char *nativeStringDataFile = env->GetStringUTFChars(testFileDir,
			NULL);
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);
	const char *nativeStringLogFile = env->GetStringUTFChars(logfile, NULL);
	FILE* fpLog = fopen(nativeStringLogFile, "a");
	Logger::setOutputFile(fpLog);

	char cachedStrIndexPath[1024];
	char cachedStrDataFile[1024];
	strcpy(cachedStrIndexPath, nativeStringIndexPath);
	strcpy(cachedStrDataFile, nativeStringDataFile);

	string strIndexPath(cachedStrIndexPath);
	string strTestFile(cachedStrDataFile);

	srch2::instantsearch::TermType termType = PREFIX;

	if (!isGeo) {
		string data_file = strTestFile + "/data.txt";

		Logger::console("Read data from %s", data_file.c_str());
		Logger::console("Save index to %s", strIndexPath.c_str());

		buildIndex(data_file, strIndexPath, 5000000);
	} else {
		string geo_data_file = strTestFile + "/geo_data.txt";

		Logger::console("Read data from %s", geo_data_file.c_str());
		Logger::console("Save index to %s", strIndexPath.c_str());

		buildGeoIndex(geo_data_file, strIndexPath, 5000000);
	}

	clock_t begin = clock();
	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, strIndexPath, "");
	Indexer *index = Indexer::load(indexMetaData);
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	Logger::console("Index loaded. time spend : %.5f seconds",
			getTimeSpan(begin));

	const Analyzer *analyzer = index->getAnalyzer();

	if (!isGeo) {
		warmUp(analyzer, indexSearcher);

		string single_exact_keywords_file = strTestFile + "/single_exact";
		string double_exact_keywords_file = strTestFile + "/double_exact";
		string single_fuzzy_keywords_file = strTestFile + "/single_fuzzy";
		string double_fuzzy_keywords_file = strTestFile + "/double_fuzzy";

		Logger::console("Read single exact queries keywords from %s",
				single_exact_keywords_file.c_str());
		Logger::console("Read double exact queries keywords from %s",
				double_exact_keywords_file.c_str());
		Logger::console("Read single fuzzy queries keywords from %s",
				single_fuzzy_keywords_file.c_str());
		Logger::console("Read double fuzzy queries keywords from %s",
				double_fuzzy_keywords_file.c_str());
		readQueriesAndDoQueries(single_exact_keywords_file, "single exact",
				analyzer, indexSearcher, 0, termType);
		readQueriesAndDoQueries(double_exact_keywords_file, "double exact",
				analyzer, indexSearcher, 0, termType);
		readQueriesAndDoQueries(single_fuzzy_keywords_file, "single fuzzy",
				analyzer, indexSearcher, 1, termType);
		readQueriesAndDoQueries(double_fuzzy_keywords_file, "double fuzzy",
				analyzer, indexSearcher, 1, termType);
		Logger::console("query mem usage %d kb. ", getRAMUsageValue());
	} else {
		warmUpGeo(analyzer, indexSearcher);

		string geo_single_exact_keywords_file = strTestFile
				+ "/single_geo_exact";
		string geo_double_exact_keywords_file = strTestFile
				+ "/double_geo_exact";
		string geo_single_fuzzy_keywords_file = strTestFile
				+ "/single_geo_fuzzy";
		string geo_double_fuzzy_keywords_file = strTestFile
				+ "/double_geo_fuzzy";

		Logger::console("Read single exact geo queries keywords from %s",
				geo_single_exact_keywords_file.c_str());
		Logger::console("Read double exact geo queries keywords from %s",
				geo_double_exact_keywords_file.c_str());
		Logger::console("Read single fuzzy geo queries keywords from %s",
				geo_single_fuzzy_keywords_file.c_str());
		Logger::console("Read double exact geo queries keywords from %s",
				geo_double_fuzzy_keywords_file.c_str());

		readGeoQueriesAndDoQueries(geo_single_exact_keywords_file,
				"single exact geo", analyzer, indexSearcher, 0, termType);
		readGeoQueriesAndDoQueries(geo_double_exact_keywords_file,
				"double exact geo", analyzer, indexSearcher, 0, termType);

		readGeoQueriesAndDoQueries(geo_single_fuzzy_keywords_file,
				"single fuzzy geo", analyzer, indexSearcher, 1, termType);
		readGeoQueriesAndDoQueries(geo_double_fuzzy_keywords_file,
				"double fuzzy geo", analyzer, indexSearcher, 1, termType);
		Logger::console("query mem usage %d kb. ", getRAMUsageValue());
	}

	env->ReleaseStringUTFChars(testFileDir, nativeStringDataFile);
	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);

	delete indexSearcher;
	delete index;
	delete indexMetaData;
	if (fpLog) {
		fclose(fpLog);
	}
	Logger::console("End");
}
#ifdef __cplusplus
}
#endif
