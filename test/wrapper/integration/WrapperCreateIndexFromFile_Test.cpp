#include <cassert>
#include <string>
#include <iostream>

#include <instantsearch/Indexer.h>
#include "wrapper/JSONRecordParser.h"
#include "wrapper/Srch2KafkaConsumer.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"

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

void test1(int argc, char** argv) {
    string srch2_config_file(getenv("configFile"));
    cout << "Config File: " << srch2_config_file << endl;

//    srch2_config_file = "/home/iman/srch2/repos/srch2-ngn/test/wrapper/conf.ini-WrapperCreateIndexFromJsonFile_Test.xml";
   srch2http::ConfigManager *serverConf = new srch2http::ConfigManager(srch2_config_file);
   serverConf->loadConfigFile();

   std::stringstream message;

//   message << "DocumentLimit: " << string(serverConf->getDocumentLimit()).c_str();
//   message << "MemoryLimit: " << string(serverConf->getMemoryLimit()).c_str();
//
//   message << "getIndexPath: " << string(serverConf->getIndexPath()).c_str();
//   message << "getFilePath: " << string(serverConf->getFilePath()).c_str();
//   message << "getPrimaryKey: " << string(serverConf->getPrimaryKey()).c_str();
//   message << "getSearchType: " << string(serverConf->getSearchType()).c_str();
//   message << "getIsPrimSearchable: " << string(serverConf->getIsPrimSearchable()).c_str();
//   message << "getIsFuzzyTermsQuery: " << string(serverConf->getIsFuzzyTermsQuery()).c_str();
//   message << "getQueryTermType: " << string(serverConf->getQueryTermType()).c_str();
//   message << "getStemmerFlag: " << string(serverConf->getStemmerFlag()).c_str();
//
//
//   message << "getSynonymFilePath: " << string(serverConf->getSynonymFilePath()).c_str();
//   message << "getSynonymKeepOrigFlag: " << string(serverConf->getSynonymKeepOrigFlag()).c_str();
//   message << "getStopFilePath: " << string(serverConf->getStopFilePath()).c_str();
//   message << "getStemmerFile: " << string(serverConf->getStemmerFile()).c_str();
//   message << "getSrch2Home: " << string(serverConf->getSrch2Home()).c_str();
//   message << "getQueryTermBoost: " << string(serverConf->getQueryTermBoost()).c_str();
//   message << "getQueryTermSimilarityBoost: " << string(serverConf->getQueryTermSimilarityBoost()).c_str();
//
//   message << "getQueryTermLengthBoost: " << string(serverConf->getQueryTermLengthBoost()).c_str();
//   message << "getPrefixMatchPenalty: " << string(serverConf->getPrefixMatchPenalty()).c_str();
//   message << "getSupportAttributeBasedSearch: " << string(serverConf->getSupportAttributeBasedSearch()).c_str();
//   message << "getDefaultResultsToRetrieve: " << string(serverConf->getDefaultResultsToRetrieve()).c_str();
//   message << "getAttributeToSort: " << string(serverConf->getAttributeToSort()).c_str();
//   message << "getOrdering: " << string(serverConf->getOrdering()).c_str();
//   message << "getQueryTermSimilarityBoost: " << string(serverConf->getQueryTermSimilarityBoost()).c_str();
//   message << "getRecordAllowedSpecialCharacters: " << string(serverConf->getRecordAllowedSpecialCharacters()).c_str();
//
//
//   message << "getCacheSizeInBytes: " << string(serverConf->getCacheSizeInBytes()).c_str();
//   message << "getDataSourceType: " << string(serverConf->getDataSourceType()).c_str();
//   message << "getIndexCreateOrLoad: " << string(serverConf->getIndexCreateOrLoad()).c_str();
//   message << "getWriteApiType: " << string(serverConf->getWriteApiType()).c_str();
//   message << "getSearchResponseFormat: " << string(serverConf->getSearchResponseFormat()).c_str();
//   message << "getAttributeStringForMySQLQuery: " << string(serverConf->getAttributeStringForMySQLQuery()).c_str();
//
//
//
//   message << "getLicenseKeyFileName: " << string(serverConf->getLicenseKeyFileName()).c_str();
//   message << "getTrieBootstrapDictFileName: " << string(serverConf->getTrieBootstrapDictFileName()).c_str();
//   message << "getHTTPServerAccessLogFile: " << string(serverConf->getHTTPServerAccessLogFile()).c_str();
//   message << "getHTTPServerLogLevel: " << string(serverConf->getHTTPServerLogLevel()).c_str();
//   message << "getHTTPServerErrorLogFile: " << string(serverConf->getHTTPServerErrorLogFile()).c_str();
//
//   message << "getHTTPServerDocumentRoot: " << string(serverConf->getHTTPServerDocumentRoot()).c_str();
//   message << "getHTTPServerListeningHostname: " << string(serverConf->getHTTPServerListeningHostname()).c_str();
//   message << "getHTTPServerListeningPort: " << string(serverConf->getHTTPServerListeningPort()).c_str();
//   message << "isRecordBoostAttributeSet: " << string(serverConf->isRecordBoostAttributeSet()).c_str();
//   message << "getIndexType: " << string(serverConf->getIndexType()).c_str();
//   message << "getIndexType: " << string(serverConf->getIndexType()).c_str();
//
//   message << "getAttributeLatitude: " << string(serverConf->getAttributeLatitude()).c_str();
//   message << "getDefaultSpatialQueryBoundingBox: " << string(serverConf->getDefaultSpatialQueryBoundingBox()).c_str();
//   message << "getSearchResponseJSONFormat: " << string(serverConf->getSearchResponseJSONFormat()).c_str();
//   message << "getNumberOfThreads: " << string(serverConf->getNumberOfThreads()).c_str();
//   message << "getMergeEveryNSeconds: " << string(serverConf->getMergeEveryNSeconds()).c_str();
//   message << "getMergeEveryMWrites: " << string(serverConf->getMergeEveryMWrites()).c_str();
//
//   message << "getScoringExpressionString: " << string(serverConf->getScoringExpressionString()).c_str();
//   message << "getAttributeRecordBoostName: " << string(serverConf->getAttributeRecordBoostName()).c_str();
//   message << "getMergeEveryMWrites: " << string(serverConf->getMergeEveryMWrites()).c_str();
//   message << "getMergeEveryMWrites: " << string(serverConf->getMergeEveryMWrites()).c_str();

//      const map<string, pair<unsigned, unsigned> > * getSearchableAttributes() const;
//      const vector<string> * getAttributesToReturnName() const;
//
//      const vector<string> * getSortableAttributesName() const;
//      const vector<srch2::instantsearch::FilterType> * getSortableAttributesType() const;
//      const vector<string> * getSortableAttributesDefaultValue() const;

   cout << message.str() << endl;

//      const std::string& getAttributeLatitude() const;
//      const std::string& getAttributeLongitude() const;

}


