#include "Cluster.h"


#include "core/util/Assert.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

Cluster::Cluster(const Cluster & cluster){
	this->clusterName = cluster.clusterName;
	this->clusterState = cluster.clusterState;
	this->masterNodeId = cluster.masterNodeId;
	// move core objects
//	for(std::vector<CoreInfo_t *>::const_iterator coreItr = cluster.cores.begin(); coreItr != cluster.cores.end(); ++coreItr){
//		this->cores.push_back(dynamic_cast<CoreInfo_t *>(*coreItr));
//	}
	this->cores = cluster.cores;
	// copy shardInformation
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr = cluster.shardInformation.begin() ;
			shardInfoItr != cluster.shardInformation.end() ; ++shardInfoItr){
		// new key
		Node * newNode = new Node(*(shardInfoItr->first));
		// new value
		vector<CoreShardContainer *> newCoresVector;
		for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = shardInfoItr->second.begin();
				coreShardContainerItr != shardInfoItr->second.end(); ++coreShardContainerItr){
			CoreShardContainer * newCoreShardContainer = new CoreShardContainer(**coreShardContainerItr);
			newCoresVector.push_back(newCoreShardContainer);
		}
		this->shardInformation.insert(std::make_pair(newNode, newCoresVector));
	}

};

string Cluster::getClusterName() const{
	string clusterName =  this->clusterName;
	return clusterName;
}
CLUSTERSTATE Cluster::getClusterState() const{
	CLUSTERSTATE clusterState =  this->clusterState;
	return clusterState;
}
void Cluster::setClusterName(const std::string& clusterName){
	this->clusterName = clusterName;
}
void Cluster::setClusterState(const CLUSTERSTATE & clusterState){
	this->clusterState = clusterState;
}
unsigned Cluster::getMasterNodeId() const{
	return masterNodeId;
}
void Cluster::setMasterNodeId(const unsigned & nodeId){
	this->masterNodeId = nodeId;
}


// access methods for shardInformation and cores vector
// can only be accessed from writeview, these two functions give writable handles to all
// data in the cluster
std::map<Node *, std::vector<CoreShardContainer * > > * Cluster::getShardInformation(){
	return &(this->shardInformation);
}

std::vector<CoreShardContainer * > * Cluster::getNodeShardInformation_Writeview(unsigned nodeId){
	std::map<Node *, std::vector<CoreShardContainer * > >::iterator currentNodeInfoItr =
			getNodeShardInformationEntry_Writeview(nodeId);
	return &(currentNodeInfoItr->second);
}

std::vector<CoreInfo_t *> * Cluster::getCores(){
	return &(this->cores);
}
CoreInfo_t * Cluster::getCoreByName_Writeview(const string & coreName){
	for(std::vector<CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		if((*coreItr)->getName().compare(coreName) == 0){
			return *coreItr;
		}
	}
	return NULL;
}

// can be accessed from writeview and readview
// shard access methods
void Cluster::getAllShardInformation(std::map<const Node *, std::vector< const CoreShardContainer * > > & shardInformation) const{
	for(std::map<Node *, std::vector< CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
			nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		vector<const CoreShardContainer *> nodeShardInfo;
		std::pair<const Node *, vector<const CoreShardContainer *> > nodeEntry =
				std::make_pair(nodeInfoItr->first, nodeShardInfo);
		for(std::vector< CoreShardContainer * >::const_iterator coreShardContainerItr = nodeInfoItr->second.begin();
				coreShardContainerItr != nodeInfoItr->second.end(); ++coreShardContainerItr){
			nodeEntry.second.push_back(*coreShardContainerItr);
		}
		shardInformation.insert(nodeEntry);
	}
}
void Cluster::getNodeShardInformation(unsigned nodeId, std::vector< const CoreShardContainer * > & coreShardContainers) const{
	std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeEntryItr =
			getNodeShardInformationEntry(nodeId);
	if(nodeEntryItr == shardInformation.end()){
		return;
	}
	for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = nodeEntryItr->second.begin();
			coreShardContainerItr != nodeEntryItr->second.end(); ++coreShardContainerItr){
		coreShardContainers.push_back(*coreShardContainerItr);
	}
}
const CoreShardContainer * Cluster::getNodeCoreShardInformation(unsigned nodeId, unsigned coreId) const{
	std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeEntryItr =
			getNodeShardInformationEntry(nodeId);
	if(nodeEntryItr == shardInformation.end()){
		return NULL;
	}
	for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = nodeEntryItr->second.begin();
			coreShardContainerItr != nodeEntryItr->second.end(); ++coreShardContainerItr){
		if((*coreShardContainerItr)->getCore()->getCoreId() == coreId){
			return (*coreShardContainerItr);
		}
	}
	return NULL;
}

