
#include "Cluster_Writeview.h"


#include "../ShardManager.h"
#include "../../transport/MessageAllocator.h"
#include "../../util/FramedPrinter.h"
#include "../../sharding/metadata_manager/Node.h"
#include "../lock_manager/LockManager.h"
#include "core/util/SerializationHelper.h"
#include "server/Srch2Server.h"
#include "server/HTTPJsonResponse.h"

#include <sstream>
#include <fstream>
using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


LocalPhysicalShard::LocalPhysicalShard(){
	indexDirectory = "";
	jsonFileCompletePath = "";
	this->server.reset();
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

LocalPhysicalShard & LocalPhysicalShard::operator=(const LocalPhysicalShard & rhs){
	if(this != &rhs){
		indexDirectory = rhs.indexDirectory;
		jsonFileCompletePath = rhs.jsonFileCompletePath;
		server = rhs.server;
	}
	return *this;
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
		const NodeId nodeId, const bool isLocal, const double load){
    this->id = id;
	this->state = state;
	this->isLocal = isLocal;
	this->nodeId = nodeId;
	this->load = load;
}
ClusterShard_Writeview::ClusterShard_Writeview(const ClusterShard_Writeview & copy){
    this->id = copy.id;
	this->state = copy.state;
	this->isLocal = copy.isLocal;
	this->nodeId = copy.nodeId;
	this->load = copy.load;
}

ClusterShard_Writeview::ClusterShard_Writeview(){
    this->id = ClusterShardId(0,0,0);
    this->state = SHARDSTATE_UNASSIGNED;
    this->isLocal = false;
    this->nodeId = 0;
    this->load = 0;
}

void * ClusterShard_Writeview::serialize(void * buffer) const{
	buffer = id.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(this->state, buffer);
	buffer = srch2::util::serializeFixedTypes(this->isLocal, buffer);
	buffer = srch2::util::serializeFixedTypes(this->nodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(this->load, buffer);
	return buffer;
}
unsigned ClusterShard_Writeview::getNumberOfBytes() const{
	unsigned numberOfBytes= 0 ;
	numberOfBytes += this->id.getNumberOfBytes();
	numberOfBytes += sizeof(this->state);
	numberOfBytes += sizeof(this->isLocal);
	numberOfBytes += sizeof(this->nodeId);
	numberOfBytes += sizeof(this->load);
	return numberOfBytes;
}
void * ClusterShard_Writeview::deserialize(void * buffer){
	buffer = this->id.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, this->state);
	buffer = srch2::util::deserializeFixedTypes(buffer, this->isLocal);
	buffer = srch2::util::deserializeFixedTypes(buffer, this->nodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, this->load);
	return buffer;
}


void ClusterShardIterator::beginClusterShardsIteration(){
    this->clusterShardsCursor = 0;
}
bool ClusterShardIterator::getNextClusterShard(ClusterShardId & shardId, double & load, ShardState & state, bool & isLocal, NodeId & nodeId){
    if(this->clusterShardsCursor >= writeview->clusterShards.size()){
        return false;
    }
    const ClusterShard_Writeview * shard = writeview->clusterShards.at(this->clusterShardsCursor++);
    shardId = shard->id;
    // TODO : loads are not handled yet
    load = 1;
    state = shard->state;
    isLocal = shard->isLocal;
    nodeId = shard->nodeId;
    return true;
}
bool ClusterShardIterator::getNextLocalClusterShard(ClusterShardId & shardId, double & load,
		LocalPhysicalShard & localPhysicalShard ){
    while(true){
        if(this->clusterShardsCursor >= writeview->clusterShards.size()){
            return false;
        }
        const ClusterShard_Writeview * shard = writeview->clusterShards.at(this->clusterShardsCursor++);
        if(! shard->isLocal){
            continue;
        }
        shardId = shard->id;
        load = shard->load;
        localPhysicalShard = writeview->localClusterDataShards.at(shardId);
        return true;
    }
    return false;
}

void NodeShardIterator::beginNodeShardsIteration(){
    this->nodeShardsCursor = 0;
}
bool NodeShardIterator::getNextNodeShard(NodeShardId & nodeShardId, bool & isLocal){
    if(this->nodeShardsCursor >= writeview->nodeShards.size()){
        return false;
    }
    const NodeShard_Writeview * shard = writeview->nodeShards.at(this->nodeShardsCursor++);
    nodeShardId = shard->id;
    isLocal = shard->isLocal;
    return true;
}
bool NodeShardIterator::getNextLocalNodeShard(NodeShardId & nodeShardId, double & load,  LocalPhysicalShard & dataInfo){
    while(true){
        if(this->nodeShardsCursor >= writeview->nodeShards.size()){
            return false;
        }
        const NodeShard_Writeview * shard = writeview->nodeShards.at(this->nodeShardsCursor++);
        if(! shard->isLocal){
            continue;
        }
        nodeShardId = shard->id;
        load = shard->load;
        dataInfo = writeview->localNodeDataShards.at(nodeShardId.partitionId);
        return true;
    }
    return false;
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
	this->currentNodeId = 0 ;
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

	for(unsigned i = 0 ; i < copy.clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = new ClusterShard_Writeview(*(copy.clusterShards.at(i)));
		this->clusterShards.push_back(shard);
	}

	for(unsigned i = 0 ; i < copy.nodeShards.size(); ++i){
		NodeShard_Writeview * shard = new NodeShard_Writeview( *(copy.nodeShards.at(i)));
		this->nodeShards.push_back(shard);
	}

	this->clusterShardsCursor = copy.clusterShardsCursor;
	this->clusterShardIdIndexes = copy.clusterShardIdIndexes;
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


void Cluster_Writeview::print(JsonResponseHandler * response) const{

	printCores(response);

	printNodes(response);

	/// cluster shards

	printClusterShards(response);



	///// node shards
	printNodeShards(response);


	printLocalShards(response);

}

void Cluster_Writeview::printCores(JsonResponseHandler * response) const{
	if(response != NULL){
		Json::Value coresJsonValue(Json::arrayValue);
		unsigned coreIdx = 0 ;
		for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
			Json::Value coreJsonValue(Json::objectValue);
			coreJsonValue["ID"] = coreItr->first;
			coreJsonValue["Name"] = coreItr->second->getName();
			coreJsonValue["distributed"] = coreItr->second->isDistributedCore() ? "distributed" : "node-core";
			if(coreItr->second->isDistributedCore()){
				coreJsonValue["#primary-shards"] = coreItr->second->getNumberOfPrimaryShards();
				coreJsonValue["#replicas"] = coreItr->second->getNumberOfReplicas();
			}
			//
			coresJsonValue[coreIdx++] = coreJsonValue;
		}
		response->setResponseAttribute("cores-info" , coresJsonValue);
		return;
	}
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
}

void Cluster_Writeview::printNodes(JsonResponseHandler * response) const{

	if(response != NULL){
		Json::Value nodesJsonInfo(Json::arrayValue);
		unsigned nodeIdx = 0;
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr =
				nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
			Json::Value nodeJsonInfo(Json::objectValue);

			nodeJsonInfo["ID"] = nodeItr->first;
			switch (nodeItr->second.first) {
				case ShardingNodeStateNotArrived:
					nodeJsonInfo["State"] = "NOT-ARRIVED";
					break;
				case ShardingNodeStateArrived:
					nodeJsonInfo["State"] = "ARRIVED";
					break;
				case ShardingNodeStateFailed:
					nodeJsonInfo["State"] = "FAILED";
					break;
			}
			if(nodeItr->second.second == NULL){
				nodeJsonInfo["Obj"] = "NULL";
			}else{
				Node * node = nodeItr->second.second;
				nodeJsonInfo["NodeName"] = node->getName();
				nodeJsonInfo["IP"] = node->getIpAddress();
				nodeJsonInfo["Port"] = node->getPortNumber();
				if(node->thisIsMe){
					nodeJsonInfo["ME"] = "YES";
				}else{
					nodeJsonInfo["ME"] = "NO";
				}
				if(node->isMaster()){
					nodeJsonInfo["Master"] = "YES";
				}else{
					nodeJsonInfo["Master"] = "NO";
				}
			}
			nodesJsonInfo[nodeIdx++] = nodeJsonInfo;
		}
		response->setResponseAttribute("nodes-info" , nodesJsonInfo);
		return;
	}
	/// nodes
	if(nodes.size() != 0){
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
			}else{
				ss << "Node object not arrived yet.";
			}
			nodesTable.printNextCell(ss.str());
		}
	}
}


