//$Id: Scalability_Test.cpp 3529 2013-07-02 10:56:21Z jiaying $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

//#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// Read data from file, build the index, and save the index to disk
void buildIndex(string dataFile, string indexDir) {
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, indexDir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(dataFile.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while (getline(data,line)) {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;

        while (getline(lineStream,cell,'^') && cellCounter < 2 ) {
            if (cellCounter == 0) {
                record->setPrimaryKey(cell.c_str());
            } else {
                record->setSearchableAttributeValue(0, cell);
            }

            cellCounter++;
        }

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();
    indexer->save();

    cout << "Index saved." << endl;

    data.close();

    delete indexer;
    delete indexMetaData;
    delete analyzer;
    delete schema;
}

// Read data from file, build the index, and save the index to disk
void buildGeoIndex(string dataFile, string indexDir) {
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, indexDir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(dataFile.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while (getline(data,line)) {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;
        float lat=0.0, lng=0.0;

        while (getline(lineStream,cell,'^') && cellCounter < 4 ) {
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

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#GeoDocs Read:" << docsCounter << endl;

    indexer->commit();
    indexer->save();

    cout << "Index saved." << endl;

    data.close();

    delete indexer;
    delete indexMetaData;
    delete analyzer;
    delete schema;
}

// Warm up the index, so that the first query in the test won't be slow
void warmUp(const Analyzer *analyzer, IndexSearcher *indexSearcher) {
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1);
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1);
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1);
}

// Warm up the geo index, so that the first query in the test won't be slow
void warmUpGeo(const Analyzer *analyzer, IndexSearcher *indexSearcher) {
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1);
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1);
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1);
}

