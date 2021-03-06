/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// This test will first build an index with 4809 records, commit, then insert one more record.
// After that, the test will query a keyword in the inserted record to see if top10 results is consistent with top25 results.
// The insert record should appear in both top10 and top25 results.

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
    schema->setScoringExpression("idf_score*doc_boost");

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "",
                                      srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 2;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites, index_dir);
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

        while(getline(lineStream,cell,'^') && cellCounter < 3 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else
            {
                float recordBoost = atof(cell.c_str());
                record->setRecordBoost(recordBoost);
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
void updateIndex(string data_file, Indexer *index, Analyzer* analyzer)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);
    schema->setScoringExpression("idf_score*doc_boost");

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

        while(getline(lineStream,cell,'^') && cellCounter < 3 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else
            {
                float recordBoost = atof(cell.c_str());
                record->setRecordBoost(recordBoost);
            }

            cellCounter++;
        }

        index->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#New Docs Inserted:" << docsCounter << endl;

    sleep(4);
    cout << "Index updated." << endl;

    data.close();

    delete schema;
}

// Read queries from file and do the search
void compareTopK1andTopK2(string path, const Analyzer *analyzer, QueryEvaluator *queryEvaluator, const unsigned k1, const unsigned k2)
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
        if(!topK1ConsistentWithTopK2(analyzer, queryEvaluator, *vectIter, k1, k2))
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
    Indexer *index = Indexer::load(indexMetaData);

    cout << "Index loaded." << endl;

    Analyzer *analyzer = getAnalyzer();

    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);

    // test pagination before update
//    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 10);
//    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 15);
//    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 20);
//    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 40);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 20);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 25);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 30);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 30);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 35);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 40);

    updateIndex(update_data_file, index, analyzer);
    
    // test pagination after update
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 10);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 15);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 20);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 5, 40);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 20);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 25);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 10, 30);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 30);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 35);
    compareTopK1andTopK2(query_file, analyzer, queryEvaluator, 20, 40);

    delete queryEvaluator;
    delete index;
    delete indexMetaData;
    delete analyzer;

    return 0;
}
