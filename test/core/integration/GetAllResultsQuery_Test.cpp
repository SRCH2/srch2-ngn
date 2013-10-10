//$Id: Scoring_Test.cpp 2988 2012-11-29 01:21:15Z oliverax $

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

Indexer *buildIndex(string data_file, string index_dir, string expression, map<string, int> &sort1Map, map<string, float> &sort2Map)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);


    schema->setNonSearchableAttribute("id_for_sort",srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "0" );
    schema->setNonSearchableAttribute("latitude", srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT, "1" );

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2::instantsearch::STANDARD_ANALYZER);

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

        while(getline(lineStream,cell,'^') && cellCounter < 5 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
                record->setNonSearchableAttributeValue(0, cell); // use the id number to be one sortable attribute
                sort1Map[cell] = atoi(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                record->setNonSearchableAttributeValue(1, cell); // use the latitude to be another sortable attribute
                sort2Map[record->getPrimaryKey()] = atof(cell.c_str());
            }

            cellCounter++;
        }

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void validateIntSortableAttrDescending(const Analyzer *analyzer, IndexSearcher *indexSearcher, bool descending, const map<string, int> &sortMap)
{
    vector<string> recordIds;

    // sort use the id number
    getGetAllResultsQueryResults(analyzer, indexSearcher, "Professional", descending, recordIds, -1, 0);

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

void validateFloatSortableAttrDescending(const Analyzer *analyzer, IndexSearcher *indexSearcher, bool descending, const map<string, float> &sortMap)
{
    vector<string> recordIds;

    // sort use the latitude
    getGetAllResultsQueryResults(analyzer, indexSearcher, "Professional", descending, recordIds, -1, 1);

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
void validate(const Analyzer *analyzer, IndexSearcher *indexSearcher, const map<string, int> &sort1Map, const map<string, float> &sort2Map)
{
    validateIntSortableAttrDescending(analyzer, indexSearcher, true, sort1Map);
    validateIntSortableAttrDescending(analyzer, indexSearcher, false, sort1Map);
    validateFloatSortableAttrDescending(analyzer, indexSearcher, true, sort2Map);
    validateFloatSortableAttrDescending(analyzer, indexSearcher, false, sort2Map);

    // TODO validate scores
}

void testScoreDefaultIndex(string index_dir, string data_file)
{
    map<string, int> sort1Map;
    map<string, float> sort2Map;

    Indexer *indexer = buildIndex(data_file, index_dir, "idf_score*doc_boost", sort1Map, sort2Map);

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

    const Analyzer *analyzer = getAnalyzer();

    validate(analyzer, indexSearcher, sort1Map, sort2Map);

    delete indexSearcher;
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
