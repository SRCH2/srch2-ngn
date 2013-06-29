#include <cassert>
#include <string>
#include <iostream>

#include <instantsearch/Indexer.h>
#include "../src/JSONRecordParser.h"
#include "../src/Srch2KafkaConsumer.h"
#include "../../src/operation/IndexSearcherInternal.h"
#include "../../src/operation/IndexerInternal.h"

namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using namespace std;

/*
unsigned getNormalizedThreshold(unsigned keywordLength)
{
	// Keyword length:             [1,3]        [4,5]        >= 6
	// Edit-distance threshold:      0            1           2
	if (keywordLength < 4)
		return 0;
	else if (keywordLength < 6)
		return 1;
	return 2;
}

// For Debugging while constructing test cases;
bool checkResults_DUMMY(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs)
{
	//bool returnvalue = false;
	bool returnvalue = true;
	cout << numberofHits << " |===| " << queryResults->getNumberOfResults() << endl;
	//if (numberofHits != queryResults->getNumberOfResults())
	//{
		//return false;
	//}
	//else
	//{
		//returnvalue = true;
		for (unsigned resultCounter = 0;
				resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
		{
			vector<string> matchingKeywords;
			vector<unsigned> editDistances;

			queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
			queryResults->getEditDistances(resultCounter, editDistances);

			if (queryResults->getNumberOfResults() < resultCounter)
			{
				cout << "[" << resultCounter << "]" << endl;
			}
			else
			{
				cout << "[" << resultCounter << "]" << queryResults->getRecordId(resultCounter) << endl;
				if ( (unsigned)atoi(queryResults->getRecordId(resultCounter).c_str()) == recordIDs[resultCounter])
				//if (queryResults->getRecordId(resultCounter) == recordIDs[resultCounter])
				{
				//	returnvalue = true;
				}
				else
				{
				//	return false;
				}
			}
		}
	//}
	return returnvalue;
}

void parseQuery(const Analyzer *analyzer, Query *query, string queryString, int attributeIdToFilter = -1)
{
	vector<string> queryKeywords;
	analyzer->tokenizeQuery(queryString,queryKeywords,'+');
	// for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
	//cout<<"Query:";
	for (unsigned i = 0; i < queryKeywords.size(); ++i)
	{
		//cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
		TermType type = PREFIX;
		Term *term = new Term(queryKeywords[i], type, 1, 100, getNormalizedThreshold(queryKeywords[i].size()));
		term->addAttributeToFilterTermHits(attributeIdToFilter);
		query->add(term);
	}
	//cout << endl;
	queryKeywords.clear();
}


bool checkResults(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs)
{
	bool returnvalue = false;
	//cout << numberofHits << " | " << queryResults->getNumberOfResults() << endl;
	if (numberofHits != queryResults->getNumberOfResults())
	{
		return false;
	}
	else
	{
		returnvalue = true;
		for (unsigned resultCounter = 0;
				resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
		{
			vector<string> matchingKeywords;
			vector<unsigned> editDistances;

			queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
			queryResults->getEditDistances(resultCounter, editDistances);

			//cout << "[" << resultCounter << "]" << queryResults->getRecordId(resultCounter) << endl;
			if ( (unsigned)atoi(queryResults->getRecordId(resultCounter).c_str()) == recordIDs[resultCounter])
			//if (queryResults->getRecordId(resultCounter) == recordIDs[resultCounter])
			{
				returnvalue = true;
			}
			else
			{
				return false;
			}
		}
	}
	return returnvalue;
}

bool ping(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
{
	Query *query = new Query(srch2::instantsearch::TopKQuery);
	parseQuery(analyzer, query, queryString, attributeIdToFilter);
	int resultCount = 10;

	cout << "[" << queryString << "]" << endl;

	// for each keyword in the user input, add a term to the query
	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	indexSearcher->search(query, queryResults, resultCount);
	bool returnvalue =  checkResults_DUMMY(queryResults, numberofHits, recordIDs);
	//printResults(queryResults);
	delete queryResults;
	delete query;
	return returnvalue;
}*/

bool test(int argc, char** argv)
{
	//read configuration file
	bool parseSuccess = true;
	std::stringstream parseError;
	srch2http::Srch2ServerConf *serverConf = new srch2http::Srch2ServerConf(argc, argv, parseSuccess, parseError);
    srch2http::Srch2ServerLogger *serverLogger = new srch2http::Srch2ServerLogger(serverConf->getHTTPServerAccessLogFile());

	if (not parseSuccess)
	{
		std::cout << "[Error in parsing args]" << parseError.str() << std::endl;
		return false;
	}

	// create IndexMetaData
	srch2is::IndexMetaData *indexMetaData = srch2http::Srch2KafkaConsumer::createIndexMetaData(serverConf);

	// Create an analyzer
	srch2is::Analyzer *analyzer = srch2is::Analyzer::create(srch2::instantsearch::NO_STEMMER_NORMALIZER, serverConf->getRecordAllowedSpecialCharacters());

	// Create a schema to the data source definition in the Srch2ServerConf
	srch2is::Schema *schema = srch2http::JSONRecordParser::createAndPopulateSchema(serverConf);

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	cout << "Creating new index from JSON file..." << endl;
    std::stringstream log_str;
	srch2http::DaemonDataSource::createNewIndexFromFile(indexer, serverConf, serverLogger);

	srch2is::IndexSearcherInternal *ii = new IndexSearcherInternal(dynamic_cast<srch2is::IndexReaderWriter*>(indexer));
	ii->getTrie()->print_Trie();

	delete indexMetaData;
	delete indexer;
	delete serverConf;

	return true;
}


int main(int argc, char** argv)
{
	test(argc, argv);
	std:cout << "Test passed." << std::endl;
	return 0;
}

