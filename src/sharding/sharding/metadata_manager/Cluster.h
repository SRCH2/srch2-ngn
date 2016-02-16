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
#ifndef __SHARDING_CONFIGURATION_CLUSTER_H__
#define __SHARDING_CONFIGURATION_CLUSTER_H__

#include "Shard.h"
#include "Node.h"
#include "Partition.h"
#include "../../configuration/CoreInfo.h"
#include "../../configuration/ShardingConstants.h"

#include "wrapper/WrapperConstants.h"
#include "core/util/Assert.h"
#include "core/util/mypthread.h"
#include "core/util/Logger.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class ClusterResourceMetadata_Readview{
public:
	ClusterResourceMetadata_Readview(unsigned versionId, string clusterName, vector<const CoreInfo_t *> cores);
	ClusterResourceMetadata_Readview(const ClusterResourceMetadata_Readview & copy);
	~ClusterResourceMetadata_Readview();

	const CorePartitionContianer * getPartitioner(unsigned coreId) const;
	const LocalShardContainer * getLocalShardContainer(unsigned coreId) const;
	void getAllCores(vector<const CoreInfo_t *> & cores) const;
	const CoreInfo_t * getCore(unsigned coreId) const;
	const CoreInfo_t * getCoreByName(const string & coreName) const;
	void getAllNodes(vector<Node> & nodes) const;
	void getAllNodeIds(vector<NodeId> & nodeIds) const;
	unsigned getTotalNumberOfNodes() const;
	Node getNode(NodeId nodeId) const;
	unsigned getCurrentNodeId() const;
	string getClusterName() const;
	unsigned getVersionId() const;

	void print() const;

	void setCurrentNodeId(NodeId nodeId);
	CorePartitionContianer * getCorePartitionContianer(unsigned coreId);
	const CorePartitionContianer * getCorePartitionContianer(unsigned coreId) const;
	LocalShardContainer * getLocalShardContainer(unsigned coreId);
	void addNode(const Node & node);

private:

	uint32_t versionId;

	string clusterName;

	// coreId => core *
	map<uint32_t, const CoreInfo_t *> allCores;
	// coreId => core partitions
	map<uint32_t, CorePartitionContianer *> corePartitioners;
	// coreId => local core shards
	map<uint32_t, LocalShardContainer *> localShardContainers;


	// nodeId => node
	map<NodeId, Node> allNodes;

	// this node id
	NodeId currentNodeId;
};




}
}

#endif // __SHARDING_CONFIGURATION_CLUSTER_H__
