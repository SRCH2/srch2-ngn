
// $Id: Searcher_API2_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $


#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"

#include "util/Assert.h"

#include <cassert>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// This Integration test is for API 2 i.e term->addAttributeToFilterTermHits(attributeId);


void buildSimpleIndex(string INDEX_DIR)
{

	// Create a schema
	srch2is::Schema *schema = srch2is::Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, by default not searchable
	//schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	// Step 2: Create records and add to the index

	// Create a record of 3 attributes
	srch2is::Record *record = new srch2is::Record(schema);
	record->setPrimaryKey(100); // give a value for the primary key
	record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_title", "Yesterday Once More");
	record->setRecordBoost(20); // set its boost value
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	record->setPrimaryKey(200);
	record->setSearchableAttributeValue(0, "George Harris");
	record->setSearchableAttributeValue(1, "Here comes the sun");
	index->addRecord(record, 0); // add the record to the index

	record->clear();
	record->setPrimaryKey(300);
	record->setSearchableAttributeValue(0, "eugene");
	record->setSearchableAttributeValue(1, "Another brick in the wall");
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	record->setPrimaryKey(400);
	record->setSearchableAttributeValue(0, "pink floyd");
	record->setSearchableAttributeValue(1, "Another brick in the wall");
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	record->setPrimaryKey(500);
	record->setSearchableAttributeValue(0, "Another brick in the wall");
	record->setSearchableAttributeValue(1, "pink floyd");
	index->addRecord(record, 0); // add the record to the index

	// Step 3: Commit and build the index
	index->commit();
	index->save(); // after commit(), no more records can be added

	// free references
	delete schema;
	delete analyzer;
	delete index;
}

bool test(string INDEX_DIR, vector<unsigned> &recordId, int attributeId, unsigned noofhits, srch2is::TermType type = srch2is::PREFIX)
{
	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *indexer = Indexer::load(indexMetaData1);

	// Create an index searcher
	srch2is::IndexSearcher *indexSearcher = srch2is::IndexSearcher::create(indexer);

	// STEP 2: Create a Query object and a QueryResults object
	srch2is::Query *query = new srch2is::Query(srch2is::TopKQuery);
	string keywords [] = {
			"another","brick"
	};

	//std::cout << "SearchableAttributeId:" << indexReader->getSchema()->getSearchableAttributeId("article_title") << std::endl;
	// For each keyword above, add a corresponding fuzzy term to the query
	for (unsigned i = 0; i < 2; ++i)
	{
		//srch2is::TermType type = srch2is::PREFIX; // prefix matching
		//srch2is::TermType type = srch2is::COMPLETE; // use it for complete matching
		unsigned termBoost = 1;
		unsigned similarityBoost = 100;
		srch2is::Term *term = srch2is::FuzzyTerm::create(keywords[i],
				type,
				termBoost,
				similarityBoost,
				getNormalizedThreshold(keywords[i].size()));
		term->addAttributeToFilterTermHits(attributeId);
		query->add(term);
	}

	srch2is::QueryResults *queryResults = new srch2is::QueryResults(new QueryResultFactory(),indexSearcher, query);

	// Step 3: Search the index and display results

	// Search for 4 records
	unsigned offset = 0;
	unsigned resultsToRetrieve = 4;

	// Do the search
	indexSearcher->search(query, queryResults, offset, resultsToRetrieve);

	// Iterate through the results
	bool returnvalue = checkResults(queryResults, noofhits, recordId);
	printResults(queryResults, offset);

	// Free the objects
	delete query;
	delete indexSearcher;
	delete indexer;

	return returnvalue;
}

/**
 * The main function
 */
int main(int argc, char **argv)
{
	// STEP 1: Create IndexReader and IndexSearcher
	// Define a folder for the index files
	string INDEX_DIR = getenv("index_dir");
	//string INDEX_DIR = ".";
	cout << INDEX_DIR <<endl;

	buildSimpleIndex(INDEX_DIR);

	// Tests API 2, i.e term->addAttributeToFilterTermHits(attributeId);

	vector<unsigned> recordId;

	recordId.push_back(500);
	ASSERT( test(INDEX_DIR, recordId, 0, 1, srch2is::PREFIX) == true);

	recordId.clear();
	recordId.push_back(300);
	recordId.push_back(400);
	ASSERT( test(INDEX_DIR, recordId, 1, 2, srch2is::PREFIX) == true);

	recordId.clear();
	recordId.push_back(300);
	ASSERT( test(INDEX_DIR, recordId, 0, 2, srch2is::PREFIX) == false);

	recordId.clear();
	recordId.push_back(500);
	ASSERT( test(INDEX_DIR, recordId, 0, 1, srch2is::COMPLETE) == true);

	recordId.clear();
	recordId.push_back(400);
	recordId.push_back(300);
	ASSERT( test(INDEX_DIR, recordId, 1, 2, srch2is::COMPLETE) == true);

	recordId.clear();
	recordId.push_back(300);
	ASSERT( test(INDEX_DIR, recordId, 0, 2, srch2is::COMPLETE) == false);


	std::cout << "Searcher API tests passed." << std::endl;

	return 0;
}
