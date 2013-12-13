// $Id: MarioIntegration_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>

#include "MapSearchTestHelper.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

using namespace std;
using namespace srch2::instantsearch;

void smallTest(string INDEX_DIR)
{

	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("article_id"); // integer, by default not searchable
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(DISABLE_STEMMER_NORMALIZER, "", "", "", SYNONYM_DONOT_KEEP_ORIGIN);

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR,  "", "");
	   	
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
	
	// Step 2: Create records and add to the index

	// Create a record of 3 attributes
	Record *record = new Record(schema);
	Point point;
	point.x = 100.0;
	point.y = 100.0;
	record->setPrimaryKey(100); // give a value for the primary key
	record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_title", "Yesterday Once More");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	point.x = 101.0;
	point.y = 101.0;
	record->setPrimaryKey(200);
	record->setSearchableAttributeValue(0, "George Harris");
	record->setSearchableAttributeValue(1, "Here comes the sun");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0); // add the record to the index

	record->clear();
	point.x = 102.0;
	point.y = 102.0;
	record->setPrimaryKey(300);
	record->setSearchableAttributeValue(0, "eugene");
	record->setSearchableAttributeValue(1, "Another brick in the wall");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	point.x = -101.0;
	point.y = -101.0;
	record->setPrimaryKey(400);
	record->setSearchableAttributeValue(0, "pink floyd");
	record->setSearchableAttributeValue(1, "Another brick in the wall");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	point.x = -100.0;
	point.y = -100.0;
	record->setPrimaryKey(500);
	record->setSearchableAttributeValue(0, "Another brick in the wall");
	record->setSearchableAttributeValue(1, "pink floyd");
	record->setLocationAttributeValue(point.x, point.y);
	index->addRecord(record, 0); // add the record to the index

	// Step 3: Commit and build the index
	index->commit(); // after commit(), no more records can be added

	// Step 4: Create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	// STEP 5: Create a Query object and a QueryResults object
	Query *query = new Query(MapQuery);
	string keywords [] = {
			"another","brick"
	};

	for (unsigned i = 0; i < 2; ++i)
	{
		Term *term = FuzzyTerm::create(keywords[i],
				PREFIX,
				getNormalizedThreshold(keywords[i].size()));
		query->add(term);
	}

	query->setRange(-150.0, -150.0, -50.0, -50.0);

	QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcher, query);

	// Step 6: Search the index and display results

	indexSearcher->search(query, queryResults);

	printResults(queryResults);

	// free references
	delete query;
	delete indexSearcher;
	delete index;
	delete analyzer;
	delete schema;
}

void bigTest(string INDEX_DIR)
{
	// STEP 1: Create index

	string filepath = INDEX_DIR+"/map-search/factual-8000records.txt";

	/// create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("factualid"); // integer, by default not searchable
	schema->setSearchableAttribute("name"); // searchable text
	schema->setSearchableAttribute("address"); // searchable text
	schema->setSearchableAttribute("phone"); // searchable text

	// create an analyzer
	Analyzer *analyzer = new Analyzer(DISABLE_STEMMER_NORMALIZER, "", "", "", SYNONYM_DONOT_KEEP_ORIGIN);

	// create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR,  "", "");
	   	
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
	
	readRecordsFromFile(filepath, index, schema);

	index->commit();

	// Step 2: Create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	// STEP 3: Create a Query object and a QueryResults object
	// fuzzy prefix search for record: 519^33.936866^-117.430088^Perez Arthur Investigations^4106 Madrona Rd^951 359-8501

	Query *query = new Query(MapQuery);
	string keywords [] = {
			"pare","medr"
	};

	for (unsigned i = 0; i < 2; ++i)
	{
		Term *term = FuzzyTerm::create(keywords[i],
				PREFIX,
				1,
				100.0,
				getNormalizedThreshold(keywords[i].size()));
		query->add(term);
	}

	query->setRange(33.926866, -117.440088, 33.946866, -117.420088);

	QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcher, query);

	// Step 4: Search the index and display results

	indexSearcher->search(query, queryResults);

	//cout << queryResults->getRecordId(0) << endl;

	ASSERT(queryResults->getNumberOfResults() == 1);
	ASSERT(queryResults->getRecordId(0).compare("519") == 0);

	printResults(queryResults);

	// free references
	delete query;
	delete indexSearcher;
	delete index;
	delete analyzer;
	delete schema;
}

/**
 * The main function
 */
int main(int argc, char **argv)
{
	string INDEX_DIR = getenv("index_dir");

	smallTest(INDEX_DIR);

	bigTest(INDEX_DIR);

	std::cout << "Map Search integration tests passed." << std::endl;

	return 0;
}
