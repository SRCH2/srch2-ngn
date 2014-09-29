//$Id: Scoring_Test.cpp 2988 2012-11-29 01:21:15Z oliverax $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"
#include "util/RecordSerializerUtil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#define MAX_QUERY_NUMBER 5000

using namespace std;
using namespace srch2::util;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

Indexer *buildIndex(string data_file, string index_dir, string expression, map<string, int> &sort1Map, map<string, float> &sort2Map)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);


    schema->setRefiningAttribute("id_for_sort",srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "0" );
    schema->setRefiningAttribute("latitude", srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT, "1" );

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "",
                                      srch2::instantsearch::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		index_dir);
    Indexer *indexer = Indexer::create(indexMetaData, schema);

    Record *record = new Record(schema);

    Schema * storedSchema = Schema::create();
    RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    RecordSerializer recSerializer(*storedSchema);

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

        while(getline(lineStream,cell,'^') && cellCounter < 5 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
                record->setRefiningAttributeValue(0, cell); // use the id number to be one sortable attribute
                recSerializer.addRefiningAttribute(0, atoi(cell.c_str()));
                sort1Map[cell] = atoi(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
                recSerializer.addSearchableAttribute(0, cell);
            }
            else if (cellCounter == 2)
            {
                record->setSearchableAttributeValue(1, cell);
                recSerializer.addSearchableAttribute(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                record->setRefiningAttributeValue(1, cell); // use the latitude to be another sortable attribute
                float latitude = atof(cell.c_str());
                recSerializer.addRefiningAttribute(1, latitude);
                sort2Map[record->getPrimaryKey()] = latitude;
            }

            cellCounter++;
        }

        RecordSerializerBuffer compactBuffer = recSerializer.serialize();
        record->setInMemoryData(compactBuffer.start, compactBuffer.length);
        recSerializer.nextRecord();

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void validateIntSortableAttrDescending(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, bool descending, const map<string, int> &sortMap)
{
    vector<string> recordIds;

    // sort use the id number
    getGetAllResultsQueryResults(analyzer, queryEvaluator, "Professional", descending, recordIds, -1, 0);

    // there are 139 records containing the word "professional" in the data.
    ASSERT(recordIds.size()==139);

    // validate the order
    for (int i=1; i<recordIds.size(); i++)
    {
        if (descending)
        {
            ASSERT( sortMap.at(recordIds[i-1]) >= sortMap.at(recordIds[i]) );
        }
        else
        {
            ASSERT( sortMap.at(recordIds[i-1]) <= sortMap.at(recordIds[i]) );
        }
    }

}

void validateFloatSortableAttrDescending(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, bool descending, const map<string, float> &sortMap)
{
    vector<string> recordIds;

    // sort use the latitude
    getGetAllResultsQueryResults(analyzer, queryEvaluator, "Professional", descending, recordIds, -1, 1);

    // there are 139 records containing the word "professional" in the data.
    ASSERT(recordIds.size()==139);

    // validate the order
    for (int i=1; i<recordIds.size(); i++)
    {
        if (descending)
        {
            ASSERT( sortMap.at(recordIds[i-1]) >= sortMap.at(recordIds[i]));
        }
        else
        {
            ASSERT( sortMap.at(recordIds[i-1]) <= sortMap.at(recordIds[i]));
        }
    }

}
void validate(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, const map<string, int> &sort1Map, const map<string, float> &sort2Map)
{
    validateIntSortableAttrDescending(analyzer, queryEvaluator, true, sort1Map);
    validateIntSortableAttrDescending(analyzer, queryEvaluator, false, sort1Map);
    validateFloatSortableAttrDescending(analyzer, queryEvaluator, true, sort2Map);
    validateFloatSortableAttrDescending(analyzer, queryEvaluator, false, sort2Map);

    // TODO validate scores
}

void testScoreDefaultIndex(string index_dir, string data_file)
{
    map<string, int> sort1Map;
    map<string, float> sort2Map;

    Indexer *indexer = buildIndex(data_file, index_dir, "idf_score*doc_boost", sort1Map, sort2Map);

    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);

    const Analyzer *analyzer = getAnalyzer();

    validate(analyzer, queryEvaluator, sort1Map, sort2Map);

    delete queryEvaluator;
    delete indexer;
    delete analyzer;

}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string data_file = index_dir + "/data";

    testScoreDefaultIndex(index_dir, data_file);

    return 0;
}
