#include <cassert>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <instantsearch/Indexer.h>
#include "wrapper/JSONRecordParser.h"
#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "wrapper/Srch2Server.h"
#include <map>
#include <vector>

namespace po = boost::program_options;
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

void parseProgramArguments(int argc, char** argv,
        po::options_description& description,
        po::variables_map& vm_command_line_args) {
    description.add_options()("help", "Prints help message")("version",
            "Prints version number of the engine")("config-file",
            po::value<string>(), "Path to the config file");
    try {
        po::store(po::parse_command_line(argc, argv, description),
                vm_command_line_args);
        po::notify(vm_command_line_args);
    } catch (exception &ex) {
        cout << "error while parsing the arguments : " << endl << ex.what()
                << endl;
        cout << "Usage: $SRCH2_HOME/bin/srch2-engine" << endl;
        cout << description << endl;
        exit(-1);
    }
}

void test1(int argc, char** argv) {
    // Parse command line arguments
    po::options_description description("Optional Arguments");
    po::variables_map vm_command_line_args;
    parseProgramArguments(argc, argv, description, vm_command_line_args);

    std::string srch2_config_file = vm_command_line_args["config-file"].as<string>();
    int status = ::access(srch2_config_file.c_str(), F_OK);
    if (status != 0) {
        std::cout << "config file = '" << srch2_config_file
                << "' not found or could not be read" << std::endl;
        return ;
    }

    cout << "##############################################" << endl;
    cout << "Test 1 Started:" << endl;
    cout << "Config File: " << srch2_config_file << endl;

    srch2http::ConfigManager *serverConf = new srch2http::ConfigManager(srch2_config_file);
    serverConf->loadConfigFile();

    std::stringstream message;
    message << "Srch2Home: " << serverConf->getSrch2Home() << "\n";
    message << "LicenseKeyFileName: " << serverConf->getLicenseKeyFileName() << "\n";
    message << "IndexPath: " << serverConf->getIndexPath() << "\n";
    message << "FilePath: " << serverConf->getFilePath() << "\n";
    message << "HTTPServerListeningHostname: " << serverConf->getHTTPServerListeningHostname() << "\n";
    message << "HTTPServerListeningPort: " << serverConf->getHTTPServerListeningPort() << "\n";
    message << "\n";

    message << "DocumentLimit: " << serverConf->getDocumentLimit() << "\n";
    message << "MemoryLimit: " << serverConf->getMemoryLimit() << "\n";
    message << "PrimaryKey: " << serverConf->getPrimaryKey() << "\n";
    message << "IsPrimSearchable: " << serverConf->getIsPrimSearchable() << "\n";
    message << "IsFuzzyTermsQuery: " << serverConf->getIsFuzzyTermsQuery() << "\n";
    message << "QueryTermPrefixType: " << serverConf->getQueryTermPrefixType() << "\n";
    message << "\n";

    message << "IndexType: " << serverConf->getIndexType() << "\n";
    message << "SearchType: " << serverConf->getSearchType() << "\n";
    message << "DataSourceType: " << serverConf->getDataSourceType() << "\n";
    message << "AttributeLatitude: " << serverConf->getAttributeLatitude()<< "\n";
    message << "AttributeLongitude: " << serverConf->getAttributeLongitude()<< "\n";
    message << "\n";

    message << "StemmerFlag: " << serverConf->getStemmerFlag() << "\n";
    message << "SynonymFilePath: " << serverConf->getSynonymFilePath() << "\n";
    message << "SynonymKeepOrigFlag: " << serverConf->getSynonymKeepOrigFlag() << "\n";
    message << "StopFilePath: " << serverConf->getStopFilePath() << "\n";
    message << "StemmerFile: " << serverConf->getStemmerFile() << "\n";
    message << "\n";

    message << "QueryTermBoost: " << serverConf->getQueryTermBoost() << "\n";
    message << "FuzzyMatchPenalty: " << serverConf->getFuzzyMatchPenalty() << "\n";
    message << "QueryTermLengthBoost: " << serverConf->getQueryTermLengthBoost() << "\n";
    message << "PrefixMatchPenalty: " << serverConf->getPrefixMatchPenalty() << "\n";
    message << "AttributeRecordBoostName: " << serverConf->getAttributeRecordBoostName() << "\n";
    message << "SupportAttributeBasedSearch: " << serverConf->getSupportAttributeBasedSearch() << "\n";
    message << "DefaultResultsToRetrieve: " << serverConf->getDefaultResultsToRetrieve() << "\n";
    message << "\n";

    message << "AttributeToSort: " << serverConf->getAttributeToSort() << "\n";
    message << "Ordering: " << serverConf->getOrdering() << "\n";
    message << "FuzzyMatchPenalty: " << serverConf->getFuzzyMatchPenalty() << "\n";
    message << "RecordAllowedSpecialCharacters: " << serverConf->getRecordAllowedSpecialCharacters() << "\n";
    message << "CacheSizeInBytes: " << serverConf->getCacheSizeInBytes() << "\n";
    message << "WriteApiType: " << serverConf->getWriteApiType() << "\n";
    message << "SearchResponseFormat: " << serverConf->getSearchResponseFormat() << "\n";

    message << "TrieBootstrapDictFileName: " << serverConf->getTrieBootstrapDictFileName() << "\n";
    message << "HTTPServerAccessLogFile: " << serverConf->getHTTPServerAccessLogFile() << "\n";
    message << "HTTPServerLogLevel: " << serverConf->getHTTPServerLogLevel() << "\n";

    message << "isRecordBoostAttributeSet: " << serverConf->isRecordBoostAttributeSet() << "\n";

    message << "DefaultSpatialQueryBoundingBox: " << serverConf->getDefaultSpatialQueryBoundingBox() << "\n";
    message << "SearchResponseJSONFormat: " << serverConf->getSearchResponseJSONFormat() << "\n";
    message << "NumberOfThreads: " << serverConf->getNumberOfThreads() << "\n";
    message << "MergeEveryNSeconds: " << serverConf->getMergeEveryNSeconds() << "\n";
    message << "MergeEveryMWrites: " << serverConf->getMergeEveryMWrites() << "\n";
    message << "ScoringExpressionString: " << serverConf->getScoringExpressionString() << "\n";

    const map<string, pair<bool, pair<string, pair<unsigned, unsigned> > > > * searchableAttributes = serverConf->getSearchableAttributes();
    map<string, pair<bool, pair<string, pair<unsigned, unsigned> > > >::const_iterator iter;
    message << "Searchable Attributes:\n";
    for (iter = searchableAttributes->begin(); iter != searchableAttributes->end(); iter++) {
        message << iter->first << "  " << iter->second.second.second.first << "  " << iter->second.second.second.second << "\n";
    }

    const vector<string> * attToReturn = serverConf->getAttributesToReturnName();
    message << "Attributes to return:\n";
    for (int i = 0; i < attToReturn->size(); i++) {
        message << (*attToReturn)[i] << "  ";
    }
    message << "\n";


    cout << message.str() << endl;


}

