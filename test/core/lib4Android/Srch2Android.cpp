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


void setStartTime(timespec *startTime){
    clock_gettime(CLOCK_REALTIME, startTime);
}

double getTimeSpan(timespec startTime){
    timespec endTime;
    clock_gettime(CLOCK_REALTIME, &endTime);
    return (double)((endTime.tv_sec - startTime.tv_sec) * 1000)
        + (double)(endTime.tv_nsec - startTime.tv_nsec) / 1000000.0;
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

void addRecord(Indexer* index, string key, string value, bool keepInMemory) {
	Record *record = new Record(index->getSchema());
	record->setPrimaryKey(key.c_str());
	record->setSearchableAttributeValue(0, value);
	if (keepInMemory) {
		record->setInMemoryData(value);
	}
	index->addRecord(record, 0);
	delete record;
}

unsigned loadDefaultData(const string &dataFile, int lineLimit,
		Indexer* indexer, const Schema *schema) {
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
		const Schema *schema) {
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
Indexer* createIndex(string indexDir, bool isGeo) {
	Schema *schema;
	if (isGeo) {
		schema = Schema::create(srch2is::LocationIndex);
	} else {
		schema = Schema::create(srch2is::DefaultIndex);
	}

	schema->setPrimaryKey("primaryKey");
	schema->setSearchableAttribute("description", 2);

	Analyzer *analyzer = new Analyzer(
			srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, 
            "", // stemmerFilePath 
            "", // stopWordFilePath
            "", // synonymFilePath
            srch2::instantsearch::SYNONYM_KEEP_ORIGIN,
            ""  // extra delimiters
            );

	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, indexDir, "");
	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);
	return indexer;
}

// Read data from file, build the index, and save the index to disk
Indexer* createIndex(string dataFile, string indexDir, int lineLimit,
		bool isGeo) {
	Indexer *indexer = createIndex(indexDir, isGeo);

    timespec timebegin;
    setStartTime(&timebegin);
	unsigned docsCounter = 0;
	if (isGeo) {
		docsCounter = loadGeoData(dataFile, lineLimit, indexer,
				indexer->getSchema());
	} else {
		docsCounter = loadDefaultData(dataFile, lineLimit, indexer,
				indexer->getSchema());
	}

	indexer->commit();
	Logger::console("#Docs Read: %d, index commited, time spend: %2.6f milliseconds",
			docsCounter, getTimeSpan(timebegin));

	return indexer;
}

void commitIndex(Indexer* indexer) {
	if (not indexer->isCommited()) {
		indexer->commit();
	}
}

void saveIndex(Indexer *indexer) {
    timespec savingbegin;
    setStartTime(&savingbegin);
	indexer->save();
	Logger::console("Index Saving time: %2.6f milliseconds",
			getTimeSpan(savingbegin));
	Logger::console("Index mem usage %d kb. ", getRAMUsageValue());
}

Indexer* loadIndex(const string& strIndexPath) {
    timespec begin ;
    setStartTime(&begin);
	IndexMetaData *indexMetaData = new IndexMetaData(new Cache(),
			mergeEveryNSeconds, mergeEveryMWrites, strIndexPath, "");
	Indexer *index = Indexer::load(indexMetaData);
	Logger::console("Index loaded. time spend : %2.6f milliseconds",
			getTimeSpan(begin));
	return index;
}

QueryResults* query(const Analyzer* analyzer, IndexSearcher* indexSearcher,
		const string& queryString, unsigned ed) {
	Logger::console("srch2::sdk::query:%s", queryString.c_str());
    timespec begin;
    setStartTime(&begin);
	Query *query = new Query(srch2::instantsearch::TopKQuery);
	parseFuzzyQueryWithEdSet(analyzer, query, queryString, ed);
	int resultCount = 10;

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);
	indexSearcher->search(query, queryResults, resultCount);
	Logger::console("srch2::sdk::queryresults:%d, time spend: %2.6f milliseconds",
			queryResults->getNumberOfResults(), getTimeSpan(begin));
	delete query;
	return queryResults;
}

}
}
