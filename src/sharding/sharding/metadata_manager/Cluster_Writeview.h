#ifndef __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__
#define __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__


#include "Shard.h"
#include "Cluster.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../configuration/CoreInfo.h"
#include "../../configuration/ShardingConstants.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class Srch2Server;

struct LocalPhysicalShard{
	boost::shared_ptr<Srch2Server> server;
	string indexDirectory;
	string jsonFileCompletePath; // if any
	LocalPhysicalShard();
	LocalPhysicalShard(boost::shared_ptr<Srch2Server> server, const string & indexDirectory, const string & jsonFileCompletePath);
	LocalPhysicalShard(const LocalPhysicalShard & copy);
	LocalPhysicalShard & operator=(const LocalPhysicalShard & rhs);

//	void setServer(boost::shared_ptr<Srch2Server> server);

	bool operator==(const LocalPhysicalShard & right) const{
		return (indexDirectory.compare(right.indexDirectory) == 0) &&
				(jsonFileCompletePath.compare(right.jsonFileCompletePath) == 0);
	}

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
};

struct ClusterShard_Writeview{

	ClusterShard_Writeview(	const ClusterShardId id, const ShardState & state,
			const NodeId nodeId, const bool isLocal, const double load);
	ClusterShard_Writeview(const ClusterShard_Writeview & copy);
	ClusterShard_Writeview();

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);


	bool operator==(const ClusterShard_Writeview & right){
		return (id == right.id) && (state == right.state) &&
				(isLocal == right.isLocal) && (nodeId == right.nodeId) && (load == right.load);
	}
	ClusterShardId id;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	double load;
};
struct NodeShard_Writeview{
	NodeShard_Writeview(NodeShardId id, bool isLocal, double load){
		this->id = id;
		this->isLocal = isLocal;
		this->load = load;
	}
	NodeShard_Writeview(const NodeShard_Writeview & copy){
		this->id = copy.id;
		this->isLocal = copy.isLocal;
		this->load = copy.load;
	}
	NodeShard_Writeview(){};

	void * serialize(void * buffer) const{
		buffer = id.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(isLocal, buffer);
		buffer = srch2::util::serializeFixedTypes(load, buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes= 0 ;
		numberOfBytes += id.getNumberOfBytes();
		numberOfBytes += sizeof(isLocal);
		numberOfBytes += sizeof(load);
		return numberOfBytes;
	}
	void * deserialize(void * buffer){
		buffer = id.deserialize(buffer);
		buffer = srch2::util::deserializeFixedTypes(buffer, isLocal);
		buffer = srch2::util::deserializeFixedTypes(buffer, load);
		return buffer;
	}

	bool operator==(const NodeShard_Writeview & right){
		return (id == right.id) &&
				(isLocal == right.isLocal) && (load == right.load);
	}


	NodeShardId id;
	bool isLocal;
	double load;
};

class Cluster_Writeview;
class ClusterShardIterator{
public:
    ClusterShardIterator(const Cluster_Writeview * writeview){
        this->clusterShardsCursor = 0;
        ASSERT(writeview != NULL);
        this->writeview = writeview;
    }
    unsigned clusterShardsCursor;
    const Cluster_Writeview * writeview;
    void beginClusterShardsIteration();
    bool getNextClusterShard(ClusterShardId & shardId, double & load, ShardState & state, bool & isLocal, NodeId & nodeId);
    bool getNextLocalClusterShard(ClusterShardId & shardId, double & load,  LocalPhysicalShard & localPhysicalShard );
};

class NodeShardIterator{
public:
    NodeShardIterator(const Cluster_Writeview * writeview){
        this->nodeShardsCursor = 0;
        ASSERT(writeview != NULL);
        this->writeview = writeview;
    }
    unsigned nodeShardsCursor;
    const Cluster_Writeview * writeview;
    void beginNodeShardsIteration() ;
    bool getNextNodeShard(NodeShardId & nodeShardId, bool & isLocal) ;
    bool getNextLocalNodeShard(NodeShardId & nodeShardId, double & load,  LocalPhysicalShard & dataInfo) ;
};

struct DataShardBackup{
	LocalPhysicalShard localPhysicalShard;
	ClusterShardId shardId;
	NodeId newNodeLocation;
};
class ClusterNodes_Writeview;
class Cluster_Writeview{
    friend class ClusterShardIterator;
    friend class NodeShardIterator;
    friend class ClusterNodes_Writeview;
    friend class ResourceMetadataManager;
public:
	string clusterName;
	NodeId currentNodeId;
	unsigned versionId;

