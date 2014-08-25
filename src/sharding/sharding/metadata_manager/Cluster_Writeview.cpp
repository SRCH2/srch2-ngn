
#include "Cluster_Writeview.h"

#include "src/core/util/SerializationHelper.h"
#include <sstream>
#include <fstream>

#include "../ShardManager.h"
#include "ResourceLocks.h"
#include "sharding/transport/MessageAllocator.h"
#include "sharding/util/FramedPrinter.h"
#include "sharding/sharding/metadata_manager/Node.h"
#include "server/Srch2Server.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


LocalPhysicalShard::LocalPhysicalShard(){
	indexDirectory = "";
	jsonFileCompletePath = "";
}
LocalPhysicalShard::LocalPhysicalShard(boost::shared_ptr<Srch2Server> server, const string & indexDirectory, const string & jsonFileCompletePath){
	this->server = server;
	this->indexDirectory = indexDirectory;
	this->jsonFileCompletePath = jsonFileCompletePath;
}
LocalPhysicalShard::LocalPhysicalShard(const LocalPhysicalShard & copy){
	this->server = copy.server;
	this->indexDirectory = copy.indexDirectory;
	this->jsonFileCompletePath = copy.jsonFileCompletePath;
}

//void LocalPhysicalShard::setServer(boost::shared_ptr<Srch2Server> server){
//	this->server = server;
//}

void * LocalPhysicalShard::serialize(void * buffer) const{
	buffer = srch2::util::serializeString(this->indexDirectory, buffer);
	buffer = srch2::util::serializeString(this->jsonFileCompletePath, buffer);
	return buffer;
}
unsigned LocalPhysicalShard::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += sizeof(unsigned) + indexDirectory.size();
	numberOfBytes += sizeof(unsigned) + jsonFileCompletePath.size();
	return numberOfBytes;
}
void * LocalPhysicalShard::deserialize(void * buffer){
	buffer = srch2::util::deserializeString(buffer, this->indexDirectory);
	buffer = srch2::util::deserializeString(buffer, this->jsonFileCompletePath);
	return buffer;
}

ClusterShard_Writeview::ClusterShard_Writeview(	const ClusterShardId id, const ShardState & state,
		const NodeId nodeId, const bool isLocal, const double load):id(id){
	this->state = state;
	this->isLocal = isLocal;
	this->nodeId = nodeId;
	this->load;
}
ClusterShard_Writeview::ClusterShard_Writeview(const ClusterShard_Writeview & copy){
	this->state = copy.state;
	this->isLocal = copy.isLocal;
	this->nodeId = copy.nodeId;
	this->load = copy.load;
}

void * ClusterShard_Writeview::serialize(void * buffer) const{
	buffer = id.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(state, buffer);
	buffer = srch2::util::serializeFixedTypes(isLocal, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(load, buffer);
	return buffer;
}
unsigned ClusterShard_Writeview::getNumberOfBytes() const{
	unsigned numberOfBytes= 0 ;
	numberOfBytes += id.getNumberOfBytes();
	numberOfBytes += sizeof(state);
	numberOfBytes += sizeof(isLocal);
	numberOfBytes += sizeof(nodeId);
	numberOfBytes += sizeof(load);
	return numberOfBytes;
}
void * ClusterShard_Writeview::deserialize(void * buffer){
	buffer = id.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, state);
	buffer = srch2::util::deserializeFixedTypes(buffer, isLocal);
	buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, load);
	return buffer;
}

Cluster_Writeview::Cluster_Writeview(unsigned versionId, string clusterName, vector<CoreInfo_t *> cores){
	this->versionId = versionId;
	this->clusterName = clusterName;
	for(unsigned i = 0; i < cores.size(); ++i){
		if(cores.at(i) == NULL){
			continue;
		}
		this->cores[cores.at(i)->getCoreId()] = cores.at(i);
	}

	// all shards are unassigned by default
	// all partitions are unlocked by default
	// per core
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * coreInfo = coreItr->second;
		// each core has some partitions ....
		for(unsigned pid = 0; pid < coreInfo->getNumberOfPrimaryShards(); ++pid){
			for(unsigned rid = 0; rid <= coreInfo->getNumberOfReplicas(); ++rid){ // R+1 shards in total
				ClusterShardId shardId(coreId, pid, rid);
				ClusterShard_Writeview * shard = new ClusterShard_Writeview(shardId, SHARDSTATE_UNASSIGNED,
						(unsigned)-1, false, 0);
				clusterShards.push_back(shard);
				clusterShardIdIndexes[shardId] = clusterShards.size()-1;
			}
		}
	}
	// this node id will change by discovery module later ...
	this->currentNodeId = (unsigned)-1 ;
}

