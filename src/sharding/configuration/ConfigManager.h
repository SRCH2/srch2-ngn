
// $Id$

#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include "src/server/util/xmlParser/pugixml.hpp"

#include <instantsearch/Schema.h>
#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <sys/types.h>
#include <boost/unordered_map.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "src/core/util/Logger.h"

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;
namespace po = boost::program_options;
using namespace pugi;

namespace srch2 {
namespace httpwrapper {

//Adding portions of new header file, beginning from here
enum ShardState {
	SHARDSTATE_ALLOCATED,  // must have a valid node
	SHARDSTATE_UNALLOCATED,
	SHARDSTATE_MIGRATING,
	SHARDSTATE_INDEXING
};


class CoreShardInfo{
public:
	unsigned coreId;
	string coreName; // because currently you can get coreInfo_t from config only by passing the name ....
	CoreShardInfo(const unsigned id, const string& name)
	: coreId(id), coreName(name) {}
	CoreShardInfo();
};


// A shard id consists of a core id, a partition id, and replica id
// E.g.: a core with 7 partitions, each of which has a primary and 4 replicas
//
//   P0  R0_1 R0_2 R0_3 R0_4
//   P1  R1_1 R1_2 R1_3 R1_4
//   ...
//   P6  R6_1 R6_2 R6_3 R6_4
//
class ConfigManager;
class ShardId {
public:
	unsigned coreId;
	unsigned partitionId; // ID for a partition, numbered 0, 1, 2, ...

	// ID for a specific primary/replica for a partition; assume #0 is always the primary shard.  For V0, replicaId is always 0
	unsigned replicaId;

	bool isPrimaryShard() const;
	std::string toString() const;

	ShardId() ;
	ShardId(unsigned coreId, unsigned partitionId, unsigned replicaId=0) ;

	bool operator==(const ShardId& rhs) const ;
	bool operator!=(const ShardId& rhs) const ;
	bool operator>(const ShardId& rhs) const ;
	bool operator<(const ShardId& rhs) const ;
	bool operator>=(const ShardId& rhs) const ;
	bool operator<=(const ShardId& rhs) const ;

};

class ShardIdComparator {
public:
	// returns s1 > s2
	bool operator() (const ShardId s1, const ShardId s2) {
		if (s1.coreId > s2.coreId)
			return true;

		if (s1.coreId < s2.coreId)
			return false;

		// they have equal coreId; we look at their partitionId
		if (s1.partitionId > s2.partitionId)
			return true;

		if (s1.partitionId < s2.partitionId)
			return false;

		// they have equal partitionId; we look at their replicaId
		if (s1.replicaId > s2.replicaId)
			return true;

		return false;
	}
};


class Shard {
public:

	Shard(){
		this->nodeId = 0;
		this->shardState = SHARDSTATE_UNALLOCATED;
		this->shardId.coreId = 0;
		this->shardId.partitionId = 0;
		this->shardId.replicaId = 0;
	}

	Shard(unsigned nodeId, unsigned coreId, unsigned partitionId = 0,
			unsigned replicaId = 0) {
		this->nodeId = nodeId;
		this->shardState = SHARDSTATE_UNALLOCATED;
		this->shardId.coreId = coreId;
		this->shardId.partitionId = partitionId;
		this->shardId.replicaId = replicaId;
	}

	//Can be used in Migration
	void setPartitionId(int partitionId) {
		this->shardId.partitionId = partitionId;
	}

	//Can be used in Migration
	void setReplicaId(int replicaId) {
		this->shardId.replicaId = replicaId;
	}

	ShardId getShardId(){
		return this->shardId;
	}

	void setShardState(ShardState newState){
		this->shardState = newState;
	}

	void setNodeId(unsigned id){
		this->nodeId = id;
	}

	ShardState getShardState(){
		return this->shardState;
	}

