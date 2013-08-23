//$Id: QuadTree_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>
#include <iomanip>
#include "geo/QuadTree.h"
#include "util/Assert.h"

#include <instantsearch/Query.h>
#include <instantsearch/Term.h>

#include <instantsearch/IndexSearcher.h>
#include <instantsearch/GlobalCache.h>

#include "../integration/MapSearchTestHelper.h"
#include "operation/TermVirtualList.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "operation/IndexData.h"

using namespace std;
using namespace srch2::instantsearch;

bool parseDouble(string &line, double &doubleNum)
{
	istringstream coordinatesStream(line);
	double num;
	vector<double> nums;
	while(coordinatesStream >> num)
	{
		nums.push_back(num);
	}
	if(nums.size() == 1)
	{
		doubleNum = nums[0];
	}
	else
	{
		return false;
	}
	return true;
}

bool parseUnsigned(string &line, unsigned &pk)
{
	istringstream coordinatesStream(line);
	unsigned num;
	vector<unsigned> nums;
	while(coordinatesStream >> num)
	{
		nums.push_back(num);
	}
	if(nums.size() == 1)
	{
		pk = nums[0];
	}
	else
	{
		return false;
	}
	return true;
}

bool parseMultiUnsigned(string &line, unsigned numOfResults, vector<unsigned> &expectedResults)
{
	istringstream coordinatesStream(line);
	unsigned num;
	vector<unsigned> nums;
	while(coordinatesStream >> num)
	{
		nums.push_back(num);
	}
	if(nums.size() == numOfResults)
	{
		expectedResults = nums;
	}
	else
	{
		return false;
	}
	return true;
}

bool parsePoint(string &line, Point &pt)
{
	istringstream coordinatesStream(line);
	double coord;
	vector<double> coords;
	while(coordinatesStream >> coord)
	{
		coords.push_back(coord);
	}
	if(coords.size() == 2)
	{
		pt.x = coords[0];
		pt.y = coords[1];
	}
	else
	{
		return false;
	}
	return true;
}

void addLocationRecord(Indexer *indexer, Schema *schema, Analyzer* analyzer, unsigned primaryKey, const string &firstAttribute, const string &secondAttribute, double pointX, double pointY)
{
	Point point;
	point.x = pointX;
	point.y = pointY;
	Record *record = new Record(schema);
	record->setPrimaryKey(primaryKey); // give a value for the primary key
	record->setSearchableAttributeValue(0, firstAttribute);
	record->setSearchableAttributeValue(1, secondAttribute);
	record->setLocationAttributeValue(point.x, point.y);

	indexer->addRecord(record, analyzer, 0);

	delete record;
}

// each record has two searchable attributes
void readRecordsFromFile(Indexer *indexer, Schema *schema, Analyzer* analyzer, const string &directoryName)
{
	string primaryKeysFile = directoryName + "/primaryKeys.txt";
	string pointsFile = directoryName + "/points.txt";
	string firstAttrsFile = directoryName + "/firstAttr.txt";
	string secondAttrsFile = directoryName + "/secondAttr.txt";

	ifstream primaryKeys(primaryKeysFile.c_str());
	ASSERT ( primaryKeys ); // cannot open primaryKeys file

	ifstream points(pointsFile.c_str());
	ASSERT  ( points ); //cannot open points file

	ifstream firstAttrs(firstAttrsFile.c_str());
	ASSERT  ( firstAttrs ); // cannot open firstAttrs file

	ifstream secondAttrs(secondAttrsFile.c_str());
	ASSERT  ( secondAttrs ); //cannot open secondAttrs file

	string primaryKeyLine;
	string pointLine;
	string firstAttrLine;
	string secondAttrLine;

	// inserting objects in the Quadtree
	while (getline(primaryKeys, primaryKeyLine)
			&& getline(points, pointLine)
			&& getline(firstAttrs, firstAttrLine)
			&& getline(secondAttrs, secondAttrLine))
	{
		unsigned primaryKey = 0;
		Point point;
		point.x = 0.0;
		point.y = 0.0;

        bool retval = parseUnsigned(primaryKeyLine, primaryKey);
		ASSERT  ( retval ); //cannot parse primaryKeys file correctly
        retval = parsePoint(pointLine, point);
		ASSERT  ( retval ); //cannot parse points file correctly"
        (void)retval;

		addLocationRecord(indexer, schema, analyzer, primaryKey, firstAttrLine, secondAttrLine, point.x, point.y);
	}
}

bool parseQueryStrings(string &line, vector<string> &strings, unsigned numOfQueryStrings)
{
	istringstream prefixesStream(line);
	string str;
	vector<string> strs;
	while(prefixesStream >> str)
	{
		strs.push_back(str);
	}
	if(strs.size() == numOfQueryStrings)
	{
		strings = strs;
	}
	else
	{
		return false;
	}
	return true;
}

