//$Id: Scalability_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// Read data from file, build the index, and save the index to disk
void buildIndex(string data_file, string index_dir)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    /// Create an Analyzer
    AnalyzerInternal *analyzer = new StandardAnalyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;

        while(getline(lineStream,cell,'^') && cellCounter < 2 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else
            {
                record->setSearchableAttributeValue(0, cell);
            }

            cellCounter++;
        }

        indexer->addRecord(record, 0);

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
void buildGeoIndex(string data_file, string index_dir)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    /// Create an Analyzer
    AnalyzerInternal *analyzer = new StandardAnalyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;
        float lat=0.0, lng=0.0;

        while(getline(lineStream,cell,'^') && cellCounter < 4 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                lng = atof(cell.c_str());
            }
            else if (cellCounter == 3)
            {
                lat = atof(cell.c_str());
            }

            cellCounter++;
        }

        record->setLocationAttributeValue(lat, lng);

        indexer->addRecord(record, 0);

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
void warmUp(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, TERM_TYPE_PREFIX);
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, TERM_TYPE_PREFIX);
    pingForScalabilityTest(analyzer, indexSearcher, "aaa+bbb", 1, TERM_TYPE_PREFIX);
}

// Warm up the geo index, so that the first query in the test won't be slow
void warmUpGeo(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1, TERM_TYPE_PREFIX);
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1, TERM_TYPE_PREFIX);
    pingToCheckIfHasResults(analyzer, indexSearcher, "aaa+bbb", 40.0, -120.0, 60.0, -90.0, 1, TERM_TYPE_PREFIX);
}

