#ifndef __SHARDING_CONFIGURATION_COREINFO_H__
#define __SHARDING_CONFIGURATION_COREINFO_H__


#include <instantsearch/Schema.h>
#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include "ShardingConstants.h"
#include "src/core/util/Assert.h"
#include "src/core/util/Logger.h"
#include "src/core/util/mypthread.h"
#include <stdint.h>


using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class ConfigManager;
class ClusterShardId;

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
		isAclEnabled = false;
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

	unsigned getCoreId() const{
		return coreId;
	}
	void setCoreId(unsigned coreId){
		this->coreId = coreId;
	}

	unsigned getNumberOfPrimaryShards() const{
		return this->numberOfPrimaryShards;
	}

	unsigned getNumberOfReplicas() const{
		return this->numberOfReplicas;
	}

	bool isDistributedCore() const{
		return (this->getNumberOfPrimaryShards() > 0);
	}

	ClusterShardId getPrimaryShardId(unsigned partitionId) const;

	CoreInfo_t(class ConfigManager *manager) : configManager(manager),  accessControlInfo(NULL)  {
		schema = NULL;
		aclCoreFlag = false;
	};

	CoreInfo_t(const CoreInfo_t &src);

	~CoreInfo_t() {
		if(schema != NULL){
			delete schema;
		}
	};
	friend class ConfigManager;


//	void getJsonFilePaths(vector<string> &paths) const{
//		for(vector<string>::const_iterator pathItr = dataFilePaths.begin(); pathItr != dataFilePaths.end(); ++pathItr){
//			paths.push_back(*pathItr);
//		}
//		if(getDataFilePath().compare("") != 0){
//			paths.push_back(getDataFilePath());
//		}
//	}
//	void setJsonFilePaths(const string & path){ dataFilePaths.push_back(path); }

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
	int getSearchType(const string &coreName) const ;
	const string &getPrimaryKey() const { return primaryKey; }
	string getFieldLatitude() const{return fieldLatitude;}
	string getFieldLongitude() const{return fieldLongitude;}
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
	int getOrdering() const ;

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

	AnalyzerType getAnalyzerType() const {	return analyzerType;};

	const string getChineseDictionaryPath() const { return chineseDictionaryFilePath;	}

	uint32_t getDocumentLimit() const;
	uint64_t getMemoryLimit() const;

	uint32_t getMergeEveryNSeconds() const;
	uint32_t getMergeEveryMWrites() const;

	uint32_t getUpdateHistogramEveryPMerges() const;
	uint32_t getUpdateHistogramEveryQWrites() const;

	// **** accessors for settings in ConfigManager (global to all cores) ****
	const string getSrch2Home() const ;
	const string getLicenseKeyFileName() const ;
	const string getHTTPServerAccessLogFile() const ;
	const Logger::LogLevel& getHTTPServerLogLevel() const;

	bool getIsFuzzyTermsQuery() const;

	float getDefaultSpatialQueryBoundingBox() const;

	unsigned int getKeywordPopularityThreshold() const;
	bool getQueryTermPrefixType() const;

	const unsigned getGetAllResultsNumberOfResultsThreshold() const;
	const unsigned getGetAllResultsNumberOfResultsToFindInEstimationMode() const;

	unsigned int getNumberOfThreads() const;


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

	void setSchema(srch2::instantsearch::Schema* schema) {
		this->schema = schema;
	};
	const srch2::instantsearch::Schema* getSchema() const {
		return this->schema;
	};

    const std::string* getRecordAclFile() const{
    	return &recordAclFilePath;
    }

    bool getHasRecordAcl() const{
    	return hasRecordAcl;
    }

//	// used for test.
//	CoreInfo_t(unsigned coreId, const string name, unsigned numberOfPartitions, unsigned replicaNumber){
//		schema = NULL;
//		configManager = NULL;
//		this->setCoreId(coreId);
//		this->name = name;
//		this->numberOfPrimaryShards = numberOfPartitions;
//		this->numberOfReplicas = replicaNumber;
//	}

    void setAttributeAclCoreId(unsigned attributeAclCoreId) {
    	this->attributeAclCoreId =  attributeAclCoreId;
    }

    unsigned getAttributeAclCoreId() const{
    	return this->attributeAclCoreId;
    }

    bool isAclCore() const{
    	return aclCoreFlag;
    }

protected:

//	vector<string> dataFilePaths;
	string name; // of core

	unsigned coreId; // starting from 0, auto increment
	// In V0, the "number_of_shards" is a one-time setting for a
	// core. In the future (possibly after V1), we can support dynamic
	// migration by allowing this number to change.
	unsigned numberOfPrimaryShards;
	// Number of replicas (additional copies) of an index (1 by
	// default). The "number_of_replicas" can be increased or
	// decreased anytime, by using the Index Update Settings API. We
	// can do it in V0 or after V1. SRCH2 will take care about load
	// balancing, relocating, gathering the results from nodes, etc.
	// ES: core.number_of_replicas: 1 // index.number_of_replicas: 1
	unsigned numberOfReplicas;

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

	// stores the value of maximum allowed retries when MongoDB listener encounters some problem.
	unsigned mongoListenerMaxRetryOnFailure;

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
	bool getRecordBoostFieldFlag() const{return recordBoostFieldFlag;}
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
	srch2::instantsearch::Schema *schema;

    // keep the access control info for this core
    AccessControlInfo* accessControlInfo;
    bool hasRecordAcl;
    string recordAclFilePath;
    unsigned attributeAclCoreId;
    bool aclCoreFlag;
};

}
}

#endif // __SHARDING_CONFIGURATION_COREINFO_H__
