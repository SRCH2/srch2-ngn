
#include "Shard.h"

#include "../../util/FramedPrinter.h"
#include "core/util/SerializationHelper.h"
#include "server/Srch2Server.h"
#include <sstream>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

ShardId::ShardId(unsigned coreId){
	this->coreId = coreId;
}
ShardId::ShardId(const ShardId & copy){
	this->coreId = copy.coreId;
}

bool ClusterShardId::isPrimaryShard() const{
	return (replicaId == 0); // replica #0 is always the primary shard
}
std::string ClusterShardId::toString() const {
	// A primary shard starts with a "P" followed by an integer id.
	// E.g., a cluster with 4 shards of core 8 will have shards named "C8_P0", "C8_R0_1", "C8_R0_2", "C8_P3".
	//
	// A replica shard starts with an "R" followed by a replica count and then its primary's id.
	// E.g., for the above cluster, replicas of "P0" will be named "8_R1_0" and "8_R2_0".
	// Similarly, replicas of "P3" will be named "8_R3_1" and "8_R3_2".
	if(coreId != unsigned(-1) || partitionId != unsigned(-1) || replicaId != unsigned(-1)){
		std::stringstream sstm;
		sstm << "C" << coreId << "_" << partitionId << "_" << replicaId;
		return sstm.str();
	}
	else{
		return "";
	}
}

bool ClusterShardId::isClusterShard() const {
	return true;
}

bool ClusterShardId::isReplica(ShardId * shardId) const{
	if(shardId == NULL || ! shardId->isClusterShard() || coreId != shardId->coreId){
		return false;
	}
	ClusterShardId * cShardId = (ClusterShardId *)shardId;
	return (partitionId == cShardId->partitionId);
}

ClusterPID ClusterShardId::getPartitionId() const{
	ClusterPID pid(coreId, partitionId);
	return pid;
}


ClusterShardId::ClusterShardId():ShardId(unsigned(-1)) {
	partitionId = unsigned(-1);
	replicaId = unsigned(-1);
}
ClusterShardId::ClusterShardId(const ClusterShardId & copy):ShardId(copy) {
	partitionId = copy.partitionId;
	replicaId = copy.replicaId;
}
ClusterShardId::ClusterShardId(unsigned coreId, unsigned partitionId, unsigned replicaId) :
	ShardId(coreId), partitionId(partitionId), replicaId(replicaId) {}

bool ClusterShardId::operator==(const ClusterShardId& rhs) const {
	return coreId == rhs.coreId && partitionId == rhs.partitionId
			&& replicaId == rhs.replicaId;
}
bool ClusterShardId::operator!=(const ClusterShardId& rhs) const {
	return ! (*this == rhs);
}
bool ClusterShardId::operator>(const ClusterShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId && (partitionId > rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId > rhs.replicaId)));
}
bool ClusterShardId::operator<(const ClusterShardId& rhs) const {
	return ! (*this == rhs || *this > rhs);
}
bool ClusterShardId::operator>=(const ClusterShardId& rhs) const {
	return  (*this > rhs || *this == rhs);
}
bool ClusterShardId::operator<=(const ClusterShardId& rhs) const {
	return  (*this < rhs || *this == rhs);
}

ClusterShardId & ClusterShardId::operator=(const ClusterShardId & rhs){
	if(this != &rhs){
		coreId = rhs.coreId;
		partitionId = rhs.partitionId;
		replicaId = rhs.replicaId;
	}
	return *this;
}


//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* ClusterShardId::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	buffer = srch2::util::serializeFixedTypes(partitionId, buffer);
	buffer = srch2::util::serializeFixedTypes(replicaId, buffer);
	return buffer;
}

//given a byte stream recreate the original object
void * ClusterShardId::deserialize(void* buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	buffer = srch2::util::deserializeFixedTypes(buffer, partitionId);
	buffer = srch2::util::deserializeFixedTypes(buffer, replicaId);
	return buffer;
}

unsigned ClusterShardId::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(coreId);
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(unsigned);
	return numberOfBytes;
}


NodeShardId::NodeShardId():ShardId(unsigned(-1)){
	this->nodeId = unsigned(-1);
	this->partitionId = unsigned(-1);
}
NodeShardId::NodeShardId(const NodeShardId & copy):ShardId(copy){
	this->nodeId = copy.nodeId;
	this->partitionId = copy.partitionId;
}
NodeShardId::NodeShardId(unsigned coreId, NodeId nodeId, unsigned partitionId):ShardId(coreId){
	this->nodeId = nodeId;
	this->partitionId = partitionId;
}

