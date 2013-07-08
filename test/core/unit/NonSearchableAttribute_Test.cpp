
//$Id: IndexSearcherInternal_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "util/Assert.h"
#include "analyzer/AnalyzerInternal.h"

#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Indexer.h>

#include <iostream>
#include <functional>
#include <vector>
#include <string>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie Trie_Internal;

const char* INDEX_DIR = ".";

bool checkContainment(vector<string> &prefixVector, const string &prefix)
{
    for (unsigned i = 0; i < prefixVector.size(); i++)
    {
        if (prefixVector.at(i) == prefix)
            return true;
    }

    return false;
}



/**
 * Keyword: Record
 *   Pink:  2, 5, 6
 *   Pinksyponzi: 4
 *
 *   Floyd: 2, 6
 *   Floydsyponzi: 4
 *
 *   Shine: 2, 3
 *   Shinesyponzi: 4
 */
void addRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text
    schema->setNonSearchableAttribute("citation", UNSIGNED, "0" , true);
    schema->setNonSearchableAttribute("price" , FLOAT, "1.25", true);
    schema->setNonSearchableAttribute("class" , TEXT, "Z" , false);

    Record *record = new Record(schema);
    Analyzer *analyzer = srch2is::Analyzer::create(srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( NULL, mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    srch2is::Indexer *index = srch2is::Indexer::create(indexMetaData, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setNonSearchableAttributeValue("citation", "1");
    record->setNonSearchableAttributeValue("price" , "10.34");
    record->setNonSearchableAttributeValue("class" , "A");
    record->setRecordBoost(10);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setNonSearchableAttributeValue("citation", "2");
    record->setNonSearchableAttributeValue("price" , "9.34");
    record->setNonSearchableAttributeValue("class" , "A");
    record->setRecordBoost(20);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setNonSearchableAttributeValue("citation", "3");
    record->setNonSearchableAttributeValue("price" , "8.34");
    record->setNonSearchableAttributeValue("class" , "B");
    record->setRecordBoost(30);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setNonSearchableAttributeValue("citation", "4");
    record->setNonSearchableAttributeValue("price" , "7.34");
    record->setNonSearchableAttributeValue("class" , "B");
    record->setRecordBoost(40);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setSearchableAttributeValue(1, "Shinesyponzi on Wish you were here");
    record->setNonSearchableAttributeValue("citation", "5");
    record->setNonSearchableAttributeValue("price" , "6.34");
    record->setNonSearchableAttributeValue("class" , "C");
    record->setRecordBoost(50);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setNonSearchableAttributeValue("citation", "6");
    record->setNonSearchableAttributeValue("price" , "5.34");
    record->setNonSearchableAttributeValue("class" , "C");
    record->setRecordBoost(60);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setNonSearchableAttributeValue("citation", "7");
    record->setNonSearchableAttributeValue("price" , "4.34");
    record->setNonSearchableAttributeValue("class" , "D");
    record->setRecordBoost(80);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setNonSearchableAttributeValue("citation", "8");
    record->setNonSearchableAttributeValue("price" , "3.34");
    record->setNonSearchableAttributeValue("class" , "E");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    ///TODO: Assert that This record must not be added
    /// 1) Repeat of primary key
    /// 2) Check when adding junk data liek &*^#^%%
    /*record->clear();
    record->setPrimaryKey(1001);
    record->setAttributeValue(0, "jimi pink");
    record->setAttributeValue(1, "comes junk 2345 $%^^#");
    record->setBoost(100);
    index->addRecordBeforeCommit(record);*/
    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

bool checkResults(QueryResults *queryResults, set<unsigned> *resultSet)
{

	if(queryResults->getNumberOfResults() != resultSet->size()) return false;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
    {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
        queryResults->getEditDistances(resultCounter, editDistances);


        LOG_REGION(0,
                cout<<"\nResult-("<<resultCounter<<") RecordId:"<<queryResults->getRecordId(resultCounter)<<"\tScore:"<<queryResults->getResultScoreString(resultCounter);
        );
        LOG_REGION(0,
                cout<<"\nMatching Keywords:"<<endl;
        );

        unsigned counter = 0;
        for(vector<string>::iterator iter = matchingKeywords.begin(); iter != matchingKeywords.end(); iter++, counter++ )
        {
            LOG_REGION(0,
                    cout<<"\t"<<*iter<<" "<<editDistances.at(counter)<<endl;
            );
        }

        if(resultSet->count(atoi(queryResults->getRecordId(resultCounter).c_str())) == false)
        {
            return false;
        }

    }
    return true;
}

void printResults(QueryResults *queryResults)
{
    cout<<"Results:"<<queryResults->getNumberOfResults()<<endl;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
    {
        cout<<"Result-("<<resultCounter<<") RecordId:"<<queryResults->getRecordId(resultCounter)<<endl;
        cout << "[" << queryResults->getInternalRecordId(resultCounter) << "]" << endl;
        cout << "[" << queryResults->getInMemoryRecordString(resultCounter) << "]" << endl;
    }
}

void Test_Complete_Exact(IndexSearcherInternal *indexSearcherInternal)
{
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(1007);
    resultSet0.insert(1006);
    resultSet0.insert(1003);
    //resultSet0.insert(4);

    resultSet1.insert(1007);
    resultSet1.insert(1003);
    //resultSet1.insert(4);

    resultSet2.insert(1003);
    //resultSet2.insert(3);
    //resultSet2.insert(4);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = { "pink", "floyd", "shine"};

    cout<<"\n***COMPLETE EXACT***\nQuery:";
    TermType type = COMPLETE;
    cout<<keywords[0]<< "\n";
    Term *term0 = ExactTerm::create(keywords[0], type, 1, 1);
    query->add(term0);
    QueryResults *queryResults = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    ASSERT( checkResults(queryResults, &resultSet0) == true);
    printResults(queryResults);

    cout<<"\nAdding Term:";
    cout<<keywords[1]<< "\n";
    Term *term1 = ExactTerm::create(keywords[1], type, 1, 1);
    query->add(term1);
    QueryResults *queryResults1 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);
    printResults(queryResults1);

    cout<<"\nAdding Term:";
    cout<<keywords[2]<< "\n";
    Term *term2 = ExactTerm::create(keywords[2], type, 1, 1);
    query->add(term2);
    QueryResults *queryResults2 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);
    printResults(queryResults2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Prefix_Exact(IndexSearcherInternal *indexSearcherInternal)
{
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(1007);
    resultSet0.insert(1006);
    resultSet0.insert(1003);
    resultSet0.insert(1005);

    resultSet1.insert(1007);
    resultSet1.insert(1003);
    resultSet1.insert(1005);

    resultSet2.insert(1003);
    resultSet2.insert(1005);
    //resultSet2.insert(4);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = {
            "pin","floy","shi"
    };

    cout<<"\n***PREFIX EXACT***\nQuery:";
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";
    Term *term0 = ExactTerm::create(keywords[0], type, 1, 1);
    query->add(term0);
    QueryResults *queryResults = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout<<"\nAdding Term:";
    cout<<keywords[1]<< "\n";
    Term *term1 = ExactTerm::create(keywords[1], type, 1, 1);
    query->add(term1);
    QueryResults *queryResults1 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout<<"\nAdding Term:";
    cout<<keywords[2]<< "\n";
    Term *term2 = ExactTerm::create(keywords[2], type, 1, 1);
    query->add(term2);
    QueryResults *queryResults2 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);
    //printResults(queryResults2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Complete_Fuzzy(IndexSearcherInternal *indexSearcherInternal)
{
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(1007);
    resultSet0.insert(1006);
    resultSet0.insert(1003);

    resultSet1.insert(1007);
    resultSet1.insert(1003);

    resultSet2.insert(1003);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = {
            "pgnk","flayd","sheine"
    };

    cout<<"\n***COMPLETE FUZZY***\nQuery:";
    TermType type = COMPLETE;
    cout<<keywords[0]<< "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);
    QueryResults *queryResults = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout<<"\nAdding Term:";
    cout<<keywords[1]<< "\n";
    Term *term1 = FuzzyTerm::create(keywords[1], type, 1, 1, 2);
    query->add(term1);
    QueryResults *queryResults1 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout<<"\nAdding Term:";
    cout<<keywords[2]<< "\n";
    Term *term2 = FuzzyTerm::create(keywords[2], type, 1, 1, 2);
    query->add(term2);
    QueryResults *queryResults2 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Prefix_Fuzzy(IndexSearcherInternal *indexSearcherInternal)
{
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(1007);
    resultSet0.insert(1006);
    resultSet0.insert(1003);
    resultSet0.insert(1005);

    resultSet1.insert(1007);
    resultSet1.insert(1003);
    resultSet1.insert(1005);

    resultSet2.insert(1003);
    resultSet2.insert(1005);
    //resultSet2.insert(4);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = {
            "pionn","fllio","shiii"
    };

    cout<<"\n***PREFIX FUZZY***\nQuery:";
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);
    QueryResults *queryResults = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout<<"\nAdding Term:";
    cout<<keywords[1]<< "\n";
    Term *term1 = FuzzyTerm::create(keywords[1], type, 1, 1, 2);
    query->add(term1);
    QueryResults *queryResults1 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout<<"\nAdding Term:";
    cout<<keywords[2]<< "\n";
    Term *term2 = FuzzyTerm::create(keywords[2], type, 1, 1, 2);
    query->add(term2);
    QueryResults *queryResults2 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;

}

void Test_Range_Query(IndexSearcherInternal *indexSearcherInternal){

    set<unsigned> resultSet0 , resultSet01, resultSet02, resultSet03,  resultSet1, resultSet2;

    // query = "pionn", citation < 6
    resultSet0.insert(1003);
    resultSet0.insert(1005);

    // query = "pionn", citation < 7
    resultSet01.insert(1003);
    resultSet01.insert(1005);
    resultSet01.insert(1006);

    // query = "pionn", price < 6
    resultSet02.insert(1006);
    resultSet02.insert(1007);


    // query = "pionn", price != 8.34
    resultSet03.insert(1006);
    resultSet03.insert(1005);
    resultSet03.insert(1007);


    // query = "pionn fllio", class < D
    resultSet1.insert(1003);
    resultSet1.insert(1005);

    // query = "pionn fllio shiii", price < 8.35
    resultSet2.insert(1003);
    resultSet2.insert(1005);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    query->setPostProcessingFilter(RANGE_CHECK); // range query
    string keywords[3] = {
            "pionn","fllio","shiii"
    };


    query->setPostProcessingFilterOperation(LESS_THAN);
    cout<<"\n***PREFIX FUZZY***\nQuery:";
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";

    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);
    query->setNonSearchableAttributeName("citation");
    query->setNonSearchableAttributeValue("6");

    QueryResults *queryResults = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout<<"\nChanging criterion to : citation < 7";
    query->setNonSearchableAttributeName("citation");
    query->setNonSearchableAttributeValue("7");
    QueryResults *queryResults01 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults01, resultCount);
    ASSERT(checkResults(queryResults01, &resultSet01));


    cout<<"\nChanging criterion to : price < 6";
    query->setNonSearchableAttributeName("price");
    query->setNonSearchableAttributeValue("6");
    QueryResults *queryResults02 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults02, resultCount);
    ASSERT(checkResults(queryResults02, &resultSet02));

    cout<<"\nChanging criterion to : price != 8.34";
    query->setNonSearchableAttributeName("price");
    query->setNonSearchableAttributeValue("8.34");
    query->setPostProcessingFilterOperation(NOT_EQUALS);
    QueryResults *queryResults03 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults03, resultCount);
    ASSERT(checkResults(queryResults03, &resultSet03));

    query->setPostProcessingFilterOperation(LESS_THAN);

    cout<<"\nAdding Term:";
    cout<<keywords[1]<< "\n";
    Term *term1 = FuzzyTerm::create(keywords[1], type, 1, 1, 2);
    query->add(term1);
    query->setNonSearchableAttributeName("class");
    query->setNonSearchableAttributeValue("D");
    QueryResults *queryResults1 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    ASSERT(checkResults(queryResults1, &resultSet1));

    cout<<"\nAdding Term:";
    cout<<keywords[2]<< "\n";
    Term *term2 = FuzzyTerm::create(keywords[2], type, 1, 1, 2);
    query->add(term2);
    query->setNonSearchableAttributeName("price");
    query->setNonSearchableAttributeValue("8.35");
    QueryResults *queryResults2 = QueryResults::create(indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    ASSERT(checkResults(queryResults2, &resultSet2));




    delete query;
    delete queryResults;
//    delete queryResults1;
//    delete queryResults2;

}

void Searcher_Tests()
{
    addRecords();

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");

    Indexer* indexer = Indexer::load(indexMetaData);

	IndexSearcherInternal *indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));

//    Test_Complete_Exact(indexSearcherInternal);
//	std::cout << "test1" << std::endl;
//
//    Test_Prefix_Exact(indexSearcherInternal);
//	std::cout << "test2" << std::endl;
//
//    Test_Complete_Fuzzy(indexSearcherInternal);
//	std::cout << "test3" << std::endl;
//
//	Test_Prefix_Fuzzy(indexSearcherInternal);
//	std::cout << "test4" << std::endl;

	Test_Range_Query(indexSearcherInternal);
	std::cout << "test5" << std::endl;

    delete indexer;
    delete indexSearcherInternal;
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }


    Searcher_Tests();

    cout << "IndexSearcherInternal Unit Tests: Passed\n";

    return 0;
}

