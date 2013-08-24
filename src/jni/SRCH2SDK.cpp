#include "SRCH2SDK.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#include "analyzer/StandardAnalyzer.h"
#include "util/Logger.h"
#include "PerformanceTool.h"

#define CHECK_DELETE(ptr)	{if(ptr!=NULL){delete ptr; ptr=NULL;}};

using namespace srch2::instantsearch;
using namespace srch2::util;
using namespace std;

namespace srch2 {
namespace sdk {
const int mergeEveryNSeconds = 3;
const int mergeEveryMWrites  = 5;

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
		schema = Schema::create(LocationIndex);
	} else {
		schema = Schema::create(DefaultIndex);
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
	parseFuzzyQueryWithEdSet(analyzer, queryString, ed, query);
	int resultCount = 10;

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);
	indexSearcher->search(query, queryResults, resultCount);
	Logger::console("srch2::sdk::queryresults:%d, time spend: %2.6f milliseconds",
			queryResults->getNumberOfResults(), getTimeSpan(begin));
	delete query;
	return queryResults;
}

void parseFuzzyQueryWithEdSet(const Analyzer *analyzer, 
        const string &queryString, int ed, Query *query){
    vector<string> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    TermType termType = TERM_TYPE_COMPLETE;
    for (unsigned i = 0; i < queryKeywords.size(); ++i){
        
        Term *term;
        if(i == (queryKeywords.size()-1)){
            termType = TERM_TYPE_PREFIX;
        }

        if(ed==0){
            term = ExactTerm::create(queryKeywords[i], termType, 1, 0.5);
        } else {
            term = FuzzyTerm::create(queryKeywords[i], termType, 1, 0.5, ed);
        }
        term->addAttributeToFilterTermHits(-1);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
}

int pingForScalabilityTest(const Analyzer *analyzer, IndexSearcher *indexSearcher, 
        const string &queryString, unsigned ed){
    Query *query = new Query(srch2::instantsearch::TopKQuery);
    parseFuzzyQueryWithEdSet(analyzer, queryString, ed, query);
    int resultCount = 10;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = QueryResults::create(indexSearcher, query);

    indexSearcher->search(query, queryResults, resultCount);
    int returnValue =  queryResults->getNumberOfResults();
    delete queryResults;
    delete query;
    return returnValue;
}

int pingToCheckIfHasResults(const Analyzer *analyzer, IndexSearcher *indexSearcher, 
        string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng, int ed){
    Query *query = new Query(srch2::instantsearch::MapQuery);

    vector<string> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    TermType termType = TERM_TYPE_COMPLETE;
    for (unsigned i = 0; i < queryKeywords.size(); ++i){
        Term *term = NULL;
        if(i == (queryKeywords.size()-1)){
            termType = TERM_TYPE_PREFIX;
        }
        if (ed>0)
            term = FuzzyTerm::create(queryKeywords[i], termType, 1, 0.5, ed);
        else
            term = ExactTerm::create(queryKeywords[i], termType, 1, 0.5);
        term->addAttributeToFilterTermHits(-1);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }

    query->setRange(lb_lat, lb_lng, rt_lat, rt_lng);

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = QueryResults::create(indexSearcher, query);

    indexSearcher->search(query, queryResults);

    int returnValue =  queryResults->getNumberOfResults();
    delete queryResults;
    delete query;
    return returnValue;
}
}
}
