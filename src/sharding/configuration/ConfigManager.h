
// $Id$

#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include "src/server/util/xmlParser/pugixml.hpp"

#include <instantsearch/Schema.h>
#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include "ShardingConstants.h"
#include "CoreInfo.h"
#include "../sharding/metadata_manager/Shard.h"
#include "../sharding/metadata_manager/Node.h"
#include "src/core/util/Assert.h"
#include "src/core/util/Logger.h"
#include "src/core/util/mypthread.h"

#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <sys/types.h>
#include <boost/unordered_map.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;
namespace po = boost::program_options;
using namespace pugi;

namespace srch2 {
namespace httpwrapper {

class ResourceMetadataManager;

// A shard id consists of a core id, a partition id, and replica id
// E.g.: a core with 7 partitions, each of which has a primary and 4 replicas
//
//   P0  R0_1 R0_2 R0_3 R0_4
//   P1  R1_1 R1_2 R1_3 R1_4
//   ...
//   P6  R6_1 R6_2 R6_3 R6_4
//

inline  enum PortType_t incrementPortType(PortType_t &oldValue)
{
	unsigned int newValue = static_cast<int> (oldValue);
	newValue++;
	return static_cast<PortType_t> (newValue);
}


// helper state between different sections of the config file
struct CoreConfigParseState_t {
	bool hasLatitude;
	bool hasLongitude;
	vector<string> searchableFieldsVector;
	vector<srch2::instantsearch::FilterType> searchableFieldTypesVector;
	vector<bool> searchableAttributesRequiredFlagVector;
	vector<string> searchableAttributesDefaultVector;
	vector<bool> searchableAttributesIsMultiValued;
	vector<bool> searchableAttributesAclFlags;
	vector<bool> searchableAttributesHighlight;

	CoreConfigParseState_t() : hasLatitude(false), hasLongitude(false) {};
};

class Ping {
private:
	unsigned pingInterval;
	unsigned pingTimeout;
	unsigned retryCount;

public:
	Ping(){
		pingInterval = 2;
		pingTimeout = 8;
		retryCount = 1;
	}

	unsigned getPingInterval(){
		return pingInterval;
	}

	unsigned getPingTimeout(){
		return pingTimeout;
	}

	unsigned getRetryCount(){
		return retryCount;
	}

	void setPingInterval(unsigned pingInterval){
		this->pingInterval = pingInterval;
	}

	void setPingTimeout(unsigned pingTimeout){
		this->pingTimeout = pingTimeout;
	}

	void setRetryCount(unsigned retryCount){
		this->retryCount = retryCount;
	}
};

class Transport{
	private:
	unsigned port;
	string ipAddress;

	public:
	Transport(){
		port = 8087;
		ipAddress = "0.0.0.0";
	}
	void setPort(unsigned port);
	void setIpAddress(const string& ipAddress);
	unsigned getPort();
	string getIpAddress();
};

class MulticastDiscovery {
private:
	std::string groupAddress;
	unsigned port;   // Default value = 92612
	unsigned ttl;
	string ipAddress;

public:
	string getIpAddress();
	string getGroupAddress();
	unsigned getPort();
	unsigned getTtl();

	void setIpAddress(string& ipAddress);
	void setGroupAddress(string& groupAddress);
	void setPort(unsigned port);
	void setTtl(unsigned ttl);

	MulticastDiscovery(){
		port = 54000;
		ttl = 2;
		groupAddress = "224.2.2.10";
		ipAddress = "0.0.0.0";
	}

};


class ConfigManager {
public:


	// map of port type enums to strings to simplify code
	struct PortNameMap_t {
	    enum PortType_t portType;
	    const char *portName;
	    const char *portPath;
	};
	static PortNameMap_t portNameMap[] ;

	static const char* const OAuthParam;

	vector<std::pair<string, unsigned > > getWellKnownHosts(){
		return this->wellKnownHost;
	}

	void setWellKnownHost(pair<string, unsigned> p){
		wellKnownHost.push_back(p);
	}

	string createSRCH2Home();
	string createClusterDir(const string& clusterName);
	string createNodeDir(const string& clusterName);
	string createCoreDir(const string& clusterName, const string& coreName);
	string createShardDir(const string& clusterName, const string& coreName, const ShardId * shardId);

