
// $Id: IndexUpdater_Test.cpp 3433 2013-06-11 03:13:10Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include <instantsearch/Analyzer.h>
#include "operation/IndexerInternal.h"
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"
#include "IntegrationTestHelper.h"
#include <stdlib.h>

#include <time.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
namespace bmis = bimaple::instantsearch;
using namespace bmis;

string INDEX_DIR = getenv("index_dir");

void addSimpleRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(bimaple::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    Analyzer *analyzer = Analyzer::create(bimaple::instantsearch::NO_STEMMER_NORMALIZER, "");
    
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "Jimi Hendrix");
    record->setSearchableAttributeValue(2, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
    record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
    record->setRecordBoost(10);
    index->addRecord(record, 0);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

void addAdvancedRecordsWithScoreSortableAttributes()
{
    ///Create Schema
    Schema *schema = Schema::create(bimaple::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    schema->setSortableAttribute("citationcount", bimaple::instantsearch::UNSIGNED, "0");
    schema->setSortableAttribute("pagerank", bimaple::instantsearch::FLOAT, "1");

    Record *record = new Record(schema);

    Analyzer *analyzer = Analyzer::create(bimaple::instantsearch::NO_STEMMER_NORMALIZER, "");
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
    Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
    record->setRecordBoost(90);
    record->setSortableAttributeValue(0,"100");
    record->setSortableAttributeValue(1,"9.1");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "Jimi Hendrix");
    record->setSearchableAttributeValue(2, "Little wing");
    record->setSortableAttributeValue(0,"200");
    record->setSortableAttributeValue(1,"3.14159265");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
    record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
    record->setSortableAttributeValue(0,"300");
    record->setSortableAttributeValue(1,"4.234");
    record->setRecordBoost(10);
    index->addRecord(record, 0);

    //indexer->print_Index();
    index->commit();
    index->save();
    cout << "records added" << endl;

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}


//Create Index "A". Deserialise "A". Update Index "A". Search "A". Serialize "A"
void test1()
{
    addSimpleRecords();
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == true);
    }
    //Query: "jimi", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }

    //Update Index
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1999);
    record->setSearchableAttributeValue(1, "steve jobs tom");
    record->setSearchableAttributeValue(2, "digital magician");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds) == true);
    }

    //indexer->print_index();

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == false);
    }

    index->commit();
    index->save(INDEX_DIR);

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Deserialise "A" from test1. Update Index "A". Search "A" and then, save "A"
//Testing Indexer::load(...) and Indexer::save(...)
void test2()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds) == true);
    }

    //Update a Deserialised Index
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 2 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    (void)analyzer;
    delete indexSearcher;
    delete index;

}

//Deserialise Index "A". Add duplicate records. Test. Serialise
void test3()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Update a Deserialised Index with Duplicate
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    //Update a Deserialised Index with Duplicate
    record->clear();
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    //Update a Deserialised Index with Duplicate
    record->clear();
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "tom tom tom tom"); // THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 2 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    delete indexSearcher;
    (void)analyzer;
    delete index;
}


//Deserialise Index "A". Delete a record. Test. Serialise
void test4()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    std::string recordId = "1998";
    index->deleteRecord(recordId, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        //recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        //recordIds.push_back(1998);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->save(INDEX_DIR);

    //delete analyzer;
    delete indexSearcher;
    (void)analyzer;
    delete index;
}

//Deserialise Index "A". Add the deleted record in test3(). Test. Serialise
void test5()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Update a Deserialised Index with Duplicate
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "jimi", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998, 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 2 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    delete indexSearcher;
    (void)analyzer;
    delete index;
}

//Test Fuzzy Query and Edit Distance
void test6()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "jemi", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jemi" , 2 , recordIds) == true);
    }

    //Query: "jobsi", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobsi" , 2 , recordIds) == true);
    }

    //Query: "shiref", hits -> 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "shiref" , 1 , recordIds) == true);
    }

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Test API 2 Queries - AttributeFilter Queries
void test7()
{
    /// positional index disabled, ignore this test for now

    /*
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);

    const Analyzer *analyzer = index->getAnalyzer();
    //indexer->print_Index();

    //Query: "jimi", AttributeToFilter = 1, hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        recordIds.push_back(1002);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds, 1) == true);
    }

    cout << "7.1 passed" << endl;
    //Query: "jimi", AttributeToFilter = 2, hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 0 , recordIds, 2) == true);
    }

    //Query: "jobs", AttributeToFilter = 2, hits ->  1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds, 2) == true);
    }

    //Query: "jobs", AttributeToFilter = 1, hits ->  1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds, 1) == true);
    }

    //Query: "tom", AttributeToFilter = 1, hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1999);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds, 1) == true);
    }

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;
    */
}

//Test Multi-Keyword Queries with Updated records
void test8()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);

    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "jimi+jobs", hits -> 1998
    {
        vector<unsigned> recordIds;
        //recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi+jobs" , 1 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "jack", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "jack" , 2 , recordIds) == true);
    }

    //Query: "tom+jack", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom+jack" , 2 , recordIds) == true);
    }

    //indexer->print_Index();

    /// positional index disabled, ignore the filter related tests for now

    /*
    //Query: "tom+jack", AttributeToFilter = 1, hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom+jack" , 2 , recordIds, 1) == true);
    }

    //Query: "tom+jack", AttributeToFilter = 2, hits ->
    {
        vector<unsigned> recordIds;
        //recordIds.push_back(1001);
        //recordIds.push_back(1003);//BUG - Due to Cache Hit
        ASSERT ( ping(analyzer, indexSearcher, "tom+jack" , 0 , recordIds, 2) == true);
    }
    */

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Test Advanced Query on Index with updates.
void test9()
{
    // Construct Records With Score Filter Attributes.
    addAdvancedRecordsWithScoreSortableAttributes();

    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == true);
    }

    //Query: "jimi", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }

    //Update Index
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1999);
    record->setSearchableAttributeValue(1, "steve jobs tom");
    record->setSearchableAttributeValue(2, "digital magician");
    record->setSortableAttributeValue(0,"400");
    record->setSortableAttributeValue(1,"2.234");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;

    //Do all update tests performed on normal index to advanced index.
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
}

