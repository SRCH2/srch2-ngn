//$Id: Daemon.cpp 1191 2011-06-01 19:55:07Z srch2.vijay $

#include "ConfReader.h"
#include "JSON.h"
#include "DB.h"

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>

#include <libxml/uri.h>
#include <boost/algorithm/string.hpp>
//#include <fcgiapp.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

#include "mongoose/mongoose.h"


using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

enum CommandType { DAEMON_LOAD, DAEMON_SEARCH, DAEMON_INSERT, DAEMON_DELETE, DAEMON_NONE };

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



void printResults(FCGX_Request &request, const QueryResults *queryResults, int idsFound, int offset)
{
	JSONObject root;
	JSONArray resultsArray;

	for(unsigned resultIter = offset; resultIter < queryResults->getNumberOfResults() ; resultIter++)
	{
		JSONArray jsonResult;
		JSONArray jsonMK;
		JSONArray jsonED;
		vector<string> matchingKeywords;
		vector<unsigned> editDistances;

		// For each result, get the matching keywords and their edit distances
		queryResults->getMatchingKeywords(resultIter, matchingKeywords);
		queryResults->getEditDistances(resultIter, editDistances);

		// Output the result information to JSON message
		jsonResult.push_back(new JSONValue((double)queryResults->getRecordId(resultIter)));
		jsonResult.push_back(new JSONValue(queryResults->getResultScore(resultIter)->getDoubleScore()));

		unsigned editDistancesIter = 0;
		for(vector<string>::iterator iter = matchingKeywords.begin();
				iter != matchingKeywords.end(); iter ++, editDistancesIter ++)
		{
			//from string to wstring
			wstring temp(iter->length(), L' ');
			copy(iter->begin(), iter->end(), temp.begin());
			//push matching keywords and edit distances to arrays
			jsonMK.push_back(new JSONValue(temp));
			unsigned ed = editDistances.at(editDistancesIter);
			jsonED.push_back(new JSONValue((double)ed));
		}

		jsonResult.push_back(new JSONValue(jsonMK));
		jsonResult.push_back(new JSONValue(jsonED));

		resultsArray.push_back(new JSONValue(jsonResult));
	}

	root[L"results_array"] = new JSONValue(resultsArray);
	JSONValue *jsonMessage = new JSONValue(root);

	//convert the JSON message from wstring to string
	wstring origin_message = jsonMessage->Stringify();
	string message(origin_message.length(), ' ');
	copy(origin_message.begin(), origin_message.end(), message.begin());

	FCGX_FPrintF(request.out, "ids found: %d, results: %s\n", idsFound, message.c_str());

	delete jsonMessage;
}

void searchCommand(vector<string> &values, Indexer *indexer, char delimiter, char delimiter2, int searchType, int searchOption1, int searchOption2, int exactFuzzy)
{
	srch2is::IndexSearcher *indexSearcher = srch2is::IndexSearcher::create(indexer);
	const Analyzer *analyzer = indexer->getAnalyzer();

	vector<string> queryKeywords;
	analyzer->tokenizeQuery(values[0], queryKeywords, delimiter);

	//use delimiter to decode options for every keyword
	string sep;
	sep += delimiter;
	vector<string> termTypes;
	boost::split(termTypes, values[1], boost::is_any_of(sep));
	vector<string> termBoosts;
	boost::split(termBoosts, values[2], boost::is_any_of(sep));
	vector<string> similarityBoosts;
	boost::split(similarityBoosts, values[3], boost::is_any_of(sep));
	vector<string> AttributeToFilterTermHits;
	boost::split(AttributeToFilterTermHits, values[4], boost::is_any_of(sep));

	Query *query = new Query((searchType==0) ? srch2is::TopKQuery : srch2is::AdvancedQuery);

	//for each keyword, build a term
	for (unsigned i = 0; i < queryKeywords.size(); ++i)
	{
		TermType type = atoi(termTypes[i].c_str())==0 ? srch2is::PREFIX : srch2is::COMPLETE;;

		unsigned termBoost = atoi(termBoosts[i].c_str());
		unsigned similarityBoost = atoi(similarityBoosts[i].c_str());
		Term *term;
		if(exactFuzzy == 0)
		{
			term = new srch2is::ExactTerm(queryKeywords[i],
									   type,
									   termBoost,
									   similarityBoost,
									   getNormalizedThreshold(getUtf8StringCharacterNumber(queryKeywords[i])));
		}else{
			term = new srch2is::FuzzyTerm(queryKeywords[i],
									   type,
									   termBoost,
									   similarityBoost,
									   getNormalizedThreshold(getUtf8StringCharacterNumber(queryKeywords[i])));
		}

		//use delimiter2 to decode the AttributeToFilterTermHits option
		vector<string> localAttriFilters;
		string sep2;
		sep2 += delimiter2;
		boost::split(localAttriFilters, AttributeToFilterTermHits[i], boost::is_any_of(sep2));
		for(unsigned j = 0; j < localAttriFilters.size(); ++j)
			if(atoi(localAttriFilters[i].c_str()) != -1)
				term->addAttributeToFilterTermHits(atoi(localAttriFilters[i].c_str()));

		query->add(term);
	}

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	int idsFound = 0;

	//do the search
	if(searchType == 0){//TopK
		int offset = searchOption1;
		int resultsToRetrieve = searchOption2;

		idsFound = indexSearcher->search(query, queryResults, offset, resultsToRetrieve);
		printResults(request, queryResults, idsFound, offset);

	}else{//Advanced
		int filterAttribute = searchOption1;
		srch2is::SortOrder order = (searchOption2 == 0) ? srch2is::Descending : srch2is::Ascending;
		query->setSortableAttribute(filterAttribute, order);
		idsFound = indexSearcher->search(query, queryResults);
		printResults(request, queryResults, idsFound, 0);
	}

	// Free the objects
	delete query;
	delete indexSearcher;
}

