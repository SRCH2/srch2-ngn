//$Id: Serialization_on_Running_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

unsigned mergeEveryNSeconds = 1;
unsigned mergeEveryMWrites = 5;
unsigned updateHistogramEveryPMerges = 1;
unsigned updateHistogramEveryQWrites = 5;

Indexer *buildIndex(string data_file, string index_dir, string expression, vector<pair<string, string> > &records_in_index)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
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

        pair<string, string> newp;
        while(getline(lineStream,cell,'^') && cellCounter < 4 )
        {
            if (cellCounter == 0)
            {
                newp.first = cell;
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                newp.second = cell;
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                newp.second += " " + cell;
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }

            cellCounter++;
        }

        records_in_index.push_back(newp);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void updateAndSaveIndex(Indexer *indexer, Analyzer* analyzer, string data_file, vector<pair<string, string> > &records_in_index)
{
    Record *record = new Record(indexer->getSchema());

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

        pair<string, string> newp;
        while(getline(lineStream,cell,'^') && cellCounter < 4 )
        {
            if (cellCounter == 0)
            {
                newp.first = cell;
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                newp.second = cell;
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                newp.second += " " + cell;
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }

            cellCounter++;
        }

        records_in_index.push_back(newp);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Added:" << docsCounter << endl;

    indexer->save();

    data.close();
}

Indexer *buildGeoIndex(string data_file, string index_dir, string expression, vector<pair<pair<string, Point>, string> > &records_in_index)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
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

        pair<pair<string, Point>, string> newp;
        while(getline(lineStream,cell,'^') && cellCounter < 6 )
        {
            if (cellCounter == 0)
            {
                newp.first.first = cell;
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                newp.second = cell;
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                newp.second += " " + cell;
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                lat = atof(cell.c_str());
            }
            else if (cellCounter == 5)
            {
                lng = atof(cell.c_str());
            }

            cellCounter++;
        }

        newp.first.second.x = lat;
        newp.first.second.y = lng;
        records_in_index.push_back(newp);

        record->setLocationAttributeValue(lat, lng);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void updateAndSaveGeoIndex(Indexer *indexer,Analyzer* analyzer, string data_file, vector<pair<pair<string, Point>, string> > &records_in_index)
{
    Record *record = new Record(indexer->getSchema());

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

        pair<pair<string, Point>, string> newp;
        while(getline(lineStream,cell,'^') && cellCounter < 6 )
        {
            if (cellCounter == 0)
            {
                newp.first.first = cell;
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                newp.second = cell;
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                newp.second += " " + cell;
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                lat = atof(cell.c_str());
            }
            else if (cellCounter == 5)
            {
                lng = atof(cell.c_str());
            }

            cellCounter++;
        }

        newp.first.second.x = lat;
        newp.first.second.y = lng;
        records_in_index.push_back(newp);

        record->setLocationAttributeValue(lat, lng);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs added:" << docsCounter << endl;

    indexer->save();

    data.close();
}

void validateDefaultIndex(const Analyzer *analyzer, IndexSearcher *indexSearcher, vector<pair<string, string> > &records_in_index)
{
    int k = 10;

    for (int i=0; i<records_in_index.size(); i++)
    {
        bool ifFound = false;
        ifFound = existsInTopK(analyzer, indexSearcher, records_in_index[i].second, records_in_index[i].first, k);
        ASSERT(ifFound);
    }

}

void validateGeoIndex(const Analyzer *analyzer, IndexSearcher *indexSearcher, vector<pair<pair<string, Point>, string> > &records_in_index)
{
    int k = 10;

    for (int i=0; i<records_in_index.size(); i++)
    {
        bool ifFound = false;
        float lb_lat = records_in_index[i].first.second.x - 0.5;
        float lb_lng = records_in_index[i].first.second.y - 0.5;
        float rt_lat = records_in_index[i].first.second.x + 0.5;
        float rt_lng = records_in_index[i].first.second.y + 0.5;
        ifFound = existsInTopKGeo(analyzer, indexSearcher, records_in_index[i].second, records_in_index[i].first.first, k, lb_lat, lb_lng, rt_lat, rt_lng);
        ASSERT(ifFound);
    }

}

void testDefaultIndex(string index_dir)
{
    vector<pair<string, string> > records_in_index;

    // load and validate the initial index

    Indexer *indexer = buildIndex(index_dir+"/data/init", index_dir, "idf_score*doc_boost", records_in_index);

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

    Analyzer *analyzer = getAnalyzer();

    validateDefaultIndex(analyzer, indexSearcher, records_in_index);
    cout << "Init Default Index Validated." << endl;

    // update the index and serialize it

    updateAndSaveIndex(indexer, analyzer, index_dir+"/data/update", records_in_index);
    sleep(mergeEveryNSeconds + 1);

    // load the index again and validate it

    Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");

    Indexer *indexerLoaded = Indexer::load(indexMetaData);
    IndexSearcher *indexSearcherLoaded = IndexSearcher::create(indexerLoaded);

    Analyzer *analyzerLoaded = getAnalyzer();

    validateDefaultIndex(analyzerLoaded, indexSearcherLoaded, records_in_index);

    cout << "Loaded Default Index Validated." << endl;

    delete indexSearcher;
    delete indexer;
    delete indexSearcherLoaded;
    delete indexerLoaded;

    cout << "Default Index Pass." << endl;
}

void testGeoIndex(string index_dir)
{
    vector<pair<pair<string, Point>, string> > records_in_index;

    // load and validate the initial index

    Indexer *indexer = buildGeoIndex(index_dir+"/data/init", index_dir, "idf_score*doc_boost", records_in_index);

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

    Analyzer *analyzer = getAnalyzer();

    validateGeoIndex(analyzer, indexSearcher, records_in_index);
    cout << "Init Geo Index Validated." << endl;

    // update the index and serialize it

    updateAndSaveGeoIndex(indexer, analyzer, index_dir+"/data/update", records_in_index);
    sleep(mergeEveryNSeconds + 1);

    // load the index again and validate it

    Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");

    Indexer *indexerLoaded = Indexer::load(indexMetaData);
    IndexSearcher *indexSearcherLoaded = IndexSearcher::create(indexerLoaded);

    Analyzer *analyzerLoaded = getAnalyzer();

    validateGeoIndex(analyzerLoaded, indexSearcherLoaded, records_in_index);

    cout << "Loaded Geo Index Validated." << endl;

    delete indexSearcher;
    delete indexer;
    delete indexSearcherLoaded;
    delete indexerLoaded;

    cout << "Geo Index Pass." << endl;
}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");

    testDefaultIndex(index_dir);
    testGeoIndex(index_dir);

    return 0;
}