Cluster_Writeview::Cluster_Writeview(const Cluster_Writeview & copy){


	this->clusterName = copy.clusterName;
	this->currentNodeId = copy.currentNodeId;
	this->versionId = copy.versionId;

	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = copy.nodes.begin();
			nodeItr != copy.nodes.end(); ++nodeItr){
		if(nodeItr->second.second != NULL){
			this->nodes[nodeItr->first] = std::make_pair(nodeItr->second.first, new Node(*(nodeItr->second.second)) );
		}else{
			this->nodes[nodeItr->first] = std::make_pair(nodeItr->second.first, (Node *) NULL);
		}
	}
	this->cores = copy.cores;

	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = new ClusterShard_Writeview(*(copy.clusterShards.at(i)));
		this->clusterShards.push_back(shard);
	}

	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		NodeShard_Writeview * shard = new NodeShard_Writeview( *(copy.nodeShards.at(i)));
		this->nodeShards.push_back(shard);
	}
	this->clusterShards = copy.clusterShards;
	this->clusterShardIdIndexes = copy.clusterShardIdIndexes;
	this->nodeShards = copy.nodeShards;
	// Contains an entry for each one of those cluster shards that exist on this node
	// with server information.
	// ShardId on this node => server info
	this->localClusterDataShards = copy.localClusterDataShards;

	// Server information for those independent shards that are on the current node
	// NodeShardId{currentNodeId, coreId, int-partitionId} => server information
	// internal partition id
	this->localNodeDataShards = copy.localNodeDataShards;
}
Cluster_Writeview::Cluster_Writeview(){}

bool Cluster_Writeview::isEqualDiscardingLocalShards(const Cluster_Writeview & right){
	if(clusterName.compare(right.clusterName) != 0){
		return false;
	}
	if(currentNodeId != right.currentNodeId){
		return false;
	}
	if(versionId != right.versionId){
		return false;
	}
	if(clusterShardIdIndexes != right.clusterShardIdIndexes){
		return false;
	}
	if(clusterShards.size() != right.clusterShards.size()){
		return false;
	}
	if(nodeShards.size() != right.nodeShards.size()){
		return false;
	}
	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		if(! (*(clusterShards.at(i)) == *(right.clusterShards.at(i)))){
			return false;
		}
	}
	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		if(! (*(nodeShards.at(i)) == *(right.nodeShards.at(i)))){
			return false;
		}
	}
	return true;
}

bool Cluster_Writeview::operator==(const Cluster_Writeview & right){

	if(! isEqualDiscardingLocalShards(right)){
		return false;
	}
	if(localClusterDataShards != right.localClusterDataShards){
		return false;
	}
	if(localNodeDataShards != right.localNodeDataShards){
		return false;
	}

	return true;
}

Cluster_Writeview::~Cluster_Writeview(){
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		if(nodeItr->second.second != NULL){
			delete nodeItr->second.second;
		}
	}

	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = clusterShards.at(i);
		delete shard;
	}

	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		NodeShard_Writeview * shard = nodeShards.at(i);
		delete shard;
	}
}


