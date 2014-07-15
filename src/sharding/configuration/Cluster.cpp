#include "Cluster.h"


#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"

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
	if(this->shardInformation.end() == currentNodeInfoItr){
		return NULL;
	}
	return &(currentNodeInfoItr->second);
}


// this method to be called only from clients to merge master cluster writeview with current writeview
// NOTE: make sure to also move Srch2Server shared_ptr s
// TODO : currently this functions just puts srch2Server pointers in master copy and this copy will be used
// from now on as local writeview
bool Cluster::mergeLocalIntoMaster(Cluster * localWriteview, Cluster * masterWriteview){
	//TODO
	// This implementation assumes master copy is consistent with local copy
	// 1. move over the data of this node and copy shard pointers to Srch2Servers to the master copy
	NodeId currentNodeId = localWriteview->getCurrentNode()->getId();
	std::vector<CoreShardContainer * > * currentNodeInfo = localWriteview->getNodeShardInformation_Writeview(currentNodeId);
	std::vector<CoreShardContainer * > * currentNodeInfoFromMaster = masterWriteview->getNodeShardInformation_Writeview(currentNodeId);
	// iterate over cores and for each core, find the corresponding core
	ASSERT(currentNodeInfo->size() == currentNodeInfoFromMaster->size());
	for(unsigned localCoreIndex = 0; localCoreIndex < currentNodeInfo->size(); ++localCoreIndex){
		CoreShardContainer * localCoreContainer = currentNodeInfo->at(localCoreIndex);
		for(unsigned masterCoreIndex = 0; masterCoreIndex < currentNodeInfoFromMaster->size(); ++masterCoreIndex){
			CoreShardContainer * masterCoreContainer = currentNodeInfoFromMaster->at(masterCoreIndex);
			if(masterCoreContainer->getCoreName() == localCoreContainer->getCoreName()){
				// now set srch2Server pointers in master copy
				masterCoreContainer->setCore(localCoreContainer->getCore());
				masterCoreContainer->setSrch2ServerPointers(localCoreContainer);
			}
		}
	}
	return true;
}
bool Cluster::bootstrapMergeWithClientsInfo(std::map<NodeId, Cluster * > & clusterInfos){
	//TODO
	// This implementation does not take care of any inconsistency,
	// it assumes are meta data from all clients are consistent
	// iterate over input and for each node, update the cluster writeview
	for(std::map<NodeId, Cluster * >::iterator nodeEntryItr = clusterInfos.begin();
			nodeEntryItr != clusterInfos.end(); ++nodeEntryItr){
		NodeId nodeId = nodeEntryItr->first;
		Cluster * nodeClusterInfo = nodeEntryItr->second;
		// TODO : use the cluster information of this node to update writeview
		std::vector<CoreShardContainer * > * nodeShardInformation =
				nodeClusterInfo->getNodeShardInformation_Writeview(nodeId);
		std::vector<CoreShardContainer * > * masterNodeShardInfo = this->getNodeShardInformation_Writeview(nodeId);
		ASSERT(masterNodeShardInfo != NULL);
		ASSERT(nodeShardInformation != NULL);
		ASSERT(masterNodeShardInfo->size() == nodeShardInformation->size());
		for(unsigned clientCIndex = 0 ; clientCIndex < nodeShardInformation->size(); ++clientCIndex){ // iterate over client core containers
			for(unsigned masterCIndex = 0 ; masterCIndex < masterNodeShardInfo->size(); ++masterCIndex){ // iterate over master core containers and find the one with similar coreName
				if(nodeShardInformation->at(clientCIndex)->getCoreName() !=
						masterNodeShardInfo->at(masterCIndex)->getCoreName()){
					continue;
				}
				// same core
				// insert primary shards and replica shards of client into master
				vector<Shard *> * masterPrimShards = masterNodeShardInfo->at(masterCIndex)->getPrimaryShards();
				ASSERT(masterPrimShards->size() == 0);
				masterNodeShardInfo->at(masterCIndex)->setPrimaryShards(*(nodeShardInformation->at(clientCIndex)->getPrimaryShards()));
				vector<Shard *> * masterReplicaShards = masterNodeShardInfo->at(masterCIndex)->getReplicaShards();
				ASSERT(masterReplicaShards->size() == 0);
				masterNodeShardInfo->at(masterCIndex)->setReplicaShards(*(nodeShardInformation->at(clientCIndex)->getReplicaShards()));
			}
		}
	}
	return true;
}

/*
	unsigned masterNodeId;
	// map which contains all shard information of all nodes
	// node -> vector<CoreShardContainer * >
	std::map<Node *, std::vector<CoreShardContainer * > > shardInformation;
	// cores in the cluster
	std::vector<CoreInfo_t *> cores;
 */

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* Cluster::serialize(void * buffer){

    buffer = srch2::util::serializeString(clusterName, buffer);
    buffer = srch2::util::serializeFixedTypes(clusterState, buffer);
    buffer = srch2::util::serializeFixedTypes(masterNodeId, buffer);
    buffer = srch2::util::serializeFixedTypes((unsigned)(shardInformation.size()), buffer);
	for(std::map<Node *, std::vector<CoreShardContainer * > >::iterator nodeEntryItr =
			shardInformation.begin(); nodeEntryItr != shardInformation.end(); ++nodeEntryItr){
		// serialize node
		buffer = nodeEntryItr->first->serializeForNetwork(buffer);
		// serialize the size of vector
	    buffer = srch2::util::serializeFixedTypes((unsigned)(nodeEntryItr->second.size()), buffer);
		for(std::vector<CoreShardContainer * >::iterator coreShardConItr =
				nodeEntryItr->second.begin(); coreShardConItr != nodeEntryItr->second.end(); ++coreShardConItr){
			buffer = (*coreShardConItr)->serializeForNetwork(buffer);
		}
	}
	return buffer;
}

