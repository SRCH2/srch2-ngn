#include "Srch2Android.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#include "analyzer/StandardAnalyzer.h"
#include "util/Logger.h"

#define CHECK_DELETE(ptr)	{if(ptr!=NULL){delete ptr; ptr=NULL;}};

using namespace std;
using srch2::util::Logger;

namespace srch2 {
namespace sdk {

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

string printQueryResult(QueryResults* queryResults, Indexer* indexer) {
	vector<unsigned> attributeBitmaps;
	vector < vector<unsigned> > attributes;
	vector < string > matchedKeywords;
	Logger::console("Result count:%d", queryResults->getNumberOfResults());

	stringstream ss;
	string recordStr;
	int icount = queryResults->getNumberOfResults();
	for (int i = 0; i < icount; i++) {
		queryResults->getMatchedAttributeBitmaps(i, attributeBitmaps);
		queryResults->getMatchingKeywords(i, matchedKeywords);
		queryResults->getMatchedAttributes(i, attributes);
		ss << "{";
		for (int j = 0; j < attributeBitmaps.size(); j++) {
			ss << matchedKeywords[j] << ",";
		}
		ss << "}";

		unsigned internalRecordId = queryResults->getInternalRecordId(i);
		recordStr = indexer->getInMemoryData(internalRecordId);
		Logger::console("record : %s", recordStr.c_str());
		ss << recordStr << endl;
	}
	Logger::console("%s", ss.str().c_str());
	return ss.str();
}

unsigned loadDefaultData(const string &dataFile, int lineLimit,
		Indexer* indexer, Schema *schema) {
	Record *record = new Record(schema);
	string line;

	ifstream data(dataFile.c_str());

	/// the file should have two fields, seperated by '^'
	/// the first field is the primary key, the second field is a searchable attribute
	unsigned cline = 0;
	while (getline(data, line)) {
		if (++cline > lineLimit) {
			cline--;
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
				record->setInMemoryData(cell);
			}
			cellCounter++;
		}
		indexer->addRecord(record, 0);
		record->clear();
	}
	data.close();
	delete record;
	return cline;
}

unsigned loadGeoData(const string &dataFile, int lineLimit, Indexer* indexer,
		Schema *schema) {
	Record *record = new Record(schema);

	string line;
	ifstream data(dataFile.c_str());

	unsigned cline = 0;
	while (getline(data, line)) {
		if (++cline > lineLimit) {
			--cline;
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
				record->setInMemoryData(cell);
			} else if (cellCounter == 2) {
				lng = atof(cell.c_str());
			} else if (cellCounter == 3) {
				lat = atof(cell.c_str());
			}
			cellCounter++;
		}
		record->setLocationAttributeValue(lat, lng);
		indexer->addRecord(record, 0);
		record->clear();
	}
	data.close();
	delete record;
	return cline;
}

// Read data from file, build the index, and save the index to disk
Indexer* createIndex(string dataFile, string indexDir, int lineLimit,
		bool isGeo) {

	Schema *schema;

	if (isGeo) {
		schema = Schema::create(srch2is::LocationIndex);
	} else {
		schema = Schema::create(srch2is::DefaultIndex);
	}

	schema->setPrimaryKey("primaryKey");
	schema->setSearchableAttribute("description", 2);

	AnalyzerInternal *analyzer = new StandardAnalyzer(
			srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, indexDir, "");
	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	clock_t timebegin = clock();
	unsigned docsCounter = 0;
	if (isGeo) {
		docsCounter = loadGeoData(dataFile, lineLimit, indexer, schema);
	} else {
		docsCounter = loadDefaultData(dataFile, lineLimit, indexer, schema);
	}

	indexer->commit();
	Logger::console("#Docs Read: %d, index commited, time spend: %.5f seconds",
			docsCounter, getTimeSpan(timebegin));

	// ? Should I delete those value? or it deleted by indexer ?
//	delete indexMetaData;
//	delete analyzer;
	return indexer;
}

void saveIndex(Indexer *indexer) {
	clock_t savingbegin = clock();
	indexer->save();
	Logger::console("Index Saving time: %.5f seconds",
			getTimeSpan(savingbegin));
	Logger::console("Index mem usage %d kb. ", getRAMUsageValue());
}

Indexer* loadIndex(const string& strIndexPath) {
	clock_t begin = clock();
	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, strIndexPath, "");
	Indexer *index = Indexer::load(indexMetaData);
	Logger::console("Index loaded. time spend : %.5f seconds",
			getTimeSpan(begin));
	return index;
}

QueryResults* query(const Analyzer* analyzer, IndexSearcher* indexSearcher,
		const string& queryString, unsigned ed,
		srch2::instantsearch::TermType termType) {
	Logger::console("srch2::sdk::query:%s", queryString.c_str());
	Query *query = new Query(srch2::instantsearch::TopKQuery);
	parseFuzzyQueryWithEdSet(analyzer, query, queryString, ed, termType);
	int resultCount = 10;

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);
	indexSearcher->search(query, queryResults, resultCount);
	Logger::console("srch2::sdk::queryresults:%d",
			queryResults->getNumberOfResults());
	delete query;
	return queryResults;
}

}
}