void Cluster_Writeview::print(){

	vector<string> coreHeaders;
	for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		stringstream ss;
		ss << "ID:" << coreItr->first << "%";
		ss << "Name:" << coreItr->second->getName() ;
		coreHeaders.push_back(ss.str());
	}
	vector<string> coreLabels;
	srch2::util::TableFormatPrinter coresTable("Cores", 40, coreHeaders, coreLabels );
	coresTable.printColumnHeaders();

	printNodes();

	/// cluster shards

	ClusterShardId id;double load;ShardState state;bool isLocal;NodeId nodeId;NodeShardId nodeShardId;
	if(clusterShards.size() == 0){
		cout << "cluster shards : empty." << endl;
	}else{
		for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
			vector<string> clusterHeaders;
			vector<string> clusterLabels;
			unsigned counter = 0;
			while(counter++ < coreItr->second->getNumberOfPrimaryShards()){
				stringstream ss;
				ss << "Partition#" << counter-1;
				clusterLabels.push_back(ss.str());
			}
			counter = 0;
			while(counter++ < coreItr->second->getNumberOfReplicas() + 1){
				stringstream ss;
				ss << "Replica#" << counter-1;
				clusterHeaders.push_back(ss.str());
			}
			srch2::util::TableFormatPrinter clusterShardsTable("Core name : " + coreItr->second->getName(), 120, clusterHeaders, clusterLabels);
			clusterShardsTable.printColumnHeaders();
			clusterShardsTable.startFilling();
			beginClusterShardsIteration();
			while(getNextClusterShard(id, load, state, isLocal, nodeId)){
				if(id.coreId != coreItr->first){
					continue;
				}
				stringstream ss;
				if(isLocal){
					ss << "********%";
				}
				ss << "Shard ID : " << id.toString() << "%";
				ss << "Node Location : " << nodeId << "%";
				ss << "Load: " << load << "%";
				switch(state){
				case SHARDSTATE_UNASSIGNED:
					ss << "State : UNASSIGNED" << "%";
					break;
				case SHARDSTATE_PENDING:
					ss << "State : PENDING" << "%";
					break;
				case SHARDSTATE_READY:
					ss << "State : READY" << "%";
					break;
				}
				clusterShardsTable.printNextCell(ss.str());

			}
		}
	}



	///// node shards
	if(nodeShards.size() == 0){
		cout << "Node shards : empty." << endl;
	}else{
		vector<string> nodeShardHeaders;
		vector<string> nodeShardLabels;
		unsigned counter = 0;
		while(counter++ < nodeShards.size()){
			stringstream ss;
			ss << "ID : " << nodeShards.at(counter-1)->id.toString();
			nodeShardHeaders.push_back(ss.str());
		}

		nodeShardLabels.push_back("Node Shard Information:");
		srch2::util::TableFormatPrinter nodeShardsTable("Node Shards :" , 120, nodeShardHeaders, nodeShardLabels);
		nodeShardsTable.printColumnHeaders();
		nodeShardsTable.startFilling();
		beginNodeShardsIteration();
		while(getNextNodeShard(nodeShardId,isLocal)){
			stringstream ss;
			if(isLocal){
				ss << "********%";
			}
			ss << "Node Location : " << nodeShardId.nodeId ;

			nodeShardsTable.printNextCell(ss.str());
		}
	}


	if(localClusterDataShards.size() == 0){
		cout << "Local cluster shards : empty." << endl;
	}else{
		//////// Cluster physical shards
		vector<string> localClusterShardHeaders;
		vector<string> localClusterShardLabels;
		localClusterShardLabels.push_back("Shard info:");
		for(map<ClusterShardId, LocalPhysicalShard >::const_iterator shardItr = localClusterDataShards.begin(); shardItr != localClusterDataShards.end(); ++shardItr){
			localClusterShardHeaders.push_back(shardItr->first.toString());
		}
		srch2::util::TableFormatPrinter localClusterShardsTable("Cluster local shards :" , 120, localClusterShardHeaders, localClusterShardLabels);
		localClusterShardsTable.printColumnHeaders();
		localClusterShardsTable.startFilling();
		for(map<ClusterShardId, LocalPhysicalShard >::const_iterator shardItr = localClusterDataShards.begin(); shardItr != localClusterDataShards.end(); ++shardItr){
			localClusterShardsTable.printNextCell(shardItr->second.indexDirectory);
		}
	}



	if(localNodeDataShards.size() == 0){
		cout << "Local node shards : empty." << endl;
	}else{
		//////// Node physical shards
		vector<string> localNodeShardHeaders;
		vector<string> localNodeShardLabels;
		localNodeShardLabels.push_back("Shard info :");
		for(map<unsigned,  LocalPhysicalShard >::const_iterator shardItr = localNodeDataShards.begin(); shardItr != localNodeDataShards.end(); ++shardItr){
			stringstream ss;
			ss << "Internal PID : " << shardItr->first;
			localNodeShardHeaders.push_back(ss.str());
		}
		srch2::util::TableFormatPrinter localNodeShardsTable("Node local shards :" , 120, localNodeShardHeaders, localNodeShardLabels);
		localNodeShardsTable.printColumnHeaders();
		localNodeShardsTable.startFilling();
		for(map<unsigned,  LocalPhysicalShard >::const_iterator shardItr = localNodeDataShards.begin(); shardItr != localNodeDataShards.end(); ++shardItr){
			stringstream ss;
			ss << "Index size : " ;
			if(shardItr->second.server){

				ss << shardItr->second.server->getIndexer()->getNumberOfDocumentsInIndex();
			}
			localNodeShardsTable.printNextCell(shardItr->second.indexDirectory + "%" + ss.str());
		}
	}

}

