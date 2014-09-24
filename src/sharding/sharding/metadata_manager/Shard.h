#ifndef __SHARDING_CONFIGURATION_SHARD_H__
#define __SHARDING_CONFIGURATION_SHARD_H__

#include "../../util/FramedPrinter.h"
#include "../../configuration/CoreInfo.h"
#include "src/core/util/Assert.h"
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <map>
#include <vector>
#include <set>




using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;


struct NodeOperationId{
	NodeId nodeId;
	unsigned operationId;
	NodeOperationId();
	NodeOperationId(const NodeOperationId & id);
	NodeOperationId & operator=(const NodeOperationId & rhs);
	NodeOperationId(NodeId nodeId, unsigned operationId = 0);
	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool operator==(const NodeOperationId & right) const;
	bool operator>(const NodeOperationId & right) const;
	bool operator<(const NodeOperationId & right) const;
	string toString() const;
};

class ShardId{
public:
	ShardId(unsigned coreId);
	ShardId(const ShardId & copy);
	unsigned coreId;
	virtual ~ShardId(){};
	virtual std::string toString() const = 0;
	virtual bool isClusterShard() const = 0 ;

    void* serialize(void * buffer) const;
    void * deserialize(void* buffer);
    unsigned getNumberOfBytes() const;

};

class ClusterShardId;
typedef ClusterShardId ClusterPID;
class ClusterShardId : public ShardId{
public:
	ClusterShardId() ;
	ClusterShardId(const ClusterShardId & copy) ;
	ClusterShardId(unsigned coreId, unsigned partitionId, unsigned replicaId=0) ;

	unsigned partitionId; // ID for a partition, numbered 0, 1, 2, ...
	// ID for a specific primary/replica for a partition; assume #0 is always the primary shard.  For V0, replicaId is always 0
	unsigned replicaId;

	bool isPrimaryShard() const;
	std::string toString() const;
	bool isClusterShard() const ;

	ClusterPID getPartitionId() const;

	bool operator==(const ClusterShardId& rhs) const ;
	bool operator!=(const ClusterShardId& rhs) const ;
	bool operator>(const ClusterShardId& rhs) const ;
	bool operator<(const ClusterShardId& rhs) const ;
	bool operator>=(const ClusterShardId& rhs) const ;
	bool operator<=(const ClusterShardId& rhs) const ;
	ClusterShardId & operator=(const ClusterShardId & rhs);

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer) const;

    //given a byte stream recreate the original object
    void * deserialize(void* buffer);

    unsigned getNumberOfBytes() const;

};


class  NodeShardId : public ShardId{
public:
	NodeShardId();
	NodeShardId(const NodeShardId & copy);
	NodeShardId(unsigned coreId, NodeId nodeId, unsigned partitionId);

	NodeId nodeId;
	unsigned partitionId;

	std::string toString() const;
	std::string _toString() const;
	bool isClusterShard() const ;
	bool operator==(const NodeShardId& rhs) const ;
	bool operator!=(const NodeShardId& rhs) const ;
	bool operator>(const NodeShardId& rhs) const ;
	bool operator<(const NodeShardId& rhs) const ;
	bool operator>=(const NodeShardId& rhs) const ;
	bool operator<=(const NodeShardId& rhs) const ;
	NodeShardId & operator=(const NodeShardId & rhs);

    void* serialize(void * buffer) const;
    void * deserialize(void* buffer);
    unsigned getNumberOfBytes() const;
};

class ShardIdComparator {
public:
	// returns s1 > s2
	bool operator() (const ClusterShardId & s1, const ClusterShardId & s2);
	bool operator() (const NodeShardId & s1, const NodeShardId & s2);
};


class Shard{
public:
	Shard(boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory, const string & jsonFileCompletePath, double load = 0);
	double getLoad() const;
	boost::shared_ptr<Srch2Server> getSrch2Server() const;
	string getIndexDirectory() const;
	string getJsonFileCompletePath() const;
	virtual string getShardIdentifier() const = 0;
	virtual ~Shard(){};
private:
	boost::shared_ptr<Srch2Server> srch2Server;
	string indexDirectory;
	string jsonFileCompletePath;
	double load;
};