//Test Advanced Query Index for Sorted Results
void test10()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "jimi+jobs", hits -> 1998 ;descending ;sortAttribute 0;
    {
        vector<unsigned> recordIds;
        //recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "jimi+jobs" , 1 , recordIds,-1,0) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999 ;descending ;sortAttribute 0;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1003);
        recordIds.push_back(1001);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "tom" , 3 , recordIds,-1,0) == true);
    }

    //Query: "jack", hits -> 1001, 1003 ;descending ;sortAttribute 0;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1003);
        recordIds.push_back(1001);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "jack" , 2 , recordIds,-1,0) == true);
    }

    //Query: "jack", hits -> 1001, 1003 ;descending ;sortAttribute 1;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "jack" , 2 , recordIds,-1,1) == true);
    }

    //Query: "tom+jack", hits -> 1001, 1003 ;descending ;sortAttribute 1;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "tom+jack" , 2 , recordIds,-1,1) == true);
    }

    //indexer->print_Index();

    //Query: "tom+jack", hits -> 1001, 1003 ;descending ;sortAttribute 0;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1003);
        recordIds.push_back(1001);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "tom+jack" , 2 , recordIds,-1,0) == true);
    }

    /// positional index disabled, ignore the filter related tests for now

    /*
    //Query: "tom+jack", AttributeToFilter = 1, hits -> 1001, 1003;descending ;sortAttribute 1;
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "tom+jack" , 2 , recordIds, 1,1) == true);
    }

    //Query: "tom+jack", AttributeToFilter = 2, hits ->
    {
        vector<unsigned> recordIds;
        //recordIds.push_back(1001);
        //recordIds.push_back(1003);
        ASSERT ( pingGetAllResultsQuery(analyzer, indexSearcher, "tom+jack" , 0 , recordIds, 2,0) == true);
    }
    */

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Test commit of index with no records and then add records and update it.
void test11()
{
    ///Create Schema
    Schema *schema = Schema::create(bmis::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);
    Analyzer *analyzer = Analyzer::create(bimaple::instantsearch::NO_STEMMER_NORMALIZER, "");
    unsigned mergeEveryNSeconds = 3;    
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
           
    Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

    // This commit fails. As there are no records in the index.
    ASSERT(index->commit() == 0);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "Jimi Hendrix");
    record->setSearchableAttributeValue(2, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    ASSERT(index->commit() == 1);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
    record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
    record->setRecordBoost(10);
    index->addRecord(record, 0);


    index->commit();
    index->save(INDEX_DIR);

    IndexSearcher *indexSearcher = IndexSearcher::create(index);

    //ASSERT(indexer->commit() == 1);

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003); // Should not be seen before commit
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == true);
    }

    //Query: "jimi", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }

    //Update Index
    //Record *record = new Record(index->getSchema());
    record->clear();
    record->setPrimaryKey(1999);
    record->setSearchableAttributeValue(1, "steve jobs tom");
    record->setSearchableAttributeValue(2, "digital magician");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1002
    /*{
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        //ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , recordIds) == true);
        ping(analyzer, indexSearcher, "jobs" , 1 , recordIds);
    }*/

    //indexer->print_index();

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == false);
    }

    index->save(INDEX_DIR);

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    delete indexSearcher;
    delete index;
}



int main(int argc, char **argv)
{

    cout << INDEX_DIR << endl;

    //Create Index "A". Deserialise "A". Update Index "A". Search "A". Serialize "A"
    // Testing IndexerInternal constructor
    test1();
    cout << "test1 passed" << endl;

    //Deserialise "A" from test1. Update Index "A". Search "A" and then, save "A"
    //Testing Indexer::load(...) and Indexer::save(...)
    test2();
    cout << "test2 passed" << endl;

    //Deserialise Index "A". Add duplicate records. Test. Serialise
    test3();
    cout << "test3 passed" << endl;

    //Deserialise Index "A". Delete a record. Test. Serialise
    test4();
    cout << "test4 passed" << endl;

    //Deserialise Index "A". Add the deleted record in test3(). Test. Serialise
    test5();
    cout << "test5 passed" << endl;

    //Test Fuzzy Query and Edit Distance
    test6();
    cout << "test6 passed" << endl;

    //Test API 2 Queries - AttributeFilter Queries
    test7();
    cout << "test7 passed" << endl;

    //Test Multi-Keyword Queries with Updated records
    test8();
    cout << "test8 passed" << endl;

    //Test Updating an Advanced Query Index
    test9();
    cout << "test9 passed" << endl;

    //Test Advanced Query on Index with updates.
    test10();
    cout << "test10 passed" << endl;

    //Test commit of index with no records and then add records and update it.
    test11();
    cout << "test11 passed" << endl;

    cout<<"Index updater tests Succesful!!"<<endl;

    return 0;
}