void Cluster_Writeview::printNodes(){

	/// nodes
	if(nodes.size() == 0){
		cout << "Nodes : empty." << endl;
	}else{
		vector<string> nodeHeaders;
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
			stringstream ss;
			ss << "ID-" << nodeItr->first << "%";
			nodeHeaders.push_back(ss.str());
		}
		vector<string> nodeLables;
		nodeLables.push_back("Node Information:");
		srch2::util::TableFormatPrinter nodesTable("Nodes", 120, nodeHeaders, nodeLables );
		nodesTable.printColumnHeaders();
		nodesTable.startFilling();
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
			stringstream ss;
			switch (nodeItr->second.first) {
				case ShardingNodeStateNotArrived:
					ss << "STATE-NOT-ARRIVED" << "%";
					break;
				case ShardingNodeStateArrived:
					ss << "STATE-ARRIVED" << "%";
					break;
				case ShardingNodeStateFailed:
					ss << "STATE-FAILED" << "%";
					break;
			}

			if(nodeItr->second.second != NULL){
				ss << nodeItr->second.second->toString();
			}
			nodesTable.printNextCell(ss.str());
		}
	}
}

void Cluster_Writeview::getArrivedNodes(vector<NodeId> & arrivedNodes, bool returnThisNode) const{
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		if(! returnThisNode){
			if(nodeItr->first == currentNodeId){
				continue;
			}
		}
		if(nodeItr->second.first == ShardingNodeStateArrived && nodeItr->second.second != NULL){
            arrivedNodes.push_back(nodeItr->first);
		}
	}
}

void Cluster_Writeview::getAllNodes(std::vector<const Node *> & localCopy) const{
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		localCopy.push_back(nodeItr->second.second);
	}
}

void Cluster_Writeview::addNode(Node * node){

    if(node == NULL){
        ASSERT(false);
        return;
    }
	if(nodes.find(node->getId()) == nodes.end()){ // new node.
		nodes[node->getId()] = std::make_pair(ShardingNodeStateNotArrived, node);
	}else{
        nodes[node->getId()].second = node;
	}
}

void Cluster_Writeview::setNodeState(NodeId nodeId, ShardingNodeState state){

    if(nodes.find(nodeId) == nodes.end()){ // new node.
        nodes[nodeId] = std::make_pair(state, (Node*)NULL);
    }else{
        nodes[nodeId].first = state;
    }
}

void Cluster_Writeview::removeNode(const NodeId & failedNodeId){

	// node part
	if(this->nodes.find(failedNodeId) == this->nodes.end()){
		this->nodes[failedNodeId] = std::make_pair(ShardingNodeStateFailed,(Node *) NULL);
	}else{
		this->nodes[failedNodeId].first = ShardingNodeStateFailed;
		if(this->nodes[failedNodeId].second != NULL){
			delete this->nodes[failedNodeId].second;
			this->nodes[failedNodeId].second = NULL;
		}
	}

	// cluster shard ids
	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		ClusterShard_Writeview * clusterShard = clusterShards.at(i);
		if(clusterShard->nodeId == failedNodeId && clusterShard->state == SHARDSTATE_READY){
			clusterShard->state = SHARDSTATE_UNASSIGNED;
			clusterShard->isLocal = false;
			clusterShard->load = 0;
			clusterShard->nodeId = (unsigned)-1;
		}
	}

	// node shards
	vector<NodeShard_Writeview *> nodeShardsNewCopy;
	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		NodeShard_Writeview * nodeShard = nodeShards.at(i);
		if(nodeShard->id.nodeId == failedNodeId){
			// we delete this node id.
			delete nodeShard;
		}else{
			nodeShardsNewCopy.push_back(nodeShard);
		}
	}
	nodeShards = nodeShardsNewCopy;

}

void Cluster_Writeview::setCurrentNodeId(NodeId currentNodeId){
	this->currentNodeId = currentNodeId;

	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = clusterShards.at(i);
		if(shard->isLocal){
			shard->nodeId = currentNodeId;
		}
	}

	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		NodeShard_Writeview * shard = nodeShards.at(i);
		if(shard->isLocal){
			shard->id.nodeId = currentNodeId;
		}
	}
}

void Cluster_Writeview::fixAfterDiskLoad(Cluster_Writeview * oldWrireview){
	// we should make sure all cluster shard ids are either local,
	// or we change them to PENDING
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	beginClusterShardsIteration();
	while(getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state == SHARDSTATE_UNASSIGNED){
			continue;
		}
		ClusterShard_Writeview * shard = this->clusterShards.at(this->clusterShardsCursor-1);
		if(! isLocal ){
			shard->state = SHARDSTATE_PENDING;
			shard->load = 0;
			shard->nodeId = (unsigned)-1;
		}else{
			shard->nodeId = oldWrireview->currentNodeId;
		}
	}

	// and remove those node shards that are not ours
	beginNodeShardsIteration();
	vector<NodeShard_Writeview *> fixedNodeShards;
	while(getNextNodeShard(nodeShardId, isLocal)){
		NodeShard_Writeview * shard = this->nodeShards.at(this->nodeShardsCursor-1);
		if(! isLocal){
			delete shard;
		}else{
			shard->id.nodeId = oldWrireview->currentNodeId;
			fixedNodeShards.push_back(shard);
		}
	}
	this->nodeShards = fixedNodeShards;

	this->currentNodeId = oldWrireview->currentNodeId;
	// and cores ...
	this->cores = oldWrireview->cores;

}