void searchIndex(Indexer *indexer, double minX, double minY, double maxX, double maxY, vector<string> &keywords, vector<unsigned> &expectedResults, bool isFuzzy = false)
{
	//GlobalCache *cache = GlobalCache::create(100000,1000);
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	//IndexSearcher *indexSearcher = IndexSearcher::create(indexer, cache);

	Query *query = new Query(MapQuery);

	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		if(isFuzzy)
		{
			Term *term = FuzzyTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0,
					getNormalizedThresholdGeo(keywords[i].size()));
			query->add(term);
		}
		else
		{
			Term *term = ExactTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0);
			query->add(term);
		}
	}

	query->setRange(minX, minY, maxX, maxY);

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults);

	vector<unsigned> ids;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
	//	cout << queryResults->getRecordId(i) << endl;
		ids.push_back((unsigned)atoi(queryResults->getRecordId(i).c_str()));
	}
	//cout << "--- " << expectedResults.size() << endl;

	ASSERT( queryResults->getNumberOfResults() ==  expectedResults.size());

	ASSERT( ifAllFoundResultsAreCorrect(expectedResults, ids) );

	ASSERT( ifAllExpectedResultsAreFound(expectedResults, ids) );

	delete queryResults;
	delete query;

	//delete cache;
	delete indexSearcher;
}

void searchIndexCircle(Indexer *indexer, double x, double y, double radius, vector<string> &keywords, vector<unsigned> &expectedResults, bool isFuzzy = false)
{
	//GlobalCache *cache = GlobalCache::create(100000,1000);
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	//IndexSearcher *indexSearcher = IndexSearcher::create(indexer, cache);

	Query *query = new Query(MapQuery);

	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		if(isFuzzy)
		{
			Term *term = FuzzyTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0,
					getNormalizedThresholdGeo(keywords[i].size()));
			query->add(term);
		}
		else
		{
			Term *term = ExactTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0);
			query->add(term);
		}
	}

	query->setRange(x, y, radius);

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults);

	vector<unsigned> ids;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
	//	cout << queryResults->getRecordId(i) << endl;
		ids.push_back((unsigned)atoi(queryResults->getRecordId(i).c_str()));
	}
	//cout << "--- " << expectedResults.size() << endl;

	ASSERT( queryResults->getNumberOfResults() ==  expectedResults.size());

	ASSERT( ifAllFoundResultsAreCorrect(expectedResults, ids) );

	ASSERT( ifAllExpectedResultsAreFound(expectedResults, ids) );

	delete queryResults;
	delete query;

	//delete cache;
	delete indexSearcher;
}

void searchIndexCheckResultsNumberOnly(Indexer *indexer, double minX, double minY, double maxX, double maxY, vector<string> &keywords, unsigned number, bool isFuzzy = false)
{
	//GlobalCache *cache = GlobalCache::create(100000,1000);
	//IndexSearcher *indexSearcher = IndexSearcher::create(indexer, cache);
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

	Query *query = new Query(MapQuery);

	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		if(isFuzzy)
		{
			Term *term = FuzzyTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0,
					getNormalizedThresholdGeo(keywords[i].size()));
			query->add(term);
		}
		else
		{
			Term *term = ExactTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0);
			query->add(term);
		}
	}

	query->setRange(minX, minY, maxX, maxY);

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults);

	cout << "number of results: " << queryResults->getNumberOfResults() << endl;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		vector<string> matchingKeywords;
		queryResults->getMatchingKeywords(i, matchingKeywords);
		cout << queryResults->getRecordId(i) << " | Score: " << queryResults->getResultScore(i)
											<<  " | Match: " << matchingKeywords[0] << endl;
	}

	ASSERT( queryResults->getNumberOfResults() ==  number);

	delete queryResults;
	delete query;

	//delete cache;
	delete indexSearcher;
}