bool test2(int argc, char** argv) {
    // Parse command line arguments
    po::options_description description("Optional Arguments");
    po::variables_map vm_command_line_args;
    parseProgramArguments(argc, argv, description, vm_command_line_args);

    std::string srch2_config_file = vm_command_line_args["config-file"].as<string>();
    int status = ::access(srch2_config_file.c_str(), F_OK);
    if (status != 0) {
        std::cout << "config file = '" << srch2_config_file
               << "' not found or could not be read" << std::endl;
        return -1;
    }
    cout << "##############################################" << endl;
    cout << "Test 2 Started:" << endl;

    srch2http::ConfigManager *serverConf = new srch2http::ConfigManager(srch2_config_file);
    serverConf->loadConfigFile();

    string srch2home = serverConf->getSrch2Home();
    string licenseKeyFile =serverConf->getLicenseKeyFileName();
    cout << "-  srch2home: " << srch2home << endl;
    cout << "-  Lincense File: " << licenseKeyFile << endl;

    string accessLogFile = serverConf->getHTTPServerAccessLogFile();
    cout << "-  Log File: " << accessLogFile << endl;

    string dataFile =  serverConf->getFilePath();
    cout << "-  Data File: " << dataFile << endl;
    serverConf->setFilePath(dataFile);

    cout << "-  index File: " << serverConf->getIndexPath()<< endl;
    // check the license file
    LicenseVerifier::testFile(licenseKeyFile);

    FILE *logFile = fopen(accessLogFile.c_str(), "a");
    if (logFile == NULL) {
        Logger::setOutputFile(stdout);
        Logger::error("Open Log file %s failed.", serverConf->getHTTPServerAccessLogFile().c_str());
    } else {
        Logger::setOutputFile(logFile);
    }
    Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

    // create IndexMetaData
    srch2http::Srch2Server srch2Server;
    srch2is::IndexMetaData *indexMetaData = srch2Server.createIndexMetaData(serverConf);

    // Create an analyzer
    srch2is::Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "", "", "",
            SYNONYM_DONOT_KEEP_ORIGIN, serverConf->getRecordAllowedSpecialCharacters());

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
	if (logFile)
		fclose(logFile);

    return true;
}

int main(int argc, char** argv) {
    test1(argc, argv);
    test2(argc, argv);
    return 0;
}
