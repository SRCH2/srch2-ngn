//$Id: IndexSearcherInternal_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "util/Assert.h"
#include "util/Logger.h"
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
using srch2::util::Logger;

typedef Trie Trie_Internal;

const char* INDEX_DIR = ".";

bool checkContainment(vector<string> &prefixVector, const string &prefix) {
    for (unsigned i = 0; i < prefixVector.size(); i++) {
        if (prefixVector.at(i) == prefix)
            return true;
    }

    return false;
}

// TODO Needs change in   indexSearcherInternal->computeActiveNodeSet(term) to run.
void ActiveNodeSet_test() {
    /*   // construct a trie for the searcher
     Trie *trie = new Trie();

     unsigned invertedIndexOffset;
     trie->addKeyword("cancer", invertedIndexOffset);
     trie->addKeyword("canada", invertedIndexOffset);
     trie->addKeyword("canteen", invertedIndexOffset);
     trie->addKeyword("can", invertedIndexOffset);
     trie->addKeyword("cat", invertedIndexOffset);
     trie->addKeyword("dog", invertedIndexOffset);
     trie->commit();
     */

    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);
    Analyzer *analyzer = srch2is::Analyzer::create(
            srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "", "", "",
            SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData(
            new Cache(134217728, 20000), mergeEveryNSeconds, mergeEveryMWrites,
            INDEX_DIR, "");
    srch2is::Indexer *indexer = srch2is::Indexer::create(indexMetaData,
            analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_title",
            "cancer canada canteen can cat dog");
    record->setInMemoryData("test string");
    indexer->addRecord(record, 0);
    indexer->commit();

    IndexSearcherInternal *indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));

    unsigned threshold = 2;
    Term *term = FuzzyTerm::create("nce", TERM_TYPE_PREFIX, 1, 1, threshold);
    PrefixActiveNodeSet *prefixActiveNodeSet = indexSearcherInternal
            ->computeActiveNodeSet(term);
    vector<string> similarPrefixes;
    prefixActiveNodeSet->getComputedSimilarPrefixes(
            indexSearcherInternal->getTrie(), similarPrefixes);

    unsigned sim_size = similarPrefixes.size();

    (void) sim_size;

    ASSERT(similarPrefixes.size() == 2);
    ASSERT(checkContainment(similarPrefixes, "c"));
    //ASSERT(checkContainment(similarPrefixes, "ca"));
    ASSERT(checkContainment(similarPrefixes, "cance"));
    similarPrefixes.clear();

    delete term;
    delete indexMetaData;
    delete indexer;
    // We don't need to delete prefixActiveNodeSet since it's cached and will be
    // deleted in the destructor of indexSearchInternal
    delete indexSearcherInternal;
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
void addRecords() {
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);
    Analyzer *analyzer = srch2is::Analyzer::create(
            srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "", "", "",
            SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData(NULL,
            mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    srch2is::Indexer *index = srch2is::Indexer::create(indexMetaData, analyzer,
            schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors",
            "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title",
            "come Yesterday Once More");
    record->setRecordBoost(10);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setRecordBoost(20);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setRecordBoost(30);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setRecordBoost(40);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setSearchableAttributeValue(1,
            "Shinesyponzi on Wish you were here");
    record->setRecordBoost(50);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setRecordBoost(60);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setRecordBoost(80);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
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

bool checkResults(QueryResults *queryResults, set<unsigned> *resultSet) {
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults();
            resultCounter++) {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
        queryResults->getEditDistances(resultCounter, editDistances);


        Logger::debug("Result-(%d) RecordId:%s\tScore:%s",
        		resultCounter,
        		(queryResults->getRecordId(resultCounter)).c_str(), queryResults->getResultScore(resultCounter).toString().c_str());
        Logger::debug("Matching Keywords:");

        unsigned counter = 0;
        for (vector<string>::iterator iter = matchingKeywords.begin();
                iter != matchingKeywords.end(); iter++, counter++) {
            Logger::debug("\t%s %d", (*iter).c_str(),
                    editDistances.at(counter));
        }

        if (resultSet->count(
                atoi(queryResults->getRecordId(resultCounter).c_str()))
                == false) {
            return false;
        }

    }
    return true;
}

void printResults(QueryResults *queryResults) {
    cout << "Results:" << queryResults->getNumberOfResults() << endl;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults();
            resultCounter++) {
        cout << "Result-(" << resultCounter << ") RecordId:"
                << queryResults->getRecordId(resultCounter) << endl;
        cout << "[" << queryResults->getInternalRecordId(resultCounter) << "]"
                << endl;
        cout << "[" << queryResults->getInMemoryRecordString(resultCounter)
                << "]" << endl;
    }
}