void searchIndexNoCheck(Indexer *indexer, double minX, double minY, double maxX, double maxY, vector<string> &keywords, bool isFuzzy = false)
{

	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

	Query *query = new Query(MapQuery);

	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		if(isFuzzy)
		{
			Term *term = FuzzyTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0,
					getNormalizedThresholdGeo(keywords[i].size()));
			query->add(term);
		}
		else
		{
			Term *term = ExactTerm::create(keywords[i],
					TERM_TYPE_PREFIX,
					1,
					100.0);
			query->add(term);
		}
	}

	query->setRange(minX, minY, maxX, maxY);

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults);

	cout << "number of results: " << queryResults->getNumberOfResults() << endl;
	/*for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		vector<string> matchingKeywords;
		queryResults->getMatchingKeywords(i, matchingKeywords);
		cout << queryResults->getRecordId(i) << " | Score: " << queryResults->getResultScore(i)
											<<  " | Match: " << matchingKeywords[0] << endl;
	}*/

	delete queryResults;
	delete query;

	delete indexSearcher;
}
/*
void searchIndexTwoSteps(Indexer *indexer, double minX, double minY, double maxX, double maxY, vector<string> &keywords, unsigned exactMatchNumber, vector<unsigned> &resultIds)
{
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

	Query *exactQuery = new Query(MapQuery);

	// do exact search first anyway
	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		Term *term = ExactTerm::create(keywords[i],
				PREFIX,
				1,
				100.0);
		exactQuery->add(term);
	}

	exactQuery->setRange(minX, minY, maxX, maxY);

	QueryResults *queryResults = QueryResults::create(indexSearcher, exactQuery);
	indexSearcher->search(exactQuery, queryResults);

	std::set<std::string> exactVisitedList;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		exactVisitedList.insert(queryResults->getRecordId(i));
	}
//	cout << "exact size: " << exactVisitedList.size() << endl;

	// if exact search can't find enough results, do fuzzy search
	if(queryResults->getNumberOfResults() < exactMatchNumber)
	{
		Query *fuzzyQuery = new Query(MapQuery);
		for (unsigned i = 0; i < keywords.size(); ++i)
		{
			Term *term = FuzzyTerm::create(keywords[i],
					PREFIX,
					1,
					100.0,
					getNormalizedThreshold(keywords[i].size()));
			fuzzyQuery->add(term);
		}
		fuzzyQuery->setRange(minX, minY, maxX, maxY);
		QueryResults *fuzzyQueryResults = QueryResults::create(indexSearcher, fuzzyQuery);
		indexSearcher->search(fuzzyQuery, fuzzyQueryResults);

//		cout << "fuzzy size: " << fuzzyQueryResults->getNumberOfResults() << endl;
		// combine fuzzy results and exact results

		QueryResultsInternal *exact_qs = dynamic_cast<QueryResultsInternal *>(queryResults);
		QueryResultsInternal *fuzzy_qs = dynamic_cast<QueryResultsInternal *>(fuzzyQueryResults);
		for(unsigned  i = 0; i < fuzzyQueryResults->getNumberOfResults(); i++)
		{
			string recordId = fuzzyQueryResults->getRecordId(i);
			if ( !exactVisitedList.count(recordId) )
				exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[i]);
		}

		delete fuzzyQueryResults;
		delete fuzzyQuery;
	}

	cout << "number of results: " << queryResults->getNumberOfResults() << endl;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		//vector<string> matchingKeywords;
		//queryResults->getMatchingKeywords(i, matchingKeywords);
		//cout << queryResults->getRecordId(i) << " | Score: " << queryResults->getResultScore(i)
		//									<<  " | Match: " << matchingKeywords[0] << endl;
		resultIds.push_back(atoi(queryResults->getRecordId(i).c_str()));
	}

	//ASSERT( queryResults->getNumberOfResults() ==  number);

	delete queryResults;
	delete exactQuery;

	delete indexSearcher;
}
*/