	string getSRCH2HomeDir();
	string getClusterName();
	string getClusterDir(const string& clusterName);
	string getNodeDir(const string& clusterName);
	string getCoreDir(const string& clusterName, const string& coreName);
	string getShardDir(const string& clusterName, const string& coreName, const ShardId * shardId);

	bool tryLockNodeName();
	void unlockNodeName();

	void renameDir(const string & src, const string & target);

	//It returns the number of files/directory deleted, if the returned value is 0, that means nothing got deleted.
	uint removeDir(const string& path);

	string getCurrentNodeName() const;

	Ping& getPing(){
		return this->ping;
	}

	MulticastDiscovery& getMulticastDiscovery(){
		return this->mDiscovery;
	}

	Transport& getTransport(){
		return this->transport;
	}

    CoreInfo_t * getCoreByName(const string &coreName) const{
    	for(unsigned coreIdx = 0 ; coreIdx < clusterCores.size(); ++coreIdx){
    		if(clusterCores.at(coreIdx)->getName().compare(coreName) == 0){
    			return clusterCores.at(coreIdx);
    		}
    	}
    	return NULL;
    }

    vector<CoreInfo_t *>::iterator coreInfoIterateBegin() {
    	return clusterCores.begin();
    }
    vector<CoreInfo_t *>::iterator coreInfoIterateEnd() {
    	return clusterCores.end();
    }

private:
    static string authorizationKey;

    // help in parsing and making the first Cluster readview
    string clusterNameStr ;
    string nodeNameStr;
    vector<CoreInfo_t *> clusterCores;
    vector<CoreInfo_t *> clusterAclCores;


	volatile bool isLocked; //both read / write use this lock.
//	DiscoveryParams discovery; // TODO : should we keep this member ?
	Ping ping;
	MulticastDiscovery mDiscovery;
	Transport transport;
	// <config>
	string licenseKeyFile;
	string httpServerListeningHostname;
	string httpServerListeningDefaultPortStr;
	unsigned short int httpServerListeningDefaultPort;
	string srch2Home;
	unsigned int numberOfThreads;
	unsigned int numberOfInternalThreads; // for internal use only
	unsigned int heartBeatTimer;

	int nodelockFd;

	vector<std::pair<string, unsigned > > wellKnownHost;

	// <config><keywordPopularitythreshold>
	unsigned keywordPopularityThreshold;

	// <config><updatehandler><updateLog>
	Logger::LogLevel loglevel;
	string httpServerAccessLogFile;
	string httpServerErrorLogFile;
	uint32_t writeReadBufferInBytes;
	float defaultSpatialQueryBoundingBox;
	string attributeStringForMySQLQuery;

	//vector<string> searchableAttributes;
	//vector<unsigned> attributesBoosts;
	int ordering;
	//string httpServerDocumentRoot;
	string configFile;

	// related to optimizing getAllResults. If the estimated number of results is
	// greater than getAllResultsNumberOfResultsThreshold, getAllResults only find
	// getAllResultsNumberOfResultsToFindInEstimationMode results.
	unsigned getAllResultsNumberOfResultsThreshold;
	unsigned getAllResultsNumberOfResultsToFindInEstimationMode;
	void splitString(string str, const string& delimiter, vector<string>& result);
	void splitBoostFieldValues(string boostString, map <string, unsigned>& boosts);
	bool isOnlyDigits(string& str);
	bool isFloat(string str);
	//Validate Functions
	bool isValidFieldType(string& fieldType, bool isSearchable);
	bool isValidFieldDefaultValue(string& defaultValue, srch2::instantsearch::FilterType fieldType, bool isMultiValued);
	bool isValidBoostFieldValues(map<string, unsigned>& boostMap);
	bool isValidBool(string& fieldType);
	bool isValidBoostFields(const CoreInfo_t *coreInfo, map <string, unsigned>& boosts);
	bool isValidQueryTermBoost(string& quertTermBoost);
	bool isValidIndexCreateOrLoad(string& indexCreateLoad);
	bool isValidRecordScoreExpession(string& recordScoreExpression);
	bool isValidFuzzyMatchPenalty(string& fuzzyMatchPenalty);
	bool isValidQueryTermSimilarityThreshold(string & qTermEditDistanceNormFactor);
	bool isValidQueryTermLengthBoost(string& queryTermLengthBoost);
	bool isValidPrefixMatch(string& prefixmatch);
	bool isValidCacheSize(string& cacheSize);
	bool isValidRows(string& rows);
	bool isValidMaxSearchThreads(string& maxSearchThreads);
	bool isValidBooleanValue(string& fieldBasedSearch);