void Cluster_Writeview::saveWriteviewOnDisk(string absDirectoryPath){
	  ofstream outfile;
	  outfile.open((absDirectoryPath + "/Cluster.idx").c_str(), ios::binary | ios::out);
	  if(! outfile.is_open()){
		  return;
	  }
	  unsigned numberOfBytes = getNumberOfBytes();
	  outfile.write((char *)&numberOfBytes, sizeof(unsigned));
	  MessageAllocator ma;
	  void * serializedCluster = ma.allocateByteArray(numberOfBytes);
	  serialize(serializedCluster);
	  outfile.write((char *)serializedCluster,numberOfBytes);
	  ma.deallocateByreArray(serializedCluster, numberOfBytes);
	  outfile.close();

}

Cluster_Writeview * Cluster_Writeview::loadWriteviewFromDisk(string absDirectoryPath){

	Cluster_Writeview * writeview = new Cluster_Writeview();
	ifstream infile;
	infile.open((absDirectoryPath + "/Cluster.idx").c_str(), ios::binary | ios::in);
	if(! infile.is_open()){
		delete writeview;
		return NULL;
	}
	unsigned numberOfBytes = 0;
	infile.read((char *)&numberOfBytes, sizeof(unsigned));
	MessageAllocator ma;
	void * serializedCluster = ma.allocateByteArray(numberOfBytes);
	infile.read((char *)serializedCluster, numberOfBytes);
	writeview->deserialize(serializedCluster);
	ma.deallocateByreArray(serializedCluster, numberOfBytes);
	infile.close();
	return writeview;
}

void * Cluster_Writeview::serialize(void * buffer, bool includeLocalInfoFlag) const{
	buffer = srch2::util::serializeString(clusterName, buffer);
	buffer = srch2::util::serializeFixedTypes(currentNodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(versionId, buffer);
	if(includeLocalInfoFlag){
		buffer = srch2::util::serializeMapDynamicToDynamic(localClusterDataShards, buffer);
		buffer = srch2::util::serializeMapFixedToDynamic(localNodeDataShards, buffer);
	}
	buffer = srch2::util::serializeVectorOfDynamicTypePointers(clusterShards, buffer);
	buffer = srch2::util::serializeMapDynamicToFixed(clusterShardIdIndexes, buffer);
	buffer = srch2::util::serializeVectorOfDynamicTypePointers(nodeShards, buffer);
	return buffer;
}
unsigned Cluster_Writeview::getNumberOfBytes(bool includeLocalInfoFlag) const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned) + clusterName.size();
	numberOfBytes += sizeof(currentNodeId);
	numberOfBytes += sizeof(versionId);
	if(includeLocalInfoFlag){
		numberOfBytes += srch2::util::getNumberOfBytesMapDynamicToDynamic(localClusterDataShards);
		numberOfBytes += srch2::util::getNumberOfBytesMapFixedToDynamic(localNodeDataShards);
	}
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypePointers(clusterShards);
	numberOfBytes += srch2::util::getNumberOfBytesMapDynamicToFixed(clusterShardIdIndexes);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypePointers(nodeShards);
	return numberOfBytes;
}
void * Cluster_Writeview::deserialize(void * buffer, bool includeLocalInfoFlag ){
	buffer = srch2::util::deserializeString(buffer, clusterName);
	buffer = srch2::util::deserializeFixedTypes(buffer, currentNodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, versionId);
	if(includeLocalInfoFlag){
		buffer = srch2::util::deserializeMapDynamicToDynamic(buffer, localClusterDataShards);
		buffer = srch2::util::deserializeMapFixedToDynamic(buffer, localNodeDataShards);
	}
	buffer = srch2::util::deserializeVectorOfDynamicTypePointers(buffer, clusterShards);
	buffer = srch2::util::deserializeMapDynamicToFixed(buffer, clusterShardIdIndexes);
	buffer = srch2::util::deserializeVectorOfDynamicTypePointers(buffer, nodeShards);
	return buffer;
}

// cluster shard id => array index
unsigned Cluster_Writeview::INDEX(const ClusterShardId & shardId){
	if(clusterShardIdIndexes.find(shardId) == clusterShardIdIndexes.end()){
		ASSERT(false);
		return 0;
	}
	return clusterShardIdIndexes[shardId];
}


