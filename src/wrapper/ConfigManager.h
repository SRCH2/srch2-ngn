//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#ifndef __WRAPPER__SRCH2SERVERCONG_H__
#define __WRAPPER__SRCH2SERVERCONG_H__
#include "util/xmlParser/pugixml.hpp"
#include <instantsearch/Schema.h>
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "util/Logger.h"

using namespace std;
using namespace srch2::util;
using namespace pugi;

namespace srch2 {
namespace httpwrapper {

typedef enum {
    KAFKAWRITEAPI = 0, HTTPWRITEAPI = 1
} WriteApiType;

typedef enum {
    INDEXCREATE = 0, INDEXLOAD = 1
} IndexCreateOrLoad;

typedef enum {
    FILEBOOTSTRAP_FALSE = 0, FILEBOOTSTRAP_TRUE = 1
} DataSourceType;

class ConfigManager {
private:

	// <config>
	string licenseKeyFile;
	string httpServerListeningHostname;
	string httpServerListeningPort;
	string srch2Home;
	string indexPath;
	string filePath;


	// <confgi><indexConfig>
	bool recordBoostFieldFlag;
	string recordBoostField;
	unsigned queryTermBoost;
	IndexCreateOrLoad indexCreateOrLoad;


	// <config><query><rankingAlgorithm>
	string scoringExpressionString;


	// <config><query>
	float queryTermSimilarityBoost;
	float queryTermLengthBoost;
	float prefixMatchPenalty;
	vector<string> sortableAttributes;
	vector<srch2::instantsearch::FilterType> sortableAttributesType; // Float or unsigned
	vector<string> sortableAttributesDefaultValue;
	unsigned cacheSizeInBytes;
	int resultsToRetrieve;
	int numberOfThreads;
	int searchType;
	bool exactFuzzy;
	bool queryTermType;


	// <config><query><queryResponseWriter>
	int searchResponseJsonFormat;
	int searchResponseFormat;
	vector<string> attributesToReturn;


	// <config><query>
	DataSourceType dataSourceType;
	WriteApiType writeApiType;


	// <config><updatehandler>
	uint64_t memoryLimit;
	uint32_t documentLimit;

	// <config><updatehandler><mergePolicy>
	unsigned mergeEveryNSeconds;
	unsigned mergeEveryMWrites;

	// <config><updatehandler><updateLog>
	Logger::LogLevel loglevel;
    string httpServerAccessLogFile;
    string httpServerErrorLogFile;

    // <schema><fields>
	string fieldLatitude;
	string fieldLongitude;
	int indexType;

	// <schema>
	string primaryKey;

	// <schema><types><fieldType><analyzer><filter>
	bool stemmerFlag;
	std::string stemmerFile;
	std::string synonymFilterFilePath;
	bool synonymKeepOrigFlag;
	std::string stopFilterFilePath;

	string trieBootstrapDictFile;
	string kafkaBrokerHostName;
	uint16_t kafkaBrokerPort;
	string kafkaConsumerTopicName; //Customer name
	uint32_t kafkaConsumerPartitionId;
	uint32_t pingKafkaBrokerEveryNSeconds;
	uint32_t writeReadBufferInBytes;

	float defaultSpatialQueryBoundingBox;

	string attributeStringForMySQLQuery;

	//vector<string> searchableAttributes;
	// < keyword, < offset, boost > >
	map<string, pair<unsigned, unsigned> > searchableAttributesTriple;

	//vector<unsigned> attributesBoosts;

	std::string allowedRecordTokenizerCharacters;
	int isPrimSearchable;
	bool supportAttributeBasedSearch;
	int attributeToSort;
	int ordering;
	//string httpServerDocumentRoot;
    string configFile;


    void splitString(string str, const string& delimiter, vector<string>& result);
    void splitBoostFieldValues(string boostString, map <string, unsigned>& boosts);

    bool isOnlyDigits(string& str);
    bool isFloat(string str);
    //Validate Functions
    bool isValidFieldType(string& fieldType);
    bool isValidBoostFieldValues(map<string, unsigned>& boostMap);
    bool isValidBool(string& fieldType);
    bool isValidBoostFields(map <string, unsigned>& boosts);
    bool isValidQueryTermBoost(string& quertTermBoost);
    bool isValidIndexCreateOrLoad(string& indexCreateLoad);
    bool isValidRecordScoreExpession(string& recordScoreExpression);
    bool isValidQueryTermSimilarityBoost(string& queryTermSimilarityBoost);
    bool isValidQueryTermLengthBoost(string& queryTermLengthBoost);
    bool isValidPrefixMatch(string& prefixmatch);
    bool isValidSortField(vector<string>& sortField);
    bool isValidSortFieldType(vector<string>& sortFieldType);
    bool isValidSortFieldDefaultValue(vector<string>& sortFieldDefaultValue);
    bool isValidCacheSize(string& cacheSize);
    bool isValidRows(string& rows);
    bool isValidMaxSearchThreads(string& maxSearchThreads);
    bool isValidFieldBasedSearch(string& fieldBasedSearch);