	bool isValidQueryTermFuzzyType(string& queryTermFuzzyType);
	bool isValidQueryTermPrefixType(string& queryTermPrefixType);
	bool isValidResponseFormat(string& responseFormat);
	bool isValidResponseContentType(string responseContentType);
	bool isValidMaxDoc(string& maxDoc);
	bool isValidMaxMemory(string& maxMemory);
	bool isValidMergeEveryNSeconds(string& mergeEveryNSeconds);
	bool isValidMergeEveryMWrites(string& mergeEveryMWrites);
	bool isValidKeywordPopularityThreshold(string kpt);
	bool isValidGetAllResultsMaxResultsThreshold(string kpt);
	bool isValidGetAllResultsKAlternative(string kpt);
	bool isValidLogLevel(string& logLevel);
	bool isValidIndexType(string& indexType);
	bool isValidSearcherType(string& searcherType);

	srch2::instantsearch::FilterType parseFieldType(string& fieldType);
	int parseFacetType(string& facetType);
	void lowerCaseNodeNames(xml_node &node);
	void trimSpacesFromValue(string &fieldValue, const char *fieldName, std::stringstream &parseWarnings, const char *append = NULL);
	bool isNumber(const string &s);

protected:
	// <config><cores>
	string defaultCoreName;
	bool defaultCoreSetFlag; // false unless <cores defaultCoreName="..."> has been parsed

