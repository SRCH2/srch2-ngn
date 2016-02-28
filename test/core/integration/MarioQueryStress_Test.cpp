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
#include "MapSearchTestHelper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// variables to measure the elapsed time
struct timespec tstart;
struct timespec tend;

int main(int argc, char **argv)
{
	string INDEX_DIR = getenv("index_dir");
	//string filepath = INDEX_DIR+"/map-search/factual-15Mrecords.txt";
	string filepath = INDEX_DIR+"/map-search/factual-5Mrecords.txt";
	//string filepath = INDEX_DIR+"/map-search/factual-8000records.txt";

	/// create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("factualid"); // integer, by default not searchable
	schema->setSearchableAttribute("name"); // searchable text
	schema->setSearchableAttribute("address"); // searchable text
	schema->setSearchableAttribute("phone"); // searchable text
	schema->setSearchableAttribute("fax"); // searchable text
	schema->setSearchableAttribute("category"); // searchable text

	// create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

	// create an index writer
	unsigned mergeEveryNSeconds = 3;	
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites,"", INDEX_DIR, "");
	   	
	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	readRecordsFromFile(filepath, index, schema);

	index->commit();
	index->save();

//	Indexer *indexer = Indexer::load(INDEX_DIR+"/15MIndex");

	// Create an index searcher
	IndexSearcher *indexSearcher = IndexSearcher::create(index);

	string line;
	vector<Rectangle> ranges;
	vector< vector<string> > queries;

	string query_file = getenv("query_file");
	ifstream infile (query_file.c_str(), ios_base::in);
	while (getline(infile, line, '\n'))
	{
		vector<string> query;
		Rectangle range;
		bool pass = parseLine(line, query, range);
		if (!pass)
		{
			cout << line << endl;
			continue;
		}
		queries.push_back(query);
		ranges.push_back (range);
	}

	clock_gettime(CLOCK_REALTIME, &tstart);

	for( unsigned vectIter = 0; vectIter < queries.size(); vectIter++ )
	{
		Query *query = new Query(MapQuery);

		for (unsigned i = 0; i < queries[vectIter].size(); ++i)
		{
			Term *term = FuzzyTerm::create(queries[vectIter][i],
					PREFIX,
					1,
					100.0,
					getNormalizedThreshold(queries[vectIter][i].size()));
			query->add(term);
	//		cout << queries[vectIter][i] << " ";
		}

		query->setRange(ranges[vectIter].min.x, ranges[vectIter].min.y, ranges[vectIter].max.x, ranges[vectIter].max.y);

		/*cout << ranges[vectIter].min.x << " "
			 << ranges[vectIter].max.x << " "
			 << ranges[vectIter].min.y << " "
			 << ranges[vectIter].max.y << " " << endl;
*/
		QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcher, query);

		indexSearcher->search(query, queryResults);
		//cout << queryResults->getNumberOfResults() << endl;

		delete queryResults;
		delete query;
	}

	clock_gettime(CLOCK_REALTIME, &tend);
	double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

	delete indexSearcher;
	delete index;

	cout << "Executed " << queries.size() << " queries in " << ts2 << " milliseconds." << endl;

	cout << "Mario - QueryStressTest passed!" << endl;

	return 0;
}
