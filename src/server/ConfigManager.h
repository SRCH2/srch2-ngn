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

// This class is used to collect information from the config file and pass them other modules
// in the system.
class SearchableAttributeInfoContainer {
public:
    SearchableAttributeInfoContainer(){
        attributeName = "";
        // Just because it must have a default value, TEXT has no meaning or value here
        attributeType = srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
	required = false;
	defaultValue = "";
	offset = 0;
	boost = 1;
	isMultiValued = false;
	highlight = false;
	isAclEnabled = false;
    }
    SearchableAttributeInfoContainer(const string & name,
            srch2::instantsearch::FilterType type,
				     const bool required,
				     const string & defaultValue ,
				     const unsigned offset,
				     const unsigned boost,
				     const bool isMultiValued,
				     bool highlight = false,
				     bool isAclEnabled = false){
        this->attributeName = name;
        this->attributeType = type;
        this->required = required;
        this->defaultValue = defaultValue;
        this->offset = offset;
        this->boost = boost;
        this->isMultiValued = isMultiValued;
        this->highlight = highlight;
        this->isAclEnabled = isAclEnabled;
    }
    // NO GETTER OR SETTERS ARE IMPLEMENTED FOR THESE MEMBERS
    // BECAUSE THIS CLASS IS MEANT TO BE A VERY SIMPLE CONTAINER WHICH ONLY CONTAINS THE
    // VALUES AND HAS NO BEHAVIOUR
    string attributeName;
    srch2::instantsearch::FilterType attributeType;
    bool required;
    string defaultValue;
    unsigned offset;
    unsigned boost;
    bool isMultiValued;
    bool highlight;
    bool isAclEnabled;
};

class RefiningAttributeInfoContainer {
public:
    RefiningAttributeInfoContainer(){
        attributeName = "";
	// Just because it must have a default value, TEXT has no meaning or value here
	attributeType = srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
	defaultValue = "";
	required = false;
	isMultiValued = false;
    }
    RefiningAttributeInfoContainer(const string & name,
				   srch2::instantsearch::FilterType type,
				   const string & defaultValue,
				   const bool required,
				   const bool isMultiValued,
				   const bool isAclEnabled){
        this->attributeName = name;
	this->attributeType = type;
	this->defaultValue = defaultValue;
	this->required = required;
	this->isMultiValued = isMultiValued;
	this->isAclEnabled = isAclEnabled;
    }
    // NO GETTER OR SETTERS ARE IMPLEMENTED FOR THESE MEMBERS
    // BECAUSE THIS CLASS IS MEANT TO BE A VERY SIMPLE CONTAINER WHICH ONLY CONTAINS THE
    // VALUES AND HAS NO BEHAVIOUR
    string attributeName;
    srch2::instantsearch::FilterType attributeType;
    string defaultValue;
    bool required;
    bool isMultiValued;
    bool isAclEnabled;
};

class CoreInfo_t;

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

// enum to allow loop iteration over listening ports
enum PortType_t {
    SearchPort = 0,
    SuggestPort,
    InfoPort,
    DocsPort,
    UpdatePort,
    SavePort,
    ExportPort,
    ResetLoggerPort,
    SearchAllPort,
    ShutDownAllPort,
    AttributeAclReplace,
    AttributeAclAppend,
    AttributeAclDelete,
    RecordAclReplace,
    RecordAclAppend,
    RecordAclDelete,
    AclAddRecordsForRole,
    AclAppendRecordsForRole,
    AclDeleteRecordsForRole,
    FeedbackPort,
    EndOfPortType // stop value - not valid (also used to indicate all/default ports)
};

inline  enum PortType_t incrementPortType(PortType_t &oldValue)
{
    unsigned int newValue = static_cast<int> (oldValue);
    newValue++;
    return static_cast<PortType_t> (newValue);
}

class ConfigManager {
public:
    typedef std::map<const string, CoreInfo_t *> CoreInfoMap_t;
    static const char* const OAuthParam;

private:
    static string authorizationKey;

