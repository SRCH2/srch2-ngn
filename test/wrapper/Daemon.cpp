//$Id: Daemon.cpp 1280 2011-07-06 17:13:55Z srch2.vijay $

#include "ConfReader.h"
#include "JSON.h"
#include "DaemonDataSource.h"

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

#include <stdlib.h>
#include <iostream>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

enum CommandType { DAEMON_SEARCH, DAEMON_INSERT, DAEMON_DELETE, DAEMON_NONE };

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

unsigned parseRequest(string query, vector<string> &values)
{
	//define key's that can be read from the query string
	string type = "type";
	//for search
	string searchQuery = "search";
	string keywords = "keywords";
	string termTypes = "termTypes";
	string termBoosts = "termBoosts";
	string similarityBoosts = "similarityBoosts";
	string addAttributeToFilterTermHits = "addAttributeToFilterTermHits";
	string options = "options";
	//for insert
	string insertQuery = "insert";
	string primaryKey = "primaryKey"; // also for delete
	string attributes = "attributes";
	string recordBoost = "recordBoost";
	//for delete
	string deleteQuery = "delete";

	//read the query string
	const char *p = query.c_str();

	unsigned commandType = DAEMON_NONE;

	cout << "request: " << p << endl;

	//begin to parse the query string
	while(*p)
	{
		const char *p1 = p;
		char *s;
		//keep reading the words until a '=' or the end, treat the word read so far as a "key"
		while(*p && *p != '=')p++;
		if(*p)
		{
			const char *p2 = p;
			//keep reading the words until a '&' or the end, treat the word read so far as a "value"
			while(*p && *p != '&')p++;
			if(p - p2 - 1)
			{
				s = (char *)malloc(p - p2 - 1);
				xmlURIUnescapeString(p2 + 1, p - p2 - 1, s);
				string key = string(p1, p2 - p1);
				string value = string(s, p - p2 - 1);
				if(key.compare(type) == 0)
				{
					//tell which kind of query it is
					if(commandType == DAEMON_NONE)
					{
						if(value.compare(searchQuery) == 0){
							type = keywords;
							commandType = DAEMON_SEARCH;
						}
						else if(value.compare(insertQuery) == 0){
							type = primaryKey;
							commandType = DAEMON_INSERT;
						}
						else if(value.compare(deleteQuery) == 0){
							type = primaryKey;
							commandType = DAEMON_DELETE;
						}
						else
							return DAEMON_NONE;
					}
					//parse search query
					//e.g. query string "type=search&keywords=foo+bar&termTypes=0+1&termBoosts=1+2&similarityBoosts=100+200&addAttributeToFilterTermHits=0+1&options=10+-1+0"
					//     with delimiter = '+'
					//          option 1: only for topk, number of results to retrieve, default value: 10
					//          option 2: only for advanced, id of the attribute used to sort the results, default value: -1
					//          option 3: only for advanced, 0 if ascending, 1 if descending, default value: 0
					//     corresponding to two search terms:
					//     srch2is::Term *term1 = Term("foo",
					//                              srch2is::PREFIX,
					//                              1, //termBoost
					//                              100, //similarityBoost
					//                              getNormalizedThreshold(2));
			        //                              term1->addAttributeToFilterTermHits(0);
					//     srch2is::Term *term2 = Term("bar",
					//                              srch2is::COMPLETE,
					//                              2, //termBoost
					//                              200, //similarityBoost
					//                              getNormalizedThreshold(2));
					//                              term1->addAttributeToFilterTermHits(1);
					else if(commandType == DAEMON_SEARCH)
					{
						if(type.compare(keywords) == 0){
							cout << "keywords: " << value.c_str() << endl;
							type = termTypes;
							values.push_back(value);
						}
						else if(type.compare(termTypes) == 0){
							cout << "termTypes: " << value.c_str() << endl;
							type = termBoosts;
							values.push_back(value);
						}
						else if(type.compare(termBoosts) == 0){
							cout << "termBoosts: " << value.c_str() << endl;
							type = similarityBoosts;
							values.push_back(value);
						}
						else if(type.compare(similarityBoosts) == 0){
							cout << "similarityBoosts: " << value.c_str() << endl;
							type = addAttributeToFilterTermHits;
							values.push_back(value);
						}
						else if(type.compare(addAttributeToFilterTermHits) == 0){
							cout << "addAttributeToFilterTermHits: " << value.c_str() << endl;
							type = options;
							values.push_back(value);
						}
						else if(type.compare(options) == 0){
							cout << "options: " << value.c_str() << endl;
							values.push_back(value);
						}
						else
							return DAEMON_NONE;
					}
					//parse insert query
					//e.g. query string "type=insert&primaryKey=123&attributes=foo+bar&recordBoost=50"
					else if(commandType == DAEMON_INSERT)
					{
						if(type.compare(primaryKey) == 0){
							type = attributes;
							values.push_back(value);
						}
						else if(type.compare(attributes) == 0){
							type = recordBoost;
							values.push_back(value);
						}
						else if(type.compare(recordBoost) == 0){
							values.push_back(value);
						}
						else
							return DAEMON_NONE;
					}
					//parse delete query
					//e.g. query string "type=delete&primaryKey=123""
					else if(commandType == DAEMON_DELETE)
					{
						values.push_back(value);
						return commandType;
					}
				}
			}
			if(*p)p++;
		}
	}

	cout << "type: " << commandType << endl;

	return commandType;
}