void Cluster_Writeview::printClusterShards(JsonResponseHandler * response) const{
	ClusterShardId id;double load;ShardState state;bool isLocal;NodeId nodeId;NodeShardId nodeShardId;

	if(response != NULL){
		Json::Value coresJson(Json::arrayValue);
		unsigned coreIdx = 0;
		for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
			Json::Value coreClusterShardsJson(Json::objectValue);
			coreClusterShardsJson["core-name"] = coreItr->second->getName();
			coreClusterShardsJson["is-acl-core"] = coreItr->second->isAclCore() ? "YES" : "NO";
			coreClusterShardsJson["distributed"] = coreItr->second->isDistributedCore() ? "YES" : "NO";
			coreClusterShardsJson["num-primary-shards"] = coreItr->second->getNumberOfPrimaryShards();
			coreClusterShardsJson["num-replica-shards"] = coreItr->second->getNumberOfReplicas();
			Json::Value shardsJson(Json::arrayValue);
			unsigned shardIdx = 0 ;
			ClusterShardIterator cShardItr(this);
			while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
				if(id.coreId != coreItr->first){
					continue;
				}
				Json::Value shardJson(Json::objectValue);

				shardJson["ID"] = id.toString();
				shardJson["location"] = nodeId;
				if(isLocal){
					shardJson["local"] = "YES";
					if(localClusterDataShards.find(id) == localClusterDataShards.end()){
						shardJson["ERROR"] = "Local cluster shard which is not available in local shards.";
					}else{
						shardJson["index-directory"] = localClusterDataShards.at(id).indexDirectory;
						shardJson["json-file-path"] = localClusterDataShards.at(id).jsonFileCompletePath;
						shardJson["num-records"] = localClusterDataShards.at(id).server->getIndexer()->getNumberOfDocumentsInIndex();
					}
				}else{
					shardJson["local"] = "NO";
				}
				shardJson["load"] = load;
				switch(state){
				case SHARDSTATE_UNASSIGNED:
					shardJson["State"] = "UNASSIGNED";
					break;
				case SHARDSTATE_PENDING:
					shardJson["State"] = "PENDING";
					break;
				case SHARDSTATE_READY:
					shardJson["State"] = "READY";
					break;
				}

				shardsJson[shardIdx ++] = shardJson;
			}

			coreClusterShardsJson["cluster-shards"] = shardsJson;

			//
			coresJson[coreIdx ++] = coreClusterShardsJson;
		}

		response->setResponseAttribute("cluster-shards" , coresJson);

		return;
	}


	if(clusterShards.size() > 0){
		for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = cores.begin(); coreItr != cores.end(); ++coreItr){
			if(coreItr->second->isAclCore()){
				continue;
			}
			if(! coreItr->second->isDistributedCore()){
				continue;
			}
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
			ClusterShardIterator cShardItr(this);
			cShardItr.beginClusterShardsIteration();
			unsigned numberOfAssignedShardsOfAclCore = 0;
			unsigned numberOfLocalShardsOfAclCore = 0;
			while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
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


			// print information about acl cores
			vector<string> aclHeaders;
			aclHeaders.push_back("No. of Partitions");
			aclHeaders.push_back("No. of Replicas");
			aclHeaders.push_back("No. of Assigned Shards");
			aclHeaders.push_back("No. of Local Shards");
			vector<string> aclLabels;
			for(map<unsigned, CoreInfo_t *>::const_iterator aclCoreItr = cores.begin(); aclCoreItr != cores.end(); ++aclCoreItr){
				if(! (aclCoreItr->second->isAclCore() && aclCoreItr->second->getCoreId() == coreItr->second->getAttributeAclCoreId())){
					continue;
				}
				aclLabels.push_back(aclCoreItr->second->getName());
			}
			srch2::util::TableFormatPrinter helperCoresTable("Core name :%" + coreItr->second->getName() + "%Acl core",
					120, aclHeaders, aclLabels);
			helperCoresTable.printColumnHeaders();
			helperCoresTable.startFilling();
			for(map<unsigned, CoreInfo_t *>::const_iterator aclCoreItr = cores.begin(); aclCoreItr != cores.end(); ++aclCoreItr){
				if(! (aclCoreItr->second->isAclCore() && aclCoreItr->second->getCoreId() == coreItr->second->getAttributeAclCoreId())){
					continue;
				}
				const CoreInfo_t * coreInfo = aclCoreItr->second;
				{
					stringstream ss;
					ss << coreInfo->getNumberOfPrimaryShards();
					helperCoresTable.printNextCell(ss.str());
				}
				{
					stringstream ss;
					ss << coreInfo->getNumberOfReplicas();
					helperCoresTable.printNextCell(ss.str());
				}
				unsigned numberOfAssignedShardsOfAclCore = 0;
				unsigned numberOfLocalShardsOfAclCore = 0;

				ClusterShardIterator aclShardItr(this);
				aclShardItr.beginClusterShardsIteration();
				while(aclShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
					if(id.coreId != coreInfo->getCoreId()){
						continue;
					}
					if(state != SHARDSTATE_UNASSIGNED){
						numberOfAssignedShardsOfAclCore ++;
						if(isLocal){
							numberOfLocalShardsOfAclCore ++;
						}
					}

				}

				{
					stringstream ss;
					ss << numberOfAssignedShardsOfAclCore ;
					helperCoresTable.printNextCell(ss.str());
				}
				{
					stringstream ss;
					ss << numberOfLocalShardsOfAclCore;
					helperCoresTable.printNextCell(ss.str());
				}
			}
		}
	}
}


