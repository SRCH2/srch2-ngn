/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
	void getAllWriteTargets(const ClusterPID & pid, NodeId currentNodeId, vector<NodeTargetShardInfo> & targets) const;
	void getAllTargets(vector<NodeTargetShardInfo> & target) const;
	void getAllShardIds(vector<ClusterShardId> & allShardIds) const;
	const unsigned getCoreId() const	;
	const unsigned getTotalNumberOfPartitions() const;
	const unsigned getReplicationDegree() const;
    // computes the hash value of a string
    unsigned hashDJB2(const char *str) const;
    // returns false if this core does not have any cluster shards (only nodes shards)
    // otherwise returns true and sets pid
    bool getClusterPartitionId(const string & primaryKey, ClusterPID & pid) const;

private:
	const CorePartitionContianer * partitionContainer;

	void addReadPartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const;
	void addWritePartitionToNodeTargetContainer(const ClusterPartition * partition, NodeTargetShardInfo & targets ) const;
	void addPartitionToNodeTargetContainer(const NodePartition * partition, NodeTargetShardInfo & targets ) const;

};

}
}


#endif // __SHARDING_PROCESSOR_PARTITIONER_H_