	unsigned getNodeId(){
		return this->nodeId;
	}

private:
	ShardId shardId;
	ShardState shardState;
	unsigned nodeId;
};



// enum to allow loop iteration over listening ports
enum PortType_t {
	SearchPort,
	SuggestPort,
	InfoPort,
	DocsPort,
	UpdatePort,
	SavePort,
	ExportPort,
	ResetLoggerPort,
	EndOfPortType // stop value - not valid (also used to indicate all/default ports)
};

inline  enum PortType_t incrementPortType(PortType_t &oldValue)
{
	unsigned int newValue = static_cast<int> (oldValue);
	newValue++;
	return static_cast<PortType_t> (newValue);
}


class Node {
public:

	Node()
	{
		this->nodeId = 0;
		this->nodeName = "";
		this->ipAddress = "0.0.0.0";
		this->portNumber = 8087;
		this->nodeMaster = true;
		this->nodeData = true;
		this->dataDir = "";
		this->homeDir = "";
		this->numberOfThreads = 1;
		this->thisIsMe = false;
		//coreToShardsMap has to be initialized

	}

	Node(const std::string& nodeName, const std::string& ipAddress,
			unsigned portNumber, bool thisIsMe){
		this->nodeId = 0;
		this->nodeName = nodeName;
		this->ipAddress = ipAddress;
		this->portNumber = portNumber;
		this->thisIsMe = thisIsMe;
		this->nodeMaster = true;
		this->nodeData = true;
		this->dataDir = "";
		this->homeDir = "";
		this->numberOfThreads = 1;
	}

	Node(std::string& nodeName, std::string& ipAddress, unsigned portNumber,bool thisIsMe, bool nodeMaster, bool nodeData,std::string& dataDir, std::string& homeDir) {
		this->nodeId = 0;
		this->nodeName = nodeName;
		this->ipAddress = ipAddress;
		this->portNumber = portNumber;
		this->thisIsMe = thisIsMe;
		this->nodeMaster = nodeMaster;
		this->nodeData = nodeData;
		this->dataDir = dataDir;
		this->homeDir = homeDir;
		this->numberOfThreads = 1; // default value is 1

	}

	std::string getHomeDir() const {
		return this->homeDir;
	}
	std::string getDataDir() const {
		return this->dataDir;
	}

	bool isMaster() const {
		return nodeMaster;
	}

	bool isData() const {
		return nodeData;
	}

	std::string getName() const {
		return this->nodeName;
	}

	std::string getIpAddress() const {
		return this->ipAddress;
	}

	unsigned int getId() const {
		return this->nodeId;
	}

	void setId(unsigned nodeId){
		this->nodeId = nodeId;
	}

	unsigned int getPortNumber() const{
		return this->portNumber;
	}

	void setPortNumber(unsigned portNumber) {
		this->portNumber = portNumber;
	}


	Shard getShardById(const ShardId& shardId);
	void addShard(const ShardId& shardId);
	void removeShard(const ShardId& shardId);

	unsigned getTotalPrimaryShards(); // for all the cores on this node
	unsigned getTotalReplicaShards(); // for all the cores on this node

	bool thisIsMe; // temporary for V0

	unsigned short getPort(PortType_t portType) const;
	void setPort(PortType_t portType, unsigned short portNumber);

	string serialize() {
		stringstream ss;
		ss.write((const char *)&nodeId, sizeof(nodeId));
		unsigned size = nodeName.size();
		ss.write((const char *)&size, sizeof(unsigned));
		ss.write(nodeName.c_str(), nodeName.size());
		size = ipAddress.size();
		ss.write((const char *)&size, sizeof(unsigned));
		ss.write(ipAddress.c_str(), ipAddress.size());
		ss.write((const char *)&portNumber, sizeof(portNumber));
		ss.write((const char *)&thisIsMe, sizeof(thisIsMe));
		ss.write((const char *)&nodeData, sizeof(nodeData));
		ss.write((const char *)&nodeMaster, sizeof(nodeMaster));
		return ss.str();
	}

