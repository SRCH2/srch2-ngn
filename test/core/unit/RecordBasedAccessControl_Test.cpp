/*
 * RecordBaseAccessControl_Test.cpp
 *
 *  Created on: Aug 28, 2014
 *      Author: mahdi
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>
#include <iomanip>
#include "util/Assert.h"

#include <instantsearch/Query.h>
#include <instantsearch/Term.h>

#include <instantsearch/GlobalCache.h>

#include "operation/IndexerInternal.h"
#include "operation/IndexData.h"
#include "instantsearch/QueryEvaluator.h"
#include "operation/QueryEvaluatorInternal.h"

#include "test/core/integration/IntegrationTestHelper.h"

using namespace std;
using namespace srch2::instantsearch;

Indexer *indexer;
Schema *schema;
Analyzer *analyzer;

void addRecord(Indexer *indexer, Schema *schema, Analyzer* analyzer, unsigned primaryKey,
		const string &firstAttribute, const string &secondAttribute, vector<string> &roleIds)
{
	Record *record = new Record(schema);
	record->setPrimaryKey(primaryKey); // give a value for the primary key
	record->setSearchableAttributeValue(0, firstAttribute);
	record->setSearchableAttributeValue(1, secondAttribute);
	for(unsigned i = 0 ; i < roleIds.size() ; ++i)
		record->addRoleId(roleIds[i]);

	indexer->addRecord(record, analyzer);

	delete record;
}

void testInsert(){

	// Create 3 records of 2 attributes and add them to the index
	vector<string> roleIds;
	roleIds.push_back("100");
	addRecord(indexer, schema, analyzer, 1, "Tom Smith and Sara", "Yesterday Once More", roleIds);
	roleIds.clear();

	roleIds.push_back("100");
	roleIds.push_back("200");
	addRecord(indexer, schema, analyzer, 2, "Tom Smith and Jack Lennon", "Yesterday Once More", roleIds);
	roleIds.clear();

	roleIds.push_back("200");
	roleIds.push_back("300");
	addRecord(indexer, schema, analyzer, 3, "Tom Smith and Peter", "Yesterday Once More", roleIds);
	roleIds.clear();

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);

    // test search with ACL
    vector<unsigned> recordIds;
    recordIds.push_back(1);
    recordIds.push_back(2);
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "100") == true);

    recordIds.clear();
    recordIds.push_back(3);
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 1 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "300") == true);

    recordIds.clear();
    recordIds.push_back(2);
    recordIds.push_back(3);
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "200") == true);

    // test search without ACL
    recordIds.clear();
    recordIds.push_back(1);
    recordIds.push_back(2);
    recordIds.push_back(3);
    ASSERT ( ping(analyzer, queryEvaluator, "tom", 3 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

}

void testAddRole(){

    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);

    vector<unsigned> recordIds;
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 0 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "400") == true);

    recordIds.clear();
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 0 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "500") == true);

	vector<string> roleIds;
	roleIds.push_back("400");
	roleIds.push_back("500");
	indexer->aclRoleAdd("1",roleIds);

    recordIds.clear();
    recordIds.push_back(1);
    ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 1 , recordIds,
    		vector<unsigned>(), ATTRIBUTES_OP_AND, "500") == true);

    roleIds.clear();
	roleIds.push_back("400");
	indexer->aclRoleAdd("2",roleIds);

	recordIds.clear();
	recordIds.push_back(1);
	recordIds.push_back(2);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "400") == true);
}

void testDeleteRole(){
	QueryEvaluatorRuntimeParametersContainer runtimeParameters;
	QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);

	vector<unsigned> recordIds;
	recordIds.push_back(1);
	recordIds.push_back(2);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "400") == true);

	vector<string> roleIds;
	roleIds.push_back("400");
	roleIds.push_back("500");
	indexer->aclRoleDelete("1",roleIds);

	recordIds.clear();
	recordIds.push_back(2);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 1 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "400") == true);

	roleIds.clear();
	roleIds.push_back("400");
	roleIds.push_back("500");
	indexer->aclRoleDelete("2",roleIds);

	recordIds.clear();
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 0 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "400") == true);

}

void testSerialization(string directoryName){
	indexer->commit();
	indexer->save(directoryName);

	Schema *schema2 = Schema::create(LocationIndex);
	schema2->setPrimaryKey("list_id"); // integer, by default not searchable
	schema2->setSearchableAttribute("title", 2); // searchable text
	schema2->setSearchableAttribute("address", 7); // searchable text
	Analyzer *analyzer2 = new Analyzer(NULL, NULL, NULL, NULL, "", srch2is::STANDARD_ANALYZER);
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	unsigned updateHistogramEveryPMerges = 1;
	unsigned updateHistogramEveryQWrites = 5;
	CacheManager *cache = new CacheManager(134217728);
	IndexMetaData *indexMetaData = new IndexMetaData(cache,
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			directoryName);

	Indexer *indexer2 = Indexer::load(indexMetaData);

	QueryEvaluatorRuntimeParametersContainer runtimeParameters;
	QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer2, &runtimeParameters);

	vector<unsigned> recordIds;
	recordIds.push_back(1);
	recordIds.push_back(2);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "100") == true);

	recordIds.clear();
	recordIds.push_back(3);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 1 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "300") == true);

	recordIds.clear();
	recordIds.push_back(2);
	recordIds.push_back(3);
	ASSERT ( ping_WithACL(analyzer, queryEvaluator, "tom", 2 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND, "200") == true);

	// test search without ACL
	recordIds.clear();
	recordIds.push_back(1);
	recordIds.push_back(2);
	recordIds.push_back(3);
	ASSERT ( ping(analyzer, queryEvaluator, "tom", 3 , recordIds,
			vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

}

void init(string directoryName){
	// Create a schema
	schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	analyzer = new Analyzer(NULL, NULL, NULL, NULL, "", srch2is::STANDARD_ANALYZER);

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	unsigned updateHistogramEveryPMerges = 1;
	unsigned updateHistogramEveryQWrites = 5;
	CacheManager *cache = new CacheManager(134217728);
	IndexMetaData *indexMetaData = new IndexMetaData( cache,
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			directoryName);

	indexer = Indexer::create(indexMetaData, analyzer, schema);
}

int main(int argc, char *argv[])
{

	bool verbose = false;
	if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
		verbose = true;
	}

	const string directoryName = getenv("index_dir");
	//string directoryName = "../test/unit/test_data";

	init(directoryName);

	cout << " Test 1: Insert new records with ACL ..." << endl;
	testInsert();
	cout << " Test 1 passed." << endl;

	cout << " Test 2: Allow more roles for a record ..." << endl;
	testAddRole();
	cout << " Test 2 passed." << endl;

	cout << " Test 3: Disallow roles for a record ..." << endl;
	testDeleteRole();
	cout << " Test 3 passed." << endl;

	cout << " Test 4: Serialization ..." << endl;
	testSerialization(directoryName);
	cout << " Test 4 passed." << endl;
	return 0;
}