void Test_Complete_Exact(IndexSearcherInternal *indexSearcherInternal) {
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
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = { "pink", "floyd", "shine"};

    cout << "\n***COMPLETE EXACT***\nQuery:";
    TermType termType = TERM_TYPE_COMPLETE;
    cout << keywords[0] << "\n";
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    query->add(term0);
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);

    indexSearcherInternal->search(query, queryResults, resultCount);
    ASSERT(checkResults(queryResults, &resultSet0) == true);
    printResults(queryResults);

    cout << "\nAdding Term:";
    cout << keywords[1] << "\n";
    Term *term1 = ExactTerm::create(keywords[1], termType, 1, 1);
    query->add(term1);
    QueryResults *queryResults1 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);
    printResults(queryResults1);

    cout << "\nAdding Term:";
    cout << keywords[2] << "\n";
    Term *term2 = ExactTerm::create(keywords[2], termType, 1, 1);
    query->add(term2);
    QueryResults *queryResults2 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);
    printResults(queryResults2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Prefix_Exact(IndexSearcherInternal *indexSearcherInternal) {
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
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = {
            "pin","floy","shi"
    };

    cout << "\n***PREFIX EXACT***\nQuery:";
    TermType termType = TERM_TYPE_PREFIX;
    cout << keywords[0] << "\n";
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    query->add(term0);
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout << "\nAdding Term:";
    cout << keywords[1] << "\n";
    Term *term1 = ExactTerm::create(keywords[1], termType, 1, 1);
    query->add(term1);
    QueryResults *queryResults1 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout << "\nAdding Term:";
    cout << keywords[2] << "\n";
    Term *term2 = ExactTerm::create(keywords[2], termType, 1, 1);
    query->add(term2);
    QueryResults *queryResults2 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);
    //printResults(queryResults2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Complete_Fuzzy(IndexSearcherInternal *indexSearcherInternal) {
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(1007);
    resultSet0.insert(1006);
    resultSet0.insert(1003);

    resultSet1.insert(1007);
    resultSet1.insert(1003);

    resultSet2.insert(1003);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = {
            "pgnk","flayd","sheine"
    };

    cout << "\n***COMPLETE FUZZY***\nQuery:";
    TermType type = TERM_TYPE_COMPLETE;
    cout << keywords[0] << "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout << "\nAdding Term:";
    cout << keywords[1] << "\n";
    Term *term1 = FuzzyTerm::create(keywords[1], type, 1, 1, 2);
    query->add(term1);
    QueryResults *queryResults1 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout << "\nAdding Term:";
    cout << keywords[2] << "\n";
    Term *term2 = FuzzyTerm::create(keywords[2], type, 1, 1, 2);
    query->add(term2);
    QueryResults *queryResults2 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;
}

void Test_Prefix_Fuzzy(IndexSearcherInternal *indexSearcherInternal) {
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
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = {
            "pionn","fllio","shiii"
    };

    cout << "\n***PREFIX FUZZY***\nQuery:";
    TermType type = TERM_TYPE_PREFIX;
    cout << keywords[0] << "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
    checkResults(queryResults, &resultSet0);

    cout << "\nAdding Term:";
    cout << keywords[1] << "\n";
    Term *term1 = FuzzyTerm::create(keywords[1], type, 1, 1, 2);
    query->add(term1);
    QueryResults *queryResults1 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults1, resultCount);
    checkResults(queryResults1, &resultSet1);

    cout << "\nAdding Term:";
    cout << keywords[2] << "\n";
    Term *term2 = FuzzyTerm::create(keywords[2], type, 1, 1, 2);
    query->add(term2);
    QueryResults *queryResults2 = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults2, resultCount);
    checkResults(queryResults2, &resultSet2);

    delete query;
    delete queryResults;
    delete queryResults1;
    delete queryResults2;

}
void Searcher_Tests() {
    addRecords();

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData(
            new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");

    Indexer* indexer = Indexer::load(indexMetaData);

    IndexSearcherInternal *indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));

    Test_Complete_Exact(indexSearcherInternal);
    std::cout << "test1" << std::endl;

    Test_Prefix_Exact(indexSearcherInternal);
    std::cout << "test2" << std::endl;

    Test_Complete_Fuzzy(indexSearcherInternal);
    std::cout << "test3" << std::endl;

    Test_Prefix_Fuzzy(indexSearcherInternal);
    std::cout << "test4" << std::endl;

    delete indexer;
    delete indexSearcherInternal;
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }

    ActiveNodeSet_test();

    Searcher_Tests();

    cout << "IndexSearcherInternal Unit Tests: Passed\n";

    return 0;
}
