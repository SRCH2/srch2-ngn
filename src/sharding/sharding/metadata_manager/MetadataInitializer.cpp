

#include "MetadataInitializer.h"
#include "ResourceMetadataManager.h"
#include "DataShardInitializer.h"
#include "Cluster_Writeview.h"

#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


void MetadataInitializer::initializeNode(){
	// grab writeview for initialization
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview_write(xLock);

	// 1. if there is a copy of writeview on the disk, load it and add information to the available writeview
	Cluster_Writeview * newWriteview;
	newWriteview = loadFromDisk(writeview->clusterName);
	if(newWriteview != NULL){ // loaded writeview is present.
		if(checkValidityOfLoadedWriteview(newWriteview, writeview)){
			// fix the new writeview
			newWriteview->fixAfterDiskLoad(writeview);
		}else{ // new writeview is not usable
			delete newWriteview;
			newWriteview = NULL;
		}
	}// else : no writeview was present from disk.

	if(newWriteview == NULL){ // either no loaded writeview or it was invalid and unusable
		// 2.2. add new json files
		addNewJsonFileShards(writeview);
		return;
	}
	// 2. if there are some shards to load or create, do it and put them in the writeview as local shards
	// 2.1. load existing shards
	loadShards(newWriteview);
	// 2.2. add new json files
	addNewJsonFileShards(newWriteview);

	SP(const ClusterNodes_Writeview) nodesWriteview = metadataManager->getClusterNodesWriteview_read();
	metadataManager->setWriteview(newWriteview, false);
	return;
}


// 1. assigns the primary shard of each partition to this node
// 2. starts empty search engines for all primary shards.
void MetadataInitializer::initializeCluster(bool shouldLock){
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = NULL;
	if(shouldLock){
		writeview = metadataManager->getClusterWriteview_write(xLock);
	}else{
		writeview = metadataManager->getClusterWriteview_nolock();
	}
	// assign the parimary shard of each partition to this node
	std::set<ClusterShardId> unassignedPartitions;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		ClusterShardId pid(id.coreId,id.partitionId);
		if(state == SHARDSTATE_UNASSIGNED){
			if(unassignedPartitions.count(pid) == 0){
				unassignedPartitions.insert(pid);
			}
		}
		if(state == SHARDSTATE_PENDING || state == SHARDSTATE_READY){
			if(unassignedPartitions.count(pid) != 0){
				unassignedPartitions.erase(pid);
			}
		}
	}
	for(std::set<ClusterShardId>::iterator pidItr = unassignedPartitions.begin(); pidItr != unassignedPartitions.end(); ++pidItr){
		const ClusterShardId & shardId = *pidItr;

		string indexDirectory = configManager->getShardDir(writeview->clusterName,
				writeview->cores[pidItr->coreId]->getName(), &shardId);
		if(indexDirectory.compare("") == 0){
			indexDirectory = configManager->createShardDir(writeview->clusterName,
				writeview->cores[pidItr->coreId]->getName(), &shardId);
		}
		EmptyShardBuilder emptyShard(new ClusterShardId(shardId), indexDirectory);
		emptyShard.prepare(shouldLock);
		writeview->assignLocalClusterShard(*pidItr, LocalPhysicalShard(emptyShard.getShardServer(), indexDirectory, ""));
	}


}

void MetadataInitializer::updateWriteviewForJsonFileShard(Cluster_Writeview * newWriteview,
		const NodeShardId & shardId, InitialShardBuilder * builder ){
	if(builder->isSuccessful()){ // we must put this new shard in writeview
		LocalPhysicalShard physicalShard(builder->getShardServer(),
						builder->getIndexDirectory(),
						builder->getJsonFileCompletePath());
		newWriteview->addLocalNodeShard(shardId, 1, physicalShard);
	}else{  // we must leave the json file

	}
}