//given a byte stream recreate the original object
Cluster * Cluster::deserialize(void* buffer){
	Cluster * newObj = new Cluster();
    buffer = srch2::util::deserializeString(buffer, newObj->clusterName);
    buffer = srch2::util::deserializeFixedTypes(buffer, newObj->clusterState);
    buffer = srch2::util::deserializeFixedTypes(buffer, newObj->masterNodeId);
    unsigned shardInfoSize = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, shardInfoSize);
	for(unsigned infoIndex = 0 ; infoIndex < shardInfoSize; ++infoIndex){
		// serialize node
		Node * newNode = Node::deserializeForNetwork(buffer);
		buffer = (char *)buffer + newNode->getNumberOfBytesForNetwork();
		// serialize the size of vector
		unsigned vectorSize = 0;
	    buffer = srch2::util::deserializeFixedTypes(buffer, vectorSize);
	    vector<CoreShardContainer *> cores;
		for(unsigned coreIndex = 0; coreIndex < vectorSize ; ++coreIndex){
			CoreShardContainer * newContainer = CoreShardContainer::deserializeForNetwork(buffer);
			buffer = (char *)buffer + newContainer->getNumberOfBytesForNetwork();
			cores.push_back(newContainer);
		}
		newObj->shardInformation.insert(std::make_pair(newNode, cores));
	}
	return newObj;
}

unsigned Cluster::getNumberOfBytesForNetwork(){
	// calculate number of bytes
	unsigned numberOfBytes = 0;
	// number of bytes for name
	numberOfBytes += sizeof(unsigned) + clusterName.size();
	// number of bytes ford cluster state
	numberOfBytes += sizeof(CLUSTERSTATE);
	// number of bytes for master node id
	numberOfBytes += sizeof(unsigned);
	// number of bytes for shardInformation
	// 1. number of entries in the map
	numberOfBytes += sizeof(unsigned);
	// 2. iterate on all entries and add their needed number of bytes
	for(std::map<Node *, std::vector<CoreShardContainer * > >::iterator nodeEntryItr =
			shardInformation.begin(); nodeEntryItr != shardInformation.end(); ++nodeEntryItr){
		numberOfBytes += nodeEntryItr->first->getNumberOfBytesForNetwork(); // size of node
		numberOfBytes += sizeof(unsigned); // size of coreContainer vector
		for(std::vector<CoreShardContainer * >::iterator coreShardConItr =
				nodeEntryItr->second.begin(); coreShardConItr != nodeEntryItr->second.end(); ++coreShardConItr){
			numberOfBytes += (*coreShardConItr)->getNumberOfBytesForNetwork();
		}
	}
	return numberOfBytes;
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
	return masterNodeId == nodeId;
}

bool Cluster::isMasterEligibleNode(unsigned nodeId) const{
	return getNodeById(nodeId)->isMasterEligible();
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

void Cluster::addNewNode(const Node& newNode) {

	// first check whether node is already present or not.
	unsigned scanned = 0;
	unsigned totalNodes = getTotalNumberOfNodes();
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
						nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		if(nodeInfoItr->first->getId() == newNode.getId()){
			break;
		}
		++scanned;
	}
	if (scanned == totalNodes) {
		Node * node = new Node(newNode);
		this->shardInformation[node] = std::vector<CoreShardContainer * >();
		for(std::vector<CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
			CoreShardContainer * coreShardContainer = new CoreShardContainer(*coreItr);
			this->shardInformation[node].push_back(coreShardContainer);
		}
	}
	return;
}

string Cluster::serializeClusterNodes() {
	stringstream ss;

	unsigned int size = getTotalNumberOfNodes();
	ss.write((const char *)&size, sizeof(size));
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator nodeInfoItr = this->shardInformation.begin();
					nodeInfoItr != this->shardInformation.end(); ++nodeInfoItr){
		Node *node = nodeInfoItr->first;
		string serializedNode = node->serialize();
		size = serializedNode.size();
		ss.write((const char *)&size, sizeof(size));
		ss.write(serializedNode.c_str(), size);
	}
	return ss.str();
}


void Cluster::print() const{
	Logger::console("Cluster name : %s", clusterName.c_str());
	Logger::console("Cluster master node id : %d", masterNodeId);
	Logger::console("Number of nodes : %d", shardInformation.size() );
	for(std::map<Node *, std::vector<CoreShardContainer * > >::const_iterator shardInfoItr =
			shardInformation.begin(); shardInfoItr != shardInformation.end(); ++shardInfoItr){
		Logger::console("Node %d, isMaster: %d", shardInfoItr->first->getId(), shardInfoItr->first->isMaster());
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
