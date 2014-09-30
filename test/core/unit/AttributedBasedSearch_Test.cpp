//$Id: AttributedBasedSearch_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include <analyzer/AnalyzerInternal.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "analyzer/StandardAnalyzer.h"
#include "UnitTestHelper.h"
#include "analyzer/AnalyzerContainers.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
IndexMetaData *indexMetaData;
Indexer *buildIndex(string data_file, string index_dir, string expression)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex, srch2::instantsearch::POSITION_INDEX_FIELDBIT);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    SynonymContainer *syn = SynonymContainer::getInstance(string(""), SYNONYM_DONOT_KEEP_ORIGIN);
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance("");
    AnalyzerInternal *simpleAnlyzer = new StandardAnalyzer(NULL, NULL, prot, syn, string(""));

    Analyzer *analyzer = new Analyzer(NULL, NULL, prot, syn, "", srch2is::STANDARD_ANALYZER);

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

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    delete record;
    delete analyzer;
    delete schema;
    prot->free();
    syn->free();

    return indexer;
}

void fireSearch(QueryEvaluator * queryEvaluator, vector<unsigned> filter, ATTRIBUTES_OP attrOp, unsigned k, const vector<string> &searchKeywords,
                unsigned numOfResults, const vector<string> &resultIds, const vector<vector<vector<unsigned> > > &resultAttributeBitmap)
{
    
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    QueryResultFactory * resultFactory = new QueryResultFactory();
    QueryResults * queryResults = new QueryResults(resultFactory,queryEvaluator, query);

    for (unsigned i = 0; i < searchKeywords.size(); ++i)
    {
        TermType termType = TERM_TYPE_PREFIX;
        Term *term = ExactTerm::create(searchKeywords[i], termType, 1, 0.5);
        term->addAttributesToFilter(filter, attrOp);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }


    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, 10, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
//    QueryEvaluatorRuntimeParametersContainer runTimeParameters;
//    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer,&runTimeParameters );
    cout << "record found :" << queryResults->getNumberOfResults() << endl;
    ASSERT(queryResults->getNumberOfResults() == numOfResults);

    vector<vector<unsigned> > matchedAttributeBitmap;
    for (unsigned i = 0; i< numOfResults; ++i)
    {
        ASSERT(queryResults->getRecordId(i) == resultIds[i]);
        queryResults->getMatchedAttributes(i, matchedAttributeBitmap);
        for(int j = 0; j< matchedAttributeBitmap.size(); j++)
        	for(int k = 0; k< matchedAttributeBitmap[j].size(); k++)
        		ASSERT(matchedAttributeBitmap[j][k] == resultAttributeBitmap[i][j][k]);
    }

    delete queryResults;
    delete query;
    delete resultFactory;

}

void test(string index_dir, string data_file)
{
    Indexer *indexer = buildIndex(data_file, index_dir, "idf_score*doc_boost");

    QueryEvaluatorRuntimeParametersContainer runTimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer,&runTimeParameters );

    vector<unsigned> filter;
    int k = 10;

    vector<string> searchKeywords;
    searchKeywords.push_back("jack");

    vector<string> resultIds;
    vector<vector<vector<unsigned> > > resultAttributeBitmap;
    unsigned numOfResults = 0;

    // case 1
    filter.push_back(1); // set the second bit to 1, meaning the keyword need to appear in the second searchable attribute
    
    resultIds.push_back("4");
    resultIds.push_back("2");
    resultIds.push_back("3");
    numOfResults = 3;
    resultAttributeBitmap.resize(3);
    resultAttributeBitmap[0].push_back(filter);
    resultAttributeBitmap[1].push_back(filter);
    resultAttributeBitmap[2].push_back(filter);
    // Do OR operation.
    fireSearch(queryEvaluator, filter, ATTRIBUTES_OP_OR, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 1 passed." << endl;

    // case 2
    filter.clear();
    filter.push_back(0); // set the first bit to 1, meaning the keyword need to appear in the first searchable attribute

    resultIds.push_back("4");
    resultIds.push_back("1");
    numOfResults = 2;
    resultAttributeBitmap.resize(2);
    resultAttributeBitmap[0].push_back(filter);
    resultAttributeBitmap[1].push_back(filter);

    // Do OR operation.
    fireSearch(queryEvaluator, filter, ATTRIBUTES_OP_OR, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 2 passed." << endl;

    // case 3
    filter.clear();
    filter.push_back(0);
    filter.push_back(1); // set both the first bit and second bit to 1, meaning the keyword need to appear either in the first searchable attribute or the second

    resultIds.push_back("4");
    resultIds.push_back("1");
    resultIds.push_back("2");
    resultIds.push_back("3");
    numOfResults = 4;
    resultAttributeBitmap.resize(4);
    resultAttributeBitmap[0].push_back(filter);
    vector<unsigned> result;
    result.push_back(0);
    resultAttributeBitmap[1].push_back(result);
    result.clear();
    result.push_back(1);
    resultAttributeBitmap[2].push_back(result);
    resultAttributeBitmap[3].push_back(result);

    // Do OR operation.
    fireSearch(queryEvaluator, filter, ATTRIBUTES_OP_OR, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 3 passed." << endl;

    // case 4
    filter.clear();
    filter.push_back(0);
    filter.push_back(1); // set both the first bit and second bit to 1,

    resultIds.push_back("4");
    numOfResults = 1;
    resultAttributeBitmap.resize(1);
    resultAttributeBitmap[0].push_back(filter);
    // Do AND operation.
    fireSearch(queryEvaluator, filter, ATTRIBUTES_OP_AND, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();
    resultAttributeBitmap.clear();

    cout << "case 4 passed." << endl;

    // case 5
    filter.clear(); // no filter

    resultIds.push_back("4");
    resultIds.push_back("1");
    resultIds.push_back("2");
    resultIds.push_back("3");
    numOfResults = 4;
    resultAttributeBitmap.resize(4);
    result.clear();
    result.push_back(0);
    result.push_back(1);
    resultAttributeBitmap[0].push_back(result);
    result.clear();
    result.push_back(0);
    resultAttributeBitmap[1].push_back(result);
    result.clear();
    result.push_back(1);
    resultAttributeBitmap[2].push_back(result);
    resultAttributeBitmap[3].push_back(result);

    fireSearch(queryEvaluator, filter, ATTRIBUTES_OP_OR, k, searchKeywords, numOfResults, resultIds, resultAttributeBitmap);

    resultIds.clear();

    cout << "case 5 passed." << endl;

    delete queryEvaluator;
    delete indexer;
    delete indexMetaData;

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