std::string NodeShardId::toString() const{
	std::stringstream sstm;
	sstm << "N" << coreId << "_" << partitionId ;
	return sstm.str();
}

std::string NodeShardId::_toString() const{
	std::stringstream sstm;
	sstm << "N" << coreId << "_" << nodeId << "_" << partitionId ;
	return sstm.str();
}


bool NodeShardId::isClusterShard() const {
	return false;
}

bool NodeShardId::isReplica(ShardId * shardId) const{
	return false;
}

bool NodeShardId::operator==(const NodeShardId& rhs) const {
	return (coreId == rhs.coreId)&&(nodeId == rhs.nodeId)&&(partitionId == rhs.partitionId);
}
bool NodeShardId::operator!=(const NodeShardId& rhs) const {
	return !(*this == rhs);
}
bool NodeShardId::operator>(const NodeShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(nodeId > rhs.nodeId ||
							(nodeId == rhs.nodeId && partitionId > rhs.partitionId)));
}
bool NodeShardId::operator<(const NodeShardId& rhs) const {
	return !((*this == rhs) || (*this > rhs));
}
bool NodeShardId::operator>=(const NodeShardId& rhs) const {
	return (*this > rhs) || (*this == rhs);
}
bool NodeShardId::operator<=(const NodeShardId& rhs) const {
	return !(*this > rhs);
}

NodeShardId & NodeShardId::operator=(const NodeShardId & rhs){
	if(this != &rhs){
		coreId = rhs.coreId;
		nodeId = rhs.nodeId;
		partitionId = rhs.partitionId;
	}
	return *this;
}

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* NodeShardId::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(partitionId, buffer);
	return buffer;
}

//given a byte stream recreate the original object
void * NodeShardId::deserialize(void* buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, partitionId);
	return buffer;
}

unsigned NodeShardId::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(coreId);
	numberOfBytes += sizeof(NodeId);
	numberOfBytes += sizeof(unsigned);
	return numberOfBytes;
}

/////////////////////////////// ShardIdComparator
bool ShardIdComparator::operator() (const ClusterShardId & s1, const ClusterShardId & s2) {
	if (s1.coreId > s2.coreId)
		return true;

	if (s1.coreId < s2.coreId)
		return false;

	// they have equal coreId; we look at their partitionId
	if (s1.partitionId > s2.partitionId)
		return true;

	if (s1.partitionId < s2.partitionId)
		return false;

	// they have equal partitionId; we look at their replicaId
	if (s1.replicaId > s2.replicaId)
		return true;

	return false;
}

bool ShardIdComparator::operator() (const NodeShardId & s1, const NodeShardId & s2){
	if (s1.coreId > s2.coreId)
		return true;

	if (s1.coreId < s2.coreId)
		return false;

	// they have equal coreId; we look at their partitionId
	if (s1.partitionId > s2.partitionId)
		return true;

	if (s1.partitionId < s2.partitionId)
		return false;

	// they have equal partitionId; we look at their replicaId
	if (s1.nodeId > s2.nodeId)
		return true;

	return false;
}



///////////////////////////////////////// Shard and ClusterShard and NodeShard
Shard::Shard(boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory, const string & jsonFileCompletePath, double load){
	this->srch2Server = srch2Server;
	this->indexDirectory = indexDirectory;
	this->jsonFileCompletePath = jsonFileCompletePath;
	this->load = load;
}

boost::shared_ptr<Srch2Server> Shard::getSrch2Server() const{
	return this->srch2Server;
}
string Shard::getIndexDirectory() const{
	return this->indexDirectory;
}
string Shard::getJsonFileCompletePath() const{
	return this->jsonFileCompletePath;
}
double Shard::getLoad() const{
	return load;
}


///// ClusterShard
ClusterShard::ClusterShard(const ClusterShardId & shardId,
		boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
		const string & jsonFileCompletePath, double load):
		Shard(srch2Server, indexDirectory, jsonFileCompletePath, load),shardId(shardId){}
const ClusterShardId ClusterShard::getShardId() const {
	return shardId;
}
string ClusterShard::getShardIdentifier() const {
	return shardId.toString();
}
ShardId * ClusterShard::cloneShardId() const{
	return new ClusterShardId(this->shardId);
}
bool ClusterShard::operator==(const ClusterShard & right) const{
	return (this->shardId == right.shardId);
}