void printResults(const QueryResults *queryResults, int idsFound, int offset)
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
		jsonResult.push_back(new JSONValue((double)queryResults->getResultScore(resultIter)));

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

	cout << "ids found: " << idsFound << " results: " << message.c_str() << endl;

	delete jsonMessage;
}

void searchCommand(vector<string> &values, Indexer *indexer, char delimiter, int searchType, int exactFuzzy)
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
	vector<string> options;
	boost::split(options, values[5], boost::is_any_of(sep));

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
			cout << queryKeywords[i] << endl;
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

		int filter = atoi(AttributeToFilterTermHits[i].c_str());
		if(filter != -1)
			term->addAttributeToFilterTermHits(filter);

		query->add(term);
	}

	QueryResults *queryResults = QueryResults::create(indexSearcher, query);

	int idsFound = 0;

	//do the search
	if(searchType == 0){//TopK
		int offset = 0;
		int resultsToRetrieve = atoi(options[0].c_str());

		idsFound = indexSearcher->search(query, queryResults, 0, resultsToRetrieve);
		printResults(queryResults, idsFound, offset);

	}else{//Advanced
		int sortAttribute = atoi(options[1].c_str());
		srch2is::SortOrder order = (atoi(options[2].c_str()) == 0) ? srch2is::Ascending : srch2is::Descending;
		query->setSortableAttribute(sortAttribute, order);
		idsFound = indexSearcher->search(query, queryResults);
		printResults(queryResults, idsFound, 0);
	}

	// Free the objects
	delete query;
	delete analyzer;
	delete indexSearcher;
}

void insertCommand(vector<string> &values, Indexer *indexer, string index_dir, char delimiter, int ifPrimSearchable)
{
	srch2is::Record *record = new Record(indexer->getSchema());

	//read in the primary key of the record we want to insert
	unsigned primaryKey = (unsigned)atoi(values[0].c_str());
	record->setPrimaryKey(primaryKey);
	cout << "primaryKey: " << primaryKey << endl;

	//use delimiter to decode attributes
	string sep;
	sep += delimiter;
	vector<string> attributes;
	cout << values[1] << endl;
	boost::split(attributes, values[1], boost::is_any_of(sep));

	//add searchable attributes to the record
	unsigned iter;
	for(iter=ifPrimSearchable; iter<(attributes.size() + ifPrimSearchable); iter++)
	{
		record->setSearchableAttributeValue(iter, attributes[iter-ifPrimSearchable]);
		cout << "attri " << iter << ": " << attributes[iter-ifPrimSearchable] << endl;
	}

	//add recordBoost if available
	int recordBoost = atoi(values[2].c_str());
	if(recordBoost != -1)
	{
		record->setRecordBoost(recordBoost);
		cout << "recordBoost: " << recordBoost << endl;
	}

	//add the record to the index
	indexer->addRecord(record);

	//save the index to the disk
	indexer->save(index_dir);

	delete record;
}

void deleteCommand(vector<string> &values, Indexer *indexer, string index_dir)
{
	//set the primary key of the record we want to delete
	unsigned primaryKey = (unsigned)atoi(values[0].c_str());

	//delete the record from the index
	indexer->deleteRecord(primaryKey);

	//save the index to the disk
	indexer->save(index_dir);

	cout << "primaryKey: " << primaryKey << endl;
}

int main(int argc, char **argv)
{
	//string query = "type=search&keywords=brick&termTypes=1&termBoosts=1&similarityBoosts=100&addAttributeToFilterTermHits=2&options=10+-1+0";
	string query = "type=insert&primaryKey=777&attributes=p k+brick&recordBoost=1";
	//string query = "type=delete&primaryKey=300";

	//read configuration file
	string CONF_DIR = getenv("conf_dir");
	ConfReader cReader(CONF_DIR.c_str());
	string INDEX_DIR = cReader.getIndexPath();

	//load the index from the data source, build new indexes if necessary
	Indexer *indexer = DaemonDataSource::loadIndex(cReader);//NULL;

	//Begin to read parameters from the configuration file

	//get the delimiter
	char delimiter = cReader.getDelimiter();

	//TopK: 0; Advanced: 1
	int searchType = cReader.getSearchType();

	//Exact term: 0; Fuzzy term: 1
	int exactFuzzy = cReader.getIsFuzzyTermsQuery();

	//If the primary key is NOT searchable: 0; else: 1
	int ifPrimSearchable  = cReader.getIfPrimSearchable();

	//get the format of the records
    string primaryKeyName = cReader.getPrimaryKey();

	unsigned commandType = DAEMON_NONE;
	vector<string> values;

	//parse the request, tell the query type
	commandType = parseRequest(query, values);

	switch(commandType)
	{
		case DAEMON_SEARCH:
			searchCommand(values, indexer, delimiter, searchType, exactFuzzy);
			break;
		case DAEMON_INSERT:
			insertCommand(values, indexer, INDEX_DIR, delimiter, ifPrimSearchable);
			break;
		case DAEMON_DELETE:
			deleteCommand(values, indexer, INDEX_DIR);
			break;
		default:
			cout << "Invalid Request. Please check the request variables." << endl;
	}

	values.clear();

	delete indexer;
}