void Cluster_Writeview::assignLocalClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShardInfo){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	ASSERT(shard->id == shardId);
	shard->isLocal = true;
	shard->nodeId = this->currentNodeId;
	shard->load = 0;
	ASSERT(shard->state == SHARDSTATE_UNASSIGNED);
	shard->state = SHARDSTATE_READY;
	localClusterDataShards[shardId] = physicalShardInfo;
}

void Cluster_Writeview::setClusterShardServer(const ClusterShardId & shardId, boost::shared_ptr<Srch2Server> server){
	ASSERT(localClusterDataShards.find(shardId) != localClusterDataShards.end());
	localClusterDataShards.find(shardId)->second.server = server;
}

void Cluster_Writeview::unassignClusterShard(const ClusterShardId & shardId){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	shard->state = SHARDSTATE_UNASSIGNED;
	if(shard->isLocal){
		this->localClusterDataShards.erase(shardId);
	}
}

void Cluster_Writeview::assignExternalClusterShard(const ClusterShardId & shardId, const NodeId & nodeId, const double & load){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	ASSERT(shard->id == shardId);
	shard->isLocal = false;
	shard->nodeId = nodeId;
	shard->load = 0;
	shard->state = SHARDSTATE_READY;

	if(this->localClusterDataShards.find(shardId) != this->localClusterDataShards.end()){
		this->localClusterDataShards.erase(shardId);
	}
}

void Cluster_Writeview::moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId){
	moveClusterShard(shardId, destNodeId, LocalPhysicalShard());
}

void Cluster_Writeview::moveClusterShard(const ClusterShardId & shardId, const LocalPhysicalShard & physicalShard){
	moveClusterShard(shardId, ShardManager::getCurrentNodeId(), physicalShard);
}

void Cluster_Writeview::moveClusterShard(const ClusterShardId & shardId, const NodeId & destNodeId, const LocalPhysicalShard & physicalShardInfo){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	if(shard->isLocal){
		shard->isLocal = false;
		shard->nodeId = destNodeId;
		ASSERT(shard->state == SHARDSTATE_READY);
		// erase the physical shard
		localClusterDataShards.erase(shardId);
	}else{
		shard->isLocal = true;
		shard->nodeId = destNodeId;
		ASSERT(shard->state == SHARDSTATE_READY);
		localClusterDataShards[shardId] = physicalShardInfo;
	}
}

void Cluster_Writeview::beginClusterShardsIteration(){
	this->clusterShardsCursor = 0;
}
bool Cluster_Writeview::getNextClusterShard(ClusterShardId & shardId, double & load, ShardState & state, bool & isLocal, NodeId & nodeId){
	if(this->clusterShardsCursor >= this->clusterShards.size()){
		return false;
	}
	ClusterShard_Writeview * shard = this->clusterShards[this->clusterShardsCursor++];
	shardId = shard->id;
	// TODO : loads are not handled yet
	load = 1;
	state = shard->state;
	isLocal = shard->isLocal;
	nodeId = shard->nodeId;
	return true;
}
bool Cluster_Writeview::getNextLocalClusterShard(ClusterShardId & shardId, double & load, LocalPhysicalShard & localPhysicalShard ){
	while(true){
		if(this->clusterShardsCursor >= this->clusterShards.size()){
			return false;
		}
		ClusterShard_Writeview * shard = this->clusterShards[this->clusterShardsCursor++];
		if(! shard->isLocal){
			continue;
		}
		shardId = shard->id;
		load = shard->load;
		localPhysicalShard = this->localClusterDataShards[shardId];
		return true;
	}
	return false;
}

void Cluster_Writeview::addLocalNodeShard(const NodeShardId & nodeShardId, const double load, const LocalPhysicalShard & physicalShardInfo){
    ASSERT(this->currentNodeId == nodeShardId.nodeId);
	ASSERT(this->localNodeDataShards.find(nodeShardId.partitionId) == this->localNodeDataShards.end());
    for(unsigned i = 0 ; i < this->nodeShards.size(); ++i){
        if(this->nodeShards.at(i)->id == nodeShardId){
            return;
        }
    }
	this->nodeShards.push_back(new NodeShard_Writeview(nodeShardId, true, load));
	this->localNodeDataShards[nodeShardId.partitionId] = physicalShardInfo;
}

void Cluster_Writeview::addExternalNodeShard(const NodeShardId & nodeShardId, const double load){
    ASSERT(this->currentNodeId != nodeShardId.nodeId);
    for(unsigned i = 0 ; i < this->nodeShards.size(); ++i){
        if(this->nodeShards.at(i)->id == nodeShardId){
            return;
        }
    }
	this->nodeShards.push_back(new NodeShard_Writeview(nodeShardId, false, load));
}