string ClusterShard::toString() const{
	stringstream ss;

	ss << "ShardId : " << shardId.toString() << "%";
	ss << "Index size : " << this->getSrch2Server()->getIndexer()->getNumberOfDocumentsInIndex();
	return ss.str();
}

NodeShard::NodeShard(const NodeShardId & shardId, boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
		const string & jsonFileCompletePath,double load):
		Shard(srch2Server, indexDirectory, jsonFileCompletePath, load),
			shardId(shardId){}

const NodeShardId NodeShard::getShardId() const{
	return shardId;
}
string NodeShard::getShardIdentifier() const {
	return shardId._toString();
}
ShardId * NodeShard::cloneShardId() const{
	return new NodeShardId(this->shardId);
}
bool NodeShard::operator==(const NodeShard & right) const{
	return (this->shardId == right.shardId);
}

string NodeShard::toString() const{
	stringstream ss;

	ss << "ShardId : " << shardId._toString() << "%";
	ss << "Index size : " << this->getSrch2Server()->getIndexer()->getNumberOfDocumentsInIndex();
	return ss.str();
}

NodeTargetShardInfo::NodeTargetShardInfo(const NodeId nodeId, const unsigned coreId):nodeId(nodeId), coreId(coreId){

}

void NodeTargetShardInfo::addClusterShard(ClusterShardId shardId){
	ASSERT(shardId.coreId == coreId);
	if(std::find(targetClusterShards.begin(), targetClusterShards.end(), shardId) == targetClusterShards.end()){
		targetClusterShards.push_back(shardId);
	}
}

void NodeTargetShardInfo::addNodeShard(const NodeShardId & shardId){
	ASSERT(shardId.coreId == coreId);
	ASSERT(shardId.nodeId == nodeId);
	if(std::find(targetNodeShards.begin(), targetNodeShards.end(),
			shardId) == targetNodeShards.end()){
		targetNodeShards.push_back(shardId);
	}
}

const unsigned NodeTargetShardInfo::getCoreId() const{
	return coreId;
}

const NodeId NodeTargetShardInfo::getNodeId() const {
	return nodeId;
}

vector<ClusterShardId> NodeTargetShardInfo::getTargetClusterShards() const{
	return targetClusterShards;
}
vector<NodeShardId> NodeTargetShardInfo::getTargetNodeShards() const{
	return targetNodeShards;
}



//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* NodeTargetShardInfo::serialize(void * bufferWritePointer) const{

    bufferWritePointer = srch2::util::serializeFixedTypes(nodeId, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes(coreId, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)(targetClusterShards.size()), bufferWritePointer);
    for(unsigned i =0; i<targetClusterShards.size(); i++){
    	bufferWritePointer = targetClusterShards.at(i).serialize(bufferWritePointer);
    }
    bufferWritePointer = srch2::util::serializeFixedTypes((unsigned)(targetNodeShards.size()), bufferWritePointer);
    for(unsigned i =0; i<targetNodeShards.size(); i++){
    	bufferWritePointer = targetNodeShards.at(i).serialize(bufferWritePointer);
    }
    return bufferWritePointer;
}

unsigned NodeTargetShardInfo::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += sizeof(NodeId);
    numberOfBytes += sizeof(unsigned); // coreId
    numberOfBytes += sizeof(unsigned); // targetClusterShards size
    for(unsigned shardIndex = 0 ; shardIndex < targetClusterShards.size(); ++shardIndex){
    	numberOfBytes += targetClusterShards.at(shardIndex).getNumberOfBytes();
    }
    numberOfBytes += sizeof(unsigned); // targetClusterShards size
    for(unsigned shardIndex = 0 ; shardIndex < targetNodeShards.size(); ++shardIndex){
    	numberOfBytes += targetNodeShards.at(shardIndex).getNumberOfBytes();
    }
    return numberOfBytes;
}

//given a byte stream recreate the original object
void * NodeTargetShardInfo::deserialize(void* buffer){

	if(buffer == NULL){
		ASSERT(false);
		return NULL;
	}

    buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
    buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
    unsigned sizeOfVector = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfVector);
    for(unsigned i =0; i<sizeOfVector; i++){
    	ClusterShardId shardId;
    	buffer = shardId.deserialize(buffer);
    	targetClusterShards.push_back(shardId);
    }
    buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfVector);
    for(unsigned i =0; i<sizeOfVector; i++){
    	NodeShardId shardId;
    	buffer = shardId.deserialize(buffer);
    	targetNodeShards.push_back(shardId);
    }
    return buffer;
}