    // <config>
    string licenseKeyFile;
    string httpServerListeningHostname;
    string httpServerListeningPort;
    string srch2Home;

    unsigned int numberOfThreads;
    unsigned int heartBeatTimer;

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

protected:
    CoreInfoMap_t coreInfoMap;

    // <config><cores>
    string defaultCoreName;
    bool defaultCoreSetFlag; // false unless <cores defaultCoreName="..."> has been parsed

    // parsing helper functions for modularity
    void parseIndexConfig(const xml_node &indexConfigNode, CoreInfo_t *coreInfo, map<string, unsigned> &boostsMap, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseDbParameters(const xml_node &dbNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    //CoreConfigParseState_t argument is added to parseQuery function because coreInfo's searchableAttributesInfo is not populated when this function is called, it is required to check responseContent
    void parseQuery(CoreConfigParseState_t *coreParseState, const xml_node &queryNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseSingleCore(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseMultipleCores(const xml_node &coresNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    // parse all data source settings (can handle multiple cores or default/no core)
    void parseDataConfiguration(const xml_node &configNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    // parse all settings for a single data source, either under <config> or within a <core>
    void parseDataFieldSettings(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    void parseUpdateHandler(const xml_node &updateHandlerNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);
    
    void parseSchema(const xml_node &schemaNode, CoreConfigParseState_t *coreParseState, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    bool setSearchableRefiningFromIndexedAttribute(const xml_node &field,
            bool &isSearchable, bool &isRefining, std::stringstream &parseError,
            bool &configSuccess, CoreInfo_t *coreInfo);

    bool setSearchableAndRefining(const xml_node &field, bool &isSearchable,
            bool &isRefining, std::stringstream &parseError,
            bool &configSuccess, CoreInfo_t *coreInfo);

    bool setFieldFlagsFromFile(const xml_node &field, bool &isMultiValued, bool &isSearchable,
    		bool &isRefining, bool &isHighlightEnabled, bool & isAclEnabled,
    		std::stringstream &parseError, bool &configSuccess,
    		CoreInfo_t *coreInfo);

    bool setCoreParseStateVector(bool isSearchable, bool isRefining, bool isMultiValued,
    		bool isHighlightEnabled, bool isAclEnabled, CoreConfigParseState_t *coreParseState,
    		CoreInfo_t *coreInfo, std::stringstream &parseError, const xml_node &field);

    bool setRefiningStateVectors(const xml_node &field, bool isMultiValued,
    		bool isRefining, vector<string> &RefiningFieldsVector,
    		vector<srch2::instantsearch::FilterType> &RefiningFieldTypesVector,
    		vector<bool> &RefiningAttributesRequiredFlagVector,
    		vector<string> &RefiningAttributesDefaultVector,
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
public:
    ConfigManager(const string& configfile);
    virtual ~ConfigManager();

    CoreInfo_t *getCoreInfoMap(const string &coreName) const;
    CoreInfoMap_t::iterator coreInfoIterateBegin() { return coreInfoMap.begin(); }
    CoreInfoMap_t::iterator coreInfoIterateEnd() { return coreInfoMap.end(); }
	const CoreInfo_t *getCoreInfo(const string &coreName) const { return ((CoreInfoMap_t) coreInfoMap)[coreName]; }

    void _setDefaultSearchableAttributeBoosts(const string &coreName, const vector<string> &searchableAttributesVector);

    void parse(const pugi::xml_document& configDoc, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

    const std::string& getCustomerName() const; //XXX: REMOVE?

    const std::string& getIndexPath(const string &coreName) const;
    const std::string& getPrimaryKey(const string &coreName) const;

    const map<string, SearchableAttributeInfoContainer > * getSearchableAttributes(const string &coreName) const;

    const map<string, RefiningAttributeInfoContainer > * getRefiningAttributes(const string &coreName) const;

    //const vector<unsigned>* getAttributesBoosts() const;
    const std::string& getAttributeRecordBoostName(const string &coreName) const;
    //string getDefaultAttributeRecordBoost() const;
    const std::string& getRecordAllowedSpecialCharacters(const string &coreName) const;
    int getSearchType(const string &coreName) const;
    int getIsPrimSearchable(const string &coreName) const;
    bool getIsFuzzyTermsQuery() const;
    bool getQueryTermPrefixType() const;
    bool getStemmerFlag(const string &coreName) const;
    const string &getSynonymFilePath(const string &coreName) const;
    const string &getProtectedWordsFilePath(const string &coreName) const;
    bool getSynonymKeepOrigFlag(const string &coreName) const; // Synonym: if we want to keep the original word or replace the synonym with it.
    const string &getStopFilePath(const string &coreName) const; // StopFilter File Path
    const string &getStemmerFile(const string &coreName) const; // stemmer file
    const string &getSrch2Home() const; // Srch2Home Directory
    unsigned getQueryTermBoost(const string &coreName) const;
    bool getSupportAttributeBasedSearch(const string &coreName) const;

    int getOrdering() const;

    unsigned getKeywordPopularityThreshold() const ;

    unsigned int getNumberOfThreads() const;

    unsigned int getHeartBeatTimer() const;

    const std::string& getAttributeStringForMySQLQuery() const;

    const std::string& getLicenseKeyFileName() const;

    const std::string& getHTTPServerAccessLogFile() const;
    const Logger::LogLevel& getHTTPServerLogLevel() const;
    const std::string& getHTTPServerListeningHostname() const;
    const std::string& getHTTPServerListeningPort() const;

    bool isRecordBoostAttributeSet(const string &coreName) const;

    int getIndexType(const string &coreName) const;
    bool getSupportSwapInEditDistance(const string &coreName) const;
    const std::string& getAttributeLatitude(const string &coreName) const;
    const std::string& getAttributeLongitude(const string &coreName) const;
    float getDefaultSpatialQueryBoundingBox() const;

    bool isFacetEnabled(const string &coreName) const;

    const vector<string> * getFacetAttributes(const string &coreName) const ;
    const vector<int> * getFacetTypes(const string &coreName) const;
    const vector<string> * getFacetStarts(const string &coreName) const ;
    const vector<string> * getFacetEnds(const string &coreName) const ;
    const vector<string> * getFacetGaps(const string &coreName) const ;

    //loadConfigFile should not exit the engine, hence bool is returned to indicate if engine should exit or not. Also it is helpful in ctest cases.
    bool loadConfigFile() ;

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

    CoreInfo_t *getDefaultCoreInfo() const;

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
    static const char* const authorizationKeyTag;
    static const char* const accessLogFileString;
    static const char* const analyzerString;
    static const char* const cacheSizeString;
    static const char* const configString;
    static const char* const dataDirString;
    static const char* const dataFileString;
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
    static const char* const indexConfigString;
    static const char* const indexedString;
    static const char* const aclString;
    static const char* const multiValuedString;
    static const char* const indexTypeString;
    //static const char* const licenseFileString;
    static const char* const listenerWaitTimeString;
    static const char* const listeningHostStringString;
    static const char* const listeningPortString;
    static const char* const locationLatitudeString;
    static const char* const locationLongitudeString;
    static const char* const logLevelString;
    static const char* const maxDocsString;
    static const char* const maxMemoryString;
    static const char* const maxSearchThreadsString;
    static const char* const mergeEveryMWritesString;
    static const char* const mergeEveryNSecondsString;
    static const char* const mergePolicyString;
    static const char* const nameString;
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

    static const char* const searchPortString;
    static const char* const suggestPortString;
    static const char* const infoPortString;
    static const char* const docsPortString;
    static const char* const updatePortString;
    static const char* const savePortString;
    static const char* const exportPortString;
    static const char* const resetLoggerPortString;

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

class AccessControlInfo{
public:
	string resourceCoreName;
	string roleCoreName;
	string aclDataFileName;
	AccessControlInfo(string &resourceCoreName, string &roleCoreName){
		this->resourceCoreName = resourceCoreName;
		this->roleCoreName = roleCoreName;
	};
};

// definitions for data source(s) (srch2Server objects within one HTTP server)
class CoreInfo_t {

public:
    CoreInfo_t(class ConfigManager *manager) : configManager(manager), accessControlInfo(NULL) {};
    CoreInfo_t(const CoreInfo_t &src);

    friend class ConfigManager;

    // **** accessors for settings in every core ****
    const string &getName() const { return name; }

    const string &getDataDir() const { return dataDir; }
    const string &getIndexPath() const { return indexPath; }
    DataSourceType getDataSourceType() const { return dataSourceType; }
    const string &getDataFile() const { return dataFile; }
    const string &getDataFilePath() const { return dataFilePath; }

    // THIS FUNCTION IS JUST FOR WRAPPER TEST
    void setDataFilePath(const string& path);

    const map<string, string> * getDbParameters() const {
        return &dbParameters;
    }
    const string& getDatabaseSharedLibraryPath() const {
        return dbSharedLibraryPath;
    }
    const string& getDatabaseSharedLibraryName() const {
        return dbSharedLibraryName;
    }

    int getIndexType() const { return indexType; }
    int getSearchType() const { return searchType; }
    int getSearchType(const string &coreName) const { return configManager->getSearchType(coreName); }
    const string &getPrimaryKey() const { return primaryKey; }
    int getIsPrimSearchable() const { return isPrimSearchable; }

    const std::string& getScoringExpressionString() const;
    float getFuzzyMatchPenalty() const;
    float getQueryTermSimilarityThreshold() const ;
    float getQueryTermLengthBoost() const;
    float getPrefixMatchPenalty() const;
    int getAttributeToSort() const;
    const vector<string> *getAttributesToReturn() const
        { return &attributesToReturn; }
    void setAttributesToReturn(vector<string> attributesToReturn)
        { this->attributesToReturn = attributesToReturn; }
    unsigned getDefaultNumberOfSuggestionsToReturn() const
        { return defaultNumberOfSuggestions; }

    srch2::instantsearch::ResponseType getSearchResponseFormat() const;
    int getSearchResponseJSONFormat() const;

    uint32_t getCacheSizeInBytes() const;
    int getDefaultResultsToRetrieve() const;

    bool isPositionIndexWordEnabled() const { return enableWordPositionIndex; }
    bool isPositionIndexCharEnabled() const { return enableCharOffsetIndex; }

    bool getSupportSwapInEditDistance() const
        { return supportSwapInEditDistance; }
    bool getSupportAttributeBasedSearch() const { return supportAttributeBasedSearch; }
    void setSupportAttributeBasedSearch(bool flag) { supportAttributeBasedSearch = flag; }
    unsigned getQueryTermBoost() const { return queryTermBoost; }
    int getOrdering() const { return configManager->getOrdering(); }

    const map<string, SearchableAttributeInfoContainer > *getSearchableAttributes() const
        { return &searchableAttributesInfo; }
    const map<string, RefiningAttributeInfoContainer > *getRefiningAttributes() const
      { return &refiningAttributesInfo; }
    bool isRecordBoostAttributeSet() const { return recordBoostFieldFlag; }
    const std::string& getAttributeRecordBoostName() const { return recordBoostField; }

    bool isFacetEnabled() const { return facetEnabled; }
    const vector<string> *getFacetAttributes() const { return &facetAttributes; }
    const vector<string> *getFacetStarts() const { return &facetStarts; }
    const vector<string> *getFacetEnds() const { return &facetEnds; }
    const vector<string> *getFacetGaps() const { return &facetGaps; }
    const vector<int> *getFacetTypes() const { return &facetTypes; }

    string getAttributeLatitude() const { return fieldLatitude; }
    string getAttributeLongitude() const { return fieldLongitude; }

    bool getStemmerFlag() const { return stemmerFlag; }
    bool getSynonymKeepOrigFlag() const { return synonymKeepOrigFlag; }
    const string &getStemmerFile() const { return stemmerFile; }
    const string &getSynonymFilePath() const { return synonymFilterFilePath; }
    const string &getStopFilePath() const { return stopFilterFilePath; }
    const string &getProtectedWordsFilePath() const { return protectedWordsFilePath; }
    const string& getRecordAllowedSpecialCharacters() const
        { return allowedRecordTokenizerCharacters; }
    AnalyzerType getAnalyzerType() const { return analyzerType; }

    const string& getChineseDictionaryPath() const { return chineseDictionaryFilePath;}

    uint32_t getDocumentLimit() const;
    uint64_t getMemoryLimit() const;

    uint32_t getMergeEveryNSeconds() const;
    uint32_t getMergeEveryMWrites() const;

    uint32_t getUpdateHistogramEveryPMerges() const;
    uint32_t getUpdateHistogramEveryQWrites() const;

    // **** accessors for settings in ConfigManager (global to all cores) ****
    const string &getSrch2Home() const { return configManager->getSrch2Home(); }
    const string& getLicenseKeyFileName() const { return configManager->getLicenseKeyFileName(); }
    const string& getHTTPServerListeningHostname() const
	    { return configManager->getHTTPServerListeningHostname(); }
    const string& getHTTPServerListeningPort() const { return configManager->getHTTPServerListeningPort(); }
    const string& getHTTPServerAccessLogFile() const { return configManager->getHTTPServerAccessLogFile(); }
    const Logger::LogLevel& getHTTPServerLogLevel() const
        { return configManager->getHTTPServerLogLevel(); }

    bool getIsFuzzyTermsQuery() const;

    float getDefaultSpatialQueryBoundingBox() const
	    { return configManager->getDefaultSpatialQueryBoundingBox(); }

    unsigned int getKeywordPopularityThreshold() const
        { return configManager->getKeywordPopularityThreshold(); }
    bool getQueryTermPrefixType() const;

    const unsigned getGetAllResultsNumberOfResultsThreshold() const
        { return configManager->getGetAllResultsNumberOfResultsThreshold(); }
    const unsigned getGetAllResultsNumberOfResultsToFindInEstimationMode() const
        { return configManager->getGetAllResultsNumberOfResultsToFindInEstimationMode(); }

    unsigned int getNumberOfThreads() const { return configManager->getNumberOfThreads(); }


    const vector<std::pair<unsigned, string> >& getHighlightAttributeIdsVector() const { return highlightAttributes; }
    void setHighlightAttributeIdsVector(vector<std::pair<unsigned, string> >& in) { highlightAttributes = in; }

    void getExactHighLightMarkerPre(string& markerStr) const{
    	markerStr = exactHighlightMarkerPre;
    }

    void getExactHighLightMarkerPost(string& markerStr) const{
    	markerStr = exactHighlightMarkerPost;
    }
    void getFuzzyHighLightMarkerPre(string& markerStr) const{
    	markerStr = fuzzyHighlightMarkerPre;
    }

    void getFuzzyHighLightMarkerPost(string& markerStr) const{
    	markerStr = fuzzyHighlightMarkerPost;
    }
    void getHighLightSnippetSize(unsigned& snippetSize) const{
    	snippetSize = highlightSnippetLen;
    }

    unsigned short getPort(PortType_t portType) const;
    void setPort(PortType_t portType, unsigned short portNumber);

    AccessControlInfo* getAccessControlInfo() const{
    	return this->accessControlInfo;
    }

    void setAccessControlInfo(AccessControlInfo* accessControlInfo){
    	this->accessControlInfo = accessControlInfo;
    }

    const std::string& getAttibutesAclFile() const {
    	return attrAclFilePath;
    }

    const std::string* getRecordAclFile() const{
    	return &recordAclFilePath;
    }

    bool getHasRecordAcl() const{
    	return hasRecordAcl;
    }

protected:
    string name; // of core

    ConfigManager *configManager;

    // <config>
    string dataDir;
    string indexPath; // srch2Home + dataDir
    DataSourceType dataSourceType;
    string dataFile;
    string dataFilePath;

    // database related settings
    map<string, string>  dbParameters;
    string dbSharedLibraryName;
    string dbSharedLibraryPath;

    int isPrimSearchable;

    // <schema>
    string primaryKey;

    // <schema><fields>
    string fieldLatitude;
    string fieldLongitude;

    int indexType;

    map<string , SearchableAttributeInfoContainer> searchableAttributesInfo;
    map<string , RefiningAttributeInfoContainer > refiningAttributesInfo;

    // <IndexConfig>
    bool supportSwapInEditDistance;

    bool enableWordPositionIndex;
    bool enableCharOffsetIndex;

    bool recordBoostFieldFlag;
    string recordBoostField;
    string getrecordBoostField() const { return recordBoostField; }
    unsigned queryTermBoost;
    IndexCreateOrLoad indexCreateOrLoad;
    IndexCreateOrLoad getindexCreateOrLoad() const { return indexCreateOrLoad; }

    // <config><query>
    int searchType;

    // <config><query><rankingAlgorithm>
    string scoringExpressionString;

    // <config><query>
    float fuzzyMatchPenalty;
    float queryTermSimilarityThreshold;
    float queryTermLengthBoost;
    float prefixMatchPenalty;
    vector<string> sortableAttributes;
    vector<srch2::instantsearch::FilterType> sortableAttributesType; // Float or unsigned
    vector<string> sortableAttributesDefaultValue;
    int attributeToSort;
    unsigned cacheSizeInBytes;
    int resultsToRetrieve;
    bool exactFuzzy;
    bool queryTermPrefixType;

    unsigned defaultNumberOfSuggestions;

    // <config><query><queryResponseWriter>
    srch2::instantsearch::ResponseType searchResponseContent;
    int searchResponseJsonFormat;
    vector<string> attributesToReturn;

    // <config><query>
    bool supportAttributeBasedSearch;

    // facet
    bool facetEnabled;
    vector<int> facetTypes; // 0 : simple , 1 : range
    vector<string> facetAttributes;
    vector<string> facetStarts;
    vector<string> facetEnds;
    vector<string> facetGaps;

    // <schema><types><fieldType><analyzer><filter>
    bool stemmerFlag;
    std::string stemmerFile;
    std::string synonymFilterFilePath;
    bool synonymKeepOrigFlag;
    std::string stopFilterFilePath;
    std::string protectedWordsFilePath;
    std::string attrAclFilePath;

    AnalyzerType analyzerType;
    std::string chineseDictionaryFilePath;

    // characters to specially treat as part of words, and not as a delimiter
    std::string allowedRecordTokenizerCharacters;

    // <core><updatehandler>
    uint64_t memoryLimit;
    uint32_t documentLimit;

    // <config><updatehandler><mergePolicy>
    unsigned mergeEveryNSeconds;
    unsigned mergeEveryMWrites;

    // no config option for this yet
    unsigned updateHistogramEveryPMerges;
    unsigned updateHistogramEveryQWrites;
    vector<std::pair<unsigned, string> > highlightAttributes;
    string exactHighlightMarkerPre;
    string exactHighlightMarkerPost;
    string fuzzyHighlightMarkerPre;
    string fuzzyHighlightMarkerPost;
    unsigned highlightSnippetLen;

    // array of local HTTP ports (if any) index by port type enum
    vector<unsigned short> ports;

    // keep the access control info for this core
    AccessControlInfo* accessControlInfo;
    bool hasRecordAcl;
    string recordAclFilePath;


};

}
}

#endif /* __WRAPPER__SRCH2SERVERCONG_H__ */