void Cluster_Writeview::removeNodeShard(const NodeShardId & nodeShardId){
	vector<NodeShard_Writeview *> newNodeShards;
	for(unsigned i = 0; i < nodeShards.size(); ++i){
		if(nodeShards.at(i)->id == nodeShardId){
			delete nodeShards.at(i);
			localNodeDataShards.erase(nodeShardId.partitionId);
		}else{
			newNodeShards.push_back(nodeShards.at(i));
		}
	}
	nodeShards = newNodeShards;
}

void Cluster_Writeview::setNodeShardServer(const NodeShardId & nodeShardId, boost::shared_ptr<Srch2Server> server){
	ASSERT(this->localNodeDataShards.find(nodeShardId.partitionId) != this->localNodeDataShards.end());
	this->localNodeDataShards.find(nodeShardId.partitionId)->second.server = server;
}

void Cluster_Writeview::beginNodeShardsIteration(){
	this->nodeShardsCursor = 0;
}
bool Cluster_Writeview::getNextNodeShard(NodeShardId & nodeShardId, bool & isLocal){
	if(this->nodeShardsCursor >= this->nodeShards.size()){
		return false;
	}
	NodeShard_Writeview * shard = this->nodeShards[this->nodeShardsCursor++];
	nodeShardId = shard->id;
	isLocal = shard->isLocal;
	return true;
}
bool Cluster_Writeview::getNextLocalNodeShard(NodeShardId & nodeShardId, double & load,  LocalPhysicalShard & dataInfo){
	while(true){
		if(this->nodeShardsCursor >= this->nodeShards.size()){
			return false;
		}
		NodeShard_Writeview * shard = this->nodeShards[this->nodeShardsCursor++];
		if(! shard->isLocal){
			continue;
		}
		nodeShardId = shard->id;
		load = shard->load;
		dataInfo = this->localNodeDataShards[nodeShardId.partitionId];
		return true;
	}
	return false;
}


void Cluster_Writeview::getFullUnassignedPartitions(vector<ClusterPID> & fullUnassignedPartitions ){

	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	map<ClusterPID, bool> isFullUnassigned;

	beginClusterShardsIteration();
	while(getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state == SHARDSTATE_UNASSIGNED){
			if(isFullUnassigned.find(id.getPartitionId()) == isFullUnassigned.end()){
				isFullUnassigned[id.getPartitionId()] = true;
			}
		}else{ // pending and ready
			isFullUnassigned[id.getPartitionId()] = false;
		}
	}
	for(map<ClusterPID, bool>::iterator pidItr = isFullUnassigned.begin(); pidItr != isFullUnassigned.end(); ++pidItr){
		if(pidItr->second){
			fullUnassignedPartitions.push_back(pidItr->first);
		}
	}
}

bool Cluster_Writeview::isPartitionPending(const ClusterPID & pid){

	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	beginClusterShardsIteration();
	while(getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(id.getPartitionId() == pid && state == SHARDSTATE_PENDING){
			return true;
		}
	}
	return false;
}

// Reviewed. Don't touch.
void Cluster_Writeview::fixClusterMetadataOfAnotherNode(Cluster_Writeview * cluster){
	if(cluster == NULL){
		ASSERT(false);
		return;
	}
	ASSERT(cluster->localClusterDataShards.empty());
	ASSERT(cluster->localNodeDataShards.empty());
	// 1. isLocal values are from the perspective of src node of this "cluster" info
	for(unsigned i = 0 ; i < cluster->clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = cluster->clusterShards.at(i);
		if(shard->state != SHARDSTATE_READY){
			shard->isLocal = false;
			shard->load = 0 ;
			shard->nodeId = 0;
			continue;
		}
		if(shard->nodeId != this->currentNodeId){
			shard->isLocal = false;
		}else{
			shard->isLocal = true;
		}
	}

	for(unsigned i = 0 ; i < cluster->nodeShards.size(); ++i){
		NodeShard_Writeview * shard = cluster->nodeShards.at(i);
		if(shard->id.nodeId != this->currentNodeId){
			shard->isLocal = false;
		}else{
			shard->isLocal = true;
		}
	}


	// 2. for every local cluster shard, if this shard is PENDING or UNASSIGNED, add it from our writeview
	for(unsigned i = 0 ; i < clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = clusterShards.at(i);
		if(shard->isLocal){
			bool succeed = false;
			for(unsigned j = 0 ; j < cluster->clusterShards.size(); ++j){
				ClusterShard_Writeview * clusterShard = cluster->clusterShards.at(j);
				if(clusterShard->id != shard->id){
					continue;
				}
				if(clusterShard->state == SHARDSTATE_PENDING || clusterShard->state == SHARDSTATE_UNASSIGNED){
					clusterShard->isLocal = true;
					clusterShard->load = shard->load;
					clusterShard->state = SHARDSTATE_READY;
					clusterShard->nodeId = shard->nodeId;
					ASSERT(shard->nodeId == currentNodeId);
					succeed = true;
					break;
				}else{
					ASSERT(false);// somebody else has this shard
					break;
				}
			}
			if(succeed){
				cluster->localClusterDataShards[shard->id] = localClusterDataShards[shard->id];
			}
		}
	}

	// 3. for every local node shard, just add it to the cluster info
	for(unsigned i = 0 ; i < nodeShards.size(); ++i){
		NodeShard_Writeview * shard = nodeShards.at(i);
		if(shard->isLocal){
			cluster->nodeShards.push_back(new NodeShard_Writeview(*shard));
			cluster->localNodeDataShards[shard->id.partitionId] = localNodeDataShards[shard->id.partitionId];
		}
	}

	// 4.
	cluster->currentNodeId = currentNodeId;
	// 5.
	cluster->nodes = nodes;
	cluster->cores = cores;
}