void insertCommand(FCGX_Request &request, vector<string> &values, Indexer *indexer, string index_dir, char delimiter, int ifPrimSearchable)
{
	srch2is::Record *record = new Record(indexer->getSchema());

	//read in the primary key of the record we want to insert
	unsigned primaryKey = (unsigned)atoi(values[0].c_str());
	record->setPrimaryKey(primaryKey);
	FCGX_FPrintF(request.out, "primaryKey: %d\n", primaryKey);

	//use delimiter to decode attributes
	string sep;
	sep += delimiter;
	vector<string> attributes;
	boost::split(attributes, values[1], boost::is_any_of(sep));

	//add searchable attributes to the record
	unsigned iter;
	for(iter=ifPrimSearchable; iter<attributes.size(); iter++)
	{
		record->setSearchableAttributeValue(iter, attributes[iter]);
		FCGX_FPrintF(request.out, "attri %d: %s\n", iter, attributes[iter].c_str());
	}

	//add recordBoost if available
	int recordBoost = atoi(values[2].c_str());
	if(recordBoost != -1)
	{
		record->setRecordBoost(recordBoost);
		FCGX_FPrintF(request.out, "recordBoost: %d\n", recordBoost);
	}

	//add the record to the index
	indexer->addRecord(record);

	//save the index to the disk
	indexer->save(index_dir);

	delete record;
}

void deleteCommand(FCGX_Request &request, vector<string> &values, Indexer *indexer, string index_dir)
{
	//set the primary key of the record we want to delete
	unsigned primaryKey = (unsigned)atoi(values[0].c_str());
	FCGX_FPrintF(request.out, "primaryKey: %d\n", primaryKey);

	//delete the record from the index
	indexer->deleteRecord(primaryKey);

	//save the index to the disk
	indexer->save(index_dir);
}

int main(int argc, char **argv)
{
	//read configuration file
	string CONF_DIR = getenv("conf_dir");
	ConfReader cReader(CONF_DIR.c_str());

	//get delimiters
	char delimiter = cReader.getDelimiter();
	char delimiter2 = cReader.getDelimiter2();

	//get the path of the csv file
	string csv = cReader.getCsvPath();


	//load the index from the data source
	Indexer *indexer = DaemonIndexLoader::loadIndex(cReader);//NULL;
	string INDEX_DIR = cReader.getIndexPath();

	/*FILE * pFile;
	pFile = fopen((INDEX_DIR+"/CL1.idx").c_str(), "r");// detect if there are index files
	if (pFile != NULL)
	{
		indexer = Indexer::load(INDEX_DIR);
		fclose (pFile);
	}
	else
	{
		return;
	}*/

	int searchType = cReader.getSearchType();//TopK: 0; Advanced: 1
	int searchOption1, searchOption2;

	//if TopK search,     searchOption1 is offset,           searchOption2 is resultsToRetrieve
	//if advanced search, searchOption1 is filterAttributes, searchOption2 is order
	if(searchType == 0){
		searchOption1 = cReader.getOffset();
		searchOption2 = cReader.getResultsToRetrieve();
	}else{
		searchOption1 = cReader.getSortableAttributes();
		searchOption2 = cReader.getOrder();
	}

	//If the primary key is NOT searchable: 0; else: 1
	int ifPrimSearchable  = cReader.getIfPrimSearchable();

	//get the format of the records
    string primaryKeyName = cReader.getPrimaryKey();
    vector<string> attributesNames = cReader.getAttributes();
    vector<unsigned> attributesBoosts = cReader.getAttributesBoosts();

	//Exact term: 0; Fuzzy term: 1
	int exactFuzzy = cReader.getExactFuzzy();

	//Initialize FastCGI
	FCGX_Init();
	FCGX_Request request;
	FCGX_InitRequest(&request, 0, 0);

	unsigned commandType = DAEMON_NONE;
	vector<string> values;

	while (FCGX_Accept_r(&request) == 0)
	{
		FCGX_PutS("Content-Type: application/json\r\n\r\n", request.out);

		//parse the request, tell the query type
		commandType = parseRequest(request, values);

		switch(commandType)
		{
			//case DAEMON_LOAD:
			//	loadCommand(request, csv, indexer, INDEX_DIR, primaryKeyName, attributesNames, attributesBoosts, ifPrimSearchable);
			//	break;
			case DAEMON_SEARCH:
				searchCommand(request, values, indexer, delimiter, delimiter2, searchType, searchOption1, searchOption2, exactFuzzy);
				break;
			case DAEMON_INSERT:
				insertCommand(request, values, indexer, INDEX_DIR, delimiter, ifPrimSearchable);
				break;
			case DAEMON_DELETE:
				deleteCommand(request, values, indexer, INDEX_DIR);
				break;
			default:
				//TODO output warnings
				FCGX_FPrintF(request.out, "Invalid Request. Please check the request variables.");
				continue;
		}
		values.clear();
	}

	delete indexer;
}
