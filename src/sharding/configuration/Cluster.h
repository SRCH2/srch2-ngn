#ifndef __SHARDING_CONFIGURATION_CLUSTER_H__
#define __SHARDING_CONFIGURATION_CLUSTER_H__

#include "Shard.h"
#include "Node.h"
#include "CoreInfo.h"

#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include "ShardingConstants.h"
#include "src/core/util/Assert.h"
#include "src/core/util/mypthread.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "src/core/util/Logger.h"
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

/*
 * this class implements a two level MGL:
 * the clusterMetadataLock is the top of tree, and under it
 * there will be one lock per Shard.
 */
class ConfigManager;
class Cluster {
	friend class ConfigManager;
public:


	/* Access functions for cluster name and state***********************************************/
	string getClusterName() const;
	void setClusterName(const string & clusterName);
	CLUSTERSTATE getClusterState() const;
	void setClusterState(const CLUSTERSTATE & clusterState);
	unsigned getMasterNodeId() const;
	void setMasterNodeId(const unsigned & nodeId);


	// access methods for shardInformation and cores vector
	// can only be accessed from writeview, these two functions give writable handles to all
	// data in the cluster
	std::map<Node *, std::vector<CoreShardContainer * > > * getShardInformation();
	std::vector<CoreInfo_t *> * getCores();
	CoreInfo_t * getCoreByName_Writeview(const string & coreName);
	std::vector<CoreShardContainer * > * getNodeShardInformation_Writeview(unsigned nodeId);

	// can be accessed from writeview and readview
	// shard access methods
	void getAllShardInformation(std::map<const Node *, std::vector< const CoreShardContainer * > > & shardInformation) const;
	void getNodeShardInformation(unsigned nodeId, std::vector< const CoreShardContainer * > & coreShardContainers) const;
	const CoreShardContainer * getNodeCoreShardInformation(unsigned nodeId, unsigned coreId) const;
	void addCorePrimaryShards(unsigned coreId, vector<const Shard *> & primaryShards) const;
	unsigned getCoreTotalNumberOfPrimaryShards(unsigned coreId) const;
	void getCorePrimaryShardReplicas(unsigned coreId, const ShardId & primaryShardId, vector<const Shard *> & replicas) const;
	// core access methods
	void getCores(std::vector<const CoreInfo_t *> & cores) const;
	const CoreInfo_t * getCoreById(unsigned coreId) const;
	const CoreInfo_t * getCoreByName(const string & coreName) const;
	unsigned getCoreIdByCoreName(const string & coreName) const;
	string getCoreNameByCoreId(unsigned coreId) const;
	// node access methods
	void getAllNodes(vector<const Node *> & nodes) const;
	const Node * getNodeById(unsigned id) const;
	const Node* getCurrentNode() const;
	unsigned getTotalNumberOfNodes() const;
	bool isMasterNode(unsigned nodeId) const;
	bool isShardLocal(const ShardId& shardId) const;
	const Shard * getShard(const ShardId & shardId) const;
	void addNewNode(const Node& newNode);
	string serializeClusterNodes();
	void clear() { shardInformation.clear() ; }
	void print() const;

private:
	// The constructor of this class is private because it's singleton and
	// there is only one object which is created by the static functions
	// holdClusterForRead and holdClusterForReadWrite
	Cluster(){
		// NOTE: it's not used now
		clusterState = CLUSTERSTATE_GREEN;
	};
	Cluster(const Cluster & cluster);

	// helper functions
	std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator getNodeShardInformationEntry(unsigned nodeId) const;
	std::map<Node *, std::vector<CoreShardContainer * > >::iterator getNodeShardInformationEntry_Writeview(unsigned nodeId);
	// the name of this cluster
	string       clusterName;
	// the state of this cluster
	// TODO : the presence of this class comes from our research on elastic search
	// but still we don't have a reason to use it! If this is the case after first production
	// this member must be removed from the codebase.
	CLUSTERSTATE clusterState;
	// the node Id of master
	unsigned masterNodeId;
	// map which contains all shard information of all nodes
	// node -> vector<CoreShardContainer * >
	std::map<Node *, std::vector<CoreShardContainer * > > shardInformation;
	// cores in the cluster
	std::vector<CoreInfo_t *> cores;

};

}
}

#endif // __SHARDING_CONFIGURATION_CLUSTER_H__