void Cluster_Writeview::printNodeShards(JsonResponseHandler * response) const{

	ClusterShardId id;double load;ShardState state;bool isLocal;NodeId nodeId;NodeShardId nodeShardId;
	if(response != NULL){

		Json::Value nodeShardsJson(Json::arrayValue);

		NodeShardIterator nShardItr(this);
		nShardItr.beginNodeShardsIteration();
		unsigned nodeShardIdx  = 0;
		while(nShardItr.getNextNodeShard(nodeShardId,isLocal)){
			Json::Value shardJson(Json::objectValue);
			shardJson["core-id"] = nodeShardId.coreId;
			shardJson["core-name"] = this->cores.find(nodeShardId.coreId)->second->getName();
			shardJson["location"] = nodeShardId.nodeId;
			shardJson["partition-id"] = nodeShardId.partitionId;
			if(isLocal){
				if(localNodeDataShards.find(nodeShardId.partitionId) == localNodeDataShards.end()){
					shardJson["ERROR"] = "Local node shard does not exist in local node shards";
				}else{
					shardJson["index-directory"] = localNodeDataShards.at(nodeShardId.partitionId).indexDirectory;
					shardJson["json-file-path"] = localNodeDataShards.at(nodeShardId.partitionId).jsonFileCompletePath;
					shardJson["num-records"] = localNodeDataShards.at(nodeShardId.partitionId).server->getIndexer()->getNumberOfDocumentsInIndex();
				}
				shardJson["local"] = "YES";
			}else{
				shardJson["local"] = "NO";
			}
			nodeShardsJson[nodeShardIdx ++] = shardJson;
		}

		response->setResponseAttribute("node-shards" , nodeShardsJson);
		return;
	}


	if(nodeShards.size() > 0){
		vector<string> nodeShardHeaders;
		vector<string> nodeShardLabels;
		unsigned counter = 0;
		while(counter++ < nodeShards.size()){
			stringstream ss;
			ss << "ID : " << nodeShards.at(counter-1)->id._toString();
			nodeShardHeaders.push_back(ss.str());
		}

		nodeShardLabels.push_back("Node Shard Information:");
		srch2::util::TableFormatPrinter nodeShardsTable("Node Shards :" , 120, nodeShardHeaders, nodeShardLabels);
		nodeShardsTable.printColumnHeaders();
		nodeShardsTable.startFilling();
		NodeShardIterator nShardItr(this);
		nShardItr.beginNodeShardsIteration();
		while(nShardItr.getNextNodeShard(nodeShardId,isLocal)){
			stringstream ss;
			if(isLocal){
				ss << "********%";
			}
			ss << "Node Location : " << nodeShardId.nodeId ;

			nodeShardsTable.printNextCell(ss.str());
		}
	}
}


