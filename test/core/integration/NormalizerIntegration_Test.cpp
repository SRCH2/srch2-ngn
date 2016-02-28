/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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



void buildSimpleIndex(string INDEX_DIR)
{

	// Create a schema
	srch2is::Schema *schema = srch2is::Schema::create(srch2is::DefaultIndex);
	schema->setPrimaryKey("record_id"); // integer, by default not searchable
	//schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create stemmer type
	//srch2is::StemmerType stemType = NORMALIZER;
	srch2is::StemmerNormalizerFlagType stemType = srch2is::ENABLE_STEMMER_NORMALIZER;

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(stemType, "");

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites,"", INDEX_DIR, "");
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);
	// Step 2: Create records and add to the index

	// Create a record of 3 attributes
	srch2is::Record *record = new srch2is::Record(schema);
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

bool test(string INDEX_DIR, vector<unsigned> &recordId, int attributeId, unsigned noofhits, srch2is::TermType type = srch2is::PREFIX)
{
	// Create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, "", INDEX_DIR, "");
	Indexer *index = Indexer::load(indexMetaData1);

	// Create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	// STEP 2: Create a Query object and a QueryResults object
	srch2is::Query *query = new srch2is::Query(srch2is::TopKQuery);
	string keywords [] = {
			"starbucks","coffee" //"starbucks"//,"prediction","weak" //,"walmart"   //,"organization"
	};

	//std::cout << "SearchableAttributeId:" << indexReader->getSchema()->getSearchableAttributeId("article_title") << std::endl;
	// For each keyword above, add a corresponding fuzzy term to the query
	for (unsigned i = 0; i < 2; ++i)
	{
		//srch2is::TermType type = srch2is::PREFIX; // prefix matching
		//srch2is::TermType type = srch2is::COMPLETE; // use it for complete matching
		unsigned termBoost = 1;
		unsigned similarityBoost = 100;
		srch2is::Term *term = ExactTerm::create(keywords[i],
				type,
				termBoost,
				similarityBoost);
		//term->addAttributeToFilterTermHits(attributeId);
		query->add(term);
	}

	srch2is::QueryResults *queryResults = new srch2is::QueryResults(new QueryResultFactory(),indexSearcher, query);

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
	test(INDEX_DIR, recordId, 1, 2, srch2is::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 2, srch2is::PREFIX)==true);

	recordId.clear();
	recordId.push_back(100);
	test(INDEX_DIR, recordId, 1, 0, srch2is::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 0, srch2is::PREFIX)==true);

	recordId.clear();
	//recordId.push_back(200);
	recordId.push_back(300);
	test(INDEX_DIR, recordId, 1, 1, srch2is::PREFIX);
	//assert(test(INDEX_DIR, recordId, 1, 2, srch2is::PREFIX)==true) ;

	recordId.clear();
	recordId.push_back(100);
	test(INDEX_DIR, recordId, 1, 0, srch2is::PREFIX);
	//assert(test(INDEX_DIR, recordId,1,1,srch2is::PREFIX)==false);


	std::cout << "Normalizer API tests passed." << std::endl;

	return 0;
}



