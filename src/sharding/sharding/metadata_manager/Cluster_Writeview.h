#ifndef __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__
#define __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__


#include "sharding/configuration/ShardingConstants.h"
#include "Shard.h"
#include "core/util/SerializationHelper.h"
#include "src/core/util/Assert.h"
#include "sharding/configuration/CoreInfo.h"
#include "Cluster.h"

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
	ClusterShard_Writeview(){};

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

class Cluster_Writeview{
public:
	string clusterName;
	NodeId currentNodeId;
	unsigned versionId;

	// Contains an entry for each one of those cluster shards that exist on this node
	// with server information.
	// ShardId on this node => server info
	map<ClusterShardId, LocalPhysicalShard > localClusterDataShards;

	// Server information for those independent shards that are on the current node
	// NodeShardId{currentNodeId, coreId, int-partitionId} => server information
	// internal partition id
	map<unsigned,  LocalPhysicalShard > localNodeDataShards;


	//////////////////// Runtime information which should be serialized /////////////////////////
	map<NodeId, std::pair<ShardingNodeState, Node *> > nodes;
	map<unsigned, CoreInfo_t *> cores;

	Cluster_Writeview(unsigned versionId, string clusterName, vector<CoreInfo_t *> cores);
	Cluster_Writeview(const Cluster_Writeview & copy);
	Cluster_Writeview();
	bool operator==(const Cluster_Writeview & right);
	bool isEqualDiscardingLocalShards(const Cluster_Writeview & right);
	void print();

	void printNodes();

	~Cluster_Writeview();

	void assignLocalClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShardInfo);
	void assignExternalClusterShard(const ClusterShardId & shardId, const NodeId & nodeId, const double & load);
	void unassignClusterShard(const ClusterShardId & shardId);
	void setClusterShardServer(const ClusterShardId & shardId, boost::shared_ptr<Srch2Server> server);
	void moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId);
	void moveClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShard);

	void beginClusterShardsIteration();
	bool getNextClusterShard(ClusterShardId & shardId, double & load, ShardState & state, bool & isLocal, NodeId & nodeId);
	bool getNextLocalClusterShard(ClusterShardId & shardId, double & load,  LocalPhysicalShard & localPhysicalShard );

	void addLocalNodeShard(const NodeShardId & nodeShardId, const double load, const LocalPhysicalShard & physicalShardInfo);
	void addExternalNodeShard(const NodeShardId & nodeShardId, const double load);
	void removeNodeShard(const NodeShardId & nodeShardId);
	void setNodeShardServer(const NodeShardId & nodeShardId, boost::shared_ptr<Srch2Server> server);

	void beginNodeShardsIteration() ;
	bool getNextNodeShard(NodeShardId & nodeShardId, bool & isLocal) ;
	bool getNextLocalNodeShard(NodeShardId & nodeShardId, double & load,  LocalPhysicalShard & dataInfo) ;


	// these partitions don't have write access yet.
	void getFullUnassignedPartitions(vector<ClusterPID> & fullUnassignedPartitions );
	// returns true if this partition has least one pending shard
	bool isPartitionPending(const ClusterPID & pid);


	void fixClusterMetadataOfAnotherNode(Cluster_Writeview * cluster);

	ClusterResourceMetadata_Readview * getNewReadview() ;



	// Does not return this node by default
	void getArrivedNodes(vector<NodeId> & allNodes, bool returnThisNode = false) const;
	void getAllNodes(std::vector<const Node *> & localCopy) const;
	void addNode(const Node & node);
	void setNodeState(NodeId nodeId, ShardingNodeState state);
	void removeNode(const NodeId & failedNodeId);
	void setCurrentNodeId(NodeId currentNodeId);
	unsigned getTotalNumberOfNodes(){return nodes.size();};

	void fixAfterDiskLoad(Cluster_Writeview * newWrireview);

	unsigned getLocalNodeTotalLoad(){
		return localClusterDataShards.size()+localNodeDataShards.size();
	}

	// Serialization methods
	void saveWriteviewOnDisk(string absDirectoryPath);
	static Cluster_Writeview * loadWriteviewFromDisk(string absDirectoryPath);
	void * serialize(void * buffer, bool includeLocalInfoFlag = true) const;
	unsigned getNumberOfBytes(bool includeLocalInfoFlag = true) const;
	void * deserialize(void * buffer, bool includeLocalInfoFlag = true);

	vector<ClusterShard_Writeview *> & getClusterShards(){
		return clusterShards;
	}

	vector<NodeShard_Writeview *> & getNodeShards(){
		return nodeShards;
	}
private:
	unsigned clusterShardsCursor;
	vector<ClusterShard_Writeview *> clusterShards;
	map<ClusterShardId, unsigned> clusterShardIdIndexes;
	unsigned nodeShardsCursor;
	vector<NodeShard_Writeview *> nodeShards;

	// cluster shard id => array index
	unsigned INDEX(const ClusterShardId & shardId);
	void moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId, const LocalPhysicalShard & physicalShardInfo);
};

}
}

#endif // __SHARDING_SHARDING_CLUSTER_WRITEVIEW_H__