void MetadataInitializer::addNewJsonFileShards(Cluster_Writeview * newWriteview){
	ASSERT(newWriteview != NULL);
	// coreId to a list of json file paths
	map<unsigned, string > jsonFilesToBeUsed;
	// I) check all json files, and keep the path if it's a new json file, per core ...
	map<unsigned, unsigned > nodeShardPartitionIdOffset;
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = newWriteview->cores.begin(); coreItr != newWriteview->cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * coreInfo = coreItr->second;
		nodeShardPartitionIdOffset[coreId] = coreInfo->getNumberOfPrimaryShards();
		const string & dataFilePath = coreInfo->getDataFilePath();
		for(map<unsigned,  LocalPhysicalShard >::iterator nodeShardItr = newWriteview->localNodeDataShards.begin();
				nodeShardItr != newWriteview->localNodeDataShards.end(); ++nodeShardItr){
			if(nodeShardPartitionIdOffset[coreId] <= nodeShardItr->first){
				nodeShardPartitionIdOffset[coreId] = nodeShardItr->first + 1;
			}
		}
		if(dataFilePath.compare("") != 0){
			bool isNew = true;
			for(map<unsigned,  LocalPhysicalShard >::iterator nodeShardItr = newWriteview->localNodeDataShards.begin();
					nodeShardItr != newWriteview->localNodeDataShards.end(); ++nodeShardItr){
				if(nodeShardItr->second.jsonFileCompletePath.compare(dataFilePath) == 0){ // the same json file
					isNew = false;
					break;
				}
			}
			if(isNew){
				jsonFilesToBeUsed[coreId] = dataFilePath;
			}
		}
	}


	for(map<unsigned, CoreInfo_t *>::iterator coreItr = newWriteview->cores.begin(); coreItr != newWriteview->cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * coreInfo = coreItr->second;
		if(jsonFilesToBeUsed.find(coreId) != jsonFilesToBeUsed.end()){
			ASSERT(coreInfo->getNumberOfPrimaryShards() == 0);
			unsigned newShardPartitionId = coreId * 100 + nodeShardPartitionIdOffset[coreId];
			NodeShardId shardId(coreId, newWriteview->currentNodeId, newShardPartitionId);
			string indexDirectory = configManager->getShardDir(newWriteview->clusterName,
					newWriteview->cores[coreId]->getName(), &shardId);
			if(indexDirectory.compare("") == 0){
				indexDirectory = configManager->createShardDir(newWriteview->clusterName,
										newWriteview->cores[coreId]->getName(), &shardId);
			}
			InitialShardBuilder shardBuilder(new NodeShardId(shardId), indexDirectory, jsonFilesToBeUsed[coreId]);
			shardBuilder.prepare(false);
			updateWriteviewForJsonFileShard(newWriteview, shardId, &shardBuilder);
		}
//		}else if(coreInfo->isAclCore()){
//			// Acl core, we must make an empty shard and it will automatically load the acl data if needed.
//			unsigned newShardPartitionId = coreId * 200 + nodeShardPartitionIdOffset[coreId];
//			NodeShardId shardId(coreId, newWriteview->currentNodeId, newShardPartitionId);
//			string indexDirectory = configManager->getShardDir(newWriteview->clusterName,
//					newWriteview->cores[coreId]->getName(), &shardId);
//			if(indexDirectory.compare("") == 0){
//				indexDirectory = configManager->createShardDir(newWriteview->clusterName,
//										newWriteview->cores[coreId]->getName(), &shardId);
//			}
//
//			EmptyShardBuilder shardBuilder(new NodeShardId(shardId), indexDirectory);
//			shardBuilder.prepare(false);
//			updateWriteviewForJsonFileShard(newWriteview, shardId, &shardBuilder);
//			// TODO : update writeview for acl shards, even needed ?
//		}
	}

	return;
}