/*
void searchIndexFourSteps(Indexer *indexer, double minX, double minY, double maxX, double maxY, vector<string> &keywords, unsigned minResultSize, unsigned numOfElementsThreshold, vector<unsigned> &resultIds)
{
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

	Query *exactQuery = new Query(MapQuery);

	std::set<std::string> visitedList;

	unsigned numOfResultsSoFar = 0;

	// get the smaller query range
	QuadTree *quadTree = dynamic_cast<IndexReaderWriter *>(indexer)->index->quadTree;
	double smallerMinX = 0.0;
	double smallerMinY = 0.0;
	double smallerMaxX = 0.0;
	double smallerMaxY = 0.0;

	timespec ts1;
	timespec ts2;
	clock_gettime(CLOCK_REALTIME, &ts1);
	bool gotSmaller = quadTree->getSmallerRangeWithNumOfElementsThreshold(minX, minY, maxX, maxY, smallerMinX, smallerMinY, smallerMaxX, smallerMaxY, numOfElementsThreshold);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to probe the range: " << (ts2.tv_sec - ts1.tv_sec) * 1000 + ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;
	cout << "original range: " << minX << " , " << minY << " , " << maxX << " , " << maxY << endl;
	cout << "smaller range: " << smallerMinX << " , " << smallerMinY << " , " << smallerMaxX << " , " << smallerMaxY << endl;

	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		Term *term = ExactTerm::create(keywords[i],
				PREFIX,
				1,
				100.0);
		exactQuery->add(term);
	}

	// do exact search on the smaller range first
	exactQuery->setRange(smallerMinX, smallerMinY, smallerMaxX, smallerMaxY);
	QueryResults *queryResults = QueryResults::create(indexSearcher, exactQuery);
	indexSearcher->search(exactQuery, queryResults);

	numOfResultsSoFar = queryResults->getNumberOfResults();

	cout << "exact search with (smaller) range result size: " << numOfResultsSoFar << endl;

	// if can't find enough results
	if(numOfResultsSoFar < minResultSize)
	{
		if(gotSmaller) // do exact search on the original range
		{
			delete queryResults;
			exactQuery->setRange(minX, minY, maxX, maxY);
			queryResults = QueryResults::create(indexSearcher, exactQuery);
			indexSearcher->search(exactQuery, queryResults);

			numOfResultsSoFar = queryResults->getNumberOfResults();
			cout << "exact search with original range result size: " << numOfResultsSoFar << endl;
		}

		for(unsigned  i = 0; i < numOfResultsSoFar; i++)
		{
			visitedList.insert(queryResults->getRecordId(i));
		}

		if(numOfResultsSoFar < minResultSize)  // if still can't find enough results, do fuzzy search on the original range
		{
			Query *fuzzyQuery = new Query(MapQuery);
			for (unsigned i = 0; i < keywords.size(); ++i)
			{
				Term *term = FuzzyTerm::create(keywords[i],
						PREFIX,
						1,
						100.0,
						getNormalizedThreshold(keywords[i].size()));
				fuzzyQuery->add(term);
			}
			fuzzyQuery->setRange(minX, minY, maxX, maxY);
			QueryResults *fuzzyQueryResults = QueryResults::create(indexSearcher, fuzzyQuery);
			indexSearcher->search(fuzzyQuery, fuzzyQueryResults);

			numOfResultsSoFar = fuzzyQueryResults->getNumberOfResults();
			cout << "fuzzy search with original range result size: " << numOfResultsSoFar << endl;

			// combine fuzzy results and exact results
			QueryResultsInternal *exact_qs = dynamic_cast<QueryResultsInternal *>(queryResults);
			QueryResultsInternal *fuzzy_qs = dynamic_cast<QueryResultsInternal *>(fuzzyQueryResults);
			for(unsigned  i = 0; i < numOfResultsSoFar; i++)
			{
				string recordId = fuzzyQueryResults->getRecordId(i);
				if ( !visitedList.count(recordId) )
					exact_qs->sortedFinalResults.push_back(fuzzy_qs->sortedFinalResults[i]);
			}

			delete fuzzyQueryResults;
			delete fuzzyQuery;
		}
	}

	cout << "number of results: " << queryResults->getNumberOfResults() << endl;
	for(unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
//		vector<string> matchingKeywords;
//		queryResults->getMatchingKeywords(i, matchingKeywords);
//		cout << queryResults->getRecordId(i) << " | Score: " << queryResults->getResultScore(i)
//											<<  " | Match: " << matchingKeywords[0] << endl;
		resultIds.push_back(atoi(queryResults->getRecordId(i).c_str()));
	}

	//ASSERT( queryResults->getNumberOfResults() ==  number);

	delete queryResults;
	delete exactQuery;

	delete indexSearcher;
}
*/
void readAndExcuteTestCasesFromFile(Indexer *indexer, string directoryName, bool isFuzzy)
{
	string minXFile = directoryName + "/minx.txt";
	string minYFile = directoryName + "/miny.txt";
	string maxXFile = directoryName + "/maxx.txt";
	string maxYFile = directoryName + "/maxy.txt";
	string preQNumFile = directoryName + "/preQNum.txt";
	string preQFile = directoryName + "/preQ.txt";
	string resNumFile = directoryName + "/resNum.txt";
	string resFile = directoryName + "/res.txt";

	ifstream minXstream(minXFile.c_str());
	ASSERT ( minXstream );
	ifstream minYstream(minYFile.c_str());
	ASSERT ( minYstream );
	ifstream maxXstream(maxXFile.c_str());
	ASSERT ( maxXstream );
	ifstream maxYstream(maxYFile.c_str());
	ASSERT ( maxYstream );
	ifstream preQNumstream(preQNumFile.c_str());
	ASSERT ( preQNumstream );
	ifstream preQstream(preQFile.c_str());
	ASSERT ( preQstream );
	ifstream resNumstream(resNumFile.c_str());
	ASSERT ( resNumstream );
	ifstream resstream(resFile.c_str());
	ASSERT ( resstream );

	string minXLine, minYLine, maxXLine, maxYLine, preQNumLine, preQLine, resNumLine, resLine;

	while (getline(minXstream, minXLine)
		&& getline(minYstream, minYLine)
		&& getline(maxXstream, maxXLine)
		&& getline(maxYstream, maxYLine)
		&& getline(preQNumstream, preQNumLine)
		&& getline(preQstream, preQLine)
		&& getline(resNumstream, resNumLine)
		&& getline(resstream, resLine))
		{
			double minX, minY, maxX, maxY;
			unsigned numOfQueryPrefixes = 0;
			unsigned numOfResults = 0;
			vector<Prefix> prefixes;
			vector<string> strings;
			vector<unsigned> expectedResults;

			parseDouble(minXLine, minX);
			parseDouble(minYLine, minY);
			parseDouble(maxXLine, maxX);
			parseDouble(maxYLine, maxY);
			parseUnsigned(preQNumLine, numOfQueryPrefixes);

			parseQueryStrings(preQLine, strings, numOfQueryPrefixes);

			parseUnsigned(resNumLine, numOfResults);
			parseMultiUnsigned(resLine, numOfResults, expectedResults);

			searchIndex(indexer, minX, minY, maxX, maxY, strings, expectedResults, isFuzzy);
		}

	minXstream.close();
	minYstream.close();
	maxXstream.close();
	maxYstream.close();
	preQNumstream.close();
	preQstream.close();
	resNumstream.close();
	resstream.close();
}