ClusterResourceMetadata_Readview * Cluster_Writeview::getNewReadview(){
	// we need cores for construction
	vector<CoreInfo_t *> coresVector;
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		coresVector.push_back(coreItr->second);
	}

	ClusterResourceMetadata_Readview * newReadview = new ClusterResourceMetadata_Readview(++versionId, clusterName, coresVector);
	newReadview->setCurrentNodeId(currentNodeId);

	// add nodes
	for(map<NodeId, std::pair<ShardingNodeState, Node *> > ::iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
		if(nodeItr->second.first != ShardingNodeStateFailed){
			newReadview->addNode(*(nodeItr->second.second));
		}
	}

	// per core ...
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		CoreInfo_t * core = coreItr->second;
		// global parts
		CorePartitionContianer * corePartitionContainer = newReadview->getCorePartitionContianer(coreId);
		if(corePartitionContainer == NULL){
			ASSERT(false);
			return NULL;
		}
		ClusterShardId id;
		NodeShardId nodeShardId;
		ShardState state;
		bool isLocal;
		NodeId nodeId;
		LocalPhysicalShard physicalShard;
		double load;

		beginClusterShardsIteration();
		while(getNextClusterShard(id, load, state, isLocal, nodeId)){
			if(id.coreId == coreId && state == SHARDSTATE_READY){
				corePartitionContainer->addClusterShard(nodeId, id);
			}
		}

		beginNodeShardsIteration();
		while(getNextNodeShard(nodeShardId, isLocal)){
			corePartitionContainer->addNodeShard(nodeShardId.nodeId, nodeShardId.partitionId);
		}

		// set locks of partitions of this core
		vector<ClusterShardId> lockedPartitions;
		ShardManager::getShardManager()->getLockManager()->getLockedPartitions(lockedPartitions);
		for(unsigned i = 0 ; i < lockedPartitions.size(); i++){
			if(lockedPartitions.at(i).coreId == coreId){
				corePartitionContainer->setPartitionLock(lockedPartitions.at(i).partitionId, PartitionLock_Locked);
			}
		}
		beginClusterShardsIteration();
		while(getNextClusterShard(id, load, state, isLocal, nodeId)){
			if(id.coreId == coreId && state == SHARDSTATE_PENDING){
				corePartitionContainer->setPartitionLock(id.partitionId, PartitionLock_Locked);
			}
		}

		// local parts
		LocalShardContainer * localShardContainer = newReadview->getLocalShardContainer(coreId);
		if(localShardContainer == NULL){
			ASSERT(false);
			return NULL;
		}

		beginClusterShardsIteration();
		while(getNextLocalClusterShard(id, load,  physicalShard)){
			if(id.coreId == coreId){
				localShardContainer->addClusterShard(id, physicalShard.server,
						physicalShard.indexDirectory,  physicalShard.jsonFileCompletePath, load);
			}
		}

		beginNodeShardsIteration();
		while(getNextLocalNodeShard(nodeShardId, load,  physicalShard)){
			if(nodeShardId.coreId == coreId){
				localShardContainer->addNodeShard(nodeShardId, physicalShard.server,
						physicalShard.indexDirectory,  physicalShard.jsonFileCompletePath, load);
			}
		}
	}

	return newReadview;
}








}
}
