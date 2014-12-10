#include "Cluster.h"
#include <iostream>

#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"
#include "../ShardManager.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


ClusterResourceMetadata_Readview::ClusterResourceMetadata_Readview(unsigned versionId,
		string clusterName, vector<const CoreInfo_t *> cores){
	this->versionId = versionId;
	this->clusterName = clusterName;
	for(unsigned coreIdx = 0; coreIdx < cores.size() ; ++coreIdx){
		unsigned coreId = cores.at(coreIdx)->getCoreId();
		allCores[coreId] = cores.at(coreIdx);
		corePartitioners[coreId] = new CorePartitionContianer(coreId, cores.at(coreIdx)->getNumberOfPrimaryShards(),
				cores.at(coreIdx)->getNumberOfReplicas());
		localShardContainers[coreId] = new LocalShardContainer(coreId, currentNodeId);
	}
}

ClusterResourceMetadata_Readview::ClusterResourceMetadata_Readview(const ClusterResourceMetadata_Readview & copy){
	this->versionId = copy.versionId;
	this->clusterName = copy.clusterName;
	this->currentNodeId = copy.currentNodeId;
	this->allCores = copy.allCores;
	this->allNodes = copy.allNodes;

	for(map<unsigned, CorePartitionContianer *>::const_iterator coreItr = copy.corePartitioners.begin();
			coreItr != copy.corePartitioners.end(); ++coreItr ){
		this->corePartitioners[coreItr->first] = new CorePartitionContianer(*(coreItr->second));
	}
	for(map<unsigned, LocalShardContainer *>::const_iterator coreItr = copy.localShardContainers.begin();
			coreItr != copy.localShardContainers.end(); ++coreItr ){
		this->localShardContainers[coreItr->first] = new LocalShardContainer(*(coreItr->second));
	}
}

ClusterResourceMetadata_Readview::~ClusterResourceMetadata_Readview(){

	for(unsigned coreIdx = 0; coreIdx < allCores.size() ; ++coreIdx){
		unsigned coreId = allCores.at(coreIdx)->getCoreId();
		delete corePartitioners[coreId];
		delete localShardContainers[coreId];
	}

	boost::shared_lock<boost::shared_mutex> sLock(ShardManager::getShardManagerGuard());
	ShardManager * shardManager = ShardManager::getShardManager();
	if(shardManager == NULL || shardManager->isCancelled()){
		return;
	}
	pthread_t & rvReleaseThread = *(shardManager->getNewThread());
	unsigned * vid = new unsigned;
	*vid = this->versionId;
    if (pthread_create(&rvReleaseThread, NULL, ShardManager::resolveReadviewRelease , vid) != 0){
        // Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return;
    }
    pthread_detach(rvReleaseThread);

}

const CorePartitionContianer * ClusterResourceMetadata_Readview::getPartitioner(unsigned coreId) const{
	if(corePartitioners.find(coreId) == corePartitioners.end()){
		return NULL;
	}
	return corePartitioners.find(coreId)->second;
}

const LocalShardContainer * ClusterResourceMetadata_Readview::getLocalShardContainer(unsigned coreId) const{
	if(localShardContainers.find(coreId) == localShardContainers.end()){
		return NULL;
	}
	return localShardContainers.find(coreId)->second;
}

void ClusterResourceMetadata_Readview::getAllCores(vector<const CoreInfo_t *> & cores) const{
	for(map<unsigned, const CoreInfo_t *>::const_iterator coreItr = allCores.begin(); coreItr != allCores.end(); ++coreItr){
//		if(coreItr->second->isAclCore()){
//			continue;
//		}
		cores.push_back(coreItr->second);
	}
}

const CoreInfo_t * ClusterResourceMetadata_Readview::getCore(unsigned coreId) const{
	if(allCores.find(coreId) == allCores.end()){
		return NULL;
	}
	return allCores.find(coreId)->second;
}

const CoreInfo_t * ClusterResourceMetadata_Readview::getCoreByName(const string & coreName) const{
	for(map<unsigned, const CoreInfo_t *>::const_iterator coreItr = allCores.begin(); coreItr != allCores.end(); ++coreItr){
		if(coreItr->second->getName().compare(coreName) == 0){
			return coreItr->second;
		}
	}
	return NULL;
}


void ClusterResourceMetadata_Readview::getAllNodes(vector<Node> & nodes) const{
	for(map<NodeId, Node>::const_iterator nodeItr = allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		nodes.push_back(nodeItr->second);
	}
}
void ClusterResourceMetadata_Readview::getAllNodeIds(vector<NodeId> & nodeIds) const{
	for(map<NodeId, Node>::const_iterator nodeItr = allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		nodeIds.push_back(nodeItr->first);
	}
}

unsigned ClusterResourceMetadata_Readview::getTotalNumberOfNodes() const{
	return allNodes.size();
}

Node ClusterResourceMetadata_Readview::getNode(NodeId nodeId) const{
	if(allNodes.find(nodeId) == allNodes.end()){
		return Node();
	}
	return allNodes.find(nodeId)->second;
}
unsigned ClusterResourceMetadata_Readview::getCurrentNodeId() const{
	return this->currentNodeId;
}
string ClusterResourceMetadata_Readview::getClusterName() const{
	return this->clusterName;
}

unsigned ClusterResourceMetadata_Readview::getVersionId() const{
	return versionId;
}


void ClusterResourceMetadata_Readview::print() const{
	cout << "Readview : " << endl;

	cout << "Cluster name : " << clusterName << endl;
	cout << "Version id : " << versionId << endl;

	cout << "Number of nodes : " << allNodes.size() << endl;
	cout << "Current node id : " << currentNodeId << endl;

	for(map<unsigned, const CoreInfo_t *>::const_iterator coreItr = allCores.begin(); coreItr != allCores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		//
		cout << "Partition info : =======================" << endl;
		const CorePartitionContianer * container = corePartitioners.at(coreId);
		container->print();

		//
		cout << "Local info : =======================" << endl;
		const LocalShardContainer * localContainer = localShardContainers.at(coreId);
		localContainer->print();
	}
}
void ClusterResourceMetadata_Readview::setCurrentNodeId(NodeId nodeId){
	this->currentNodeId = nodeId;
}

CorePartitionContianer * ClusterResourceMetadata_Readview::getCorePartitionContianer(unsigned coreId){
	if(corePartitioners.find(coreId) == corePartitioners.end()){
		ASSERT(false);
		return NULL;
	}
	return corePartitioners.find(coreId)->second;
}

const CorePartitionContianer * ClusterResourceMetadata_Readview::getCorePartitionContianer(unsigned coreId) const{
	if(corePartitioners.find(coreId) == corePartitioners.end()){
		ASSERT(false);
		return NULL;
	}
	return corePartitioners.find(coreId)->second;
}

LocalShardContainer * ClusterResourceMetadata_Readview::getLocalShardContainer(unsigned coreId){
	if(localShardContainers.find(coreId) == localShardContainers.end()){
		ASSERT(false);
		return NULL;
	}
	return localShardContainers.find(coreId)->second;
}

void ClusterResourceMetadata_Readview::addNode(const Node & node){
	allNodes[node.getId()] = node;
}

}
}
