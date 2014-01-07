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
#include <ConfigManager.h>

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
	srch2http::CoreInfo_t *core = serverConf->getDefaultCoreInfo();

    std::stringstream message;
    message << "Srch2Home: " << core->getSrch2Home() << "\n";
    message << "LicenseKeyFileName: " << core->getLicenseKeyFileName() << "\n";
    message << "IndexPath: " << core->getIndexPath() << "\n";
    message << "FilePath: " << core->getDataFilePath() << "\n";
    message << "HTTPServerListeningHostname: " << core->getHTTPServerListeningHostname() << "\n";
    message << "HTTPServerListeningPort: " << core->getHTTPServerListeningPort() << "\n";
    message << "\n";

    message << "DocumentLimit: " << core->getDocumentLimit() << "\n";
    message << "MemoryLimit: " << core->getMemoryLimit() << "\n";
    message << "PrimaryKey: " << core->getPrimaryKey() << "\n";
    message << "IsPrimSearchable: " << core->getIsPrimSearchable() << "\n";
    message << "IsFuzzyTermsQuery: " << core->getIsFuzzyTermsQuery() << "\n";
    message << "QueryTermPrefixType: " << core->getQueryTermPrefixType() << "\n";
    message << "\n";

    message << "IndexType: " << core->getIndexType() << "\n";
    message << "SearchType: " << core->getSearchType() << "\n";
    message << "DataSourceType: " << core->getDataSourceType() << "\n";
    message << "AttributeLatitude: " << core->getAttributeLatitude()<< "\n";
    message << "AttributeLongitude: " << core->getAttributeLongitude()<< "\n";
    message << "\n";

    message << "StemmerFlag: " << core->getStemmerFlag() << "\n";
    message << "SynonymFilePath: " << core->getSynonymFilePath() << "\n";
    message << "SynonymKeepOrigFlag: " << core->getSynonymKeepOrigFlag() << "\n";
    message << "StopFilePath: " << core->getStopFilePath() << "\n";
    message << "StemmerFile: " << core->getStemmerFile() << "\n";
    message << "\n";

    message << "QueryTermBoost: " << core->getQueryTermBoost() << "\n";
    message << "FuzzyMatchPenalty: " << core->getFuzzyMatchPenalty() << "\n";
    message << "QueryTermLengthBoost: " << core->getQueryTermLengthBoost() << "\n";
    message << "PrefixMatchPenalty: " << core->getPrefixMatchPenalty() << "\n";
    message << "AttributeRecordBoostName: " << core->getAttributeRecordBoostName() << "\n";
    message << "SupportAttributeBasedSearch: " << core->getSupportAttributeBasedSearch() << "\n";
    message << "DefaultResultsToRetrieve: " << core->getDefaultResultsToRetrieve() << "\n";
    message << "\n";

    message << "AttributeToSort: " << core->getAttributeToSort() << "\n";
    message << "Ordering: " << core->getOrdering() << "\n";
    message << "FuzzyMatchPenalty: " << core->getFuzzyMatchPenalty() << "\n";
    message << "RecordAllowedSpecialCharacters: " << core->getRecordAllowedSpecialCharacters() << "\n";
    message << "CacheSizeInBytes: " << core->getCacheSizeInBytes() << "\n";
    message << "WriteApiType: " << core->getWriteApiType() << "\n";
    message << "SearchResponseFormat: " << core->getSearchResponseFormat() << "\n";

    message << "HTTPServerAccessLogFile: " << core->getHTTPServerAccessLogFile() << "\n";
    message << "HTTPServerLogLevel: " << core->getHTTPServerLogLevel() << "\n";

    message << "isRecordBoostAttributeSet: " << core->isRecordBoostAttributeSet() << "\n";

    message << "DefaultSpatialQueryBoundingBox: " << core->getDefaultSpatialQueryBoundingBox() << "\n";
    message << "SearchResponseJSONFormat: " << core->getSearchResponseJSONFormat() << "\n";
    message << "NumberOfThreads: " << core->getNumberOfThreads() << "\n";
    message << "MergeEveryNSeconds: " << core->getMergeEveryNSeconds() << "\n";
    message << "MergeEveryMWrites: " << core->getMergeEveryMWrites() << "\n";
    message << "ScoringExpressionString: " << core->getScoringExpressionString() << "\n";

    const map<string, srch2http::SearchableAttributeInfoContainer > * searchableAttributes = core->getSearchableAttributes();
    map<string, srch2http::SearchableAttributeInfoContainer >::const_iterator iter;
    message << "Searchable Attributes:\n";
    for (iter = searchableAttributes->begin(); iter != searchableAttributes->end(); iter++) {
        message << iter->first << "  " << iter->second.offset << "  " << iter->second.boost << "\n";
    }

    const vector<string> * attToReturn = core->getAttributesToReturn();
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
	srch2http::CoreInfo_t *core = serverConf->getDefaultCoreInfo();

    string srch2home = core->getSrch2Home();
    string licenseKeyFile =core->getLicenseKeyFileName();
    cout << "-  srch2home: " << srch2home << endl;
    cout << "-  Lincense File: " << licenseKeyFile << endl;

    string accessLogFile = core->getHTTPServerAccessLogFile();
    cout << "-  Log File: " << accessLogFile << endl;

    string dataFile =  core->getDataFilePath();
    cout << "-  Data File: " << dataFile << endl;
    core->setDataFilePath(dataFile);

    cout << "-  index File: " << core->getIndexPath()<< endl;
    // check the license file
    LicenseVerifier::testFile(licenseKeyFile);

    FILE *logFile = fopen(accessLogFile.c_str(), "a");
    if (logFile == NULL) {
        Logger::setOutputFile(stdout);
        Logger::error("Open Log file %s failed.", core->getHTTPServerAccessLogFile().c_str());
    } else {
        Logger::setOutputFile(logFile);
    }
    Logger::setLogLevel(core->getHTTPServerLogLevel());

    // create IndexMetaData
    srch2http::Srch2Server srch2Server;
    srch2is::IndexMetaData *indexMetaData = srch2Server.createIndexMetaData(core);

    // Create an analyzer
    srch2is::Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL,
                                               core->getRecordAllowedSpecialCharacters());

    // Create a schema to the data source definition in the Srch2ServerConf
    srch2is::Schema *schema = srch2http::JSONRecordParser::createAndPopulateSchema(core);

    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    cout << "Creating new index from JSON file..." << endl;
    std::stringstream log_str;
    srch2http::DaemonDataSource::createNewIndexFromFile(indexer, core);
    indexer->commit();
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
