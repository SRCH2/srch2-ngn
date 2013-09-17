//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#ifndef __WRAPPER__SRCH2SERVERCONG_H__
#define __WRAPPER__SRCH2SERVERCONG_H__
#include "util/xmlParser/pugixml.hpp"
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
using namespace pugi;

namespace srch2 {
namespace httpwrapper {

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
	float queryTermSimilarityThreshold;
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

    bool supportSwapInEditDistance;
	float defaultSpatialQueryBoundingBox;

	srch2::instantsearch::ResponseType searchResponseFormat;
	string attributeStringForMySQLQuery;

	//vector<string> searchableAttributes;

    // < name, <required, <default, <offset, boost> > > >
    map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > > searchableAttributesInfo;

	string attributeRecordBoost;


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

	std::string allowedRecordTokenizerCharacters;
	int isPrimSearchable;
	bool supportAttributeBasedSearch;

	bool enablePositionIndex;
	int attributeToSort;
	int ordering;
	//string httpServerDocumentRoot;
    string configFile;

    // mongo db related settings
	string mongoHost;
	string mongoPort;
	string mongoDbName;
	string mongoCollection;
	unsigned mongoListenerWaitTime;


    void splitString(string str, const string& delimiter, vector<string>& result);
    void splitBoostFieldValues(string boostString, map <string, unsigned>& boosts);

    bool isOnlyDigits(string& str);
    bool isFloat(string str);
    //Validate Functions
    bool isValidFieldType(string& fieldType, bool isSearchable);
    bool isValidFieldDefaultValue(string& defaultValue, srch2::instantsearch::FilterType fieldType);
    bool isValidBoostFieldValues(map<string, unsigned>& boostMap);
    bool isValidBool(string& fieldType);
    bool isValidBoostFields(map <string, unsigned>& boosts);
    bool isValidQueryTermBoost(string& quertTermBoost);
    bool isValidIndexCreateOrLoad(string& indexCreateLoad);
    bool isValidRecordScoreExpession(string& recordScoreExpression);
    bool isValidQueryTermSimilarityBoost(string& queryTermSimilarityBoost);
    bool isValidQueryTermSimilarityThreshold(string & qTermEditDistanceNormFactor);
    bool isValidQueryTermLengthBoost(string& queryTermLengthBoost);
    bool isValidPrefixMatch(string& prefixmatch);
    bool isValidCacheSize(string& cacheSize);
    bool isValidRows(string& rows);
    bool isValidMaxSearchThreads(string& maxSearchThreads);
    bool isValidBooleanValue(string& fieldBasedSearch);

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

    srch2::instantsearch::FilterType parseFieldType(string& fieldType);
    int parseFacetType(string& facetType);

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
	string getSrch2Home() const; // Srch2Home Directory
	unsigned getQueryTermBoost() const;
	float getQueryTermSimilarityBoost() const;
	float getQueryTermSimilarityThreshold() const ;
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

	srch2::instantsearch::ResponseType getSearchResponseFormat() const;
	const std::string& getAttributeStringForMySQLQuery() const;
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
	bool getSupportSwapInEditDistance() const;
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

	// Mongo related getter/setter
	const string& getMongoServerHost() const{
		return mongoHost;
	}
	const string& getMongoServerPort() const{
		return mongoPort;
	}
    const string& getMongoDbName() const{
    	return mongoDbName;
    }
    const string& getMongoCollection () const{
    	return mongoCollection;
    }
    const unsigned getMongoListenerWaitTime () const{
    	return mongoListenerWaitTime;
    }
    // THIS FUNCTION IS JUST FOR WRAPPER TEST
    void setFilePath(const string& dataFile);

    bool isPositionIndexEnabled() const;

};

}
}

#endif /* __WRAPPER__SRCH2SERVERCONG_H__ */
