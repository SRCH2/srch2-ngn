//$Id: AttributedBasedSearch_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include <analyzer/AnalyzerInternal.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

Indexer *buildIndex(string data_file, string index_dir, string expression)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex, srch2::instantsearch::FIELDBITINDEX);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    Analyzer *analyzer = new StandardAnalyzer(srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

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
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }

            cellCounter++;
        }

        indexer->addRecord(record, 0);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void fireSearch(IndexSearcher *indexSearcher, unsigned filter, unsigned k, const vector<string> &searchKeywords,
                unsigned numOfResults, const vector<string> &resultIds, const vector<vector<unsigned> > &resultAttributeBitmap)
{
    
    Query *query = new Query(srch2::instantsearch::TopKQuery);
    query->setPostProcessingFilter(NO_FILTER);
    QueryResults *queryResults = QueryResults::create(indexSearcher, query);

    for (unsigned i = 0; i < searchKeywords.size(); ++i)
    {
        TermType type = PREFIX;
        Term *term = ExactTerm::create(searchKeywords[i], type, 1, 0.5);
        term->addAttributeToFilterTermHits(filter);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }

    indexSearcher->search(query, queryResults, k);

    ASSERT(queryResults->getNumberOfResults() == numOfResults);

    vector<unsigned> matchedAttributeBitmap;
    for (unsigned i = 0; i< numOfResults; ++i)
    {
        ASSERT(queryResults->getRecordId(i) == resultIds[i]);
        queryResults->getMatchedAttributeBitmaps(i, matchedAttributeBitmap);
        for(int j = 0; j< matchedAttributeBitmap.size(); j++)
        	ASSERT(matchedAttributeBitmap[j] == resultAttributeBitmap[i][j]);
    }

    delete queryResults;
    delete query;

}

void test(string index_dir, string data_file)
{
    Indexer *indexer = buildIndex(data_file, index_dir, "idf_score*doc_boost");

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

    unsigned filter = 0;
    int k = 10;

    vector<string> searchKeywords;
    searchKeywords.push_back("jack");

    vector<string> resultIds;
    vector<vector<unsigned> > resultAttributeBitmap;
    unsigned numOfResults = 0;

    // case 1
    filter = 2; // set the second bit to 1, meaning the keyword need to appear in the second searchable attribute
    
    resultIds.push_back("2");
    resultIds.push_back("3");
    resultIds.push_back("4");
    numOfResults = 3;
    resultAttributeBitmap.resize(3);
    resultAttributeBitmap[0].push_back(2);
    resultAttributeBitmap[1].push_back(2);
    resultAttributeBitmap[2].push_back(3);

    fireSearch(indexSearcher, filter, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 1 passed." << endl;

    // case 2
    filter = 1; // set the first bit to 1, meaning the keyword need to appear in the first searchable attribute

    resultIds.push_back("1");
    resultIds.push_back("4");
    numOfResults = 2;
    resultAttributeBitmap.resize(2);
    resultAttributeBitmap[0].push_back(1);
    resultAttributeBitmap[1].push_back(3);

    fireSearch(indexSearcher, filter, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 2 passed." << endl;

    // case 3
    filter = 3; // set both the first bit and second bit to 1, meaning the keyword need to appear either in the first searchable attribute or the second

    resultIds.push_back("1");
    resultIds.push_back("2");
    resultIds.push_back("3");
    resultIds.push_back("4");
    numOfResults = 4;
    resultAttributeBitmap.resize(4);
    resultAttributeBitmap[0].push_back(1);
    resultAttributeBitmap[1].push_back(2);
    resultAttributeBitmap[2].push_back(2);
    resultAttributeBitmap[3].push_back(3);

    fireSearch(indexSearcher, filter, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 3 passed." << endl;

    // case 4
    filter = 3; // set both the first bit and second bit to 1, 
    filter |= 0x80000000; // set the highest bit to 1, meaning the keyword need to appear BOTH in the first searchable attribute and the second

    resultIds.push_back("4");
    numOfResults = 1;
    resultAttributeBitmap.resize(1);
    resultAttributeBitmap[0].push_back(3);

    fireSearch(indexSearcher, filter, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 4 passed." << endl;

    // case 5
    filter = 0x7fffffff; // no filter

    resultIds.push_back("1");
    resultIds.push_back("2");
    resultIds.push_back("3");
    resultIds.push_back("4");
    numOfResults = 4;
    resultAttributeBitmap.resize(4);
    resultAttributeBitmap[0].push_back(1);
    resultAttributeBitmap[1].push_back(2);
    resultAttributeBitmap[2].push_back(2);
    resultAttributeBitmap[3].push_back(3);

    fireSearch(indexSearcher, filter, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();

    cout << "case 5 passed." << endl;

    delete indexSearcher;
    delete indexer;

}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string data_file = index_dir + "/data";

    test(index_dir, data_file);

    return 0;
}
