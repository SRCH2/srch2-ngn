#ifndef __SHARDING_PROCESSOR_PARTITIONER_H_
#define __SHARDING_PROCESSOR_PARTITIONER_H_

using namespace std;
#include <string>
#include "instantsearch/Record.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/sharding/metadata_manager/Shard.h"
#include "sharding/sharding/metadata_manager/Partition.h"

using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {



class CorePartitioner{
public:
	CorePartitioner(const CorePartitionContianer * partitionContainer);
	virtual ~CorePartitioner(){};

    unsigned getRecordValueToHash(Record * record);
    unsigned getRecordValueToHash(string primaryKeyStringValue);

	void getAllReadTargets(vector<NodeTargetShardInfo> & targets) const;
	void getAllWriteTargets(unsigned hashKey, NodeId currentNodeId, vector<NodeTargetShardInfo> & targets) const;
	void getAllTargets(vector<NodeTargetShardInfo> & target) const;
	void getAllShardIds(vector<ClusterShardId> & allShardIds) const;
	const unsigned getCoreId() const	;
	const unsigned getTotalNumberOfPartitions() const;
	const unsigned getReplicationDegree() const;
    // computes the hash value of a string
    unsigned hashDJB2(const char *str) const;

private:
	const CorePartitionContianer * partitionContainer;

	void addReadPartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const;
	void addWritePartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const;
	void addPartitionToNodeTargetContainer(const NodePartition * partition, NodeTargetShardInfo & targets ) const;

};

}
}


#endif // __SHARDING_PROCESSOR_PARTITIONER_H_