void Cluster_Writeview::printLocalShards(JsonResponseHandler * response) const{
	if(response != NULL){
		return;// TODO add local information later.
	}
	if(localClusterDataShards.size() != 0){
		//////// Cluster physical shards
		vector<string> localClusterShardHeaders;
		vector<string> localClusterShardLabels;
		localClusterShardLabels.push_back("Shard info:");
		for(map<ClusterShardId, LocalPhysicalShard >::const_iterator shardItr = localClusterDataShards.begin(); shardItr != localClusterDataShards.end(); ++shardItr){
			if(this->cores.at(shardItr->first.coreId)->isAclCore()){
				continue;
			}
			localClusterShardHeaders.push_back(shardItr->first.toString());
		}
		srch2::util::TableFormatPrinter localClusterShardsTable("Cluster local shards :" , 120, localClusterShardHeaders, localClusterShardLabels);
		localClusterShardsTable.printColumnHeaders();
		localClusterShardsTable.startFilling();
		for(map<ClusterShardId, LocalPhysicalShard >::const_iterator shardItr = localClusterDataShards.begin(); shardItr != localClusterDataShards.end(); ++shardItr){
			if(this->cores.at(shardItr->first.coreId)->isAclCore()){
				continue;
			}
			stringstream ss;
			ss << "Index size : " ;
			if(shardItr->second.server){

				ss << shardItr->second.server->getIndexer()->getNumberOfDocumentsInIndex();
			}
			localClusterShardsTable.printNextCell(shardItr->second.indexDirectory + "%" + ss.str());
		}
	}



	if(localNodeDataShards.size() != 0){
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
    ClusterShardIterator cShardItr(this);
    cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state == SHARDSTATE_UNASSIGNED){
			continue;
		}
		ClusterShard_Writeview * shard = this->clusterShards.at(cShardItr.clusterShardsCursor-1);
		if(! isLocal ){
			shard->state = SHARDSTATE_PENDING;
			shard->load = 1;
			shard->nodeId = (unsigned)-1;
		}else{
			shard->nodeId = oldWrireview->currentNodeId;
		}
	}

	// and remove those node shards that are not ours
    NodeShardIterator nShardItr(this);
    nShardItr.beginNodeShardsIteration();
	vector<NodeShard_Writeview *> fixedNodeShards;
	while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
		NodeShard_Writeview * shard = this->nodeShards.at(nShardItr.nodeShardsCursor - 1);
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
	if(! this->nodes.empty() || ! oldWrireview->nodes.empty()){
		ASSERT(false);
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr = this->nodes.begin();
				nodeItr != this->nodes.end(); ++nodeItr){
			delete nodeItr->second.second;
		}
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr = oldWrireview->nodes.begin();
						nodeItr != oldWrireview->nodes.end(); ++nodeItr){
			delete nodeItr->second.second;
		}
		this->nodes.clear();
		oldWrireview->nodes.clear();
	}

}