string NodeTargetShardInfo::toString() const{
	stringstream ss;
	ss << "Node " << nodeId << "%";
	ss << "Core " << coreId << "%";
	ss << "Cluster shards : ";
	for(unsigned i = 0 ; i < targetClusterShards.size(); ++i){
		if(i != 0){
			ss << "|" ;
		}
		ss << targetClusterShards.at(i).toString();
	}
	ss << "%";

	ss << "Node shards : ";
	for(unsigned i = 0 ; i < targetNodeShards.size(); ++i){
		if(i != 0){
			ss << "|";
		}
		ss << targetNodeShards.at(i)._toString();
	}
	ss << "%";
	return ss.str();
}


LocalShardContainer::LocalShardContainer(const unsigned coreId, const NodeId nodeId):
		coreId(coreId), nodeId(nodeId){}

LocalShardContainer::LocalShardContainer(const LocalShardContainer & copy):coreId(copy.coreId),nodeId(copy.nodeId){
	/*
	 * 	// partitionId => list of shards on this node
	map<unsigned, vector<ClusterShard *> > localClusterShards;
	// node internal partitionid => nodeshard
	map<unsigned, NodeShard * > localNodeShards;
	 */
	for(map<unsigned, vector<ClusterShard *> >::const_iterator clusterShardItr = copy.localClusterShards.begin();
			clusterShardItr != copy.localClusterShards.end(); ++clusterShardItr){
		this->localClusterShards[clusterShardItr->first] = vector<ClusterShard *>();
		for(unsigned i = 0 ; i < clusterShardItr->second.size(); ++i){
			this->localClusterShards[clusterShardItr->first].push_back(new ClusterShard(*(clusterShardItr->second.at(i))));
		}
	}

	for(map<unsigned, NodeShard * >::const_iterator nodeShardItr = copy.localNodeShards.begin();
			nodeShardItr != copy.localNodeShards.end(); ++nodeShardItr){
		this->localNodeShards[nodeShardItr->first] = new NodeShard(*(nodeShardItr->second));
	}
}

unsigned LocalShardContainer::getCoreId() const	{
	return coreId;
}

NodeId LocalShardContainer::getNodeId() const{
	return nodeId;
}

void LocalShardContainer::getShards(const NodeTargetShardInfo & targets, vector<const Shard *> & shards) const{
	//cluster partitions
	const vector<ClusterShardId> & clusterShardIdTargets = targets.getTargetClusterShards();
	for(vector<ClusterShardId>::const_iterator shardIdTarget = clusterShardIdTargets.begin();
			shardIdTarget != clusterShardIdTargets.end(); ++shardIdTarget){
		if(localClusterShards.find(shardIdTarget->partitionId) == localClusterShards.end()){
			continue;
		}
		// move on ClusterShard s and use the one which has the same shardId
		const vector<ClusterShard *> & partitionClusterShards = localClusterShards.find(shardIdTarget->partitionId)->second;
		for(vector<ClusterShard *>::const_iterator clusterShard = partitionClusterShards.begin();
				clusterShard != partitionClusterShards.end(); ++clusterShard){
			if((*clusterShard)->getShardId() == *shardIdTarget){
				shards.push_back(*clusterShard);
			}
		}
	}

	// node partitions
	const vector<NodeShardId> & nodeShardIdTargets = targets.getTargetNodeShards();
	for(vector<NodeShardId>::const_iterator nodeShardIdTargetItr = nodeShardIdTargets.begin();
			nodeShardIdTargetItr != nodeShardIdTargets.end(); ++nodeShardIdTargetItr){
		if(localNodeShards.find(nodeShardIdTargetItr->partitionId) == localNodeShards.end()){
			continue;
		}
		const NodeShard * nodeShard = localNodeShards.find(nodeShardIdTargetItr->partitionId)->second;
		shards.push_back(nodeShard);
	}
}