class ClusterShard : public Shard{
public:
	ClusterShard(const ClusterShardId & shardId,
			boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
			const string & jsonFileCompletePath, double load = 0);
	const ClusterShardId getShardId() const ;
	string getShardIdentifier() const ;
	bool operator==(const ClusterShard & right) const;


	string toString() const;
private:
	const ClusterShardId shardId;
};

class NodeShard : public Shard{
public:
	NodeShard(const NodeShardId & shardId, boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
			const string & jsonFileCompletePath,double load = 0);
	const NodeShardId getShardId() const ;
	string getShardIdentifier() const ;
	bool operator==(const NodeShard & right) const;
	string toString() const;
private:
	const NodeShardId shardId;
};

class NodeTargetShardInfo{
public:
	NodeTargetShardInfo(const NodeId nodeId, const unsigned coreId);
	NodeTargetShardInfo(){};
	NodeTargetShardInfo(const NodeTargetShardInfo & copy){
		this->nodeId = copy.nodeId;
		this->coreId = copy.coreId;
		this->targetClusterShards = copy.targetClusterShards;
		this->targetNodeShards = copy.targetNodeShards;
	};
	NodeTargetShardInfo & operator=(const NodeTargetShardInfo & rhs){
		if(this != &rhs){
			this->nodeId = rhs.nodeId;
			this->coreId = rhs.coreId;
			this->targetClusterShards = rhs.targetClusterShards;
			this->targetNodeShards = rhs.targetNodeShards;
		}
		return *this;
	}
	void addClusterShard(ClusterShardId shardId);
	void addNodeShard(const NodeShardId & shardId);

	const unsigned getCoreId() const;
	const NodeId getNodeId() const;
	vector<ClusterShardId> getTargetClusterShards() const;
	vector<NodeShardId> getTargetNodeShards() const;

    void* serialize(void * buffer);
    unsigned getNumberOfBytes() const;
    void * deserialize(void* buffer);

    string toString() const;

    static void printTargets(const vector<NodeTargetShardInfo> & targets){
		vector<string> targetHeaders;
		for(unsigned i = 0 ; i < targets.size(); ++i){
			targetHeaders.push_back(targets.at(i).toString());
		}
		vector<string> targetLables;
		targetLables.push_back("Info:");
		srch2::util::TableFormatPrinter nodesTable("Node Targets", 120, targetHeaders, targetLables );
		nodesTable.printColumnHeaders();
    }

private:
	NodeId nodeId;
	unsigned coreId;
	vector<ClusterShardId> targetClusterShards;
	vector<NodeShardId> targetNodeShards;

};

class LocalShardContainer{
public:

	LocalShardContainer(const unsigned coreId, const NodeId nodeId);
	LocalShardContainer(const LocalShardContainer & copy);
	unsigned getCoreId() const	;
	NodeId getNodeId() const;
	void getShards(const NodeTargetShardInfo & targets, vector<const Shard *> & shards) const;
	void getClusterShards(unsigned partitionId, vector<const ClusterShard *> & clusterShards) const;
	const NodeShard * getNodeShard(unsigned internalPartitionId) const ;
	void getNodeShards(map<unsigned, const NodeShard * > & localNodeShards) const;

	void addClusterShard(const ClusterShardId & shardId,
			boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
			const string & jsonCompletePath, double load);
	void addNodeShard(const NodeShardId & shardId, boost::shared_ptr<Srch2Server> srch2Server,const string & indexDirectory,
			const string & jsonCompletePath,double load);

	void print() const;
private:
	const unsigned coreId;
	const NodeId nodeId; // always equal to current node Id
	// partitionId => list of shards on this node
	map<unsigned, vector<ClusterShard *> > localClusterShards;
	// node internal partitionid => nodeshard
	map<unsigned, NodeShard * > localNodeShards;
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARD_H__
