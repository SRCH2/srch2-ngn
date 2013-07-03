// $Id: NormalizerIntegration_Test.cpp 3027 2012-12-05 02:22:07Z oliverax $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"

#include <cassert>
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



void buildSimpleIndex(string INDEX_DIR)
{

	// Create a schema
	bmis::Schema *schema = bmis::Schema::create(bmis::DefaultIndex);
	schema->setPrimaryKey("record_id"); // integer, by default not searchable
	//schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create stemmer type
	//bmis::StemmerType stemType = NORMALIZER;
	bmis::StemmerNormalizerType stemType = bmis::STEMMER_NORMALIZER;

	// Create an analyzer
	Analyzer *analyzer = Analyzer::create(stemType, "");

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites,"", INDEX_DIR, "");
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
	// Step 2: Create records and add to the index

	// Create a record of 3 attributes
	bmis::Record *record = new bmis::Record(schema);
	record->setPrimaryKey(100);
	record->setSearchableAttributeValue(0, "Wal-mart");
	record->setSearchableAttributeValue(1, "Wal mart is the biggest convenient store in United states");
	index->addRecord(record, 0); // add the record to the index


	// Create another record
	record->clear();
	record->setPrimaryKey(200);
	record->setSearchableAttributeValue(0, "Cheese-cake factory");
	record->setSearchableAttributeValue(1, "cheese cake factory serves awesome cheese-cakes loved by children. It tastes great with coffee ");
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	record->setPrimaryKey(300);
	record->setSearchableAttributeValue(0, "star-bucks");
	record->setSearchableAttributeValue(1, "star bucks is one of the famous coffee stores in United States");
	index->addRecord(record, 0); // add the record to the index

	index->commit();
	index->save(); // after commit(), no more records can be added


	// free references
	delete schema;
	delete analyzer;
	delete index;

}

bool test(string INDEX_DIR, vector<unsigned> &recordId, int attributeId, unsigned noofhits, bmis::TermType type = bmis::PREFIX)
{
	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *index = Indexer::load(indexMetaData1);

	// Create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	// STEP 2: Create a Query object and a QueryResults object
	bmis::Query *query = new bmis::Query(bmis::TopKQuery);
	string keywords [] = {
			"starbucks","coffee" //"starbucks"//,"prediction","weak" //,"walmart"   //,"organization"
	};

	//std::cout << "SearchableAttributeId:" << indexReader->getSchema()->getSearchableAttributeId("article_title") << std::endl;
	// For each keyword above, add a corresponding fuzzy term to the query
	for (unsigned i = 0; i < 2; ++i)
	{
		//bmis::TermType type = bmis::PREFIX; // prefix matching
		//bmis::TermType type = bmis::COMPLETE; // use it for complete matching
		unsigned termBoost = 1;
		unsigned similarityBoost = 100;
		bmis::Term *term = ExactTerm::create(keywords[i],
				type,
				termBoost,
				similarityBoost);
		//term->addAttributeToFilterTermHits(attributeId);
		query->add(term);
	}

	bmis::QueryResults *queryResults = bmis::QueryResults::create(indexSearcher, query);

	// Step 3: Search the index and display results

	// Search for 4 records
	unsigned offset = 0;
	unsigned resultsToRetrieve = 4;
	//bool isStemmed;

	// Do the search
	indexSearcher->search(query, queryResults, offset, resultsToRetrieve);

	// Iterate through the results
	bool returnvalue = checkResults(queryResults, noofhits, recordId);
	//	bool stemValidation = (stemmed & isStemmed);
	printResults(queryResults, offset);
	//cout<<boolalpha;
	cout << "return value: "<< returnvalue <<endl;


	// Free the objects
	delete query;
	delete indexSearcher;
	delete index;
	return (returnvalue); //& stemValidation);
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

	recordId.clear();
	recordId.push_back(300);
	recordId.push_back(200);
	test(INDEX_DIR, recordId, 1, 2, bmis::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 2, bmis::PREFIX)==true);

	recordId.clear();
	recordId.push_back(100);
	test(INDEX_DIR, recordId, 1, 0, bmis::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 0, bmis::PREFIX)==true);

	recordId.clear();
	//recordId.push_back(200);
	recordId.push_back(300);
	test(INDEX_DIR, recordId, 1, 1, bmis::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 2, bmis::PREFIX)==true) ;

	recordId.clear();
	recordId.push_back(100);
	test(INDEX_DIR, recordId, 1, 0, bmis::PREFIX);
	//assert(test(INDEX_DIR, recordId,1,1,bmis::PREFIX)==false);


	std::cout << "Normalizer API tests passed." << std::endl;

	return 0;
}