	void deserialize(char *serlializedNode) {

		char *buffer = serlializedNode;
		nodeId = *(unsigned *)buffer;
		buffer += sizeof(nodeId);

		unsigned size = *(unsigned *)buffer;
		buffer += sizeof(size);
		nodeName.assign(buffer, buffer + size);
		buffer += size;

		size = *(unsigned *)buffer;
		buffer += sizeof(size);
		ipAddress.assign(buffer, buffer + size);
		buffer += size;

		portNumber = *(unsigned *)buffer;
		buffer += sizeof(portNumber);

		thisIsMe = *(bool *)buffer;
		buffer += sizeof(thisIsMe);

		nodeData = *(bool *)buffer;
		buffer += sizeof(nodeData);

		nodeMaster = *(bool *)buffer;
		buffer += sizeof(nodeMaster);
	}

private:
	unsigned nodeId;
	std::string ipAddress;
	unsigned portNumber;
	std::string nodeName;
	vector<unsigned short> ports;

	// Allow this node to be eligible as a master node (enabled by default).
	bool nodeMaster;

	// Allow this node to store data (enabled by default). If enabled, the node is eligible to store data shards.
	bool nodeData;

	// Home directory for all the index files of shards on this node.
	string homeDir;

	string dataDir;
	unsigned int numberOfThreads;
	// other node-related info
};

enum CLUSTERSTATE {
	CLUSTERSTATE_GREEN,  // all nodes are green
	CLUSTERSTATE_RED,    // all nodes are red ..possible ?
	CLUSTERSTATE_YELLOW  // not all nodes are green.
};

class CoreInfo_t;

class Cluster {
public:

	const Node* getCurrentNode(){
		vector<Node>& nodes =  (this->nodes);
		for(int i = 0; i < nodes.size(); i++){
			if(nodes[i].thisIsMe == true){
				return &nodes[i];
			}
		}
		return NULL; // should not happen
	}

	std::map<ShardId, Shard, ShardIdComparator> shardMap;

	std::vector<Node>* getNodes(){
		return &nodes;
	}

	string getClusterName() {
		return this->clusterName;
	}

	CLUSTERSTATE getClusterState();

	unsigned     getMasterNodeId();
	bool         isMasterNode(unsigned nodeId);

	void         getNodeById(unsigned id, Node& node);
	unsigned     getTotalNumberOfNodes() { return nodes.size(); };

	void setClusterName(const std::string& clusterName)
	{
		this->clusterName = clusterName;
	}

	// get the node ID and coreId for a given shard Id
	void getNodeIdAndCoreId(const ShardId& shardId, unsigned& nodeId, unsigned& coreId);

private:
	string       clusterName;
	CLUSTERSTATE clusterState;

	std::vector<Node> nodes;  // nodes in the cluster
	unsigned          masterNodeId;

	std::vector<CoreInfo_t> cores;  // cores in the cluster

	// "primary to replicas" map: from a primary shard id to ids of replica shards
	// e.g., primary shard "P0" has replicas "R1_0" and "R2_0"
	boost::unordered_map<std::string, std::vector<std::string> > primaryToReplicaMap;