    bool isValidQueryTermMatchType(string& queryTermMatchType);
    bool isValidQueryTermType(string& queryTermType);
    bool isValidResponseFormat(string& responseFormat);
    bool isValidResponseContentType(string responseContentType);
    bool isValidMaxDoc(string& maxDoc);
    bool isValidMaxMemory(string& maxMemory);
    bool isValidMergeEveryNSeconds(string& mergeEveryNSeconds);
    bool isValidMergeEveryMWrites(string& mergeEveryMWrites);
    bool isValidLogLevel(string& logLevel);
    bool isValidIndexType(string& indexType);
    bool isValidSearcherType(string& searcherType);


public:
    ConfigManager(const string& configfile);
	virtual ~ConfigManager();

//	void kafkaOptionsParse(const po::variables_map &vm, bool &configSuccess,			std::stringstream &parseError);
	void _setDefaultSearchableAttributeBoosts(			const vector<string> &searchableAttributesVector);
//	void parse(const boost::program_options::variables_map &vm,
//			bool &configSuccess, std::stringstream &parseError);

	void parse(const pugi::xml_document& configDoc, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	const std::string& getCustomerName() const; //XXX: REMOVE?
	uint32_t getDocumentLimit() const;
	uint64_t getMemoryLimit() const;

	const std::string& getIndexPath() const;
	const std::string& getFilePath() const;
	const std::string& getPrimaryKey() const;

	const map<string, pair<unsigned, unsigned> > * getSearchableAttributes() const;
	const vector<string> * getAttributesToReturnName() const;

	const vector<string> * getSortableAttributesName() const;
	const vector<srch2::instantsearch::FilterType> * getSortableAttributesType() const;
	const vector<string> * getSortableAttributesDefaultValue() const;

	//const vector<unsigned>* getAttributesBoosts() const;
	const std::string& getAttributeRecordBoostName() const;
	//string getDefaultAttributeRecordBoost() const;
	const std::string& getScoringExpressionString() const;

	const std::string& getRecordAllowedSpecialCharacters() const;
	int getSearchType() const;
	int getIsPrimSearchable() const;
	bool getIsFuzzyTermsQuery() const;
	bool getQueryTermType() const;
	bool getStemmerFlag() const;
	string getSynonymFilePath() const;
	bool getSynonymKeepOrigFlag() const; // Synonym: if we want to keep the original word or replace the synonym with it.
	string getStopFilePath() const; // StopFilter File Path
	string getStemmerFile() const; // stemmer file
	string getSrch2Home() const; // Srch2Home Directory
	unsigned getQueryTermBoost() const;
	float getQueryTermSimilarityBoost() const;
	float getQueryTermLengthBoost() const;
	float getPrefixMatchPenalty() const;
	bool getSupportAttributeBasedSearch() const;
	int getDefaultResultsToRetrieve() const;
	int getAttributeToSort() const;
	int getOrdering() const;

	uint32_t getCacheSizeInBytes() const;
	uint32_t getMergeEveryNSeconds() const;
	uint32_t getMergeEveryMWrites() const;

	int getNumberOfThreads() const;

	DataSourceType getDataSourceType() const;
	IndexCreateOrLoad getIndexCreateOrLoad() const;
	WriteApiType getWriteApiType() const;

	int getSearchResponseFormat() const;
//	const std::string& getAttributeStringForMySQLQuery() const;
	int getSearchResponseJSONFormat() const;

	const std::string& getLicenseKeyFileName() const;
	const std::string& getTrieBootstrapDictFileName() const;

	const std::string& getHTTPServerAccessLogFile() const;
	const Logger::LogLevel& getHTTPServerLogLevel() const;
	const std::string& getHTTPServerListeningHostname() const;
	const std::string& getHTTPServerListeningPort() const;

	const std::string& getKafkaBrokerHostName() const;
	uint16_t getKafkaBrokerPort() const;
	const std::string& getKafkaConsumerTopicName() const;
	uint32_t getKafkaConsumerPartitionId() const;
	uint32_t getWriteReadBufferInBytes() const;
	uint32_t getPingKafkaBrokerEveryNSeconds() const;

	bool isRecordBoostAttributeSet() const;

	int getIndexType() const;
	const std::string& getAttributeLatitude() const;
	const std::string& getAttributeLongitude() const;
	float getDefaultSpatialQueryBoundingBox() const;
    void loadConfigFile() ;

    // THIS FUNCTION IS JUST FOR WRAPPER TEST
    void setFilePath(const string& dataFile);

};

}
}

#endif /* __WRAPPER__SRCH2SERVERCONG_H__ */
