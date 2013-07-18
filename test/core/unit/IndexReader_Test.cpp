//$Id: IndexReader_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include "operation/IndexerInternal.h"
#include "index/Trie.h"
#include "instantsearch/Schema.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Record.h"
#include "util/Assert.h"

#include <iostream>
#include <functional>
#include <vector>
#include <cstring>

#include <time.h>
#include <stdio.h>

using namespace std;
using namespace srch2::instantsearch;

void test1()
{
    Schema *schema = Schema::create();
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    // create an analyzer
    Analyzer *analyzer = Analyzer::create();
    
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( NULL, mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");
    IndexerInternal *indexer = new IndexerInternal(indexMetaData, analyzer, schema);
    Record *record = new Record(schema);
    char* authorsCharStar = new char[30];
    char* titleCharStar = new char[30];

    //generate random characers
    srand ( time(NULL) );
    // create a record of 3 attributes
    for (unsigned i = 0; i < 1000; i++)
    {
        record->setPrimaryKey(i + 1000);

        sprintf(authorsCharStar,"John %cLen%cnon",(rand() % 50)+65,(rand() % 10)+65);
        string authors = string(authorsCharStar);
        record->setSearchableAttributeValue("article_authors", authors);

        sprintf(titleCharStar,"Yesterday %cOnc%ce %cMore",
                (rand()%59)+65, (rand()%59)+65, (rand()%10)+65);
        string title = string(titleCharStar);
        record->setSearchableAttributeValue("article_title", title);

        record->setRecordBoost(rand() % 100);
        indexer->addRecord(record);

        // for creating another record
        record->clear();
    }

    // build the index
    indexer->save();

    indexer->printNumberOfBytes();

    delete[] authorsCharStar;
    delete[] titleCharStar;
    delete record;
    delete indexer;
    delete analyzer;
    delete schema;
}

void test2()
{
    Schema *schema = Schema::create();

    schema->setPrimaryKey("article_id"); // integer, not searchable

    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    // create an analyzer
    Analyzer *analyzer = Analyzer::create();

    // create a record of 3 attributes
    Record *record = new Record(schema);
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Yesterday Once More");
    record->setRecordBoost(20);

    IndexerInternal *indexer = new IndexerInternal("test", analyzer, schema);

    // add a record
    indexer->addRecord(record);

    // create another record
    record->clear();
    record->setPrimaryKey(1002);

    record->setSearchableAttributeValue(1, "George Harris");
    record->setSearchableAttributeValue(2, "Here comes the sun");

    indexer->addRecord(record);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "George Harris");
    record->setSearchableAttributeValue(2, "Here comes the sun");

    // add a record
    indexer->addRecord(record);

    // build the index
    indexer->save();

    delete record;
    delete indexer;
    delete analyzer;
    delete schema;
}

void addRecords()
{
	///Create Schema
	Schema *schema = Schema::create();
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = Analyzer::create();
	Indexer *indexer = Indexer::create(".", analyzer, schema);

	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
	record->setRecordBoost(10);
	indexer->addRecord(record);

	record->clear();
	record->setPrimaryKey(1008);
	record->setSearchableAttributeValue(0, "Jimi Hendrix");
	record->setSearchableAttributeValue(1, "Little wing");
	record->setRecordBoost(90);
	indexer->addRecord(record);

	indexer->save();

	delete schema;
	delete record;
	delete analyzer;
	delete indexer;
}

void test3()
{
	addRecords();
	IndexerInternal *indexer = new IndexerInternal(".");

	indexer->print_Index();

	Record *record = new Record(indexer->getSchema());
	record->setPrimaryKey(1999);
	record->setSearchableAttributeValue(0, "steve jobs");
	record->setSearchableAttributeValue(1, "stanford speech");
	record->setRecordBoost(90);
	indexer->addRecord(record);

/*	// create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	Analyzer *analyzer = indexer->getAnalyzer();

	indexer->print_Index();

	ASSERT ( ping(analyzer, indexSearcher, "tom" , 1 , 1001) == true);
	ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , 1008) == true);
	ASSERT ( ping(analyzer, indexSearcher, "smith" , 1 , 1001) == true);
	ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , 1999) == true);

	(void)analyzer;
	delete indexSearcher;*/

	 delete indexer;
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }

    test1();

    test2();

    test3();
    cout << "IndexWriterInternal Unit Tests: Passed\n";

    return 0;
}
