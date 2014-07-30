#include "Cluster.h"


#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"
#include "sharding/sharding/ShardManager.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


ClusterResourceMetadata_Readview::ClusterResourceMetadata_Readview(unsigned versionId, string clusterName, vector<CoreInfo_t *> cores){
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

	for(map<unsigned, CorePartitionContianer *>::iterator coreItr = copy.corePartitioners.begin();
			coreItr != copy.corePartitioners.end(); ++coreItr ){
		this->corePartitioners[coreItr->first] = new CorePartitionContianer(*(coreItr->second));
	}
	for(map<unsigned, LocalShardContainer *>::iterator coreItr = copy.localShardContainers.begin();
			coreItr != copy.localShardContainers.end(); ++coreItr ){
		this->corePartitioners[coreItr->first] = new LocalShardContainer(*(coreItr->second));
	}
}

ClusterResourceMetadata_Readview::~ClusterResourceMetadata_Readview(){
	ShardManager::getShardManager()->resolveReadviewRelease(this->versionId);
}

const CorePartitionContianer * ClusterResourceMetadata_Readview::getPartitioner(unsigned coreId) const{
	if(corePartitioners.find(coreId) == corePartitioners.end()){
		return NULL;
	}
	return corePartitioners.find(coreId);
}

const LocalShardContainer * ClusterResourceMetadata_Readview::getLocalShardContainer(unsigned coreId) const{
	if(localShardContainers.find(coreId) == localShardContainers.end()){
		return NULL;
	}
	return localShardContainers.find(coreId);
}

void ClusterResourceMetadata_Readview::getAllCores(vector<const CoreInfo_t *> cores) const{
	for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = allCores.begin(); coreItr != allCores.end(); ++coreItr){
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
	for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = allCores.begin(); coreItr != allCores.end(); ++coreItr){
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
Node ClusterResourceMetadata_Readview::getNode(NodeId nodeId) const{
	if(allNodes.find(nodeId) == allNodes.end()){
		return NULL;
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