	// friend class SynchronizationManager;
};

// This class is used to collect information from the config file and pass them other modules
// in the system.
class SearchableAttributeInfoContainer {
public:
	SearchableAttributeInfoContainer(){
		attributeName = "";
		required = false;
		defaultValue = "";
		offset = 0;
		boost = 1;
		isMultiValued = false;
		highlight = false;
	}
	SearchableAttributeInfoContainer(const string & name,
			const bool required,
			const string & defaultValue ,
			const unsigned offset,
			const unsigned boost,
			const bool isMultiValued,
			bool highlight = false){
		this->attributeName = name;
		this->required = required;
		this->defaultValue = defaultValue;
		this->offset = offset;
		this->boost = boost;
		this->isMultiValued = isMultiValued;
		this->highlight = highlight;
	}
	// NO GETTER OR SETTERS ARE IMPLEMENTED FOR THESE MEMBERS
	// BECAUSE THIS CLASS IS MEANT TO BE A VERY SIMPLE CONTAINER WHICH ONLY CONTAINS THE
	// VALUES AND HAS NO BEHAVIOUR
	string attributeName;
	bool required;
	string defaultValue;
	unsigned offset;
	unsigned boost;
	bool isMultiValued;
	bool highlight;
};

class RefiningAttributeInfoContainer {
public:
	RefiningAttributeInfoContainer(){
		attributeName = "";
		// JUST BECAUSE IT MUST HAVE A DEFAULT VALUE, TEXT has no meaning or value here
		attributeType = srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
		defaultValue = "";
		required = false;
		isMultiValued = false;
	}
	RefiningAttributeInfoContainer(const string & name,
			srch2::instantsearch::FilterType type,
			const string & defaultValue,
			const bool required,
			const bool isMultiValued){
		this->attributeName = name;
		this->attributeType = type;
		this->defaultValue = defaultValue;
		this->required = required;
		this->isMultiValued = isMultiValued;
	}
	// NO GETTER OR SETTERS ARE IMPLEMENTED FOR THESE MEMBERS
	// BECAUSE THIS CLASS IS MEANT TO BE A VERY SIMPLE CONTAINER WHICH ONLY CONTAINS THE
	// VALUES AND HAS NO BEHAVIOUR
	string attributeName;
	srch2::instantsearch::FilterType attributeType;
	string defaultValue;
	bool required;
	bool isMultiValued;
};


// helper state between different sections of the config file
struct CoreConfigParseState_t {
	bool hasLatitude;
	bool hasLongitude;
	vector<string> searchableFieldsVector;
	vector<string> searchableFieldTypesVector;
	vector<bool> searchableAttributesRequiredFlagVector;
	vector<string> searchableAttributesDefaultVector;
	vector<bool> searchableAttributesIsMultiValued;
	vector<bool> highlight;