void MetadataInitializer::updateWriteviewForLoad(Cluster_Writeview * newWriteview, const ClusterShardId & shardId , InitialShardLoader * loader){
	if(loader->isSuccessful()){ // shard successfully loaded
		newWriteview->setClusterShardServer(shardId, loader->getShardServer());
		return;
	}else{ // shard didn't load, remove from writeview and fix it
		newWriteview->unassignClusterShard(shardId);
	}
}
void MetadataInitializer::updateWriteviewForLoad(Cluster_Writeview * newWriteview, const NodeShardId & shardId , InitialShardLoader * loader){
	if(loader->isSuccessful()){ // shard successfully loaded
		newWriteview->setNodeShardServer(shardId, loader->getShardServer());
		return;
	}else{ // shard didn't load, remove from writeview and fix it
		newWriteview->removeNodeShard(shardId);
		return;
	}
}

void MetadataInitializer::loadShards(Cluster_Writeview * newWriteview){
	ASSERT(newWriteview != NULL);

	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	vector<pair<ClusterShardId, InitialShardLoader *> > clusterShardsToLoad;
	ClusterShardIterator cShardItr(newWriteview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextLocalClusterShard(id,load,physicalShard)){
		InitialShardLoader * initialShardLoader = new InitialShardLoader(new ClusterShardId(id), physicalShard.indexDirectory);
		clusterShardsToLoad.push_back(std::make_pair(id, initialShardLoader));
	}

	vector<pair<NodeShardId, InitialShardLoader * > > nodeShardsToLoad;
    NodeShardIterator nShardItr(newWriteview);
    nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextLocalNodeShard(nodeShardId, load, physicalShard)){
		InitialShardLoader * initialShardLoader = new InitialShardLoader(new ClusterShardId(id), physicalShard.indexDirectory);
		nodeShardsToLoad.push_back(std::make_pair(nodeShardId, initialShardLoader));
	}

	// start threads to load indexes (if we have more than one load)
	if((clusterShardsToLoad.size() + nodeShardsToLoad.size()) == 0){
		return;
	}

	// 1. load all shards
	for(unsigned i = 0; i < clusterShardsToLoad.size(); ++i){
		clusterShardsToLoad.at(i).second->prepare(false);
	}
	for( unsigned i = 0 ; i < nodeShardsToLoad.size(); ++i){
		nodeShardsToLoad.at(i).second->prepare(false);
	}
	// 2. fix the writeview if any loads failed.
	for(unsigned  i = 0; i < clusterShardsToLoad.size() ; ++i){
		ClusterShardId shardId = clusterShardsToLoad.at(i).first;
		InitialShardLoader * loader = clusterShardsToLoad.at(i).second;
		updateWriteviewForLoad(newWriteview, shardId, loader);
		delete loader;
	}
	for(unsigned  i = 0; i < nodeShardsToLoad.size() ; ++i){
		NodeShardId shardId = nodeShardsToLoad.at(i).first;
		InitialShardLoader * loader = nodeShardsToLoad.at(i).second;
		updateWriteviewForLoad(newWriteview, shardId, loader);
		delete loader;
	}
	return;
}

bool MetadataInitializer::checkValidityOfLoadedWriteview(Cluster_Writeview * newWriteview,
		Cluster_Writeview * currentWriteview){
	//TODO
	return true;
}

// writeview must be locked when call this function
// if there is a copy of writeview on the disk, load it and add information to the available writeview
Cluster_Writeview * MetadataInitializer::loadFromDisk(const string & clusterName){
	string clusterFileDirectoryPath = configManager->getClusterDir(clusterName);
	if(clusterFileDirectoryPath.compare("") != 0){
		Cluster_Writeview * loadedWriteview = Cluster_Writeview::loadWriteviewFromDisk(clusterFileDirectoryPath);
		return loadedWriteview;
	}else{
		return NULL; // not loaded.
	}
}

void MetadataInitializer::saveToDisk(const string & clusterName){
	string clusterFileDirectoryPath = configManager->getClusterDir(clusterName);
	if(clusterFileDirectoryPath.compare("") == 0){
		clusterFileDirectoryPath = configManager->createClusterDir(clusterName);
	}
	metadataManager->writeview->saveWriteviewOnDisk(clusterFileDirectoryPath);
}


}
}
