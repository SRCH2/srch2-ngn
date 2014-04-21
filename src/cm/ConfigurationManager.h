
// $Id$

#ifndef __CONFIGURATION_MANAGER__
#define __CONFIGURATION_MANAGER__

#include <string>
#include <vector>

using std::vector;
using std::string;

namespace srch2 {

 class NodeInfo {
   string nodeName;

   string hostName;
   string listeningPort; // TODO: change it to PortType
   bool thisIsMe; // temporary for V0

   bool nodeNaster; // Allow this node to be eligible as a master node (enabled by default).
   
   bool nodeData; // Allow this node to store data (enabled by default). If enabled, the node is eligible to store data shards. 

   // An index path should be node specific, not core specific. 
   // It’s the home directory for all the index files of shards on this node. 
   // We don’t want the user to specify them for a particular core since 
   // different nodes can have different environments, and shards 
   // can move to different nodes. It can be a relative path with respect to “srch2Home” or 
   // an absolute path. If the indexes have been serialized, we should 
   // be able to read the files into memory. TODO: decide which module should do it.
   string srch2Home;
   string dataDir;

   unsigned int numberOfThreads;

   // other node-related info
 };


 class CoreSchemaInfo {
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

 class CoreIndexInfo {
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

 class CoreQueryInfo {
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

 class CoreUpdateHandlerInfo {
   uint64_t memoryLimit;
   uint32_t documentLimit;

   // <config><updatehandler><mergePolicy>
   unsigned mergeEveryNSeconds;
   unsigned mergeEveryMWrites;

   // no config option for this yet
   unsigned updateHistogramEveryPMerges;
   unsigned updateHistogramEveryQWrites;
 };
 
 class CoreMongoDBInfo {
   string mongoHost;
   string mongoPort;
   string mongoDbName;
   string mongoCollection;
   unsigned mongoListenerWaitTime;

   // stores the value of maximum allowed retries when MongoDB listener encounters some problem.
   unsigned mongoListenerMaxRetryOnFailure;
 };

 class CoreInfo {
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

    CoreSchemaInfo coreSchemaInfo;
    
    CoreIndexInfo coreIndexInfo;
    
    CoreQueryInfo coreQueryInfo;
    
    CoreUpdateHandlerInfo coreUpdateHandlerInfo;

    CoreMongoDBInfo coreMongoDBInfo;
 };

 class ClusterInfo {
   string clusterName;
   std::vector<NodeInfo> nodes;
   std::vector<CoreInfo> cores;
 };
 
 class ConfigurationManager {
   // load a configuration and populate a ClusterInfo object
   ConfigurationManager(const string& configfile);
   
    void getClusterInfo(ClusterInfo& clusterInfo);
    string licenseKeyFile;
 };

}

#endif // __CONFIGURATION_MANAGER__