	CoreConfigParseState_t() : hasLatitude(false), hasLongitude(false) {};
};

class Ping {
private:
	unsigned pingInterval;
	unsigned pingTimeout;
	unsigned retryCount;

public:
	Ping(){
		pingInterval = 1;
		pingTimeout = 1;
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

	string createSRCH2Home();
	string createClusterDir(const string& clusterName);
	string createNodeDir(const string& clusterName, const string& nodeName);
	string createCoreDir(const string& clusterName, const string& nodeName, const string& coreName);
	string createShardDir(const string& clusterName, const string& nodeName, const string& coreName, const ShardId& shardId);

	string getSRCH2HomeDir();
	string getClusterDir(const string& clusterName);
	string getNodeDir(const string& clusterName, const string& nodeName);
	string getCoreDir(const string& clusterName, const string& nodeName, const string& coreName);
	string getShardDir(const string& clusterName, const string& nodeName, const string& coreName, const ShardId& shardId);

	//It returns the number of files/directory deleted, if the returned value is 0, that means nothing got deleted.
	uint removeDir(const string& path);

	Ping& getPing(){
		return this->ping;
	}

	MulticastDiscovery& getMulticastDiscovery(){
		return this->mDiscovery;
	}

	Transport& getTransport(){
		return this->transport;
	}

	typedef std::map<const string, CoreInfo_t *> CoreInfoMap_t;
	Cluster* getCluster(){  // not safe
		return &(this->cluster);
	}

	bool isInCurrentNode(ShardId & shardId){
		// get destination node ID from config manager
		unsigned nodeId =
				this->getCluster()->shardMap[shardId].getNodeId();

		return (nodeId == this->getCurrentNodeId());
	}

	unsigned getNodeId(ShardId & shardId){
		return this->getCluster()->shardMap[shardId].getNodeId();
	}

	void removeNodeFromCluster(unsigned nodeId) {
		//spin to acquire lock
		while (!__sync_bool_compare_and_swap (&isLocked, false, true)) ;

		vector<Node>* nodes = this->cluster.getNodes();
		unsigned index = 0;
		for(; index < nodes->size(); ++index){
			if((*nodes)[index].getId() == nodeId){
				break;
			}
		}
		if (index < nodes->size())
			nodes->erase(nodes->begin() + index);

		isLocked = false;
	}

	void addNewNode(const Node& node) {
		//spin to acquire lock
		while (!__sync_bool_compare_and_swap (&isLocked, false, true)) ;

		// first check whether node is already present or not.
		vector<Node>* nodes = this->cluster.getNodes();
		unsigned index = 0;
		unsigned totalNodes = nodes->size();
		for(; index < totalNodes; ++index){
			if((*nodes)[index].getId() == node.getId()){
				break;
			}
		}
		if (index == totalNodes) {
			nodes->push_back(node);
		}
		isLocked = false;
		return;
	}

	string serializeClusterNodes() {
		while (!__sync_bool_compare_and_swap (&isLocked, false, true)) ;
		stringstream ss;
		vector<Node>* nodes = this->cluster.getNodes();
		unsigned size = nodes->size();
		ss.write((const char *)&size, sizeof(size));
		for(unsigned i = 0; i < nodes->size(); ++i){
			string serializedNode = nodes->operator[](i).serialize();
			size = serializedNode.size();
			ss.write((const char *)&size, sizeof(size));
			ss.write(serializedNode.c_str(), size);
		}
		isLocked = false;
		return ss.str();
	}

	bool isValidNode(unsigned nodeId) {
		//spin to acquire lock
		while (!__sync_bool_compare_and_swap (&isLocked, false, true)) ;

		vector<Node>* nodes = this->cluster.getNodes();
		unsigned index = 0;
		unsigned totalNodes = nodes->size();
		for(; index < totalNodes; ++index){
			if((*nodes)[index].getId() == nodeId){
				break;
			}
		}
		isLocked = false;
		if (index < totalNodes)
			return true;
		else
			return false;
	}
	unsigned getCurrentNodeId(){
		//spin to acquire lock
		while (!__sync_bool_compare_and_swap (&isLocked, false, true)) ;

		vector<Node>* nodes = this->cluster.getNodes();
		for(int i = 0; i < nodes->size(); i++){
			if(nodes->at(i).thisIsMe == true){
				isLocked = false;
				return nodes->at(i).getId();
			}
		}
		isLocked = false;
		return 0;
	}
private:
	volatile bool isLocked; //both read / write use this lock.
	Cluster cluster;
	Ping ping;
	MulticastDiscovery mDiscovery;
	Transport transport;
	// <config>
	string licenseKeyFile;
	string httpServerListeningHostname;
	string httpServerListeningPort;
	string srch2Home;
	unsigned int numberOfThreads;

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
	CoreInfoMap_t coreInfoMap;

	// <config><cores>
	string defaultCoreName;
	bool defaultCoreSetFlag; // false unless <cores defaultCoreName="..."> has been parsed

	// parsing helper functions for modularity
	void parseIndexConfig(const xml_node &indexConfigNode, CoreInfo_t *coreInfo, map<string, unsigned> &boostsMap, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseMongoDb(const xml_node &mongoDbNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseQuery(const xml_node &queryNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseSingleCore(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseMultipleCores(const xml_node &coresNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	// parse all data source settings (can handle multiple cores or default/no core)
	void parseDataConfiguration(const xml_node &configNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	// parse all settings for a single data source, either under <config> or within a <core>
	void parseDataFieldSettings(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseSchema(const xml_node &schemaNode, CoreConfigParseState_t *coreParseState, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

	void parseUpdateHandler(const xml_node &updateHandlerNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings);

public:
	ConfigManager(const string& configfile);
	virtual ~ConfigManager();
	bool verifyConsistency();
	void setNodeId();
	bool isLocal(ShardId& shardId);

	//Declaring function to parse node tags
	void parseNode(std::vector<Node>* nodes, xml_node& childNode, std::stringstream &parseWarnings, std::stringstream &parseError, bool configSuccess);

	const CoreInfoMap_t& getCoreInfoMap() const;
	CoreInfoMap_t::iterator coreInfoIterateBegin() { return coreInfoMap.begin(); }
	CoreInfoMap_t::iterator coreInfoIterateEnd() { return coreInfoMap.end(); }
	CoreInfo_t *getCoreInfo(const string &coreName) const { return ((CoreInfoMap_t) coreInfoMap)[coreName]; }

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

	void loadConfigFile() ;

	// Mongo related getter/setter
	const string& getMongoServerHost(const string &coreName) const;
	const string& getMongoServerPort(const string &coreName) const;
	const string& getMongoDbName(const string &coreName) const;
	const string& getMongoCollection (const string &coreName) const;
	const unsigned getMongoListenerWaitTime (const string &coreName) const;
	const unsigned getMongoListenerMaxRetryCount(const string &coreName) const;

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

	static const char* const nodeListeningHostNameTag;
	static const char* const nodeListeningPortTag;
	static const char* const nodeCurrentTag;
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
	static const char* const coreIdTag;
	static const char* const accessLogFileString;
	static const char* const analyzerString;
	static const char* const cacheSizeString;
	static const char* const collectionString;
	static const char* const configString;
	static const char* const dataDirString;
	static const char* const dataFileString;
	static const char* const dataSourceTypeString;
	static const char* const dbString;
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
	static const char* const hostString;
	static const char* const indexConfigString;
	static const char* const indexedString;
	static const char* const multiValuedString;
	static const char* const indexTypeString;
	static const char* const licenseFileString;
	static const char* const listenerWaitTimeString;
	static const char* const listeningHostStringString;
	static const char* const listeningPortString;
	static const char* const locationLatitudeString;
	static const char* const locationLongitudeString;
	static const char* const logLevelString;
	static const char* const maxDocsString;
	static const char* const maxMemoryString;
	static const char* const maxRetryOnFailureString;
	static const char* const maxSearchThreadsString;
	static const char* const mergeEveryMWritesString;
	static const char* const mergeEveryNSecondsString;
	static const char* const mergePolicyString;
	static const char* const mongoDbString;
	static const char* const nameString;
	static const char* const portString;
	static const char* const porterStemFilterString;
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

	static const char* const defaultFuzzyPreTag;
	static const char* const defaultFuzzyPostTag;
	static const char* const defaultExactPreTag;
	static const char* const defaultExactPostTag;

};

// definitions for data source(s) (srch2Server objects within one HTTP server)
class CoreInfo_t {

public:

	vector<ShardId> getShardsVector() const{
		return shards;
	}


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

	ShardId getPrimaryShardId(unsigned partitionId) const{
		ShardId rtn ;
		rtn.coreId = this->coreId;
		rtn.partitionId = partitionId;
		rtn.replicaId = 0;
		return rtn;
	}

	void getPartitionAllShardIds(unsigned partitionId, vector<ShardId> & shardIds) const{ // fills shardIds vector by ShardId objects of primary and replica partitions corresponding to partitionId
		for(int i = 0; i < this->shards.size(); i++){
			if(this->shards[i].partitionId == partitionId){
				shardIds.push_back(this->shards[i]);
			}
		}
	}

	CoreInfo_t(class ConfigManager *manager) : configManager(manager) {
		schema = NULL;
	};
	~CoreInfo_t() {
		if(schema != NULL){
			delete schema;
		}
	};
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

	const string &getMongoServerHost() const { return mongoHost; }
	const string &getMongoServerPort() const { return mongoPort; }
	const string &getMongoDbName() const { return mongoDbName; }
	const string &getMongoCollection() const { return mongoCollection; }
	unsigned getMongoListenerWaitTime() const { return mongoListenerWaitTime; }
	unsigned getMongoListenerMaxRetryOnFailure() const { return mongoListenerMaxRetryOnFailure; }
	unsigned getMongoListenerMaxRetryCount() const { return mongoListenerMaxRetryOnFailure; }

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

	void setSchema(srch2is::Schema* schema) {
		this->schema = schema;
	};
	srch2is::Schema* getSchema() const {
		return this->schema;
	};

	vector<ShardId> shards;
protected:

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
	unsigned numberOfReplicas; // always 0 for V0

	ConfigManager *configManager;

	// <config>
	string dataDir;
	string indexPath; // srch2Home + dataDir
	DataSourceType dataSourceType;
	string dataFile;
	string dataFilePath;

	// mongo db related settings
	string mongoHost;
	string mongoPort;
	string mongoDbName;
	string mongoCollection;
	unsigned mongoListenerWaitTime;

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

	srch2is::Schema *schema;
};



}
}

#endif // __CONFIGMANAGER_H__