void Cluster_Writeview::saveWriteviewOnDisk(const string & absDirectoryPath) const{
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

Cluster_Writeview * Cluster_Writeview::loadWriteviewFromDisk(const string & absDirectoryPath){

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


void Cluster_Writeview::assignLocalClusterShard(const ClusterShardId & shardId,
		const LocalPhysicalShard & physicalShardInfo, const double load){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	ASSERT(shard->id == shardId);
	shard->isLocal = true;
	shard->nodeId = this->currentNodeId;
	shard->load = load;
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
	shard->isLocal = false;
	shard->nodeId = (unsigned)-1;
}

void Cluster_Writeview::assignExternalClusterShard(const ClusterShardId & shardId, const NodeId & nodeId, const double & load){
	unsigned indexOfShard = INDEX(shardId);
	ClusterShard_Writeview * shard = this->clusterShards[indexOfShard];
	ASSERT(shard->id == shardId);
	if(nodeId == currentNodeId){
        ASSERT(false);
        return;
	}
	shard->isLocal = false;
	shard->nodeId = nodeId;
	shard->load = load;
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
	ASSERT(destNodeId != shard->nodeId);
	if(destNodeId == shard->nodeId){ // moving from a node to itself ? wrong!
	    return;
	}
	if(shard->isLocal){
		shard->isLocal = false;
		shard->nodeId = destNodeId;
		ASSERT(shard->state == SHARDSTATE_READY);
		// erase the physical shard
		if(localClusterDataShards.find(shardId) != localClusterDataShards.end()){
			localClusterDataShards.erase(shardId);
		}else{
		    ASSERT(false);
		}
	}else{
	    if(destNodeId == currentNodeId){
            shard->isLocal = true;
	    }else{
	        shard->isLocal = false;
	    }
		shard->nodeId = destNodeId;
		ASSERT(shard->state == SHARDSTATE_READY);
        if(destNodeId == currentNodeId){
            localClusterDataShards[shardId] = physicalShardInfo;
        }
	}
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

void Cluster_Writeview::getPatitionInvolvedNodes(const ClusterShardId & shardId, vector<NodeId> & participants) const{
	// only those nodes that have a replica of this partition
	ClusterShardId id;
	bool isLocal;
	ShardState state;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(this);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state == SHARDSTATE_UNASSIGNED){
			continue;
		}
		if(shardId.getPartitionId() == id.getPartitionId()){
			if(std::find(participants.begin(), participants.end(), nodeId) == participants.end()){
				participants.push_back(nodeId);
			}
		}
	}
	if(participants.size() == 0){
		for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr =
				nodes.begin(); nodeItr != nodes.end(); ++nodeItr){
			if(nodeItr->second.first == ShardingNodeStateArrived && nodeItr->second.second != NULL){
				participants.push_back(nodeItr->first);
			}
		}
	}
}

