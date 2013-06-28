// $Id: StemmerIntegration_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $
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
namespace srch2is = srch2::instantsearch;
using namespace srch2is;



void buildIndexWithDefaultStemmerAndNormalizer(string INDEX_DIR)
{

	// Create a schema
	srch2is::Schema *schema = srch2is::Schema::create(srch2is::DefaultIndex);
	schema->setPrimaryKey("record_id"); // integer, by default not searchable
	//schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create stemmer type
	srch2is::StemmerNormalizerType stemType = srch2is::STEMMER_NORMALIZER;

	// Create an analyzer
	srch2is::Analyzer *analyzer = srch2is::Analyzer::create(stemType, " ");

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
	
	// Step 2: Create records and add to the index

	// Create a record of 3 attributes
	srch2is::Record *record = new srch2is::Record(schema);
//	record->setPrimaryKey(100); // give a value for the primary key
//	record->setSearchableAttributeValue("record_location", "star-bucks");
//	record->setSearchableAttributeValue("record_description", "Star-bucks is one of the famous coffee stores in United States");
//	record->setRecordBoost(20); // set its boost value
//	indexer->addRecord(record); // add the record to the index

//	record->clear();
	// Create a record
	record->setPrimaryKey(100);
	record->setSearchableAttributeValue(0, "Wal-mart");
	record->setSearchableAttributeValue(1, "Wal-mart is the biggest star-bucks convenient store in United states provable");
	index->addRecord(record, 0); // add the record to the index


	// Create another record
	record->clear();
	record->setPrimaryKey(200);
	record->setSearchableAttributeValue(0, "Cheese-cake factory");
	record->setSearchableAttributeValue(1, "cheese-cake factory serves awesome cheese-cakes loved by children. It tastes great with starbucks coffee ");
	index->addRecord(record, 0); // add the record to the index

	// Create another record
	record->clear();
	record->setPrimaryKey(300);
	record->setSearchableAttributeValue(0, "starbucks");
	record->setSearchableAttributeValue(1, "Starbucks is one of the famous coffee stores in United States");
	index->addRecord(record, 0); // add the record to the index


	record->clear();
	record->setPrimaryKey(500);
	record->setSearchableAttributeValue(0, "star-bucks");
	record->setSearchableAttributeValue(1, "Star-bucks is one of the famous coffee stores in United States");
	index->addRecord(record, 0);

	// Create another record
//	record->clear();
//	record->setPrimaryKey(500);
//	record->setSearchableAttributeValue(0, "pink floyd");
//	record->setSearchableAttributeValue(1, "cambric cambyses adversary pencils");
//	indexer->addRecord(record); // add the record to the index

	// Step 3: Commit and build the index
	index->commit();
	index->save(); // after commit(), no more records can be added

	// free references
	delete schema;
	delete analyzer;
	delete index;
}


bool test(string INDEX_DIR, vector<unsigned> &recordId, int attributeId, unsigned noofhits, string keywords[], bool isStemmed,srch2is::TermType type = srch2is::PREFIX)
{
	// Create an index reader
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *indexer = Indexer::load(indexMetaData1);

	// Create an index searcher
	srch2is::IndexSearcher *indexSearcher = srch2is::IndexSearcher::create(indexer);

	// STEP 2: Create a Query object and a QueryResults object
	srch2is::Query *query = new srch2is::Query(srch2is::TopKQuery);

	//std::cout << "SearchableAttributeId:" << indexReader->getSchema()->getSearchableAttributeId("article_title") << std::endl;
	// For each keyword above, add a corresponding fuzzy term to the query
	for (unsigned i = 0; i < 2; ++i)
	{
		//srch2is::TermType type = srch2is::PREFIX; // prefix matching
		//srch2is::TermType type = srch2is::COMPLETE; // use it for complete matching
		unsigned termBoost = 1;
		unsigned similarityBoost = 100;
		srch2is::Term *term = srch2is::ExactTerm::create(keywords[i],
				type,
				termBoost,
				similarityBoost);
		//term->addAttributeToFilterTermHits(attributeId);
		query->add(term);
	}

	srch2is::QueryResults *queryResults = srch2is::QueryResults::create(indexSearcher, query);

	// Step 3: Search the index and display results

	// Search for records
	unsigned offset = 0;
	unsigned resultsToRetrieve = 4;

	// Do the search
	indexSearcher->search(query, queryResults, offset, resultsToRetrieve);


	// Iterate through the results

	bool returnvalue = checkOutput(queryResults, noofhits, isStemmed);

	printResults(queryResults, offset);
	cout<<boolalpha;
	cout << "return information: "<<returnvalue<<endl;


	// Free the objects
	delete query;
	delete indexSearcher;
	delete indexer;

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

	//Testing with Normalizer and Stemmer

	buildIndexWithDefaultStemmerAndNormalizer(INDEX_DIR);

	vector<unsigned> recordId;

	recordId.clear();
	string keywords [] = {
			"starbuck","coffee"  // starbuck is a stemmed word in the records
	};
	recordId.push_back(300);
	recordId.push_back(200);
	recordId.clear();

	//First 3 ASSERTS test for correct #records and stemmed information

	//correct record info and stem info provided
	ASSERT(test(INDEX_DIR, recordId, 1, 3, keywords, true,srch2is::PREFIX )==true);

	//correct record info and wrong stem info provided
	ASSERT(test(INDEX_DIR, recordId, 1, 3 , keywords,false,srch2is::PREFIX)==false);

	//wrong record info and correct stem info provided
	ASSERT(test(INDEX_DIR, recordId, 1, 0, keywords,true,srch2is::PREFIX)==false);


	//changing the query "starbuck" to "starbucks"
	keywords[0] = "starbucks";

	recordId.push_back(500);


    //correct record info and stem info provided
   	ASSERT(test(INDEX_DIR, recordId, 1, 3, keywords, true,srch2is::PREFIX )==true);

   	//correct record info and wrong stem info provided
    ASSERT(test(INDEX_DIR, recordId, 1, 3 , keywords,false,srch2is::PREFIX)==false);

   	//wrong record info and correct stem info provided
    ASSERT(test(INDEX_DIR, recordId, 1, 0, keywords,true,srch2is::PREFIX)==false);

    keywords[0] = "walmart"; // This is a stemmed word in the records
    keywords[1] = "store";

    //correct record and stem info
    ASSERT(test(INDEX_DIR, recordId, 1, 1, keywords, true,srch2is::PREFIX )==true);

    //correct record and wrong stem info
    ASSERT(test(INDEX_DIR, recordId, 1, 1, keywords, false,srch2is::PREFIX )==false);

    std::cout << "Stemmer API tests passed." << std::endl;

	return 0;
}