void Cluster::addCorePrimaryShards(unsigned coreId, vector<const Shard *> & primaryShards) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr = shardInformation.begin();
			shardInfoItr != shardInformation.end(); ++shardInfoItr){
		for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = shardInfoItr->second.begin();
				coreShardContainerItr != shardInfoItr->second.end(); ++coreShardContainerItr){
			if((*coreShardContainerItr)->getCore()->getCoreId() == coreId){
				(*coreShardContainerItr)->addPrimaryShards(primaryShards);
			}
		}
	}
}
unsigned Cluster::getCoreTotalNumberOfPrimaryShards(unsigned coreId) const{
	unsigned totalNumberOfCorePrimaryShards = 0;
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr = shardInformation.begin();
			shardInfoItr != shardInformation.end(); ++shardInfoItr){
		for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = shardInfoItr->second.begin();
				coreShardContainerItr != shardInfoItr->second.end(); ++coreShardContainerItr){
			if((*coreShardContainerItr)->getCore()->getCoreId() == coreId){
				totalNumberOfCorePrimaryShards += (*coreShardContainerItr)->getTotalNumberOfPrimaryShards();
			}
		}
	}
	return totalNumberOfCorePrimaryShards;
}
void Cluster::getCorePrimaryShardReplicas(unsigned coreId, const ShardId & primaryShardId, vector<const Shard *> & replicas) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr = shardInformation.begin();
			shardInfoItr != shardInformation.end(); ++shardInfoItr){
		for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = shardInfoItr->second.begin();
				coreShardContainerItr != shardInfoItr->second.end(); ++coreShardContainerItr){
			if((*coreShardContainerItr)->getCore()->getCoreId() == coreId){
				(*coreShardContainerItr)->addPrimaryShardReplicas(primaryShardId, replicas);
			}
		}
	}
}
// core access methods
void Cluster::getCores(std::vector<const CoreInfo_t *> & cores) const{
	for(std::vector<CoreInfo_t *>::const_iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		cores.push_back(*coreItr);
	}
}
const CoreInfo_t * Cluster::getCoreById(unsigned coreId) const{
	for(std::vector<CoreInfo_t *>::const_iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		if((*coreItr)->getCoreId() == coreId){
			return *coreItr;
		}
	}
	return NULL;
}
const CoreInfo_t * Cluster::getCoreByName(const string & coreName) const{
	for(std::vector<CoreInfo_t *>::const_iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		if((*coreItr)->getName().compare(coreName) == 0){
			return *coreItr;
		}
	}
	return NULL;
}
unsigned Cluster::getCoreIdByCoreName(const string & coreName) const{
	return getCoreByName(coreName)->getCoreId();
}
string Cluster::getCoreNameByCoreId(unsigned coreId) const{
	return getCoreById(coreId)->getName();
}
// node access methods
void Cluster::getAllNodes(vector<const Node *> & nodes) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
			nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		nodes.push_back(nodeInfoItr->first);
	}
}
const Node * Cluster::getNodeById(unsigned id) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
			nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		if(nodeInfoItr->first->getId() == id){
			return nodeInfoItr->first;
		}
	}
	return NULL;
}
const Node* Cluster::getCurrentNode() const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
			nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		if(nodeInfoItr->first->thisIsMe == true){
			return nodeInfoItr->first;
		}
	}
	ASSERT(false);
	return NULL;
}
unsigned Cluster::getTotalNumberOfNodes() const{
	return this->shardInformation.size();
}
bool Cluster::isMasterNode(unsigned nodeId) const{
	return getNodeById(nodeId)->isMaster();
}

bool Cluster::isShardLocal(const ShardId& shardId) const{
	const Shard * shard = getShard(shardId);
	if(shard == NULL){
		ASSERT(false);
		return false;
	}
	return (shard->getNodeId() == getCurrentNode()->getId());
}

const Shard * Cluster::getShard(const ShardId & shardId) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeEntryItr = this->shardInformation.begin();
			nodeEntryItr != this->shardInformation.end(); ++nodeEntryItr){
		for(std::vector<CoreShardContainer * >::const_iterator coreShardContainerItr = nodeEntryItr->second.begin();
				coreShardContainerItr != nodeEntryItr->second.end(); ++coreShardContainerItr){
			if((*coreShardContainerItr)->getCore()->getCoreId() == shardId.coreId){
				const Shard * shard = (*coreShardContainerItr)->getShard(shardId);
				if(shard != NULL){
					return shard;
				}
			}
		}

	}
	return NULL;
}


void Cluster::print() const{
	Logger::console("Cluster name : %s", clusterName.c_str());
	Logger::console("Cluster master node id : %d", masterNodeId);
	Logger::console("Number of nodes : %d", shardInformation.size() );
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr =
			shardInformation.begin(); shardInfoItr != shardInformation.end(); ++shardInfoItr){
		Logger::console("Node %d, isMaster: %d", shardInfoItr->first->getId(), shardInfoItr->first->thisIsMe);
		Logger::console("There are %d cores in this node.", shardInfoItr->second.size());
		for(std::vector<CoreShardContainer * >::const_iterator coreShardItr = shardInfoItr->second.begin();
				coreShardItr != shardInfoItr->second.end(); ++coreShardItr){
			Logger::console("Core %s", (*coreShardItr)->getCore()->getName().c_str());
			Logger::console("Primary shards : %d", (*coreShardItr)->getTotalNumberOfPrimaryShards());
			vector<const Shard *> primaryShards;
			(*coreShardItr)->addPrimaryShards(primaryShards);
			for(unsigned i=0; i<primaryShards.size(); ++i){
				Logger::console("Shard : ");
				Logger::console("%s", primaryShards.at(i)->toString().c_str());
			}
		}
	}
}

std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator Cluster::getNodeShardInformationEntry(unsigned nodeId) const{
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeEntryItr = this->shardInformation.begin();
			nodeEntryItr != this->shardInformation.end(); ++nodeEntryItr){
		if(nodeEntryItr->first->getId() == nodeId){
			return nodeEntryItr;
		}
	}
	return this->shardInformation.end();
}

std::map<Node *, std::vector<CoreShardContainer * > >::iterator Cluster::getNodeShardInformationEntry_Writeview(unsigned nodeId){
	for(std::map<Node *, std::vector<CoreShardContainer * > >::iterator nodeEntryItr = this->shardInformation.begin();
			nodeEntryItr != this->shardInformation.end(); ++nodeEntryItr){
		if(nodeEntryItr->first->getId() == nodeId){
			return nodeEntryItr;
		}
	}
	return this->shardInformation.end();
}


}
}
