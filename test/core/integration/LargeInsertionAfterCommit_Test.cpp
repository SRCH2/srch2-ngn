//$Id: LargeInsertionAfterCommit_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

// This test case will insert a large amount of records after the index is committed,
// then try to search each inserted record.

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#include <time.h>

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
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 2;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		index_dir);
    Indexer *indexer = Indexer::create(indexMetaData, schema);

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

// Read data from file, update the index
void updateIndex(string data_file, Indexer *index)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "", srch2is::STANDARD_ANALYZER);
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
        index->addRecord(record, analyzer);
        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    index->commit();
    sleep(11);
    cout << "Index updated." << endl;

    data.close();

    delete schema;
    delete analyzer;
}
// Read queries from file and do the search
void readQueriesAndDoQueries(string path, const Analyzer *analyzer, QueryEvaluator *queryEvaluator)
{
    string line;

    ifstream keywords(path.c_str());
    vector<string> keywordVector;

    while(getline(keywords, line))
    {
        keywordVector.push_back(line);
    }

    keywords.close();

    unsigned failedCounter = 0;

    for( vector<string>::iterator vectIter = keywordVector.begin(); vectIter!= keywordVector.end(); vectIter++ )
    {
        if(pingExactTest(analyzer, queryEvaluator, *vectIter) == 0)
        {
            failedCounter++;
            cout << *vectIter << endl;
        }
    }

    cout << "-------------------------------------------------------" << endl;
    cout << "Searched " << keywordVector.size() << " queries." << endl;
    cout << "Failed queries number: " << failedCounter << endl;
    ASSERT(failedCounter == 0);
}
int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string init_data_file = index_dir + "/init_data";
    string update_data_file = index_dir + "/update_data";
    string query_file = index_dir + "/queries";

    cout << "Read init data from " << init_data_file << endl;
    cout << "Save index to " << index_dir << endl;
    cout << "Read update data from " << update_data_file << endl;
    cout << "Read queries from " << query_file << endl;

    buildIndex(init_data_file, index_dir);

    unsigned mergeEveryNSeconds = 2;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		index_dir);
    Indexer *index = Indexer::create(indexMetaData);
    index->bootStrapFromDisk();
    cout << "Index loaded." << endl;

    updateIndex(update_data_file, index);
    
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "",
                                      srch2is::STANDARD_ANALYZER);

    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);

    readQueriesAndDoQueries(query_file, analyzer, queryEvaluator);

    delete queryEvaluator;
    delete index;
    delete indexMetaData;
    delete analyzer;


	cout << "LargeInsertionAfterCommit unit test: Passed" << endl;
    return 0;
}
