
// $Id$

#ifndef __CONFIG_MANAGER__
#define __CONFIG_MANAGER__

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>

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

class Shard {
 public:
  Shard(SHARDSTATE newState, unsigned nodeId,
	unsigned coreId, bool isReplica = false, unsigned primaryId = -1);
  void setShardState(SHARDSTATE newState);
  void setOwnerNodeId(unsigned id);
  
  SHARDSTATE getShardState();
  unsigned getOwnerNodeId();
  bool isReplica();
  unsigned getCoreId();
  unsigned getPrimaryId();

 private:
  // TODO (for Surendra and Prateek): should we add core id to the shard name?

  // shard ids are strings with following naming conventions:
  //
  // A primary shard starts with a "P" followed by an integer id.
  // E.g., a cluster with 4 shards will have shards named "P0", "P1", "P2", and "P3".
  //
  // A replica shard starts with an "R" followed by a replica count and then its primary's id.
  // E.g., for the above cluster, replicas of "P0" will be named "R1_0" and "R2_0".
  // Similarly, replicas of "P3" will be named "R1_3" and "R2_3".

  std::string shardId;
  SHARDSTATE shardState;
  unsigned nodeId;
  unsigned coreId;
  bool replicaFlag; // a flag to indicate if it's a replica
  std::string primaryId;  // if current shard is a replica
};

enum NODESTATE {
  NODESTATE_GREEN,  // node is up and all shards are working
  NODESTATE_RED,    // node is down or all shards are not working
  NODESTATE_YELLOW  // node is up and all shards are not ready
};

class Node {
 public:
  Node(unsigned nodeId, std::string& ipAddress, unsigned portNumber);

  Shard getShardById(const std::string& shardId);
  void addShard(const Shard& shardId);
  void removeShard(const std::string& shardId);

  unsigned getTotalPrimaryShards(); // for all the cores on this node
  unsigned getTotalReplicaShards(); // for all the cores on this node

  bool thisIsMe; // temporary for V0

  // TODO (for Surendra): refine this iterator
  // const Node& operator = (const Node& node);

  // an iterator to go through the shards in this node
  //class ShardIterator {
  //public:
  //unsigned first; // TODO: Ask Surendra
  //Shard second;
  //bool operator == (NodeIterator* rhs);
  //};

  //typedef NodeIterator * Iterator;
  //Iterator begin();
  //Iterator next();
  //Iterator end();

 private:
  unsigned nodeId;
  std::string ipAddress;
  unsigned portNumber;

  // coreName -> shards mapping
  // movie -> <shard0, shard1, shard3>
  // customer -> <shard2, shard3>
  boost::unordered_map<std::string, std::vector<Shard> > coreToShardsMap;

  // Allow this node to be eligible as a master node (enabled by default).
  bool nodeMaster;

  // Allow this node to store data (enabled by default). If enabled, the node is eligible to store data shards.
  bool nodeData;

  // Home directory for all the index files of shards on this node.
  string homeDir;

  unsigned int numberOfThreads;

  // other node-related info
};

class CoreSchema {
   string primaryKey;

   string fieldLatitude;
   string fieldLongitude;

   int isPrimaryKeySearchable;

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
   unsigned numberOfPrimaryShards;

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

class Cluster {
  string       getClusterName();
  CLUSTERSTATE getClusterState();

  unsigned     getMasterNodeId();
  bool         isMasterNode(unsigned nodeId);

  void         getNodeById(unsigned id, Node& node);
  unsigned     getTotalNumberOfNodes();

  // get the node ID and coreId for a given shard Id
  void         getNodeIdAndCoreId(const string& shardId, unsigned& nodeId, unsigned& coreId);

 private:
  string       clusterName;
  CLUSTERSTATE clusterState;

  std::vector<Node> nodes;  // nodes in the cluster
  unsigned          masterNodeId;

  std::vector<Core> cores;  // cores in the cluster

  // "primary to replicas" map: from a primary shard id to ids of replica shards
  // e.g., primary shard "P0" has replicas "R1_0" and "R2_0"
  boost::unordered_map<std::string, std::vector<std::string> > primaryToReplicaMap;

  // friend class SynchronizationManager;
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
   
    void getCluster(Cluster& cluster);
    string licenseKeyFile;
 };
}


}

#endif // __CONFIG_MANAGER__