// Read queries from file and do the search
void readQueriesAndDoQueries(bool isExact, string path, const Analyzer *analyzer, IndexSearcher *indexSearcher, unsigned ed, int verb) {
    string line;

    ifstream keywords(path.c_str());
    vector<string> keywordVector;

    unsigned counter = 0;

    while (getline(keywords, line)) {
        keywordVector.push_back(line);
        counter++;
    }

    keywords.close();

    timespec t1;
    timespec t2;

    int empty = 0;

    if (isExact) {//check if the query is exact query.
        ed = 0;
    }

    clock_gettime(CLOCK_REALTIME, &t1);

    for (vector<string>::iterator vectIter = keywordVector.begin(); vectIter!= keywordVector.end(); vectIter++ ) {
        timespec t1_inner;
        timespec t2_inner;

        if (verb >= 2) {
            //cout << "Query: " << *vectIter << endl;
            clock_gettime(CLOCK_REALTIME, &t1_inner);
        }

        int resultNumber= 0;

        resultNumber = pingForScalabilityTest(analyzer, indexSearcher, *vectIter, ed);

        if (verb >= 2) {
            clock_gettime(CLOCK_REALTIME, &t2_inner);
            double timeSpentOnOneQuery = (double)((t2_inner.tv_sec - t1_inner.tv_sec) * 1000) + ((double)(t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
            //TODO: revert this part
            cout << "current search:  "<<*vectIter << "------" << timeSpentOnOneQuery << " milliseconds." << " Find "<< resultNumber << " resutls."<< endl;
            //cout << "current search:  "<<*vectIter << "------" << time_span_inner << " milliseconds." << endl;
        }
        if (!resultNumber) {
            if (verb >= 1) {
                //TODO: revert this part
                //cout << "Query " << *vectIter << " has no results" << endl;
            }
            empty++;
        }
    }
    clock_gettime(CLOCK_REALTIME, &t2);

    double time_span = (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

    cout << "---- Default Index ---------------------------------------------------" << endl;
    cout << "Searched " << keywordVector.size() << " queries in " << time_span << " milliseconds." << endl;
    cout << "Each query " << time_span / keywordVector.size() << " milliseconds." << endl;
    cout << empty << " queries have no result." << endl;
    cout << "-------------------------------------------------------" << endl;
}

// Read geo queries from file and do the search
void readGeoQueriesAndDoQueries(bool isExact, string path, const Analyzer *analyzer, IndexSearcher *indexSearcher, unsigned ed,int verb, float radius) {
    string line;

    ifstream queries(path.c_str());
    vector<string> queryVector;

    unsigned counter = 0;

    while (getline(queries, line)) {
        queryVector.push_back(line);
        counter++;
    }

    queries.close();

    timespec t1;
    timespec t2;

    int empty = 0;

    if (isExact) { //check if the query is exact query.
        ed = 0;
    }

    clock_gettime(CLOCK_REALTIME, &t1);

    for ( vector<string>::iterator vectIter = queryVector.begin(); vectIter!= queryVector.end(); vectIter++ ) {
        int split = vectIter->find_first_of("^");

        string geo_part = vectIter->substr(split+1);
        int geo_split = geo_part.find_first_of("+");
        float lng = atof(geo_part.substr(0,geo_split).c_str());
        float lat = atof(geo_part.substr(geo_split+1).c_str());

        timespec t1_inner;
        timespec t2_inner;

        if (verb >= 2) {
            //cout << "Query: " << *vectIter << endl;
            clock_gettime(CLOCK_REALTIME, &t1_inner);
        }

        int resultsNumber=0;

        resultsNumber = pingToCheckIfHasResults(analyzer, indexSearcher, vectIter->substr(0,split), lat-radius, lng-radius, lat+radius, lng+radius, ed);

        if (verb >=2) {
            clock_gettime(CLOCK_REALTIME, &t2_inner);
            double timeSpentOnOneQuery = (double)((t2_inner.tv_sec - t1_inner.tv_sec) * 1000) + ((double)(t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
            cout << "current search: "<<*vectIter << "------"<< timeSpentOnOneQuery << " milliseconds." << " Find "<< resultsNumber << " resutls."<< endl;
        }

        if (resultsNumber == 0) {
            if (verb >= 1) {
                cout << "Query " << *vectIter << " has no results" << endl;
            }
            empty++;
        }
    }
    clock_gettime(CLOCK_REALTIME, &t2);

    double time_span = (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

    cout << "---- Locational Index ---------------------------------------------------" << endl;
    cout << "Searched " << queryVector.size() << " queries in " << time_span << " milliseconds." << endl;
    cout << "Each query " << time_span / queryVector.size() << " milliseconds." << endl;
    cout << empty << " queries have no result." << endl;
    cout << "-------------------------------------------------------" << endl;
}

/*
 * testsearch: search query
 * input:
 *     index_dir: index and query's dir
 *     isExact: exact or fuzzy query
 *     ed: edit distance
 *     verb: verb_level
 *         0: for performance test, no detail output;
 *         1: output queries without results
 *         2: output all queries results
 *     radius: radius for geo data, which will work only when isGeo=true
 *     isGeo: if it's geo data
 */
void testSearch(const string& dataFile, const string& indexDir, const string& queryFile, bool isExact, int ed, int verb, float radius, bool isGeo) {
    if (!isGeo) {
        cout << "Read data from " << dataFile << endl;
        cout << "Save index to " << indexDir << endl;

        buildIndex(dataFile, indexDir);
    } else {
        string geoDataFile = indexDir + "/geo_data.txt";

        cout << "Read data from " << geoDataFile << endl;
        cout << "Save index to " << indexDir << endl;

        buildGeoIndex(geoDataFile, indexDir);
    }

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;

    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, indexDir, "");
    Indexer *index = Indexer::load(indexMetaData);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);

    cout << "Index loaded." << endl;

    Analyzer *analyzer = getAnalyzer();

    if (!isGeo) {
        warmUp(analyzer, indexSearcher);
        readQueriesAndDoQueries(isExact, queryFile, analyzer, indexSearcher, ed, verb);
    } else {
        warmUpGeo(analyzer, indexSearcher);
        readGeoQueriesAndDoQueries(isExact, queryFile, analyzer, indexSearcher, ed, verb, radius);
    }

    delete indexSearcher;
    delete index;
    delete indexMetaData;
    delete analyzer;

}
int main(int argc, char **argv) {
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;
    string indexDir = getenv("index_dir");

    if (argc < 5 || argc > 6) {
        cout << "Error: parameters are not set properly."<<endl;
        exit(1);
    }
    string datafile = argv[1];
    string queryfile = argv[2];
    int ed = atoi(argv[3]);//edit_distance
    int verb = atoi(argv[4]);
    float radius = 0.5;//for geo search
    if (argc == 6)
        radius = atof(argv[5]);

    //do geo search
    if (argc==6) {
        testSearch(datafile, indexDir, queryfile, false, ed, verb, radius, true); //fuzzy geo
    } else {
        testSearch(datafile, indexDir, queryfile, false, ed, verb, radius, false);
    }


    return 0;
}
