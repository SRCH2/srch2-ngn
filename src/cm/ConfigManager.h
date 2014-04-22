
// $Id$

#ifndef __CONFIG_MANAGER__
#define __CONFIG_MANAGER__

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
//#include <boost/unordered_set.hpp>

using std::vector;
using std::string;

namespace srch2 {
namespace instantsearch {

  enum SHARDSTATE {
    SHARDSTATE_ALLOCATED,  // must have a valid node
    SHARDSTATE_UNALLOCATED,
    SHARDSTATE_MIGRATING,
    SHARDSTATE_INDEXING
  };

//
//class Shard {
//public:
//	void addRecord(Record rec);
//	void updateRecord(unsigned id, Record rec);
//	void deleteRecord(unsigned id);
//	void getInMemoryData(unsigned id);
//private:
//	Index* index;  // current srch2's index class.
//};

class ShardMetaData{
 public:
  ShardMetaData(SHARDSTATE newState, unsigned nodeId,
		unsigned coreId,bool isReplica = false, unsigned primaryId = -1);
  void setShardState(SHARDSTATE newState);
  void setOwnerNodeId(unsigned id);
  
  SHARDSTATE getShardState();
  unsigned getOwnerNodeId();
  bool isReplica();
  unsigned getCoreId();
  unsigned getPrimaryId();

private:
  // shard ids are strings with following naming conventions:
  //
  // A primary shard starts with a "P" followed by an integer id.
  // E.g., a cluster with 4 shards will have shards named "P0", "P1", "P2", and "P3".
  //
  // A replica shard starts with an "R" followed by a replica count and then its primary's id.
  // E.g., for the above cluster, replicas of "P0" will be named "R1_0" and "R2_0".
  // Similarly, replicas of "P3" will be named "R1_3" and "R2_3".
  //
  // This replica naming convention helps in debugging and understanding shard distribution.
  std::string currentShardId;
  SHARDSTATE ShardState;
  unsigned nodeId;
  unsigned coreId;
  bool replicaFlag; // a flag to indicate if it's a replica
  std::string primaryId;  // if current shard is replica
};


enum NODESTATE {
  NODESTATE_GREEN,  // node is up and all shards are working
  NODESTATE_RED,    // node is down or all shards are not working
  NODESTATE_YELLOW  // node is up and all shards are not ready
};


class Node {
public:
	Node(unsigned nodeId, unsigned ipAddress, unsigned Port);

	ShardMetaData getShardInfoById(const std::string& shardId);
	unsigned getTotalPrimaryShards();
	unsigned getTotalReplicaShards();

	const Node& operator = (const Node& node);
	void addShardInfo(const ShardMetaData& shard);
	void removeShard(const std::string& shardId);


	class NodeIterator {
	public:
		unsigned first;
		ShardMetaData second;
		bool operator == (NodeIterator* rhs);
	};
	typedef NodeIterator * Iterator;
	Iterator begin();
	Iterator next();
	Iterator end();

    bool thisIsMe; // temporary for V0

private:
	unsigned nodeId;
	std::string ipAddress;
	unsigned port;
	unsigned numberOfPrimaryShards;
	unsigned numberofReplicaShards;
	boost::unordered_map<std::string, ShardMetaData> shards;
	// "primary to replicas" map
	// e.g., primary "P0" has replicas "R1_0" and "R2_0"
	boost::unordered_map<std::string, std::vector<std::string> > primaryToReplicaMap;

	 // Allow this node to be eligible as a master node (enabled by default).
    bool nodeMaster;
    // Allow this node to store data (enabled by default). If enabled, the node is eligible to store data shards.
    bool nodeData;
    // Home directory for all the index files of shards on this node.
    // We don't want the user to specify them for a particular core since
    // different nodes can have different environments, and shards
    // can move to different nodes. We
    string dataPathHome;
    unsigned int numberOfThreads;
    // other node-related info
};

class CoreSchema {
   string primaryKey;

   string fieldLatitude;
   string fieldLongitude;

   int isPrimSearchable;

   // characters to specially treat as part of words, and not as a delimiter
   std::string allowedRecordTokenizerCharacters;

   vector<std::pair<unsigned, string> > highlightAttributes;
   string exactHighlightMarkerPre;
   string exactHighlightMarkerPost;
   string fuzzyHighlightMarkerPre;
   string fuzzyHighlightMarkerPost;
   unsigned highlightSnippetLen;
   // array of local HTTP ports (if any) index by port type enum
   // vector<unsigned short> ports;
};

class CoreIndex {
   bool supportSwapInEditDistance;

   bool enableWordPositionIndex;
   bool enableCharOffsetIndex;

   bool recordBoostFieldFlag;
   string recordBoostField;
   unsigned queryTermBoost;
   //IndexCreateOrLoad indexCreateOrLoad;
   unsigned indexCreateOrLoad;