void Cluster_Writeview::getFullUnassignedPartitions(vector<ClusterPID> & fullUnassignedPartitions ) const{

	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	map<ClusterPID, bool> isFullUnassigned;

    ClusterShardIterator cShardItr(this);
    cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
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

bool Cluster_Writeview::isPartitionPending(const ClusterPID & pid) const{

	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

    ClusterShardIterator cShardItr(this);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(id.getPartitionId() == pid && state == SHARDSTATE_PENDING){
			return true;
		}
	}
	return false;
}

double Cluster_Writeview::getLocalNodeTotalLoad() const{
	double totalLoad = 0;
	for(unsigned clusterShardIdx = 0; clusterShardIdx < clusterShards.size(); ++clusterShardIdx){
		ClusterShard_Writeview * clusterShard = clusterShards.at(clusterShardIdx);
		if(! clusterShard->isLocal){
			continue;
		}
		if(clusterShard->nodeId != currentNodeId){
			ASSERT(false);
			continue;
		}
		if(cores.find(clusterShard->id.coreId) == cores.end()){
			ASSERT(false);
			continue;
		}else{
			if(cores.at(clusterShard->id.coreId)->isAclCore()){
				totalLoad += 0.1 * clusterShard->load;
			}else{
				totalLoad += clusterShard->load;
			}
		}
	}
	for(unsigned nodeShardIdx = 0; nodeShardIdx < nodeShards.size(); ++nodeShardIdx){
		NodeShard_Writeview * nodeShard = nodeShards.at(nodeShardIdx);
		if(! nodeShard->isLocal){
			continue;
		}
		if(cores.find(nodeShard->id.coreId) == cores.end()){
			ASSERT(false);
			continue;
		}else{
			totalLoad += nodeShard->load;
		}
	}
	return totalLoad;
}