// Test the case where we only have one node(root) with a few records in the tree
void testSingleNodeQuadTree(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData( cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	// Create five records of 3 attributes and add them to the index
	addLocationRecord(indexer, schema, analyzer, 100, "Tom Smith and Jack Lennon", "Yesterday Once More", 100.0, 100.0);
	addLocationRecord(indexer, schema, analyzer, 200, "George Harris", "Here comes the sun", 101.0, 101.0);
	addLocationRecord(indexer, schema, analyzer, 300, "George Harris", "Here comes the sun", 102.0, 102.0);
	addLocationRecord(indexer, schema, analyzer, 400, "George Harris", "Here comes the sun", -101.0, -101.0);
	addLocationRecord(indexer, schema, analyzer, 500, "George Harris", "Here comes the sun", -100.0, -100.0);

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// search for the first record
	queries.push_back("tom");
	expectedResults.push_back(100);
	searchIndex(indexer, 50.0, 50.0, 150.0, 150.0, queries, expectedResults);

	queries.clear();
	expectedResults.clear();

	// search for the two records in another range
	queries.push_back("harris");
	expectedResults.push_back(400);
	expectedResults.push_back(500);
	searchIndex(indexer, -150.0, -150.0, -50.0, -50.0, queries, expectedResults);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Test using the circle range
void testCircleRange(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData( cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	// Create five records of 3 attributes and add them to the index
	addLocationRecord(indexer, schema, analyzer, 100, "Tom Smith and Jack Lennon", "Yesterday Once More", 1.0, 1.0);
	addLocationRecord(indexer, schema, analyzer, 200, "Tom Smith and Jack Lennon", "Yesterday Once More", 2.0, 2.0);

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// circle range, only covers the first record
	queries.push_back("tom");
	expectedResults.push_back(100);
	searchIndexCircle(indexer, 0.0, 0.0, 2.0, queries, expectedResults);

	// rectangle range, covers the both records
	expectedResults.push_back(200);
	searchIndex(indexer, -2.0, -2.0, 2.0, 2.0, queries, expectedResults);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Test the case where we insert 100 records that have the same latitudes and longitudes
void testInsertingRecordsWithSameLocation(const string &directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);

    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");
	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/sameLocation100");

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// search for the first record
	queries.push_back("district");
	expectedResults.push_back(10);
	searchIndex(indexer, 0.0, -150.0, 50.0, -50.0, queries, expectedResults);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Test the case where the query range is not contained by the maximum rectangle
void testSpecialQueryRange(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	// Create five records of 3 attributes and add them to the index
	addLocationRecord(indexer, schema, analyzer, 100, "Tom Smith and Jack Lennon", "Yesterday Once More", 100.0, 100.0);
	addLocationRecord(indexer, schema, analyzer, 200, "George Harris", "Here comes the sun", 101.0, 101.0);
	addLocationRecord(indexer, schema, analyzer, 300, "George Harris", "Here comes the sun", 102.0, 102.0);
	addLocationRecord(indexer, schema, analyzer, 400, "George Harris", "Here comes the sun", -101.0, -101.0);
	addLocationRecord(indexer, schema, analyzer, 500, "George Harris", "Here comes the sun", -100.0, -100.0);

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// search for the two records in a range that is out of the maximum rectangle
	queries.push_back("harris");
	searchIndex(indexer, TOP_RIGHT_X, TOP_RIGHT_Y, TOP_RIGHT_X+50.0, TOP_RIGHT_Y+50.0, queries, expectedResults);

	// search for the two records in a range that is intersected with the maximum rectangle
	expectedResults.push_back(400);
	expectedResults.push_back(500);
	searchIndex(indexer, BOTTOM_LEFT_X-50.0, BOTTOM_LEFT_Y-50.0, -50.0, -50.0, queries, expectedResults);

	// search for the two records in a range that contains the maximum rectangle
	expectedResults.push_back(200);
	expectedResults.push_back(300);
	searchIndex(indexer, BOTTOM_LEFT_X-50.0, BOTTOM_LEFT_Y-50.0, TOP_RIGHT_X+50.0, TOP_RIGHT_Y+50.0, queries, expectedResults);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

// Test the case where we insert a thousand records and see if we can get correct query results
void testThousandRecordsQuadTree(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/1K");

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// search for one prefix in one range
	queries.push_back("district");
	expectedResults.push_back(10);
	expectedResults.push_back(615);
	searchIndex(indexer, 40.0, -95.0, 43.0, -69.0, queries, expectedResults);

	expectedResults.clear();

	// search for the same prefix in another range
	expectedResults.push_back(135);
	expectedResults.push_back(353);
	searchIndex(indexer, 38.0, -95.0, 43.0, -85.0, queries, expectedResults);

	expectedResults.clear();

	// add one prefix and search for two prefixes at the same time
	queries.push_back("school");
	expectedResults.push_back(10);
	expectedResults.push_back(353);
	searchIndex(indexer, 38.0, -95.0, 43.0, -69.0, queries, expectedResults);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

void testPrefixSearch(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/1K");

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	vector<string> queries;
	vector<unsigned> expectedResults;

	// search for one prefix in one range
	queries.push_back("nort");
	expectedResults.push_back(1);
	searchIndex(indexer, 44.968721, -89.636895, 44.968735, -89.636890, queries, expectedResults);
	queries.clear();

	expectedResults.clear();

	queries.push_back("bene");
	expectedResults.push_back(703);
	searchIndex(indexer, 42.0, -90.0, 45.0, -89, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("mi");
	expectedResults.push_back(116);
	expectedResults.push_back(260);
	expectedResults.push_back(962);
	expectedResults.push_back(1003);
	searchIndex(indexer, 42.0, -90.0, 45.0, -80, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("do");
	expectedResults.push_back(962);
	expectedResults.push_back(1164);
	searchIndex(indexer, 42.0, -90.0, 45.0, -80, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("mat");
	expectedResults.push_back(12);
	searchIndex(indexer, 33.0, -90.0, 46.0, -80, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("grou");
	expectedResults.push_back(26);
	expectedResults.push_back(150);
	searchIndex(indexer, 33.0, -90.0, 46.0, -80, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("com");
	expectedResults.push_back(1);
	expectedResults.push_back(34);
	expectedResults.push_back(189);
	expectedResults.push_back(404);
	expectedResults.push_back(472);
	expectedResults.push_back(480);
	expectedResults.push_back(523);
	expectedResults.push_back(703);
	expectedResults.push_back(929);
	searchIndex(indexer, 33.0, -90.0, 46.0, -80, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("hig");
	expectedResults.push_back(237);
	expectedResults.push_back(491);
	expectedResults.push_back(1183);
	searchIndex(indexer, 33.0, -90.0, 35.0, -85, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("par");
	expectedResults.push_back(288);
	expectedResults.push_back(572);
	expectedResults.push_back(588);
	expectedResults.push_back(1058);
	searchIndex(indexer, 33.0, -90.0, 35.0, -85, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	queries.push_back("jam");
	searchIndex(indexer, 33.0, -90.0, 35.0, -85, queries, expectedResults);
	expectedResults.clear();
	queries.clear();

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

void autoGeneratedTestCases(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/1K");

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	// test single query prefix
	readAndExcuteTestCasesFromFile(indexer, directoryName+"/quadtree/autoGeneratedPrefixSearch/single-keyword", false);

	// test single query prefix with larger query range
	readAndExcuteTestCasesFromFile(indexer, directoryName+"/quadtree/autoGeneratedPrefixSearch/larger-range", false);

	// test multi query prefixes
	readAndExcuteTestCasesFromFile(indexer, directoryName+"/quadtree/autoGeneratedPrefixSearch/multi-keyword", false);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

void testFuzzySearch(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/1K");

	// commit the index
    bool retval = indexer->commit();
	ASSERT( retval == 1 );
    (void)retval;

	readAndExcuteTestCasesFromFile(indexer, directoryName+"/quadtree/fuzzy", true);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

void testSerialization(string directoryName)
{
	// Create a schema
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	readRecordsFromFile(indexer, schema, analyzer, directoryName+"/quadtree/1K");

	// serialize the index
	indexer->commit();
	indexer->save(directoryName);

	readAndExcuteTestCasesFromFile(indexer, directoryName+"/quadtree/fuzzy", true);

	delete indexer;
    delete indexMetaData;
	delete analyzer;
	delete schema;
}

void testDeserialization(string directoryName)
{
	Schema *schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text
	Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName, "");

	// load the quadtree from disk
    Indexer *indexer1 = Indexer::load(indexMetaData);
	QuadTree *qt1 = dynamic_cast<IndexReaderWriter *>(indexer1)->getQuadTree();

	// rebuild the old quadtree
	Indexer *indexer2 = Indexer::create(indexMetaData, analyzer, schema);
	readRecordsFromFile(indexer2, schema, analyzer, directoryName+"/quadtree/1K");
	indexer2->commit();
	QuadTree *qt2 = dynamic_cast<IndexReaderWriter *>(indexer2)->getQuadTree();

	// test if the loaded quadtree is exactly the same as the old one
	bool isEqual = false;
	if(qt1->equalTo(qt2) && qt2->equalTo(qt1))
		isEqual = true;
	ASSERT( isEqual == true );

	// test if the loaded quadtree works
	readAndExcuteTestCasesFromFile(indexer1, directoryName+"/quadtree/fuzzy", true);

	delete indexer2;
	delete indexer1;
	delete indexMetaData;
	delete analyzer;
	delete schema;
}

void testQuadTreePerformance(string directoryName, unsigned flag)
{
	// flag = 1: generate the quadtree, have it only in memory, gone after the test
	// flag = 2: generate the quadtree, save it on disk
	// flag = 3: read the quadtree from disk

	time_t t1, t2, t3;
	Indexer *indexer = NULL;
	Schema *schema = NULL;
	Analyzer *analyzer = NULL;
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, directoryName+"/1M", "");

	if(flag == 1 || flag == 2)
	{
		// Create a schema
		schema = Schema::create(LocationIndex);
		schema->setPrimaryKey("list_id"); // integer, by default not searchable
		schema->setSearchableAttribute("title", 2); // searchable text
		schema->setSearchableAttribute("address", 7); // searchable text

		// Create an analyzer
		analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
				"", "", "", SYNONYM_DONOT_KEEP_ORIGIN,"");

		indexer = Indexer::create(indexMetaData, analyzer, schema);

		time(&t1);
		readRecordsFromFile(indexer, schema, analyzer, directoryName+"/1M");
	}

	// commit the index
	time(&t2);
	if(flag == 1)
		indexer->commit();
	else if(flag == 2){
		indexer->commit();
		indexer->save();
	}
	else if(flag == 3)
		indexer = Indexer::load(indexMetaData);
	time(&t3);

/*
	unsigned cFilterBytes;
	unsigned oFilterBytes;
	unsigned treeBytes;
	unsigned totalNumOfKeywordsInCFilter;
	unsigned totalNumOfKeywordsInOFilter;
	dynamic_cast<IndexerInternal *>(indexer)->quadTree->getNumberOfBytes(cFilterBytes, oFilterBytes, treeBytes, totalNumOfKeywordsInCFilter, totalNumOfKeywordsInOFilter);
	cout << "cFilterBytes: " << cFilterBytes << endl
		 << "oFilterBytes: " << oFilterBytes << endl
		 << "totalNumOfKeywordsInCFilter: " << totalNumOfKeywordsInCFilter << endl
		 << "totalNumOfKeywordsInOFilter: " << totalNumOfKeywordsInOFilter << endl
		 << "treeBytes: " << treeBytes << endl;*/
	cout << dynamic_cast<IndexReaderWriter *>(indexer)->getQuadTree()->geoElementIndex.size() << endl;



	if(flag == 3)
		t1 = t2;
	cout << "insertion time: " << difftime(t2, t1) << " seconds" << endl;
	cout << "commit/save/read time: " << difftime(t3, t2) << " seconds" << endl;

	//------------- Begin to search

	timespec ts1;
	timespec ts2;

	vector<string> queries;
	queries.push_back("starb");

	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexCheckResultsNumberOnly(indexer, 37.6, -122.5, 37.8, -122.36, queries, 29, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;
	
	cout << "-------------------------------------------------------------" << endl;

	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexCheckResultsNumberOnly(indexer, 37.4, -122.5, 37.8, -122.36, queries, 30, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

	cout << "-------------------------------------------------------------" << endl;
	queries.clear();

	queries.push_back("staeb");

	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexCheckResultsNumberOnly(indexer, 40.69834, -74.010773, 40.808093, -73.951035, queries, 12, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

	cout << "-------------------------------------------------------------" << endl;
	queries.push_back("ave");

	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexCheckResultsNumberOnly(indexer, 40.69834, -74.010773, 40.808093, -73.951035, queries, 9, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

	delete indexer;
    delete indexMetaData;

	if(flag == 1 || flag == 2)
	{
		delete analyzer;
		delete schema;
	}

}

void singleTest(string directoryName, unsigned threshold)
{
	/*Index *index = NULL;
	Schema *schema = NULL;
	Analyzer *analyzer = NULL;

	// Create a schema
	schema = Schema::create(LocationIndex);
	schema->setPrimaryKey("list_id"); // integer, by default not searchable
	schema->setSearchableAttribute("title", 2); // searchable text
	schema->setSearchableAttribute("address", 7); // searchable text

	// Create an analyzer
	analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

	index = Index::create(directoryName, analyzer, schema);

	readRecordsFromFile(index, schema, directoryName+"/5M");

	// commit the index
	index->commit();
	index->save(directoryName + "/5M");
*/
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
	Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, "/home/xiang/data/factual/Dec082011/5MIndex", "");
	Indexer *indexer = Indexer::load(indexMetaData);

	timespec ts1;
	timespec ts2;

	//vector<string> queries;
	//queries.push_back("prof");
	//queries.push_back("casino");

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
	clock_gettime(CLOCK_REALTIME, &ts1);
	vector<unsigned> twoStepResultIds;
	vector<unsigned> fourStepResultIds;
	for( unsigned vectIter = 0; vectIter < queries.size(); vectIter++ )
	{
		//if(threshold == 0)
//			searchIndexTwoSteps(indexer, ranges[vectIter].min.x, ranges[vectIter].min.y, ranges[vectIter].max.x, ranges[vectIter].max.y, queries[vectIter], 30, twoStepResultIds);
		//else
//			searchIndexFourSteps(index, indexer, ranges[vectIter].min.x, ranges[vectIter].min.y, ranges[vectIter].max.x, ranges[vectIter].max.y, queries[vectIter], 30, threshold, fourStepResultIds);

		ASSERT(ifAllFoundResultsAreCorrect(twoStepResultIds, fourStepResultIds));
		twoStepResultIds.clear();
		fourStepResultIds.clear();
	}
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << (ts2.tv_sec - ts1.tv_sec) * 1000 + ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;
/*
	queries.clear();
	queries.push_back("prof");
	queries.push_back("casino");
	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexNoCheck(indexer, 40.666193, -74.106738 , 40.762478, -73.905208, queries, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

	cout << endl;

	queries.clear();
	queries.push_back("casino");
	queries.push_back("prof");
	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexNoCheck(indexer, 40.666193, -74.106738 , 40.762478, -73.905208, queries, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;

	cout << endl;

	queries.clear();
	queries.push_back("prof");
	queries.push_back("casino");
	clock_gettime(CLOCK_REALTIME, &ts1);
	searchIndexNoCheck(indexer, 40.666193, -74.106738 , 40.762478, -73.905208, queries, true);
	clock_gettime(CLOCK_REALTIME, &ts2);
	cout << "Time to search " << queries.size() << " fuzzy keywords: " << ((double)(ts2.tv_nsec - ts1.tv_nsec)) / 1000000.0 << " milliseconds" << endl;
	*/

	//delete indexer; TODO destructor of indexer
	delete indexer;
	delete indexMetaData;
	//delete cache;
}

int main(int argc, char *argv[])
{

	bool verbose = false;
	if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
		verbose = true;
	}

	//unsigned threshold = atoi(argv[1]);

	const string directoryName = getenv("directoryName");
	//string directoryName = "../test/unit/test_data";

	//--- Complete Keyword ---//

	// Test the case where we only have one node(root) with a few records in the tree
	testSingleNodeQuadTree(directoryName);
    cout << "1/10 testSingleNodeQuadTree passes." << endl;

	// Test circle range
	testCircleRange(directoryName);
    cout << "2/10 testCircleRange passes." << endl;

	// Test the case where we insert 100 records that have the same latitudes and longitudes
	testInsertingRecordsWithSameLocation(directoryName);
    cout << "3/10 testInsertingRecordsWithSameLocation passes." << endl;

	// Test the case where the query range is not contained by the maximum rectangle
	testSpecialQueryRange(directoryName);
    cout << "4/10 testSpecialQueryRange passes." << endl;

	// Test the case where we insert a thousand records and see if we can get correct query results
	testThousandRecordsQuadTree(directoryName);
    cout << "5/10 testThousandRecordsQuadTree passes." << endl;

	//--- Prefix Keyword ---//

	// Test several prefix queries on one thousand records and see if we can get correct query results
	testPrefixSearch(directoryName);
    cout << "6/10 testPrefixSearch passes." << endl;

	// Use the auto-generated test cases to test prefix queries
	autoGeneratedTestCases(directoryName);
    cout << "7/10 autoGeneratedTestCases passes." << endl;

	//--- Fuzzy Search ---//

	testFuzzySearch(directoryName);
    cout << "8/10 testFuzzySearch passes." << endl;

	//--- Serialization ---//

	testSerialization(directoryName);
    cout << "9/10 testSerialization passes." << endl;

	testDeserialization(directoryName);
    cout << "10/10 testDeserialization passes." << endl;

	// Test the performance of QuadTree
	// local test for Xiang
	//testQuadTreePerformance(directoryName, 2);
	//singleTest(directoryName, threshold);

	//singleTest(directoryName);

	cout << "All Quadtree tests passed!" << endl;

	return 0;
}