	// parsing helper functions for modularity
	void parseIndexConfig(const xml_node &indexConfigNode, CoreInfo_t *coreInfo, map<string, unsigned> &boostsMap, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	 void parseDbParameters(const xml_node &dbNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	//CoreConfigParseState_t argument is added to parseQuery function because coreInfo's searchableAttributesInfo is not
	//populated when this function is called, it is required to check responseContent
	void parseQuery(CoreConfigParseState_t *coreParseState, const xml_node &queryNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseSingleCore(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseMultipleCores(const xml_node &coresNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseSingleAccessControl(const xml_node &parentNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseAccessControls(const xml_node &accessControlsNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	// parse all data source settings (can handle multiple cores or default/no core)
	void parseAllCoreTags(const xml_node &configNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	// parse all settings for a single data source, either under <config> or within a <core>
	void parseCoreInformationTags(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseSchema(const xml_node &schemaNode, CoreConfigParseState_t *coreParseState, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	bool setSearchableRefiningFromIndexedAttribute(const xml_node &field,
			bool &isSearchable, bool &isRefining,
			std::stringstream &parseError, bool &configSuccess, CoreInfo_t *coreInfo);

	bool setSearchableAndRefining(const xml_node &field,
			bool &isSearchable, bool &isRefining,
			std::stringstream &parseError, bool &configSuccess, CoreInfo_t *coreInfo);

	bool setFieldFlagsFromFile(const xml_node &field, bool &isMultiValued,
			bool &isSearchable, bool &isRefining, bool &isHighlightEnabled, bool & isAclEnabled,
			std::stringstream &parseError, bool &configSuccess, CoreInfo_t *coreInfo);

	bool setCoreParseStateVector(bool isSearchable, bool isRefining, bool isMultiValued, bool isHighlightEnabled,
			bool isAclEnabled, CoreConfigParseState_t *coreParseState, CoreInfo_t *coreInfo, std::stringstream &parseError, const xml_node &field);

	bool setRefiningStateVectors(const xml_node &field, bool isMultiValued, bool isRefining,
			vector<string> &RefiningFieldsVector, vector<srch2::instantsearch::FilterType> &RefiningFieldTypesVector,
			vector<bool> &RefiningAttributesRequiredFlagVector, vector<string> &RefiningAttributesDefaultVector,
			vector<bool> &RefiningAttributesIsMultiValued,
			vector<bool> &refiningAttributesAclEnabledFlags, bool isAclEnabled,
			std::stringstream &parseError, CoreInfo_t *coreInfo);

	void parseFacetFields(const xml_node &schemaNode, CoreInfo_t *coreInfo, std::stringstream &parseError);

	void parseSchemaType(const xml_node &childNode, CoreInfo_t *coreInfo, std::stringstream &parseWarnings);

    void setUpStemmer(CoreInfo_t *coreInfo, const xml_node &field, std::stringstream &parseWarnings);

    void setUpChineseDictionary(CoreInfo_t * coreInfo, string &dictionaryPath, std::stringstream &parseWarnings);


    void setUpStopword(CoreInfo_t *coreInfo, const xml_node &field, std::stringstream &parseWarnings);
    void setUpProtectedWord(CoreInfo_t *coreInfo, const xml_node &field, std::stringstream &parseWarnings);
    void setUpSynonym(CoreInfo_t *coreInfo, const xml_node &field, std::stringstream &parseWarnings);
    void setUpRecordSpecialCharacters(CoreInfo_t *coreInfo, const xml_node &field);
    void setUpEnglishAnalyzer(CoreInfo_t * coreInfo, const xml_node &childNodeTemp, std::stringstream &parseWarnings);
    void setUpChineseAnalyzer(CoreInfo_t * coreInfo, string& dictionaryPath, const xml_node &childNodeTemp, std::stringstream &parseWarnings);


	void parseUpdateHandler(const xml_node &updateHandlerNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

public:
	ConfigManager(const string& configfile);
	virtual ~ConfigManager();
//	bool verifyConsistency(); // TODO : not used, to be deleted
	bool isLocal(ClusterShardId& shardId);

	//Declaring function to parse node tags
	void parseNode(std::vector<Node *>* nodes, xml_node& childNode, std::stringstream &parseWarnings, std::stringstream &parseError, bool configSuccess);

	void _setDefaultSearchableAttributeBoosts(const string &coreName, const vector<string> &searchableAttributesVector);

	void parse(const pugi::xml_document& configDoc, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	const std::string& getCustomerName() const; //XXX: REMOVE?

	const std::string& getIndexPath(const string &coreName) const;
	const std::string& getPrimaryKey(const string &coreName) const;

	const map<string, SearchableAttributeInfoContainer > * getSearchableAttributes(const string &coreName) const;

	const map<string, RefiningAttributeInfoContainer > * getRefiningAttributes(const string &coreName) const;

	//const vector<unsigned>* getAttributesBoosts() const;
	const std::string getAttributeRecordBoostName(const string &coreName) const;
	//string getDefaultAttributeRecordBoost() const;

	const std::string getRecordAllowedSpecialCharacters(const string &coreName) const;
	int getSearchType(const string &coreName) const;
	int getIsPrimSearchable(const string &coreName) const;
	bool getIsFuzzyTermsQuery() const;
	bool getQueryTermPrefixType() const;
	bool getStemmerFlag(const string &coreName) const;
	const string getSynonymFilePath(const string &coreName) const;
	const string getProtectedWordsFilePath(const string &coreName) const;
	bool getSynonymKeepOrigFlag(const string &coreName) const; // Synonym: if we want to keep the original word or replace the synonym with it.
	const string getStopFilePath(const string &coreName) const; // StopFilter File Path
	const string getStemmerFile(const string &coreName) const; // stemmer file
	const string getSrch2Home() const; // Srch2Home Directory
	unsigned getQueryTermBoost(const string &coreName) const;
	bool getSupportAttributeBasedSearch(const string &coreName) const;

	int getOrdering() const;

	unsigned getKeywordPopularityThreshold() const ;

	unsigned int getNumberOfThreads() const;
	unsigned int getNumberOfInternalThreads() const;
	unsigned int getHeartBeatTimer() const;

	const std::string& getAttributeStringForMySQLQuery() const;

	const std::string& getLicenseKeyFileName() const;

	const std::string& getHTTPServerAccessLogFile() const;
	const std::string getNewHTTPServerAccessLogFile(const string & newFile) const;
	const Logger::LogLevel& getHTTPServerLogLevel() const;
	const std::string& getHTTPServerListeningHostname() const;
	unsigned short int getHTTPServerDefaultListeningPort() const;

	bool isRecordBoostAttributeSet(const string &coreName) const;

	int getIndexType(const string &coreName) const;
	bool getSupportSwapInEditDistance(const string &coreName) const;
	const std::string getAttributeLatitude(const string &coreName) const;
	const std::string getAttributeLongitude(const string &coreName) const;
	float getDefaultSpatialQueryBoundingBox() const;

	bool isFacetEnabled(const string &coreName) const;

	const vector<string> * getFacetAttributes(const string &coreName) const ;
	const vector<int> * getFacetTypes(const string &coreName) const;
	const vector<string> * getFacetStarts(const string &coreName) const ;
	const vector<string> * getFacetEnds(const string &coreName) const ;
	const vector<string> * getFacetGaps(const string &coreName) const ;

	bool loadConfigFile(srch2http::ResourceMetadataManager * metadataManager = NULL) ;

	// Database related getter/setter
	const map<string,string> * getDbParameters(const string &coreName) const;
	const string& getDatabaseSharedLibraryName(const string &coreName) const;
	const string& getDatabaseSharedLibraryPath(const string &coreName) const;

	const unsigned getGetAllResultsNumberOfResultsThreshold() const {
		return this->getAllResultsNumberOfResultsThreshold;
	}

	const unsigned getGetAllResultsNumberOfResultsToFindInEstimationMode() const {
		return this->getAllResultsNumberOfResultsToFindInEstimationMode;
	}

	bool isPositionIndexEnabled(const string &coreName) const;

	const string &getDefaultCoreName() const
	{
		return defaultCoreName;
	}

	// true if config specifically names a default core
	const bool getDefaultCoreSetFlag() const
	{
		return defaultCoreSetFlag;
	}

    static string getAuthorizationKey();
	static void setAuthorizationKey(string &key);

    static const char* getRoleId(){
    	return aclRoleId;
    }

    static const char* getResourceId(){
    	return aclResourceId;
    }

private:

	// configuration file tag and attribute names for ConfigManager

	static const char* const multicastDiscovery;
	static const char* const multicastGroupAddress;
	static const char* const multicastIpAddress;
	static const char* const multicastPort;
	static const char* const multicastTtl;

	static const char* const transportNodeTag;
	static const char* const transportIpAddress;
	static const char* const transportPort;

	static const char* const wellKnownHosts;

	static const char* const nodeListeningHostNameTag;
	static const char* const nodeListeningPortTag;
	static const char* const nodeCurrentTag;
	static const char* const nodeNumberOfShardsTag ;
	static const char* const nodeNameTag;
	static const char* const nodeMasterTag;
	static const char* const nodeDataTag;
	static const char* const nodeHomeTag;
	static const char* const nodeDataDirTag;
	static const char* const primaryShardTag;
	static const char* const replicaShardTag;
	static const char* const clusterNameTag;
	static const int DefaultNumberOfPrimaryShards;
	static const int DefaultNumberOfReplicas;
	static const char* const DefaultClusterName;
	static const char* const pingNodeTag;
	static const char* const pingIntervalTag;
	static const char* const pingTimeoutTag;
	static const char* const retryCountTag;
	static const char* const accessLogFileString;
	static const char* const authorizationKeyTag;
	static const char* const analyzerString;
	static const char* const cacheSizeString;
	static const char* const configString;
	static const char* const dataDirString;
	static const char* const dataFileString;
	static const char* const multipleDataFilesTag;
	static const char* const dataSourceTypeString;
    static const char* const dbKeyString;
    static const char* const dbKeyValuesString;
    static const char* const dbKeyValueString;
    static const char* const dbParametersString;
    static const char* const dbSharedLibraryPathString;
    static const char* const dbSharedLibraryNameString;
    static const char* const dbValueString;
	static const char* const defaultString;
	static const char* const defaultQueryTermBoostString;
	static const char* const dictionaryString;
	static const char* const enablePositionIndexString;
	static const char* const enableCharOffsetIndexString;
	static const char* const expandString;
	static const char* const facetEnabledString;
	static const char* const attributeAclFileString;
	static const char* const facetEndString;
	static const char* const facetFieldString;
	static const char* const facetFieldsString;
	static const char* const facetGapString;
	static const char* const facetStartString;
	static const char* const facetTypeString;
	static const char* const fieldString;
	static const char* const fieldBasedSearchString;
	static const char* const fieldBoostString;
	static const char* const fieldsString;
	static const char* const fieldTypeString;
	static const char* const filterString;
	static const char* const fuzzyMatchPenaltyString;
	static const char* const hostString;
	static const char* const indexConfigString;
	static const char* const indexedString;
	static const char* const aclString;
	static const char* const multiValuedString;
	static const char* const indexTypeString;
//	static const char* const licenseFileString;
	static const char* const listenerWaitTimeString;
	static const char* const listeningHostStringString;
	static const char* const listeningPortString;
	static const char* const searchPortString;
	static const char* const suggestPortString;
	static const char* const infoPortString;
	static const char* const docsPortString;
	static const char* const updatePortString;
	static const char* const savePortString;
	static const char* const exportPortString;
	static const char* const resetLoggerPortString;
	static const char* const commitPortString;
	static const char* const mergePortString;
	static const char* const aclAttrRoleAddPortString;
	static const char* const aclAttrRoleDeletePortString;
	static const char* const aclAttrRoleAppendPortString;
	static const char* const aclRecorddRoleAddPortString;
	static const char* const aclRecordRoleAppendPortString;
	static const char* const aclRecordRoleDeletePortString;
	static const char* const clusterStatsPortString;
	static const char* const nodesStatsPortString;
	static const char* const debugStatsPortString;
	static const char* const searchAllPortString;
	static const char* const shutdownPortString;
	static const char* const nodeShutdownPortString;
	static const char* const locationLatitudeString;
	static const char* const locationLongitudeString;
	static const char* const logLevelString;
	static const char* const maxDocsString;
	static const char* const maxMemoryString;
	static const char* const maxRetryOnFailureString;
	static const char* const maxSearchThreadsString;
	static const char* const maxInternalThreadsString;
	static const char* const mergeEveryMWritesString;
	static const char* const mergeEveryNSecondsString;
	static const char* const mergePolicyString;
	static const char* const mongoDbString;
	static const char* const nameString;
	static const char* const portString;
	static const char* const porterStemFilterString;
	static const char* const tokenizerFilterString;
	static const char* const prefixMatchPenaltyString;
	static const char* const queryString;
	static const char* const queryResponseWriterString;
	static const char* const queryTermLengthBoostString;
	static const char* const queryTermFuzzyTypeString;
	static const char* const queryTermSimilarityThresholdString;
	static const char* const queryTermPrefixTypeString;
	static const char* const rankingAlgorithmString;
	static const char* const recordBoostFieldString;
	static const char* const recordScoreExpressionString;
	static const char* const refiningString;
	static const char* const requiredString;
	static const char* const responseContentString;
	static const char* const responseFormatString;
	static const char* const rowsString;
	static const char* const schemaString;
	static const char* const searchableString;
	static const char* const searcherTypeString;
	static const char* const srch2HomeString;
	static const char* const stopFilterString;
	static const char* const protectedWordFilterString;
	static const char* const supportSwapInEditDistanceString;
	static const char* const synonymFilterString;
	static const char* const synonymsString;
	static const char* const textEnString;
	static const char* const textZhString;
	static const char* const typeString;
	static const char* const typesString;
	static const char* const uniqueKeyString;
	static const char* const updateHandlerString;
	static const char* const updateLogString;
	static const char* const wordsString;
	static const char* const keywordPopularityThresholdString;
	static const char* const getAllResultsMaxResultsThreshold;
	static const char* const getAllResultsKAlternative;
	static const char* const multipleCoresString;
	static const char* const singleCoreString;
	static const char* const defaultCoreNameString;
	static const char* const hostPortString;
	static const char* const instanceDirString;
	static const char* const schemaFileString;
	static const char* const allowedRecordSpecialCharactersString;

	static const char* const highLightString;
	static const char* const highLighterString;
	static const char* const exactTagPre;
	static const char* const exactTagPost;
	static const char* const fuzzyTagPre;
	static const char* const fuzzyTagPost;
	static const char* const snippetSize;

	static const char* const accessControlString;
	static const char* const recordAclString;
	static const char* const attributeAclString;
	static const char* const aclRoleId;
	static const char* const aclResourceId;

	static const char* const defaultFuzzyPreTag;
	static const char* const defaultFuzzyPostTag;
	static const char* const defaultExactPreTag;
	static const char* const defaultExactPostTag;

	static const char* const heartBeatTimerTag;
public:
	static const char* const defaultCore;
};



}
}

#endif // __CONFIGMANAGER_H__