// Read queries from file and do the search
void readQueriesAndDoQueries(string path, string type, const Analyzer *analyzer, IndexSearcher *indexSearcher, unsigned ed, srch2::instantsearch::TermType termType)
{
    string line;

    ifstream keywords(path.c_str());
    vector<string> keywordVector;

    unsigned counter = 0;

    while(getline(keywords, line) && counter < MAX_QUERY_NUMBER)
    {
        keywordVector.push_back(line);
        counter++;
    }

    keywords.close();

    timespec t1;
    timespec t2;

    int empty = 0;

    clock_gettime(CLOCK_REALTIME, &t1);
    
    for( vector<string>::iterator vectIter = keywordVector.begin(); vectIter!= keywordVector.end(); vectIter++ )
    {
        timespec t1_inner;
        timespec t2_inner;

        cout << "Query: " << *vectIter << endl;
        clock_gettime(CLOCK_REALTIME, &t1_inner);

        bool hasRes = pingForScalabilityTest(analyzer, indexSearcher, *vectIter, ed, termType);

        clock_gettime(CLOCK_REALTIME, &t2_inner);
        double time_span_inner = (double)((t2_inner.tv_sec - t1_inner.tv_sec) * 1000) + ((double)(t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
        cout << "curren search done in " << time_span_inner << " milliseconds." << endl;

        if (!hasRes)
        {
            //cout << "Query " << *vectIter << " has no results" << endl;
            empty++;
        }
    }

    clock_gettime(CLOCK_REALTIME, &t2);

    double time_span = (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

    cout << "---- Default Index ---------------------------------------------------" << endl;
    cout << "Type: " << type << endl;
    cout << "Searched " << keywordVector.size() << " queries in " << time_span << " milliseconds." << endl;
    cout << "Each query " << time_span / keywordVector.size() << " milliseconds." << endl;
    cout << empty << " queries have no result." << endl;
}

// Read geo queries from file and do the search
void readGeoQueriesAndDoQueries(string path, string type, const Analyzer *analyzer, IndexSearcher *indexSearcher, unsigned ed, srch2::instantsearch::TermType termType)
{
    string line;

    ifstream queries(path.c_str());
    vector<string> queryVector;

    unsigned counter = 0;

    while(getline(queries, line) && counter < MAX_QUERY_NUMBER)
    {
        queryVector.push_back(line);
        counter++;
    }

    queries.close();

    timespec t1;
    timespec t2;

    int empty = 0;

    float radius = 0.5;

    clock_gettime(CLOCK_REALTIME, &t1);
    
    for( vector<string>::iterator vectIter = queryVector.begin(); vectIter!= queryVector.end(); vectIter++ )
    {
        int split = vectIter->find_first_of("^");

        string geo_part = vectIter->substr(split+1);
        int geo_split = geo_part.find_first_of("+");
        float lng = atof(geo_part.substr(0,geo_split).c_str());
        float lat = atof(geo_part.substr(geo_split+1).c_str());

        timespec t1_inner;
        timespec t2_inner;

        cout << "Query: " << *vectIter << endl;
        clock_gettime(CLOCK_REALTIME, &t1_inner);

        bool hasRes = pingToCheckIfHasResults(analyzer, indexSearcher, vectIter->substr(0,split), lat-radius, lng-radius, lat+radius, lng+radius, ed, termType);

        clock_gettime(CLOCK_REALTIME, &t2_inner);
        double time_span_inner = (double)((t2_inner.tv_sec - t1_inner.tv_sec) * 1000) + ((double)(t2_inner.tv_nsec - t1_inner.tv_nsec)) / 1000000.0;
        cout << "curren search done in " << time_span_inner << " milliseconds." << endl;

        if (!hasRes)
        {
            //cout << "Query " << *vectIter << " has no results" << endl;
            empty++;
        }
    }

    clock_gettime(CLOCK_REALTIME, &t2);

    double time_span = (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

    cout << "---- Locational Index ---------------------------------------------------" << endl;
    cout << "Type: " << type << endl;
    cout << "Searched " << queryVector.size() << " queries in " << time_span << " milliseconds." << endl;
    cout << "Each query " << time_span / queryVector.size() << " milliseconds." << endl;
    cout << empty << " queries have no result." << endl;
}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");

    bool isGeo = true;
    srch2::instantsearch::TermType termType = TERM_TYPE_PREFIX;

    if (!isGeo)
    {
        string data_file = index_dir + "/data.txt";
    
        cout << "Read data from " << data_file << endl;
        cout << "Save index to " << index_dir << endl;
    
        buildIndex(data_file, index_dir);
    }
    else
    {
        string geo_data_file = index_dir + "/geo_data.txt";
        
        cout << "Read data from " << geo_data_file << endl;
        cout << "Save index to " << index_dir << endl;

        buildGeoIndex(geo_data_file, index_dir);
    }


    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;

    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
    Indexer *index = Indexer::load(indexMetaData);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);

    cout << "Index loaded." << endl;

    const Analyzer *analyzer = index->getAnalyzer();

    if (!isGeo)
    {
        warmUp(analyzer, indexSearcher);
    
        //string single_exact_keywords_file = index_dir + "/single_exact";
        //string double_exact_keywords_file = index_dir + "/double_exact";
        string single_fuzzy_keywords_file = index_dir + "/single_fuzzy";
        string double_fuzzy_keywords_file = index_dir + "/double_fuzzy";

        //cout << "Read single exact queries keywords from " << single_exact_keywords_file << endl;
        //cout << "Read double exact queries keywords from " << double_exact_keywords_file << endl;
        cout << "Read single fuzzy queries keywords from " << single_fuzzy_keywords_file << endl;
        cout << "Read double fuzzy queries keywords from " << double_fuzzy_keywords_file << endl;
    
        //readQueriesAndDoQueries(double_exact_keywords_file, "double exact", analyzer, indexSearcher, 0, termType);
        //readQueriesAndDoQueries(single_exact_keywords_file, "single exact", analyzer, indexSearcher, 0, termType);
        readQueriesAndDoQueries(single_fuzzy_keywords_file, "single fuzzy", analyzer, indexSearcher, 1, termType);
        readQueriesAndDoQueries(double_fuzzy_keywords_file, "double fuzzy", analyzer, indexSearcher, 1, termType);
    }
    else
    {
        warmUpGeo(analyzer, indexSearcher);
    
        string geo_single_fuzzy_keywords_file = index_dir + "/single_fuzzy_geo";
        string geo_double_fuzzy_keywords_file = index_dir + "/double_fuzzy_geo";

        cout << "Read single fuzzy geo queries keywords from " << geo_single_fuzzy_keywords_file << endl;
        cout << "Read double fuzzy geo queries keywords from " << geo_double_fuzzy_keywords_file << endl;

        readGeoQueriesAndDoQueries(geo_single_fuzzy_keywords_file, "single fuzzy geo", analyzer, indexSearcher, 1, termType);
        readGeoQueriesAndDoQueries(geo_double_fuzzy_keywords_file, "double fuzzy geo", analyzer, indexSearcher, 1, termType);
    }

    delete indexSearcher;
    delete index;
    delete indexMetaData;

    return 0;
}