bool test2(int argc, char** argv)
{
//	//read configuration file
//    po::options_description description("Description");
//    po::variables_map vm_command_line_args;
//
//    description.add_options()
//    ("help", "produce a help message")
//    ("config-file", po::value<string>(), "config file");
//
//    try{
//        po::store(po::parse_command_line(argc, argv, description), vm_command_line_args);
//        po::notify(vm_command_line_args);
//    }catch(exception &ex) {
//        cout << "error while parsing the arguments : " << endl <<  ex.what() << endl;
//    }
//
//    std::string srch2_config_file = vm_command_line_args["config-file"].as<string>();

    string srch2_config_file(getenv("configFile"));
    unsigned found = srch2_config_file.find_last_of("/\\");
    string srch2home = srch2_config_file.substr(0,found);

    srch2http::ConfigManager *serverConf = new srch2http::ConfigManager(srch2_config_file);
    serverConf->loadConfigFile();

    string srch2homeSymbol = "${srch2home}";
    string licenseKeyFile = serverConf->getLicenseKeyFileName();
    if(licenseKeyFile.find(srch2homeSymbol) != string::npos){
        licenseKeyFile.replace(0,licenseKeyFile.find(srch2homeSymbol) + srch2homeSymbol.length(),srch2home);
    }
    cout << "-  srch2home: " << srch2home << endl;
    cout << "-  Lincense File: " << licenseKeyFile << endl;


	string accessLogFile = serverConf->getHTTPServerAccessLogFile();
	if(accessLogFile.find(srch2homeSymbol) != string::npos){
	    accessLogFile.replace(0,accessLogFile.find(srch2homeSymbol) + srch2homeSymbol.length(),srch2home);
    }
    cout << "-  Log File: " << accessLogFile << endl;

    string dataFile = serverConf->getFilePath();
    if(dataFile.find(srch2homeSymbol) != string::npos){
        dataFile.replace(0,dataFile.find(srch2homeSymbol) + srch2homeSymbol.length(),srch2home);
    }
    cout << "-  Data File: " << dataFile << endl;
    serverConf->setFilePath(dataFile);

	// check the license file
	LicenseVerifier::testFile(licenseKeyFile);

	FILE *logFile = fopen(accessLogFile.c_str(), "a");
	if(logFile == NULL){
		Logger::setOutputFile(stdout);
		Logger::error("Open Log file %s failed.", serverConf->getHTTPServerAccessLogFile().c_str());
    } else {
        Logger::setOutputFile(logFile);
    }
	Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

	// create IndexMetaData
	srch2is::IndexMetaData *indexMetaData = srch2http::Srch2KafkaConsumer::createIndexMetaData(serverConf);

	// Create an analyzer
	srch2is::Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
			"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, serverConf->getRecordAllowedSpecialCharacters());

	// Create a schema to the data source definition in the Srch2ServerConf
	srch2is::Schema *schema = srch2http::JSONRecordParser::createAndPopulateSchema(serverConf);

	Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

	cout << "Creating new index from JSON file..." << endl;
    std::stringstream log_str;
	srch2http::DaemonDataSource::createNewIndexFromFile(indexer, serverConf);

	srch2is::IndexSearcherInternal *ii = new IndexSearcherInternal(dynamic_cast<srch2is::IndexReaderWriter*>(indexer));
	ii->getTrie()->print_Trie();

	delete indexMetaData;
	delete indexer;
	delete serverConf;
	fclose(logFile);

	return true;
}


int main(int argc, char** argv)
{
    test1(argc, argv);
	test2(argc, argv);
	return 0;
}
