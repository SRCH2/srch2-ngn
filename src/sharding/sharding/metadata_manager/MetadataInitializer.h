#ifndef __SHARDING_SHARDING_NODE_INITIALIZER_H__
#define __SHARDING_SHARDING_NODE_INITIALIZER_H__

#include "Shard.h"
#include "DataShardInitializer.h"
#include "../../configuration/ShardingConstants.h"
#include "../../configuration/ConfigManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ResourceMetadataManager;
class InitialShardLoader;
class Cluster_Writeview;
/*
 * This class is responsible of joining a new node to the cluster
 * NOTE : we must not lock writeview in this class
 */
class MetadataInitializer{
public:

	MetadataInitializer(ConfigManager * configManager, srch2http::ResourceMetadataManager * metadataManager){
		this->metadataManager = metadataManager;
		this->configManager = configManager;
	}
	// initializes this node:
	// 1. if there is a copy of writeview on the disk, load it and add information to the available writeview
	// 2. if there are some shards to load or create, do it and put them in the writeview as local shards
	// 3. commit writeview to the readview to make it available to the readers;
	void initializeNode();

	// 1. assigns the primary shard of each partition to this node
	// 2. starts empty search engines for all primary shards.
	void initializeCluster();
	// NOTE: assumes the caller thread has sLock on the writeview
	void saveToDisk(const string & clusterName);
private:
	ResourceMetadataManager * metadataManager;
	ConfigManager * configManager;


	void updateWriteviewForJsonFileShard(Cluster_Writeview * newWriteview,
			const NodeShardId & shardId, InitialShardBuilder * builder );
	void addNewJsonFileShards(Cluster_Writeview * newWriteview);
	void updateWriteviewForLoad(Cluster_Writeview * newWriteview, const ClusterShardId & shardId , InitialShardLoader * loader);
	void updateWriteviewForLoad(Cluster_Writeview * newWriteview, const NodeShardId & shardId , InitialShardLoader * loader);
	void loadShards(Cluster_Writeview * newWriteview);
	// check validity of loaded writeview
	// for example this function checks whether there are the same number of partitions and
	// replicas for each core.
	bool checkValidityOfLoadedWriteview(Cluster_Writeview * newWriteview,
			Cluster_Writeview * currentWriteview);
	// writeview must be locked when call this function
	Cluster_Writeview *  loadFromDisk(const string & clusterName);
};

}
}


#endif // __SHARDING_SHARDING_NODE_INITIALIZER_H__