void LocalShardContainer::getClusterShards(unsigned partitionId, vector<const ClusterShard *> & clusterShards) const{
	if(localClusterShards.find(partitionId) == localClusterShards.end()){
		return;
	}
	for(unsigned i = 0 ; i < localClusterShards.find(partitionId)->second.size(); ++i){
		clusterShards.push_back(localClusterShards.find(partitionId)->second.at(i));
	}
	return;
}
const NodeShard * LocalShardContainer::getNodeShard(unsigned internalPartitionId) const {
	if(localNodeShards.find(internalPartitionId) == localNodeShards.end()){
		return NULL;
	}
	return localNodeShards.find(internalPartitionId)->second;
}
void LocalShardContainer::getNodeShards(map<unsigned, const NodeShard * > & localNodeShards) const{
	for(map<unsigned, NodeShard * >::const_iterator localNodeShardsItr =
			this->localNodeShards.begin(); localNodeShardsItr != this->localNodeShards.end(); ++localNodeShardsItr){
		localNodeShards[localNodeShardsItr->first] = localNodeShardsItr->second;
	}
}

void LocalShardContainer::addClusterShard(const ClusterShardId & shardId,
		boost::shared_ptr<Srch2Server> srch2Server, const string & indexDirectory,
		const string & jsonCompletePath, double load){
	if(localClusterShards.find(shardId.partitionId) == localClusterShards.end()){
		localClusterShards[shardId.partitionId] = vector<ClusterShard *>();
	}
	ClusterShard * newClusterShard = new ClusterShard(shardId, srch2Server, indexDirectory, jsonCompletePath, load);
	for(unsigned sid = 0 ; sid < localClusterShards[shardId.partitionId].size(); ++sid){
		if(*(localClusterShards[shardId.partitionId].at(sid)) == *newClusterShard ){
			ASSERT(false);
			return;
		}
	}
	localClusterShards[shardId.partitionId].push_back(newClusterShard);
}


void LocalShardContainer::addNodeShard(const NodeShardId & shardId, boost::shared_ptr<Srch2Server> srch2Server,const string & indexDirectory,
		const string & jsonCompletePath,double load){
	if(localNodeShards.find(shardId.partitionId) == localNodeShards.end()){
		localNodeShards[shardId.partitionId] = new NodeShard(shardId, srch2Server, indexDirectory, jsonCompletePath, load);
		return;
	}
	ASSERT(false);
	return;
}


void LocalShardContainer::print() const{
	// cluster shards;
	vector<string> clusterShardHeaders;
	vector<string> clusterShardLabels;
	unsigned numberOfColumns = 0;
	for(map<unsigned, vector<ClusterShard *> >::const_iterator partItr =
			localClusterShards.begin(); partItr != localClusterShards.end(); ++partItr){
		if(numberOfColumns < partItr->second.size()){
			numberOfColumns = partItr->second.size();
		}
		stringstream ss;
		ss << "Partition ID : " << partItr->first;
		clusterShardLabels.push_back(ss.str());
	}
	unsigned counter = numberOfColumns ;
	while(counter--){
		clusterShardHeaders.push_back("Local shard info");
	}
	stringstream ss;
	ss << "Core " << coreId ;
	srch2::util::TableFormatPrinter localClusterShardsTable("Local Cluster Shards%" + ss.str(),
			120, clusterShardHeaders, clusterShardLabels);
	localClusterShardsTable.printColumnHeaders();
	localClusterShardsTable.startFilling();
	for(map<unsigned, vector<ClusterShard *> >::const_iterator partItr =
			localClusterShards.begin(); partItr != localClusterShards.end(); ++partItr){
		for(unsigned i = 0; i < numberOfColumns ; ++i){
			string content = "";
			if(i < partItr->second.size()){
				content = partItr->second.at(i)->toString();
			}
			localClusterShardsTable.printNextCell(content);
		}
	}


	// node shards

	vector<string> nodeShardHeaders;
	nodeShardHeaders.push_back("Node shard info");
	vector<string> nodeShardLabels;
	for(map<unsigned, NodeShard * >::const_iterator nodeShardItr =
			localNodeShards.begin(); nodeShardItr != localNodeShards.end(); ++nodeShardItr){
		stringstream ss;
		ss << "Partition Id : " << nodeShardItr->first ;
		nodeShardLabels.push_back(ss.str());
	}
	srch2::util::TableFormatPrinter localNodeShardsTable("Local Node Shards%" + ss.str(),
			120, nodeShardHeaders, nodeShardLabels);
	localNodeShardsTable.printColumnHeaders();
	localNodeShardsTable.startFilling();
	for(map<unsigned, NodeShard * >::const_iterator nodeShardItr =
			localNodeShards.begin(); nodeShardItr != localNodeShards.end(); ++nodeShardItr){
		localNodeShardsTable.printNextCell(nodeShardItr->second->toString());
	}
}

}
}
