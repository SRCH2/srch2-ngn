//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#ifndef __WRAPPER__SRCH2SERVERCONG_H__
#define __WRAPPER__SRCH2SERVERCONG_H__

#include <instantsearch/Schema.h>
#include <instantsearch/Constants.h>
#include "WrapperConstants.h"
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "util/Logger.h"

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;
namespace po = boost::program_options;

namespace srch2 {
namespace httpwrapper {

class ConfigManager {
private:

	// Argument file options
	string licenseKeyFile;
	string trieBootstrapDictFile;
	uint32_t documentLimit;
	uint64_t memoryLimit;
	string httpServerListeningHostname;
	string httpServerListeningPort;
	string kafkaBrokerHostName;
	uint16_t kafkaBrokerPort;
	string kafkaConsumerTopicName; //Customer name
	uint32_t kafkaConsumerPartitionId;
	uint32_t pingKafkaBrokerEveryNSeconds;
	uint32_t writeReadBufferInBytes;
	unsigned cacheSizeInBytes;
	unsigned mergeEveryNSeconds;
	unsigned mergeEveryMWrites;

	int indexType;
	bool supportSwap;
	string attributeLatitude;
	string attributeLongitude;
	float defaultSpatialQueryBoundingBox;

	string primaryKey;
	ResponseType searchResponseFormat;
	vector<string> attributesToReturn;
	int numberOfThreads;
	string attributeStringForMySQLQuery;

	//vector<string> searchableAttributes;

    // < name, <required, <default, <offset, boost> > > >
    map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > > searchableAttributesInfo;

	string attributeRecordBoost;
	string scoringExpressionString;


	// < name, <type, <default, isSortable>>>
	map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > > nonSearchableAttributesInfo;



	// facet
	bool facetEnabled;
	vector<int> facetTypes; // 0 : simple , 1 : range
	vector<string> facetAttributes;
	vector<string> facetStarts;
	vector<string> facetEnds;
	vector<string> facetGaps;

	//vector<unsigned> attributesBoosts;

	// This is the directory that will be set during installation.
	std::string installDir;

	std::string allowedRecordTokenizerCharacters;
	int searchType;
	int isPrimSearchable;
	bool exactFuzzy;
	bool queryTermType;
	unsigned queryTermBoost;
	float queryTermSimilarityBoost;
	float queryTermLengthBoost;
	float prefixMatchPenalty;
	bool supportAttributeBasedSearch;
	bool stemmerFlag;
	std::string stemmerFile;
	std::string synonymFilterFilePath;
	bool synonymKeepOrigFlag;
	std::string stopFilterFilePath;
	DataSourceType dataSourceType;
	WriteApiType writeApiType;


	int resultsToRetrieve;
	int attributeToSort;
	int ordering;
	int searchResponseJsonFormat;
	bool recordBoostAttributeSet;

	string indexPath;
	string filePath;
	string httpServerAccessLogFile;
	Logger::LogLevel loglevel;
	string httpServerErrorLogFile;
	//string httpServerDocumentRoot;
    string configFile;

public:
    ConfigManager(std::string& configfile);
	virtual ~ConfigManager();

	void kafkaOptionsParse(const po::variables_map &vm, bool &configSuccess, std::stringstream &parseError);
	void parse(const boost::program_options::variables_map &vm, bool &configSuccess, std::stringstream &parseError);

	const std::string& getCustomerName() const;
	uint32_t getDocumentLimit() const;
	uint64_t getMemoryLimit() const;

	const std::string& getIndexPath() const;
	const std::string& getFilePath() const;
	const std::string& getPrimaryKey() const;

	const map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > > * getSearchableAttributes() const;

	const map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > > * getNonSearchableAttributes() const;

    const vector<string> * getAttributesToReturnName() const;



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
	string getInstallDir() const; // install Directory
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
	WriteApiType getWriteApiType() const;

	ResponseType getSearchResponseFormat() const;
	const std::string& getAttributeStringForMySQLQuery() const;
	int getSearchResponseJSONFormat() const;

	const std::string& getLicenseKeyFileName() const;
	const std::string& getTrieBootstrapDictFileName() const;

	const std::string& getHTTPServerAccessLogFile() const;
	const Logger::LogLevel& getHTTPServerLogLevel() const;
	const std::string& getHTTPServerErrorLogFile() const;
	const std::string& getHTTPServerDocumentRoot() const;
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
	bool getSupportSwap() const;
	const std::string& getAttributeLatitude() const;
	const std::string& getAttributeLongitude() const;
	float getDefaultSpatialQueryBoundingBox() const;

	vector<string> getAttributesToReturn() const {
		return attributesToReturn;
	}

	void setAttributesToReturn(vector<string> attributesToReturn) {
		this->attributesToReturn = attributesToReturn;
	}


	bool isFacetEnabled() const;

	const vector<string> * getFacetAttributes() const ;
	const vector<int> * getFacetTypes() const;
	const vector<string> * getFacetStarts() const ;
	const vector<string> * getFacetEnds() const ;

	const vector<string> * getFacetGaps() const ;

        void loadConfigFile() ;
};

}
}

#endif /* __WRAPPER__SRCH2SERVERCONG_H__ */