// Reviewed. Don't touch.
void Cluster_Writeview::fixClusterMetadataOfAnotherNode(Cluster_Writeview * cluster) const{
	if(cluster == NULL){
		ASSERT(false);
		return;
	}
	ASSERT(cluster->localClusterDataShards.empty());
	ASSERT(cluster->localNodeDataShards.empty());
	// 1. isLocal values are from the perspective of src node of this "cluster" info
	for(unsigned i = 0 ; i < cluster->clusterShards.size(); ++i){
		ClusterShard_Writeview * shard = cluster->clusterShards.at(i);
		if(shard->state == SHARDSTATE_UNASSIGNED || shard->state == SHARDSTATE_PENDING){
			shard->isLocal = false;
			shard->load = 0 ;
			shard->nodeId = (unsigned)-1;
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
	for(unsigned i = 0 ; i < this->clusterShards.size(); ++i){
		const ClusterShard_Writeview * shard = this->clusterShards.at(i);
		if(shard->isLocal){
		    ASSERT(shard->nodeId == currentNodeId);
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
					clusterShard->nodeId = currentNodeId;
					succeed = true;
					break;
				}else{
				    // this node already believes that we are the owner of this shard
				    if(clusterShard->nodeId == currentNodeId){
	                    clusterShard->isLocal = true;
	                    clusterShard->load = shard->load;
	                    ASSERT(clusterShard->state == SHARDSTATE_READY);
	                    clusterShard->nodeId = currentNodeId;
	                    succeed = true;
	                    break;
				    }else{
                        ASSERT(false);// somebody else has this shard
                        break;
				    }
				}
			}
			if(succeed){
				cluster->localClusterDataShards[shard->id] = this->localClusterDataShards.at(shard->id);
			}
		}
	}

	// 3. for every local node shard, just add it to the cluster info
	for(unsigned i = 0 ; i < this->nodeShards.size(); ++i){
		const NodeShard_Writeview * shard = this->nodeShards.at(i);
		if(shard->isLocal){
			cluster->nodeShards.push_back(new NodeShard_Writeview(*shard));
			cluster->localNodeDataShards[shard->id.partitionId] = this->localNodeDataShards.at(shard->id.partitionId);
		}
	}

	// 4.
	cluster->currentNodeId = currentNodeId;
	// 5.
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = this->nodes.begin();
	        nodeItr != nodes.end(); ++nodeItr){
	    Node * nodePtr = NULL;
	    if(nodeItr->second.second != NULL){
	        nodePtr = new Node(*(nodeItr->second.second));
	    }
        cluster->nodes[nodeItr->first] = std::make_pair(nodeItr->second.first,nodePtr);
	}
	cluster->cores = cores;
}

ClusterResourceMetadata_Readview * Cluster_Writeview::getNewReadview() const{
	// we need cores for construction
	vector<const CoreInfo_t *> coresVector;
	for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		coresVector.push_back(coreItr->second);
	}

	ClusterResourceMetadata_Readview * newReadview =
			new ClusterResourceMetadata_Readview(this->versionId, this->clusterName, coresVector);
	newReadview->setCurrentNodeId(this->currentNodeId);

	// add nodes
	for(map<NodeId, std::pair<ShardingNodeState, Node *> > ::const_iterator nodeItr = this->nodes.begin();
			nodeItr != this->nodes.end(); ++nodeItr){
		if(nodeItr->second.first == ShardingNodeStateArrived ){
		    if(nodeItr->second.second != NULL){
                newReadview->addNode(*(nodeItr->second.second));
		    }
		}
	}

	// per core ...
	for(map<unsigned, CoreInfo_t *>::const_iterator coreItr = this->cores.begin(); coreItr != this->cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		const CoreInfo_t * core = coreItr->second;
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

	    ClusterShardIterator cShardItr(this);
		cShardItr.beginClusterShardsIteration();
		while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
			if(id.coreId == coreId && state == SHARDSTATE_READY){
				corePartitionContainer->addClusterShard(nodeId, id);
			}
		}

        NodeShardIterator nShardItr(this);
        nShardItr.beginNodeShardsIteration();
		while(nShardItr.getNextNodeShard(nodeShardId, isLocal)){
			if(nodeShardId.coreId == coreId){
				corePartitionContainer->addNodeShard(nodeShardId.nodeId, nodeShardId.partitionId);
			}
		}

		// set locks of partitions of this core
		vector<ClusterShardId> lockedPartitions;
		ShardManager::getShardManager()->_getLockManager()->getLockedPartitions(lockedPartitions);
		for(unsigned i = 0 ; i < lockedPartitions.size(); i++){
			if(lockedPartitions.at(i).coreId == coreId){
				corePartitionContainer->setPartitionLock(lockedPartitions.at(i).partitionId, PartitionLock_Locked);
			}
		}

        ClusterShardIterator cShardItr2(this);
        cShardItr2.beginClusterShardsIteration();
		while(cShardItr2.getNextClusterShard(id, load, state, isLocal, nodeId)){
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

        ClusterShardIterator cShardItr3(this);
		cShardItr3.beginClusterShardsIteration();
		while(cShardItr3.getNextLocalClusterShard(id, load,  physicalShard)){
			if(id.coreId == coreId){
				localShardContainer->addClusterShard(id, physicalShard.server,
						physicalShard.indexDirectory,  physicalShard.jsonFileCompletePath, load);
			}
		}
        NodeShardIterator nShardItr2(this);
        nShardItr2.beginNodeShardsIteration();
		while(nShardItr2.getNextLocalNodeShard(nodeShardId, load,  physicalShard)){
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