	// Contains an entry for each one of those cluster shards that exist on this node
	// with server information.
	// ShardId on this node => server info
	map<ClusterShardId, LocalPhysicalShard > localClusterDataShards;
	vector<DataShardBackup *> localClusterDataShardBackups;

	// Server information for those independent shards that are on the current node
	// NodeShardId{currentNodeId, coreId, int-partitionId} => server information
	// internal partition id
	map<unsigned,  LocalPhysicalShard > localNodeDataShards;


	//////////////////// Runtime information which should be serialized /////////////////////////
	map<unsigned, CoreInfo_t *> cores;

	Cluster_Writeview(unsigned versionId, string clusterName, vector<CoreInfo_t *> cores);
	Cluster_Writeview(const Cluster_Writeview & copy);
	Cluster_Writeview();
	bool operator==(const Cluster_Writeview & right);
	bool isEqualDiscardingLocalShards(const Cluster_Writeview & right);
	void print() const;

	void printCores() const;
	void printNodes() const;
	void printClusterShards() const;
	void printNodeShards() const;
	void printLocalShards() const;

	~Cluster_Writeview();

	void assignLocalClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShardInfo);
	void assignExternalClusterShard(const ClusterShardId & shardId, const NodeId & nodeId, const double & load);
	void unassignClusterShard(const ClusterShardId & shardId);
	void setClusterShardServer(const ClusterShardId & shardId, boost::shared_ptr<Srch2Server> server);
	void moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId);
	void moveClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShard);

	void addLocalNodeShard(const NodeShardId & nodeShardId, const double load, const LocalPhysicalShard & physicalShardInfo);
	void addExternalNodeShard(const NodeShardId & nodeShardId, const double load);
	void removeNodeShard(const NodeShardId & nodeShardId);
	void setNodeShardServer(const NodeShardId & nodeShardId, boost::shared_ptr<Srch2Server> server);
	void fixAfterDiskLoad(Cluster_Writeview * newWrireview);
	void fixClusterMetadataOfAnotherNode(Cluster_Writeview * cluster) const;
	ClusterResourceMetadata_Readview * getNewReadview() const;

	void getPatitionInvolvedNodes(const ClusterShardId & shardId, vector<NodeId> & participants) const;
	// these partitions don't have write access yet.
	void getFullUnassignedPartitions(vector<ClusterPID> & fullUnassignedPartitions ) const;
	// returns true if this partition has least one pending shard
	bool isPartitionPending(const ClusterPID & pid) const;




	double getLocalNodeTotalLoad() const{
		return localClusterDataShards.size()+localNodeDataShards.size();
	}

	// Serialization methods
	void saveWriteviewOnDisk(const string & absDirectoryPath) const;
	static Cluster_Writeview * loadWriteviewFromDisk(const string & absDirectoryPath);
	void * serialize(void * buffer, bool includeLocalInfoFlag = true) const;
	unsigned getNumberOfBytes(bool includeLocalInfoFlag = true) const;
	void * deserialize(void * buffer, bool includeLocalInfoFlag = true);

//	vector<ClusterShard_Writeview *> & getClusterShards(){
//		return clusterShards;
//	}
//
//	vector<NodeShard_Writeview *> & getNodeShards(){
//		return nodeShards;
//	}
private:
	unsigned clusterShardsCursor;
	vector<ClusterShard_Writeview *> clusterShards;
	map<ClusterShardId, unsigned> clusterShardIdIndexes;
	vector<NodeShard_Writeview *> nodeShards;

	map<NodeId, std::pair<ShardingNodeState, Node *> > nodes;
	// Does not return this node by default
	map<NodeId, std::pair<ShardingNodeState, Node *> > & getNodes() const{
		return nodes;
	}
	void removeNode(const NodeId & failedNodeId);
	void setCurrentNodeId(NodeId currentNodeId);

	// cluster shard id => array index
	unsigned INDEX(const ClusterShardId & shardId);
	void moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId, const LocalPhysicalShard & physicalShardInfo);
};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__