   // <schema><types><fieldType><analyzer><filter>
   bool stemmerFlag;
   std::string stemmerFile;
   std::string synonymFilterFilePath;
   bool synonymKeepOrigFlag;
   std::string stopFilterFilePath;
   std::string protectedWordsFilePath;

};

class CoreQuery {
  // srch2::instantsearch::ResponseType searchResponseContent;
  int searchResponseJsonFormat;
  vector<string> attributesToReturn;

  bool supportAttributeBasedSearch;

  // facet
  bool facetEnabled;
  vector<int> facetTypes; // 0 : simple , 1 : range
  vector<string> facetAttributes;
  vector<string> facetStarts;
  vector<string> facetEnds;
  vector<string> facetGaps;

  string dataDir;
  string indexPath; // srch2Home + dataDir

  //DataSourceType dataSourceType;
  string dataFile;
  string dataFilePath;

  int indexType;

   //map<string , SearchableAttributeInfoContainer> searchableAttributesInfo;
   //map<string , RefiningAttributeInfoContainer > refiningAttributesInfo;

   int searchType;

   // <config><query><rankingAlgorithm>
   string scoringExpressionString;

   // <config><query>
   float fuzzyMatchPenalty;
   float queryTermSimilarityThreshold;
   float queryTermLengthBoost;
   float prefixMatchPenalty;

   vector<string> sortableAttributes;
   //vector<srch2::instantsearch::FilterType> sortableAttributesType; // Float or unsigned
   vector<string> sortableAttributesDefaultValue;
   int attributeToSort;
   unsigned cacheSizeInBytes;
   int resultsToRetrieve;
   bool exactFuzzy;
   bool queryTermPrefixType;

   unsigned defaultNumberOfSuggestions;
};

class CoreUpdateHandler {
  uint64_t memoryLimit;
  uint32_t documentLimit;

  // <config><updatehandler><mergePolicy>
  unsigned mergeEveryNSeconds;
  unsigned mergeEveryMWrites;

  // no config option for this yet
  unsigned updateHistogramEveryPMerges;
  unsigned updateHistogramEveryQWrites;
};

class CoreMongoDB {
  string mongoHost;
  string mongoPort;
  string mongoDbName;
  string mongoCollection;
  unsigned mongoListenerWaitTime;

  // stores the value of maximum allowed retries when MongoDB listener encounters some problem.
  unsigned mongoListenerMaxRetryOnFailure;
};

class Core {
   string coreName;

   // In V0, the "number_of_shards" is a one-time setting for a
   // core. In the future (possibly after V1), we can support dynamic
   // migration by allowing this number to change.
   unsigned numberOfShards;

   // Number of replicas (additional copies) of an index (1 by
   // default). The "number_of_replicas" can be increased or
   // decreased anytime, by using the Index Update Settings API. We
   // can do it in V0 or after V1. SRCH2 will take care about load
   // balancing, relocating, gathering the results from nodes, etc.
   // ES: core.number_of_replicas: 1 // index.number_of_replicas: 1
   unsigned numberOfReplicas; // always 0 for V0

   CoreSchema coreSchema;

   CoreIndex coreIndex;

   CoreQuery coreQuery;

   CoreUpdateHandler coreUpdateHandler;

   CoreMongoDB coreMongoDB;
};

enum CLUSTERSTATE {
	CLUSTERSTATE_GREEN,  // all nodes are green
	CLUSTERSTATE_RED,    // all nodes are red ..possible ?
	CLUSTERSTATE_YELLOW  // not all nodes are green.
};

class Cluster{
	bool isMasterNode(unsigned nodeId);
	unsigned getMasterNode();
	CLUSTERSTATE getClusterState();
	unsigned getTotalPrimaryShards();
	unsigned getTotalReplicaShards();
	Node getNodeById(unsigned id);
	unsigned getTotalNodes();
	unsigned getTotalShards();

private:
     string clusterName;

     std::vector<Node> nodes;
     std::vector<Core> cores;

	// predetermined by the config file. cannot be changed in V1 (no dynamic primaries yet)
	unsigned numberOfPrimaryShards;
	// predetermined by the config file. can be changed
	unsigned numberOfReplicaShards;
	unsigned totalShards;  // = numberOfPrimaryShards * (1 + numberOfReplicaShards)
	unsigned masterNodeId;
	CLUSTERSTATE clusterState;
	friend class SynchronizationManager;
};



//class SynchronizerManager {
//public:
//	SynchronizerManager(const ConfigurationManager& cm);
//	virtual ~SynchronizerManager();
//	// should be called in new thread otherwise current thread will block
//	void run(bool isMaster);
//private:
//	void checkHeartBeatofAllNodesInCluster(const Cluster& cluster);
//	void updateMetdata();
//	void listenForHeartBeatRequest();
//	void gatherAndSendCurrentNodeState();
//	void handleNodeFailures();
//	void handleMasterFailure();
//};
//
//class DiscoveryManager {
//};
//

 class ConfigManager {
   // load a configuration and populate a Cluster object
   ConfigManager(const string& configfile);
   
    void getClusterInfo(Cluster& cluster);
    string licenseKeyFile;
 };


}
 
}

#endif // __CONFIG_MANAGER__
